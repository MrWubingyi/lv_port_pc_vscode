#!/usr/bin/env bash
set -euo pipefail

benchmark_dir=${1:-/tmp/dashboard-image-benchmark}
duration=${2:-120}
interval_ms=${3:-200}

for value in "$duration" "$interval_ms"; do
    if [[ ! $value =~ ^[1-9][0-9]*$ ]]; then
        printf 'Duration and interval must be positive integers, got: %s\n' "$value" >&2
        exit 2
    fi
done

output_dir="$benchmark_dir/memory"
mkdir -p "$output_dir"
summary="$output_dir/summary.txt"
printf 'mode,peak_rss_kib,peak_pss_kib,peak_private_kib,peak_anonymous_kib,peak_vmsize_kib,peak_swap_kib\n' > "$summary"

app_pid=''
cleanup() {
    if [[ -n $app_pid ]] && kill -0 "$app_pid" 2>/dev/null; then
        kill -TERM "$app_pid" 2>/dev/null || true
        wait "$app_pid" 2>/dev/null || true
    fi
}
trap cleanup EXIT INT TERM

for mode in c_array png; do
    executable="$benchmark_dir/$mode/main"
    csv="$output_dir/$mode.csv"
    if [[ ! -x $executable ]]; then
        printf 'Missing %s. Run tools/benchmark_png_resources.sh first.\n' "$executable" >&2
        exit 2
    fi

    printf '\n[%s] sampling memory every %s ms for %s seconds\n' \
        "$mode" "$interval_ms" "$duration"
    printf 'elapsed_ms,rss_kib,pss_kib,private_kib,anonymous_kib,vmsize_kib,swap_kib\n' > "$csv"

    "$executable" &
    app_pid=$!
    start_ms=$(date +%s%3N)
    end_ms=$((start_ms + duration * 1000))

    while kill -0 "$app_pid" 2>/dev/null; do
        now_ms=$(date +%s%3N)
        if ((now_ms >= end_ms)); then
            break
        fi

        if [[ -r /proc/$app_pid/smaps_rollup ]]; then
            read -r rss pss private anonymous swap < <(
                awk '
                    /^Rss:/ { rss=$2 }
                    /^Pss:/ { pss=$2 }
                    /^Private_Clean:/ { pc=$2 }
                    /^Private_Dirty:/ { pd=$2 }
                    /^Anonymous:/ { anon=$2 }
                    /^Swap:/ { swap=$2 }
                    END { print rss+0, pss+0, pc+pd, anon+0, swap+0 }
                ' "/proc/$app_pid/smaps_rollup"
            )
            vmsize=$(awk '/^VmSize:/ { print $2; exit }' "/proc/$app_pid/status")
            printf '%d,%d,%d,%d,%d,%d,%d\n' \
                "$((now_ms - start_ms))" "$rss" "$pss" "$private" \
                "$anonymous" "${vmsize:-0}" "$swap" >> "$csv"
        fi
        sleep "$(printf '%d.%03d' "$((interval_ms / 1000))" "$((interval_ms % 1000))")"
    done

    cleanup
    app_pid=''

    peaks=$(awk -F, '
        NR > 1 {
            for(i=2; i<=7; i++) if($i > max[i]) max[i]=$i
        }
        END { print max[2]+0, max[3]+0, max[4]+0, max[5]+0, max[6]+0, max[7]+0 }
    ' "$csv")
    read -r peak_rss peak_pss peak_private peak_anonymous peak_vmsize peak_swap <<< "$peaks"
    printf '%s,%s,%s,%s,%s,%s,%s\n' "$mode" "$peak_rss" "$peak_pss" \
        "$peak_private" "$peak_anonymous" "$peak_vmsize" "$peak_swap" >> "$summary"
    printf '  peak RSS=%s KiB, PSS=%s KiB, private=%s KiB, anonymous=%s KiB, swap=%s KiB\n' \
        "$peak_rss" "$peak_pss" "$peak_private" "$peak_anonymous" "$peak_swap"
done

printf '\nMemory results written to %s\n' "$output_dir"
