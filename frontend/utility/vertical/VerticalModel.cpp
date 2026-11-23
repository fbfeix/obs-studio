#include "VerticalModel.hpp"

#include <algorithm>

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

CanvasResolution ResolveCanvas(const VerticalConfig &cfg, const std::vector<CanvasRef> &canvases)
{
	if (!cfg.canvasUuid.empty()) {
		for (const auto &canvas : canvases) {
			if (canvas.uuid == cfg.canvasUuid) {
				return {CanvasAction::UseExisting, canvas.uuid};
			}
		}
	}

	for (const auto &canvas : canvases) {
		if (canvas.name == kAitumCanvasName) {
			return {CanvasAction::AdoptAitum, canvas.uuid};
		}
	}

	return {CanvasAction::Create, {}};
}

obs_video_info MakeVideoInfo(const obs_video_info &main, const VerticalConfig &cfg)
{
	obs_video_info ovi = main;
	ovi.base_width = cfg.width;
	ovi.base_height = cfg.height;
	ovi.output_width = cfg.width;
	ovi.output_height = cfg.height;
	return ovi;
}

std::string UniqueName(const std::string &base, const std::vector<std::string> &taken)
{
	auto isTaken = [&taken](const std::string &name) {
		return std::find(taken.begin(), taken.end(), name) != taken.end();
	};

	if (!isTaken(base)) {
		return base;
	}

	for (int i = 2;; i++) {
		std::string candidate = base + " " + std::to_string(i);
		if (!isTaken(candidate)) {
			return candidate;
		}
	}
}

std::string VerticalRecordingPath(const std::string &mainPath)
{
	if (mainPath.empty()) {
		return {};
	}

	constexpr const char *kSuffix = "-vertical";
	size_t nameStart = mainPath.find_last_of("/\\");
	nameStart = nameStart == std::string::npos ? 0 : nameStart + 1;

	size_t dot = mainPath.find_last_of('.');
	bool hasExtension = dot != std::string::npos && dot > nameStart;
	if (!hasExtension) {
		return mainPath + kSuffix;
	}
	return mainPath.substr(0, dot) + kSuffix + mainPath.substr(dot);
}

} // namespace OBS::Vertical
