#!/usr/bin/env bash
set -euo pipefail

project_dir=$(cd "$(dirname "$0")/.." && pwd)
benchmark_dir=${1:-/tmp/dashboard-image-benchmark}
duration=${2:-10}
summary="$benchmark_dir/runtime-comparison.txt"

if [[ ! $duration =~ ^[1-9][0-9]*$ ]]; then
    printf 'Duration must be a positive integer, got: %s\n' "$duration" >&2
    exit 2
fi

mkdir -p "$benchmark_dir"
{
    printf 'Dashboard image runtime comparison\n'
    printf 'Recorded at: %s\n' "$(date --iso-8601=seconds)"
    printf 'Duration per mode: %s seconds\n' "$duration"
    printf 'Scenario: idle/static UI; no vehicle data received\n'
} > "$summary"

for mode in c_array png; do
    executable="$benchmark_dir/$mode/main"
    report="$benchmark_dir/$mode/time-v.txt"

    if [[ ! -x $executable ]]; then
        printf 'Missing %s; build both variants first with:\n  %s/tools/benchmark_png_resources.sh %s\n' \
            "$executable" "$project_dir" "$benchmark_dir" >&2
        exit 2
    fi

    printf '\nRunning %-7s for %s seconds: %s\n' "$mode" "$duration" "$executable"
    set +e
    /usr/bin/time -v -o "$report" timeout --signal=TERM "${duration}s" "$executable"
    status=$?
    set -e

    if [[ $status -ne 0 && $status -ne 124 ]]; then
        printf '%s exited unexpectedly with status %d\n' "$mode" "$status" >&2
        exit "$status"
    fi

    awk -F ': ' '
        /User time \(seconds\)/ ||
        /System time \(seconds\)/ ||
        /Percent of CPU/ ||
        /Elapsed \(wall clock\)/ ||
        /Maximum resident set size/ ||
        /Major \(requiring I\/O\) page faults/ ||
        /Minor \(reclaiming a frame\) page faults/ {
            sub(/^[[:space:]]+/, "", $1)
            printf "  %-46s %s\n", $1 ":", $2
        }
    ' "$report"

    {
        printf '\n[%s]\n' "$mode"
        cat "$report"
    } >> "$summary"
done

printf '\nCombined report:\n  %s\n' "$summary"
printf 'Full reports:\n  %s/c_array/time-v.txt\n  %s/png/time-v.txt\n' \
    "$benchmark_dir" "$benchmark_dir"
