#pragma once

#include "VerticalModel.hpp"

#include <obs.hpp>

#include <functional>
#include <string>

namespace OBS::Vertical {

/* Streams the vertical canvas to its own destination alongside the main
 * stream. Uses its own encoder instances (based on the main stream's encoder
 * type) and a custom service built from the profile's vertical settings.
 * Failures only affect the vertical stream, never the main stream. */
class VerticalStreaming {
public:
	enum class Status { Connecting, Live, Reconnecting, Stopped, Failed };
	/* Called from libobs threads; the receiver must hop to the UI thread.
	 * On Failed, code is the OBS_OUTPUT_* code (OBS_OUTPUT_ERROR when the
	 * stream could not be set up) and message the output's last error, if any. */
	using StatusFn = std::function<void(Status status, int code, const std::string &message)>;

	/* The template encoders only provide encoder type and settings; null
	 * falls back to x264/AAC. delaySec mirrors the main stream's delay. */
	bool Start(obs_encoder_t *videoTemplate, obs_encoder_t *audioTemplate, obs_canvas_t *canvas,
		   const VerticalStreamSettings &settings, uint32_t delaySec);
	void Stop(bool force);
	/* Release the output, service and encoders; required before libobs shuts down. */
	void Reset();
	[[nodiscard]] bool Active() const;
	[[nodiscard]] const std::string &LastError() const noexcept { return lastError; }

	void SetStatusCallback(StatusFn callback) { statusChanged = std::move(callback); }

private:
	static void OnStart(void *data, calldata_t *params);
	static void OnReconnect(void *data, calldata_t *params);
	static void OnReconnectSuccess(void *data, calldata_t *params);
	static void OnStop(void *data, calldata_t *params);
	void Notify(Status status, int code = OBS_OUTPUT_SUCCESS, const std::string &message = {}) const;
	bool Fail(const std::string &message);

	StatusFn statusChanged;
	std::string lastError;
	OBSServiceAutoRelease service;
	OBSEncoderAutoRelease videoEncoder;
	OBSEncoderAutoRelease audioEncoder;
	OBSOutputAutoRelease output;
	std::vector<OBSSignal> outputSignals;
};

} // namespace OBS::Vertical
