#include "VerticalCanvas.hpp"

#include <util/base.h>

namespace OBS::Vertical {

namespace {
constexpr const char *kCollectionKey = "vertical";
constexpr const char *kCurrentSceneKey = "current_scene_uuid";

bool ResetCanvasVideo(obs_canvas_t *canvas, const VerticalConfig &cfg)
{
	obs_video_info main = {};
	if (!obs_get_video_info(&main)) {
		blog(LOG_WARNING, "[vertical] Main video not initialized, cannot size vertical canvas");
		return false;
	}

	obs_video_info ovi = MakeVideoInfo(main, cfg);
	if (!obs_canvas_reset_video(canvas, &ovi)) {
		blog(LOG_WARNING, "[vertical] Failed to reset vertical canvas video to %ux%u", cfg.width, cfg.height);
		return false;
	}
	return true;
}

OBSSource FirstScene(obs_canvas_t *canvas)
{
	OBSSource first;
	auto cb = [](void *param, obs_source_t *source) {
		*static_cast<OBSSource *>(param) = source;
		return false;
	};
	obs_canvas_enum_scenes(canvas, cb, &first);
	return first;
}
} // namespace

void VerticalCanvas::LoadCanvas(obs_data_t *collection, const std::vector<OBS::Canvas> &canvases,
				const AddCanvasFn &addCanvas)
{
	OBSDataAutoRelease data = obs_data_get_obj(collection, kCollectionKey);
	config = VerticalConfig::FromData(data);
	pendingSceneUuid = data ? obs_data_get_string(data, kCurrentSceneKey) : "";

	std::vector<CanvasRef> refs;
	refs.reserve(canvases.size());
	for (const auto &existing : canvases) {
		refs.push_back({obs_canvas_get_name(existing), obs_canvas_get_uuid(existing)});
	}

	CanvasResolution resolution = ResolveCanvas(config, refs);

	OBSCanvas target;
	switch (resolution.action) {
	case CanvasAction::UseExisting:
	case CanvasAction::AdoptAitum:
		target = OBSCanvasAutoRelease(obs_get_canvas_by_uuid(resolution.uuid.c_str())).Get();
		if (target && resolution.action == CanvasAction::AdoptAitum) {
			blog(LOG_INFO, "[vertical] Adopting Aitum Vertical canvas %s", resolution.uuid.c_str());
		}
		if (target && !ResetCanvasVideo(target, config)) {
			target = nullptr;
		}
		break;
	case CanvasAction::Create: {
		obs_video_info main = {};
		if (obs_get_video_info(&main)) {
			obs_video_info ovi = MakeVideoInfo(main, config);
			target = addCanvas(kCanvasName, &ovi, kCanvasFlags);
		}
		break;
	}
	}

	if (!target) {
		blog(LOG_WARNING, "[vertical] No vertical canvas available");
		canvas = nullptr;
		config.canvasUuid.clear();
		return;
	}

	canvas = OBSGetWeakRef(target);
	config.canvasUuid = obs_canvas_get_uuid(target);
}

void VerticalCanvas::FinishLoad()
{
	OBSCanvas target = GetCanvas();
	if (!target) {
		return;
	}

	OBSSourceAutoRelease restored = pendingSceneUuid.empty() ? nullptr
								 : obs_get_source_by_uuid(pendingSceneUuid.c_str());
	pendingSceneUuid.clear();

	OBSSource scene = restored.Get();
	OBSCanvasAutoRelease sceneCanvas = scene ? obs_source_get_canvas(scene) : nullptr;
	if (!scene || sceneCanvas.Get() != target.Get()) {
		scene = FirstScene(target);
	}
	if (!scene) {
		OBSSceneAutoRelease created = obs_canvas_scene_create(target, kDefaultSceneName);
		scene = obs_scene_get_source(created);
	}

	SetCurrentScene(scene);
}

std::vector<OBSSource> VerticalCanvas::GetScenes() const
{
	std::vector<OBSSource> scenes;
	OBSCanvas target = GetCanvas();
	if (!target) {
		return scenes;
	}

	auto cb = [](void *param, obs_source_t *source) {
		static_cast<std::vector<OBSSource> *>(param)->emplace_back(source);
		return true;
	};
	obs_canvas_enum_scenes(target, cb, &scenes);
	return scenes;
}

OBSSource VerticalCanvas::CreateScene(const std::string &baseName)
{
	OBSCanvas target = GetCanvas();
	if (!target) {
		return nullptr;
	}

	std::vector<std::string> taken;
	for (const auto &scene : GetScenes()) {
		taken.emplace_back(obs_source_get_name(scene));
	}

	OBSSceneAutoRelease created = obs_canvas_scene_create(target, UniqueName(baseName, taken).c_str());
	OBSSource source = obs_scene_get_source(created);
	SetCurrentScene(source);
	return source;
}

void VerticalCanvas::RemoveScene(obs_source_t *scene)
{
	std::vector<OBSSource> scenes = GetScenes();
	if (!scene || scenes.size() <= 1) {
		return;
	}

	if (GetCurrentScene().Get() == scene) {
		for (const auto &other : scenes) {
			if (other.Get() != scene) {
				SetCurrentScene(other);
				break;
			}
		}
	}

	obs_source_remove(scene);
	NotifyChanged();
}

void VerticalCanvas::NotifyChanged() const
{
	if (changed) {
		changed();
	}
}

void VerticalCanvas::ApplyVideo()
{
	OBSCanvas target = GetCanvas();
	if (target) {
		ResetCanvasVideo(target, config);
	}
}

void VerticalCanvas::Clear()
{
	OBSCanvas target = GetCanvas();
	if (target) {
		obs_canvas_set_channel(target, 0, nullptr);
	}
	currentScene = nullptr;
	canvas = nullptr;
	pendingSceneUuid.clear();
	config = VerticalConfig{};
	NotifyChanged();
}

OBSDataAutoRelease VerticalCanvas::Save() const
{
	OBSDataAutoRelease data = config.ToData();
	OBSSource scene = GetCurrentScene();
	obs_data_set_string(data, kCurrentSceneKey, scene ? obs_source_get_uuid(scene) : "");
	return data;
}

OBSCanvas VerticalCanvas::GetCanvas() const
{
	return OBSGetStrongRef(canvas);
}

OBSSource VerticalCanvas::GetCurrentScene() const
{
	return OBSGetStrongRef(currentScene);
}

void VerticalCanvas::SetCurrentScene(obs_source_t *scene)
{
	OBSCanvas target = GetCanvas();
	if (!target || !scene) {
		return;
	}

	currentScene = OBSGetWeakRef(scene);
	obs_canvas_set_channel(target, 0, scene);
	NotifyChanged();
}

} // namespace OBS::Vertical
