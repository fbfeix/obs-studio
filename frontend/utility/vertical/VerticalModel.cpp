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

void ScenePairs::Set(const std::string &mainUuid, const std::string &verticalUuid)
{
	if (mainUuid.empty()) {
		return;
	}
	if (verticalUuid.empty()) {
		Unset(mainUuid);
		return;
	}
	pairs[mainUuid] = verticalUuid;
}

void ScenePairs::Unset(const std::string &mainUuid)
{
	pairs.erase(mainUuid);
}

std::optional<std::string> ScenePairs::Lookup(const std::string &mainUuid) const
{
	auto it = pairs.find(mainUuid);
	if (it == pairs.end()) {
		return std::nullopt;
	}
	return it->second;
}

void ScenePairs::RemoveScene(const std::string &uuid)
{
	Prune([&uuid](const std::string &candidate) { return candidate != uuid; });
}

void ScenePairs::Prune(const std::function<bool(const std::string &)> &exists)
{
	for (auto it = pairs.begin(); it != pairs.end();) {
		if (!exists(it->first) || !exists(it->second)) {
			it = pairs.erase(it);
		} else {
			++it;
		}
	}
}

obs_data_array_t *ScenePairs::ToData() const
{
	obs_data_array_t *array = obs_data_array_create();
	for (const auto &[mainUuid, verticalUuid] : pairs) {
		obs_data_t *entry = obs_data_create();
		obs_data_set_string(entry, "main", mainUuid.c_str());
		obs_data_set_string(entry, "vertical", verticalUuid.c_str());
		obs_data_array_push_back(array, entry);
		obs_data_release(entry);
	}
	return array;
}

ScenePairs ScenePairs::FromData(obs_data_array_t *array)
{
	ScenePairs result;
	if (!array) {
		return result;
	}

	size_t count = obs_data_array_count(array);
	for (size_t i = 0; i < count; i++) {
		obs_data_t *entry = obs_data_array_item(array, i);
		result.Set(obs_data_get_string(entry, "main"), obs_data_get_string(entry, "vertical"));
		obs_data_release(entry);
	}
	return result;
}

namespace {
std::string Trimmed(const char *text)
{
	std::string value = text ? text : "";
	constexpr const char *kWhitespace = " \t\r\n";
	size_t first = value.find_first_not_of(kWhitespace);
	if (first == std::string::npos) {
		return {};
	}
	size_t last = value.find_last_not_of(kWhitespace);
	return value.substr(first, last - first + 1);
}

bool HasScheme(const std::string &url, const char *scheme)
{
	return url.rfind(scheme, 0) == 0;
}
} // namespace

VerticalStreamSettings VerticalStreamSettings::FromData(obs_data_t *data)
{
	VerticalStreamSettings settings;
	if (!data) {
		return settings;
	}

	settings.enabled = obs_data_get_bool(data, "enabled");
	settings.server = Trimmed(obs_data_get_string(data, "server"));
	settings.key = Trimmed(obs_data_get_string(data, "key"));

	if (obs_data_has_user_value(data, "video_bitrate")) {
		long long bitrate = obs_data_get_int(data, "video_bitrate");
		if (bitrate >= kMinBitrate && bitrate <= kMaxBitrate) {
			settings.videoBitrate = int(bitrate);
		}
	}
	return settings;
}

obs_data_t *VerticalStreamSettings::ToData() const
{
	obs_data_t *data = obs_data_create();
	obs_data_set_bool(data, "enabled", enabled);
	obs_data_set_string(data, "server", server.c_str());
	obs_data_set_string(data, "key", key.c_str());
	obs_data_set_int(data, "video_bitrate", videoBitrate);
	return data;
}

StreamSettingsError VerticalStreamSettings::Validate() const
{
	if (server.empty()) {
		return StreamSettingsError::MissingServer;
	}

	bool rtmp = HasScheme(server, "rtmp://") || HasScheme(server, "rtmps://");
	bool keyInUrl = HasScheme(server, "srt://") || HasScheme(server, "rist://");
	if (!rtmp && !keyInUrl) {
		return StreamSettingsError::UnsupportedProtocol;
	}
	if (rtmp && key.empty()) {
		return StreamSettingsError::MissingKey;
	}
	return StreamSettingsError::None;
}

bool StreamSession::OnMainStarted(bool enabled)
{
	if (Active() || !enabled) {
		return false;
	}
	origin = Origin::WithMain;
	return true;
}

bool StreamSession::OnMainStopped()
{
	if (origin != Origin::WithMain) {
		return false;
	}
	origin = Origin::None;
	return true;
}

bool StreamSession::OnManualToggle()
{
	if (Active()) {
		origin = Origin::None;
		return false;
	}
	origin = Origin::Manual;
	return true;
}

} // namespace OBS::Vertical
