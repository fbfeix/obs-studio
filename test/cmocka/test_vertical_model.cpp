extern "C" {
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
}

#include <obs-data.h>
#include <obs.h>

#include <vector>

#include <utility/vertical/VerticalModel.hpp>

using namespace OBS::Vertical;

static void config_defaults_when_missing(void **)
{
	obs_data_t *empty = obs_data_create();

	VerticalConfig cfg = VerticalConfig::FromData(empty);

	assert_int_equal(cfg.width, 1080);
	assert_int_equal(cfg.height, 1920);
	assert_true(cfg.record);
	assert_false(cfg.stream);
	assert_true(cfg.canvasUuid.empty());

	obs_data_release(empty);
}

static void config_defaults_for_null_data(void **)
{
	VerticalConfig cfg = VerticalConfig::FromData(nullptr);

	assert_int_equal(cfg.width, 1080);
	assert_int_equal(cfg.height, 1920);
}

static void config_round_trip(void **)
{
	VerticalConfig in;
	in.width = 720;
	in.height = 1280;
	in.record = false;
	in.stream = true;
	in.canvasUuid = "5fa3b629-a2a9-4336-bcfa-38575bc5cfaa";

	obs_data_t *data = in.ToData();
	VerticalConfig out = VerticalConfig::FromData(data);
	obs_data_release(data);

	assert_int_equal(out.width, 720);
	assert_int_equal(out.height, 1280);
	assert_false(out.record);
	assert_true(out.stream);
	assert_string_equal(out.canvasUuid.c_str(), "5fa3b629-a2a9-4336-bcfa-38575bc5cfaa");
}

static void config_writes_version(void **)
{
	VerticalConfig in;
	obs_data_t *data = in.ToData();

	assert_int_equal(obs_data_get_int(data, "version"), VerticalConfig::kVersion);

	obs_data_release(data);
}

static void config_rejects_zero_size(void **)
{
	obs_data_t *data = obs_data_create();
	obs_data_set_int(data, "width", 0);
	obs_data_set_int(data, "height", 1280);

	VerticalConfig cfg = VerticalConfig::FromData(data);

	assert_int_equal(cfg.width, 1080);
	assert_int_equal(cfg.height, 1920);

	obs_data_release(data);
}

static void config_rejects_oversized(void **)
{
	obs_data_t *data = obs_data_create();
	obs_data_set_int(data, "width", 1080);
	obs_data_set_int(data, "height", 100000);

	VerticalConfig cfg = VerticalConfig::FromData(data);

	assert_int_equal(cfg.width, 1080);
	assert_int_equal(cfg.height, 1920);

	obs_data_release(data);
}

static void resolve_uses_configured_canvas(void **)
{
	VerticalConfig cfg;
	cfg.canvasUuid = "mine";
	std::vector<CanvasRef> canvases = {{"Aitum Vertical", "aitum"}, {"Vertical", "mine"}};

	CanvasResolution res = ResolveCanvas(cfg, canvases);

	assert_int_equal(static_cast<int>(res.action), static_cast<int>(CanvasAction::UseExisting));
	assert_string_equal(res.uuid.c_str(), "mine");
}

static void resolve_adopts_aitum_canvas_without_config(void **)
{
	VerticalConfig cfg;
	std::vector<CanvasRef> canvases = {{"Aitum Vertical", "aitum"}};

	CanvasResolution res = ResolveCanvas(cfg, canvases);

	assert_int_equal(static_cast<int>(res.action), static_cast<int>(CanvasAction::AdoptAitum));
	assert_string_equal(res.uuid.c_str(), "aitum");
}

static void resolve_adopts_aitum_when_configured_canvas_is_gone(void **)
{
	VerticalConfig cfg;
	cfg.canvasUuid = "deleted";
	std::vector<CanvasRef> canvases = {{"Aitum Vertical", "aitum"}};

	CanvasResolution res = ResolveCanvas(cfg, canvases);

	assert_int_equal(static_cast<int>(res.action), static_cast<int>(CanvasAction::AdoptAitum));
}

static void resolve_creates_when_nothing_matches(void **)
{
	VerticalConfig cfg;
	std::vector<CanvasRef> canvases = {{"Something else", "x"}};

	CanvasResolution res = ResolveCanvas(cfg, canvases);

	assert_int_equal(static_cast<int>(res.action), static_cast<int>(CanvasAction::Create));
	assert_true(res.uuid.empty());
}

