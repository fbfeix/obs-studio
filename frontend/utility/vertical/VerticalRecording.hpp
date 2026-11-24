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
	struct Session {
		OBSOutputAutoRelease output;
		OBSEncoderAutoRelease videoEncoder;
		std::vector<OBSEncoderAutoRelease> audioEncoders;
	};

	/* Drop finished sessions; keep those still writing their encoder backlog. */
	void PruneFinishing();

	Session current;
	/* Stopped gracefully but still draining, so a new recording can start
	 * right away without cutting off the previous file. */
	std::vector<Session> finishing;
};

} // namespace OBS::Vertical
