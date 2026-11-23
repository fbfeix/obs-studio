extern "C" {
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
}

#include <obs-data.h>

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

int main()
{
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(config_defaults_when_missing),
		cmocka_unit_test(config_defaults_for_null_data),
		cmocka_unit_test(config_round_trip),
		cmocka_unit_test(config_writes_version),
		cmocka_unit_test(config_rejects_zero_size),
		cmocka_unit_test(config_rejects_oversized),
	};

	return cmocka_run_group_tests(tests, NULL, NULL);
}
