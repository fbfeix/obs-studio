#pragma once

#include <obs-data.h>

#include <cstdint>
#include <string>

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

} // namespace OBS::Vertical
