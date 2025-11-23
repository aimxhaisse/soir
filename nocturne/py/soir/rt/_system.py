from soir._bindings.rt import (
    start_recording_,
    stop_recording_,
)

# Incremented at each buffer evaluation, used to automatically stop
# recording when record() isn't called in the current evaluation.
eval_id_ = 0
current_recording_file_ = None
recording_eval_id_ = None


def _reset() -> None:
    """Helper to reset system state for integration tests."""
    global eval_id_, current_recording_file_, recording_eval_id_

    eval_id_ = 0
    current_recording_file_ = None
    recording_eval_id_ = None


def record_(file_path: str) -> bool:
    """Internal recording function.

    Args:
        file_path: Path to the WAV file where audio will be recorded.

    Returns:
        True if recording started/continued successfully, False otherwise.
    """
    global eval_id_, current_recording_file_, recording_eval_id_

    # If we're recording to a different file, start new recording
    if current_recording_file_ != file_path:
        if current_recording_file_ is not None:
            stop_recording_()
        start_recording_(file_path)
        current_recording_file_ = file_path

    # Update the eval_id to indicate recording is active this cycle
    recording_eval_id_ = eval_id_

    return True


def post_eval_() -> None:
    """Called after each buffer evaluation.

    Automatically stops recording if record() wasn't called in this evaluation.
    """
    global eval_id_, current_recording_file_, recording_eval_id_

    # If recording was active but wasn't called this evaluation, stop it
    if current_recording_file_ is not None and recording_eval_id_ != eval_id_:
        stop_recording_()
        current_recording_file_ = None
        recording_eval_id_ = None

    eval_id_ += 1
