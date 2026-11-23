#pragma once

#include <obs.hpp>

#include <vector>

namespace OBS::Vertical {

/* Records the vertical canvas alongside the main recording. It mirrors the
 * main recording's output type, container, encoders and audio tracks (with its
 * own encoder instances, so pausing stays in sync per output) and writes
 * "<name>-vertical.<ext>" next to it. */
class VerticalRecording {
public:
	bool Start(obs_output_t *mainOutput, obs_canvas_t *canvas);
	void Stop(bool force);
	void SetPaused(bool paused);
	/* Release the output and encoders; required before libobs shuts down. */
	void Reset();
	[[nodiscard]] bool Active() const;
	[[nodiscard]] const char *LastError() const;

private:
	OBSOutputAutoRelease output;
	OBSEncoderAutoRelease videoEncoder;
	std::vector<OBSEncoderAutoRelease> audioEncoders;
};

} // namespace OBS::Vertical
