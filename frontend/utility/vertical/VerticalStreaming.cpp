#include "VerticalStreaming.hpp"

#include <util/base.h>

namespace OBS::Vertical {

namespace {
constexpr const char *kFallbackVideoEncoder = "obs_x264";
constexpr const char *kFallbackAudioEncoder = "ffmpeg_aac";
constexpr const char *kFallbackOutput = "rtmp_output";
constexpr int kKeyframeIntervalSec = 2;
constexpr int kReconnectRetries = 20;
constexpr int kReconnectDelaySec = 2;
} // namespace

bool VerticalStreaming::Start(obs_encoder_t *mainVideo, obs_encoder_t *mainAudio, obs_canvas_t *canvas,
			      const VerticalStreamSettings &settings, uint32_t delaySec)
{
	if (Active()) {
		return true;
	}
	Reset();
	lastError.clear();

	if (!canvas || !obs_canvas_has_video(canvas)) {
		return Fail("no vertical canvas");
	}

	OBSDataAutoRelease videoSettings = mainVideo ? obs_encoder_get_settings(mainVideo) : obs_data_create();
	OBSDataAutoRelease ownVideoSettings = obs_data_create();
	obs_data_apply(ownVideoSettings, videoSettings);
	obs_data_set_string(ownVideoSettings, "rate_control", "CBR");
	obs_data_set_int(ownVideoSettings, "bitrate", settings.videoBitrate);
	obs_data_set_int(ownVideoSettings, "keyint_sec", kKeyframeIntervalSec);

	const char *videoId = mainVideo ? obs_encoder_get_id(mainVideo) : kFallbackVideoEncoder;
	videoEncoder = obs_video_encoder_create(videoId, "vertical_stream_video", ownVideoSettings, nullptr);
	if (!videoEncoder) {
		return Fail(std::string("cannot create video encoder ") + videoId);
	}
	obs_encoder_set_video(videoEncoder, obs_canvas_get_video(canvas));

	OBSDataAutoRelease audioSettings = mainAudio ? obs_encoder_get_settings(mainAudio) : obs_data_create();
	const char *audioId = mainAudio ? obs_encoder_get_id(mainAudio) : kFallbackAudioEncoder;
	size_t mixer = mainAudio ? obs_encoder_get_mixer_index(mainAudio) : 0;
	audioEncoder = obs_audio_encoder_create(audioId, "vertical_stream_audio", audioSettings, mixer, nullptr);
	if (!audioEncoder) {
		return Fail(std::string("cannot create audio encoder ") + audioId);
	}
	obs_encoder_set_audio(audioEncoder, obs_get_audio());

	OBSDataAutoRelease serviceSettings = obs_data_create();
	obs_data_set_string(serviceSettings, "server", settings.server.c_str());
	obs_data_set_string(serviceSettings, "key", settings.key.c_str());
	service = obs_service_create_private("rtmp_custom", "vertical_stream_service", serviceSettings);
	if (!service) {
		return Fail("cannot create stream service");
	}

	const char *outputType = obs_service_get_preferred_output_type(service);
	if (!outputType) {
		outputType = kFallbackOutput;
	}
	output = obs_output_create(outputType, "vertical_stream", nullptr, nullptr);
	if (!output) {
		return Fail(std::string("cannot create output ") + outputType);
	}

	obs_output_set_service(output, service);
	obs_output_set_video_encoder(output, videoEncoder);
	obs_output_set_audio_encoder(output, audioEncoder, 0);
	obs_output_set_reconnect_settings(output, kReconnectRetries, kReconnectDelaySec);
	obs_output_set_delay(output, delaySec, OBS_OUTPUT_DELAY_PRESERVE);

	signal_handler_t *handler = obs_output_get_signal_handler(output);
	outputSignals.emplace_back(handler, "start", OnStart, this);
	outputSignals.emplace_back(handler, "reconnect", OnReconnect, this);
	outputSignals.emplace_back(handler, "reconnect_success", OnReconnectSuccess, this);
	outputSignals.emplace_back(handler, "stop", OnStop, this);

	Notify(Status::Connecting);
	if (!obs_output_start(output)) {
		const char *error = obs_output_get_last_error(output);
		return Fail(error ? error : "output did not start");
	}

	blog(LOG_INFO, "[vertical] Stream starting (%s, %d kbps)", outputType, settings.videoBitrate);
	return true;
}

void VerticalStreaming::Stop(bool force)
{
	if (!output || !obs_output_active(output)) {
		return;
	}

	if (force) {
		obs_output_force_stop(output);
	} else {
		obs_output_stop(output);
	}
	blog(LOG_INFO, "[vertical] Stream stopping");
}

void VerticalStreaming::Reset()
{
	Stop(true);
	outputSignals.clear();
	output = nullptr;
	service = nullptr;
	videoEncoder = nullptr;
	audioEncoder = nullptr;
}

bool VerticalStreaming::Active() const
{
	return output && obs_output_active(output);
}

void VerticalStreaming::Notify(Status status, int code, const std::string &message) const
{
	if (statusChanged) {
		statusChanged(status, code, message);
	}
}

bool VerticalStreaming::Fail(const std::string &message)
{
	lastError = message;
	blog(LOG_WARNING, "[vertical] Stream not started: %s", message.c_str());
	outputSignals.clear();
	Notify(Status::Failed, OBS_OUTPUT_ERROR, message);
	return false;
}

void VerticalStreaming::OnStart(void *data, calldata_t *)
{
	static_cast<VerticalStreaming *>(data)->Notify(Status::Live);
}

void VerticalStreaming::OnReconnect(void *data, calldata_t *)
{
	static_cast<VerticalStreaming *>(data)->Notify(Status::Reconnecting);
}

void VerticalStreaming::OnReconnectSuccess(void *data, calldata_t *)
{
	static_cast<VerticalStreaming *>(data)->Notify(Status::Live);
}

void VerticalStreaming::OnStop(void *data, calldata_t *params)
{
	auto *self = static_cast<VerticalStreaming *>(data);
	auto code = calldata_int(params, "code");
	const char *error = calldata_string(params, "last_error");

	if (code == OBS_OUTPUT_SUCCESS) {
		blog(LOG_INFO, "[vertical] Stream stopped");
		self->Notify(Status::Stopped);
		return;
	}

	std::string message = error ? error : "";
	blog(LOG_WARNING, "[vertical] Stream ended with error code %d %s", int(code), message.c_str());
	self->Notify(Status::Failed, int(code), message);
}

} // namespace OBS::Vertical
