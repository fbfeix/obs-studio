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
	};

	return cmocka_run_group_tests(tests, NULL, NULL);
}
