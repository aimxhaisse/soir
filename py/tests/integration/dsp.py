"""Shared DSP helpers for analysing recorded test audio."""

from pathlib import Path
from typing import cast

import numpy as np
import soundfile as sf  # type: ignore[import-untyped]


def rms(signal: np.ndarray) -> float:
    """Root-mean-square level of a mono signal."""
    return float(np.sqrt(np.mean(np.square(signal))))


def rms_segment(
    signal: np.ndarray, start_s: float, end_s: float, sample_rate: float
) -> float:
    """RMS of the [start_s, end_s] time window of a mono signal."""
    start = int(start_s * sample_rate)
    end = int(end_s * sample_rate)
    return rms(signal[start:end])


def read_wav(path: Path) -> tuple[np.ndarray, int]:
    """Read a WAV file as a float (samples, 2) array plus sample rate."""
    data, sample_rate = cast("tuple[np.ndarray, int]", sf.read(path, always_2d=True))
    return data, sample_rate
