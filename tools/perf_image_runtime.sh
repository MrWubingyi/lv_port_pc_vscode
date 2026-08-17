#!/usr/bin/env bash
set -euo pipefail

project_dir=$(cd "$(dirname "$0")/.." && pwd)
benchmark_dir=${1:-/tmp/dashboard-image-benchmark}
stat_duration=${2:-120}
record_duration=${3:-30}
events='task-clock,context-switches,cpu-migrations,page-faults,cycles:u,instructions:u,branches:u,branch-misses:u,cache-references:u,cache-misses:u'

for value in "$stat_duration" "$record_duration"; do
    if [[ ! $value =~ ^[1-9][0-9]*$ ]]; then
        printf 'Durations must be positive integers, got: %s\n' "$value" >&2
        exit 2
    fi
done

if ! command -v perf >/dev/null 2>&1; then
    printf 'perf is not installed. Install the linux-tools package for the running kernel.\n' >&2
    exit 2
fi

probe_error=$(mktemp)
trap 'rm -f "$probe_error"' EXIT
if ! perf stat -e task-clock true 2>"$probe_error"; then
    cat "$probe_error" >&2
    printf '\nCurrent kernel.perf_event_paranoid: %s\n' "$(cat /proc/sys/kernel/perf_event_paranoid)" >&2
    printf 'Temporarily permit unprivileged profiling, then rerun:\n' >&2
    printf '  sudo sysctl -w kernel.perf_event_paranoid=1\n' >&2
    printf 'Restore the original value after testing, commonly with:\n' >&2
    printf '  sudo sysctl -w kernel.perf_event_paranoid=4\n' >&2
    exit 2
fi

mkdir -p "$benchmark_dir/perf"
summary="$benchmark_dir/perf/summary.txt"
{
    printf 'Dashboard image perf comparison\n'
    printf 'Recorded at: %s\n' "$(date --iso-8601=seconds)"
    printf 'perf stat duration: %s seconds per mode\n' "$stat_duration"
    printf 'perf record duration: %s seconds per mode\n' "$record_duration"
    printf 'Events: %s\n' "$events"
} > "$summary"

run_timed() {
    local mode=$1
    local phase=$2
    shift 2
    set +e
    "$@"
    local status=$?
    set -e
    if [[ $status -ne 0 && $status -ne 124 ]]; then
        printf '%s %s failed with status %d\n' "$mode" "$phase" "$status" >&2
        exit "$status"
    fi
}

for mode in c_array png; do
    executable="$benchmark_dir/$mode/main"
    mode_dir="$benchmark_dir/perf/$mode"
    if [[ ! -x $executable ]]; then
        printf 'Missing %s. Run %s/tools/benchmark_png_resources.sh first.\n' \
            "$executable" "$project_dir" >&2
        exit 2
    fi
    mkdir -p "$mode_dir"

    printf '\n[%s] perf stat: %s seconds\n' "$mode" "$stat_duration"
    run_timed "$mode" stat perf stat \
        --no-big-num -x ';' -e "$events" \
        -o "$mode_dir/stat.csv" -- \
        timeout --signal=TERM "${stat_duration}s" "$executable"

    printf '[%s] perf record: %s seconds\n' "$mode" "$record_duration"
    run_timed "$mode" record perf record \
        -F 99 -g --call-graph fp \
        -o "$mode_dir/perf.data" -- \
        timeout --signal=TERM "${record_duration}s" "$executable"

    perf report --stdio --no-children --percent-limit 0.5 \
        -i "$mode_dir/perf.data" > "$mode_dir/report.txt"
    perf report --stdio --children --percent-limit 0.5 \
        -i "$mode_dir/perf.data" > "$mode_dir/report-children.txt"

    {
        printf '\n[%s counters]\n' "$mode"
        cat "$mode_dir/stat.csv"
        printf '\n[%s hottest self-time symbols]\n' "$mode"
        sed -n '1,45p' "$mode_dir/report.txt"
    } >> "$summary"
done

printf '\nResults written to %s/perf\n' "$benchmark_dir"
printf 'Summary: %s\n' "$summary"
printf 'Interactive inspection examples:\n'
printf '  perf report -i %s/perf/c_array/perf.data\n' "$benchmark_dir"
printf '  perf report -i %s/perf/png/perf.data\n' "$benchmark_dir"
