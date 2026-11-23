#include "VerticalModel.hpp"

namespace OBS::Vertical {

namespace {
constexpr int64_t kMaxDimension = 16384;

bool IsValidDimension(int64_t value)
{
	return value > 0 && value <= kMaxDimension;
}
} // namespace

VerticalConfig VerticalConfig::FromData(obs_data_t *data)
{
	VerticalConfig cfg;
	if (!data) {
		return cfg;
	}

	int64_t width = obs_data_get_int(data, "width");
	int64_t height = obs_data_get_int(data, "height");
	if (IsValidDimension(width) && IsValidDimension(height)) {
		cfg.width = static_cast<uint32_t>(width);
		cfg.height = static_cast<uint32_t>(height);
	}

	if (obs_data_has_user_value(data, "record")) {
		cfg.record = obs_data_get_bool(data, "record");
	}
	if (obs_data_has_user_value(data, "stream")) {
		cfg.stream = obs_data_get_bool(data, "stream");
	}
	cfg.canvasUuid = obs_data_get_string(data, "canvas_uuid");

	return cfg;
}

obs_data_t *VerticalConfig::ToData() const
{
	obs_data_t *data = obs_data_create();
	obs_data_set_int(data, "version", kVersion);
	obs_data_set_int(data, "width", width);
	obs_data_set_int(data, "height", height);
	obs_data_set_bool(data, "record", record);
	obs_data_set_bool(data, "stream", stream);
	obs_data_set_string(data, "canvas_uuid", canvasUuid.c_str());
	return data;
}

} // namespace OBS::Vertical
