#pragma once

#include <obs-data.h>
#include <obs.h>

#include <cstdint>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace OBS::Vertical {

struct VerticalConfig {
	static constexpr int64_t kVersion = 1;
	static constexpr uint32_t kDefaultWidth = 1080;
	static constexpr uint32_t kDefaultHeight = 1920;

	uint32_t width = kDefaultWidth;
	uint32_t height = kDefaultHeight;
	bool record = true;
	bool stream = false;
	std::string canvasUuid;

	static VerticalConfig FromData(obs_data_t *data);
	obs_data_t *ToData() const;
};

inline constexpr const char *kAitumCanvasName = "Aitum Vertical";

struct CanvasRef {
	std::string name;
	std::string uuid;
};

enum class CanvasAction { UseExisting, AdoptAitum, Create };

struct CanvasResolution {
	CanvasAction action;
	std::string uuid;
};

/* Decides which canvas backs the vertical view after a collection is loaded. */
CanvasResolution ResolveCanvas(const VerticalConfig &cfg, const std::vector<CanvasRef> &canvases);

/* Video settings for the vertical canvas: timing and color follow the main canvas,
 * resolution comes from the vertical config (no downscaling). */
obs_video_info MakeVideoInfo(const obs_video_info &main, const VerticalConfig &cfg);

/* Returns base, or "base N" with the smallest N >= 2 that is not taken. */
std::string UniqueName(const std::string &base, const std::vector<std::string> &taken);

/* File path of the vertical recording next to the main recording:
 * "/dir/name.mkv" -> "/dir/name-vertical.mkv". */
std::string VerticalRecordingPath(const std::string &mainPath);

/* Links main canvas scenes to vertical scenes (by UUID), so switching the main
 * program scene can switch the vertical canvas too. Several main scenes may
 * share one vertical scene. */
class ScenePairs {
public:
	void Set(const std::string &mainUuid, const std::string &verticalUuid);
	void Unset(const std::string &mainUuid);
	[[nodiscard]] std::optional<std::string> Lookup(const std::string &mainUuid) const;

	/* Drop every pair that references the scene on either side. */
	void RemoveScene(const std::string &uuid);
	/* Drop pairs whose scenes no longer exist. */
	void Prune(const std::function<bool(const std::string &uuid)> &exists);

	[[nodiscard]] size_t Size() const noexcept { return pairs.size(); }

	[[nodiscard]] obs_data_array_t *ToData() const;
	static ScenePairs FromData(obs_data_array_t *array);

private:
	std::map<std::string, std::string> pairs;
};

} // namespace OBS::Vertical
