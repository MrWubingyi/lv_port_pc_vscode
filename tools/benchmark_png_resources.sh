#!/usr/bin/env bash
set -euo pipefail

project_dir=$(cd "$(dirname "$0")/.." && pwd)
work_dir=${1:-/tmp/dashboard-image-benchmark}
ccache_dir="$work_dir/ccache"

build_variant() {
    local name=$1
    local use_png=$2
    local build_dir="$work_dir/$name"

    cmake -S "$project_dir" -B "$build_dir" \
        -DCMAKE_BUILD_TYPE=Release \
        -DUI_USE_PNG_ASSETS="$use_png" \
        -DLVGL_ENABLE_ASSERTS=OFF \
        -DUI_PERF_PROFILING=ON \
        -DUI_PNG_CACHE_SIZE=2097152 \
        -DUI_PNG_MEMORY_POOL_SIZE=8388608
    env CCACHE_DIR="$ccache_dir" CCACHE_TEMPDIR="$work_dir/ccache-tmp" \
        cmake --build "$build_dir" --target main -j"$(nproc)"
    cmake -E copy "$project_dir/bin/main" "$build_dir/main"
}

mkdir -p "$work_dir"
build_variant c_array OFF
build_variant png ON

printf '\n%-12s %14s %14s %14s %14s\n' \
    mode file_bytes text data_bss png_files
for mode in c_array png; do
    executable="$work_dir/$mode/main"
    read -r text_size data_size bss_size _ < <(size "$executable" | tail -n 1)
    runtime_assets=0
    if [[ $mode == png ]]; then
        runtime_assets=$(sed -n \
            's/.*UI_PNG([^,]*, "\([^"]*\)").*/\1/p' \
            "$project_dir/app/ui/images.c" | while read -r asset; do
                stat -c '%s' "$project_dir/assets/$asset"
            done | awk '{ total += $1 } END { print total + 0 }')
    fi
    printf '%-12s %14d %14d %14d %14d\n' \
        "$mode" "$(stat -c '%s' "$executable")" "$text_size" \
        "$((data_size + bss_size))" "$runtime_assets"
done

printf '\nBinaries: %s/{c_array,png}/main\n' "$work_dir"
printf 'For a runtime CPU/RSS comparison on a graphical session, run:\n'
printf '%s/tools/measure_image_runtime.sh %s\n' "$project_dir" "$work_dir"
printf 'For detailed hardware-counter and hotspot profiling, run:\n'
printf '%s/tools/perf_image_runtime.sh %s\n' "$project_dir" "$work_dir"
printf 'For detailed RSS/PSS/private-memory sampling, run:\n'
printf '%s/tools/memory_image_runtime.sh %s\n' "$project_dir" "$work_dir"
