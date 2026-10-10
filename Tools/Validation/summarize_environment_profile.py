"""Summarize a completed UE CSV capture without treating unavailable counters as zero."""
import argparse
import csv
import json
import math
import statistics
from pathlib import Path


def summarize(source, expected_frames, warmup_frames, null_rhi):
    with source.open(encoding="utf-8-sig", newline="") as stream:
        rows = list(csv.reader(stream))
    if not rows or "FrameTime" not in rows[0]:
        raise ValueError("CSV has no FrameTime column")
    header = rows[0]
    frame_index = header.index("FrameTime")
    frames = []
    for row in rows[1:]:
        try:
            value = float(row[frame_index])
        except (ValueError, IndexError):
            continue  # UE repeats the header and appends metadata after frame rows.
        if not math.isfinite(value) or value < 0:
            raise ValueError("FrameTime must be finite and nonnegative")
        frames.append(row)
    if len(frames) != expected_frames:
        raise ValueError(f"Expected {expected_frames} frames, found {len(frames)}")
    if not 0 <= warmup_frames < len(frames):
        raise ValueError("Warmup must leave at least one measured frame")
    measured = frames[warmup_frames:]
    metrics = {}
    for name in (
        "FrameTime", "GameThreadTime", "GameThreadTime_CriticalPath", "RenderThreadTime",
        "RHIThreadTime", "GPUTime", "Exclusive/GameThread/Tickables",
        "Exclusive/GameThread/CCLWorldAdvance", "Exclusive/GameThread/CCLSurfaceAdvance",
        "Exclusive/GameThread/CCLEcologyAdvance",
    ):
        if name not in header or (null_rhi and name in ("RenderThreadTime", "RHIThreadTime", "GPUTime")):
            metrics[name] = None
            continue
        index = header.index(name)
        try:
            values = [float(row[index]) for row in measured]
        except (ValueError, IndexError) as exc:
            raise ValueError(f"Incomplete numeric samples for {name}") from exc
        if any(not math.isfinite(value) or value < 0 for value in values):
            raise ValueError(f"Invalid samples for {name}")
        if name in ("GameThreadTime", "GameThreadTime_CriticalPath", "RenderThreadTime", "RHIThreadTime", "GPUTime") and not any(values):
            metrics[name] = None
            continue
        values.sort()
        metrics[name] = {
            "mean": statistics.mean(values),
            "p50": values[math.ceil(len(values) * .50) - 1],
            "p95": values[math.ceil(len(values) * .95) - 1],
            "p99": values[math.ceil(len(values) * .99) - 1],
            "max": values[-1],
        }
    world_time = {}
    for name in ("CCLWorldCompletedSeconds", "CCLWorldPendingGameSeconds", "CCLWorldUnqueuedGameSeconds"):
        if name not in header:
            world_time[name] = None
            continue
        index = header.index(name)
        values = [float(row[index]) for row in measured]
        if any(not math.isfinite(value) or value < 0 for value in values):
            raise ValueError(f"Invalid samples for {name}")
        world_time[name] = {"first": values[0], "last": values[-1], "min": min(values), "max": max(values)}
    return {
        "capture_frames": len(frames), "warmup_frames_discarded": warmup_frames,
        "measured_frames": len(measured), "null_rhi": null_rhi,
        "percentile_method": "nearest_rank", "metrics_milliseconds": metrics, "world_time_seconds": world_time,
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--csv", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--expected-frames", type=int, required=True)
    parser.add_argument("--warmup-frames", type=int, default=30)
    parser.add_argument("--null-rhi", action="store_true")
    args = parser.parse_args()
    result = summarize(args.csv, args.expected_frames, args.warmup_frames, args.null_rhi)
    args.output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(f"PROFILE SUMMARY {result['measured_frames']} measured frames: {args.output}")


if __name__ == "__main__":
    main()