static void video_info_follows_main_but_uses_vertical_size(void **)
{
	obs_video_info main = {};
	main.fps_num = 60;
	main.fps_den = 1;
	main.base_width = 1920;
	main.base_height = 1080;
	main.output_width = 1280;
	main.output_height = 720;
	main.output_format = VIDEO_FORMAT_NV12;
	main.colorspace = VIDEO_CS_709;
	main.range = VIDEO_RANGE_PARTIAL;
	main.scale_type = OBS_SCALE_BICUBIC;

	VerticalConfig cfg;
	cfg.width = 1080;
	cfg.height = 1920;

	obs_video_info v = MakeVideoInfo(main, cfg);

	assert_int_equal(v.base_width, 1080);
	assert_int_equal(v.base_height, 1920);
	assert_int_equal(v.output_width, 1080);
	assert_int_equal(v.output_height, 1920);
	assert_int_equal(v.fps_num, 60);
	assert_int_equal(v.fps_den, 1);
	assert_int_equal(v.output_format, VIDEO_FORMAT_NV12);
	assert_int_equal(v.colorspace, VIDEO_CS_709);
	assert_int_equal(v.range, VIDEO_RANGE_PARTIAL);
	assert_int_equal(v.scale_type, OBS_SCALE_BICUBIC);
}

static void unique_name_returns_base_when_free(void **)
{
	std::vector<std::string> taken = {"Scene", "Other"};

	assert_string_equal(UniqueName("Vertical Scene", taken).c_str(), "Vertical Scene");
}

static void unique_name_appends_counter(void **)
{
	std::vector<std::string> taken = {"Vertical Scene", "Vertical Scene 2"};

	assert_string_equal(UniqueName("Vertical Scene", taken).c_str(), "Vertical Scene 3");
}

static void unique_name_fills_first_gap(void **)
{
	std::vector<std::string> taken = {"Vertical Scene", "Vertical Scene 3"};

	assert_string_equal(UniqueName("Vertical Scene", taken).c_str(), "Vertical Scene 2");
}

static void vertical_path_inserts_suffix_before_extension(void **)
{
	assert_string_equal(VerticalRecordingPath("/home/felix/2026-11-23 13-00-00.mkv").c_str(),
			    "/home/felix/2026-11-23 13-00-00-vertical.mkv");
}

static void vertical_path_ignores_dots_in_directories(void **)
{
	assert_string_equal(VerticalRecordingPath("/home/felix/my.videos/recording").c_str(),
			    "/home/felix/my.videos/recording-vertical");
}

static void vertical_path_keeps_hidden_file_name(void **)
{
	assert_string_equal(VerticalRecordingPath("/tmp/.hidden").c_str(), "/tmp/.hidden-vertical");
}

static void vertical_path_empty_stays_empty(void **)
{
	assert_true(VerticalRecordingPath("").empty());
}

static void pairs_lookup_returns_vertical_scene(void **)
{
	ScenePairs pairs;
	pairs.Set("main-a", "vert-1");

	auto found = pairs.Lookup("main-a");

	assert_true(found.has_value());
	assert_string_equal(found->c_str(), "vert-1");
	assert_false(pairs.Lookup("main-b").has_value());
}

static void pairs_set_replaces_existing(void **)
{
	ScenePairs pairs;
	pairs.Set("main-a", "vert-1");
	pairs.Set("main-a", "vert-2");

	assert_string_equal(pairs.Lookup("main-a")->c_str(), "vert-2");
	assert_int_equal(pairs.Size(), 1);
}

static void pairs_allow_many_main_scenes_per_vertical(void **)
{
	ScenePairs pairs;
	pairs.Set("main-a", "vert-1");
	pairs.Set("main-b", "vert-1");

	assert_string_equal(pairs.Lookup("main-b")->c_str(), "vert-1");
	assert_int_equal(pairs.Size(), 2);
}

static void pairs_unset_removes_pair(void **)
{
	ScenePairs pairs;
	pairs.Set("main-a", "vert-1");
	pairs.Unset("main-a");

	assert_false(pairs.Lookup("main-a").has_value());
}

