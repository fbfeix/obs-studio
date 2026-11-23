#include "VerticalRecording.hpp"
#include "VerticalModel.hpp"

#include <util/base.h>

#include <string>

namespace OBS::Vertical {

bool VerticalRecording::Start(obs_output_t *mainOutput, obs_canvas_t *canvas)
{
	if (Active()) {
		return true;
	}
	Reset();
	if (!mainOutput || !canvas || !obs_canvas_has_video(canvas)) {
		blog(LOG_WARNING, "[vertical] Recording not started: no main recording or vertical canvas");
		return false;
	}

	obs_encoder_t *mainVideo = obs_output_get_video_encoder(mainOutput);
	if (!mainVideo) {
		blog(LOG_WARNING, "[vertical] Recording not started: main recording has no video encoder");
		return false;
	}

	OBSDataAutoRelease mainSettings = obs_output_get_settings(mainOutput);
	std::string mainPath = obs_data_get_string(mainSettings, "path");
	if (mainPath.empty()) {
		blog(LOG_WARNING, "[vertical] Recording not started: main recording has no file path (URL output?)");
		return false;
	}

	OBSDataAutoRelease encoderSettings = obs_encoder_get_settings(mainVideo);
	videoEncoder = obs_video_encoder_create(obs_encoder_get_id(mainVideo), "vertical_recording_video",
						encoderSettings, nullptr);
	if (!videoEncoder) {
		blog(LOG_WARNING, "[vertical] Failed to create video encoder '%s'", obs_encoder_get_id(mainVideo));
		return false;
	}
	obs_encoder_set_video(videoEncoder, obs_canvas_get_video(canvas));

	OBSDataAutoRelease outputSettings = obs_data_create();
	obs_data_apply(outputSettings, mainSettings);
	std::string path = VerticalRecordingPath(mainPath);
	obs_data_set_string(outputSettings, "path", path.c_str());

	output = obs_output_create(obs_output_get_id(mainOutput), "vertical_recording", outputSettings, nullptr);
	if (!output) {
		blog(LOG_WARNING, "[vertical] Failed to create recording output '%s'", obs_output_get_id(mainOutput));
		videoEncoder = nullptr;
		return false;
	}

	obs_output_set_video_encoder(output, videoEncoder);

	audioEncoders.clear();
	for (size_t i = 0; i < MAX_OUTPUT_AUDIO_ENCODERS; i++) {
		obs_encoder_t *mainAudio = obs_output_get_audio_encoder(mainOutput, i);
		if (!mainAudio) {
			continue;
		}

		OBSDataAutoRelease audioSettings = obs_encoder_get_settings(mainAudio);
		std::string name = "vertical_recording_audio_" + std::to_string(i);
		OBSEncoderAutoRelease audio = obs_audio_encoder_create(obs_encoder_get_id(mainAudio), name.c_str(),
								       audioSettings,
								       obs_encoder_get_mixer_index(mainAudio), nullptr);
		if (!audio) {
			blog(LOG_WARNING, "[vertical] Failed to create audio encoder for track %zu", i);
			continue;
		}
		obs_encoder_set_audio(audio, obs_get_audio());
		obs_output_set_audio_encoder(output, audio, i);
		audioEncoders.emplace_back(std::move(audio));
	}

	if (!obs_output_start(output)) {
		const char *error = obs_output_get_last_error(output);
		blog(LOG_WARNING, "[vertical] Failed to start recording: %s", error ? error : "unknown error");
		return false;
	}

	blog(LOG_INFO, "[vertical] Recording started: %s", path.c_str());
	return true;
}

void VerticalRecording::Stop(bool force)
{
	if (!output) {
		return;
	}

	if (obs_output_active(output)) {
		if (force) {
			obs_output_force_stop(output);
		} else {
			obs_output_stop(output);
		}
		blog(LOG_INFO, "[vertical] Recording stopped");
	}
}

void VerticalRecording::SetPaused(bool paused)
{
	if (Active() && obs_output_paused(output) != paused) {
		obs_output_pause(output, paused);
	}
}

void VerticalRecording::Reset()
{
	Stop(true);
	output = nullptr;
	videoEncoder = nullptr;
	audioEncoders.clear();
}

bool VerticalRecording::Active() const
{
	return output && obs_output_active(output);
}

const char *VerticalRecording::LastError() const
{
	return output ? obs_output_get_last_error(output) : nullptr;
}

} // namespace OBS::Vertical
