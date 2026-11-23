#pragma once

#include "VerticalModel.hpp"

#include <utility/OBSCanvas.hpp>

#include <obs.hpp>

#include <functional>
#include <string>
#include <vector>

namespace OBS::Vertical {

/* Owns the frontend's native vertical canvas: its video mix, its current scene
 * and its persisted configuration. Lives as long as OBSBasic. */
class VerticalCanvas {
public:
	using AddCanvasFn = std::function<obs_canvas_t *(const std::string &name, obs_video_info *ovi, int flags)>;

	static constexpr const char *kCanvasName = "Vertical";
	static constexpr const char *kDefaultSceneName = "Vertical Scene";
	static constexpr uint32_t kCanvasFlags = ACTIVATE | SCENE_REF;

	/* Called after the collection's canvases were restored and before its
	 * sources are loaded, so scene items are placed on a correctly sized canvas. */
	void LoadCanvas(obs_data_t *collection, const std::vector<OBS::Canvas> &canvases, const AddCanvasFn &addCanvas);

	/* Called after the collection's sources were loaded. */
	void FinishLoad();

	/* Re-apply the vertical video mix, e.g. after the main video was reset. */
	void ApplyVideo();

	/* Release all references before the collection's canvases are removed. */
	void Clear();

	[[nodiscard]] OBSDataAutoRelease Save() const;

	[[nodiscard]] OBSCanvas GetCanvas() const;
	[[nodiscard]] OBSSource GetCurrentScene() const;
	void SetCurrentScene(obs_source_t *scene);

	[[nodiscard]] const VerticalConfig &Config() const noexcept { return config; }

private:
	VerticalConfig config;
	OBSWeakCanvas canvas;
	OBSWeakSource currentScene;
	std::string pendingSceneUuid;
};

} // namespace OBS::Vertical