static void pairs_empty_uuid_unsets(void **)
{
	ScenePairs pairs;
	pairs.Set("main-a", "vert-1");
	pairs.Set("main-a", "");

	assert_false(pairs.Lookup("main-a").has_value());
}

static void pairs_remove_scene_drops_both_sides(void **)
{
	ScenePairs pairs;
	pairs.Set("main-a", "vert-1");
	pairs.Set("main-b", "vert-2");
	pairs.Set("main-c", "vert-1");

	pairs.RemoveScene("vert-1");
	pairs.RemoveScene("main-b");

	assert_int_equal(pairs.Size(), 0);
}

static void pairs_prune_drops_missing_scenes(void **)
{
	ScenePairs pairs;
	pairs.Set("main-a", "vert-1");
	pairs.Set("main-gone", "vert-1");
	pairs.Set("main-b", "vert-gone");

	pairs.Prune([](const std::string &uuid) { return uuid.find("gone") == std::string::npos; });

	assert_int_equal(pairs.Size(), 1);
	assert_true(pairs.Lookup("main-a").has_value());
}

static void pairs_round_trip(void **)
{
	ScenePairs in;
	in.Set("main-a", "vert-1");
	in.Set("main-b", "vert-2");

	obs_data_array_t *array = in.ToData();
	ScenePairs out = ScenePairs::FromData(array);
	obs_data_array_release(array);

	assert_int_equal(out.Size(), 2);
	assert_string_equal(out.Lookup("main-a")->c_str(), "vert-1");
	assert_string_equal(out.Lookup("main-b")->c_str(), "vert-2");
}

static void pairs_from_null_is_empty(void **)
{
	assert_int_equal(ScenePairs::FromData(nullptr).Size(), 0);
}

static void pairs_from_data_skips_incomplete_entries(void **)
{
	obs_data_array_t *array = obs_data_array_create();
	obs_data_t *complete = obs_data_create();
	obs_data_set_string(complete, "main", "main-a");
	obs_data_set_string(complete, "vertical", "vert-1");
	obs_data_t *incomplete = obs_data_create();
	obs_data_set_string(incomplete, "main", "main-b");
	obs_data_array_push_back(array, complete);
	obs_data_array_push_back(array, incomplete);
	obs_data_release(complete);
	obs_data_release(incomplete);

	ScenePairs pairs = ScenePairs::FromData(array);
	obs_data_array_release(array);

	assert_int_equal(pairs.Size(), 1);
}

int main()
{
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(config_defaults_when_missing),
		cmocka_unit_test(config_defaults_for_null_data),
		cmocka_unit_test(config_round_trip),
		cmocka_unit_test(config_writes_version),
		cmocka_unit_test(config_rejects_zero_size),
		cmocka_unit_test(config_rejects_oversized),
		cmocka_unit_test(resolve_uses_configured_canvas),
		cmocka_unit_test(resolve_adopts_aitum_canvas_without_config),
		cmocka_unit_test(resolve_adopts_aitum_when_configured_canvas_is_gone),
		cmocka_unit_test(resolve_creates_when_nothing_matches),
		cmocka_unit_test(video_info_follows_main_but_uses_vertical_size),
		cmocka_unit_test(unique_name_returns_base_when_free),
		cmocka_unit_test(unique_name_appends_counter),
		cmocka_unit_test(unique_name_fills_first_gap),
		cmocka_unit_test(vertical_path_inserts_suffix_before_extension),
		cmocka_unit_test(vertical_path_ignores_dots_in_directories),
		cmocka_unit_test(vertical_path_keeps_hidden_file_name),
		cmocka_unit_test(vertical_path_empty_stays_empty),
		cmocka_unit_test(pairs_lookup_returns_vertical_scene),
		cmocka_unit_test(pairs_set_replaces_existing),
		cmocka_unit_test(pairs_allow_many_main_scenes_per_vertical),
		cmocka_unit_test(pairs_unset_removes_pair),
		cmocka_unit_test(pairs_empty_uuid_unsets),
		cmocka_unit_test(pairs_remove_scene_drops_both_sides),
		cmocka_unit_test(pairs_prune_drops_missing_scenes),
		cmocka_unit_test(pairs_round_trip),
		cmocka_unit_test(pairs_from_null_is_empty),
		cmocka_unit_test(pairs_from_data_skips_incomplete_entries),
	};

	return cmocka_run_group_tests(tests, NULL, NULL);
}
