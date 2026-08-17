# 使用 perf 对比 PNG 与 C 数组图片

## 关闭 LVGL 断言

基准构建通过以下 CMake 参数关闭 LVGL 的五类运行时断言：

```sh
-DLVGL_ENABLE_ASSERTS=OFF
```

它会将 `LV_USE_ASSERT_NULL`、`LV_USE_ASSERT_MALLOC`、
`LV_USE_ASSERT_STYLE`、`LV_USE_ASSERT_MEM_INTEGRITY` 和
`LV_USE_ASSERT_OBJ` 全部设为 `0`。普通开发构建仍默认启用断言；只有基准脚本
显式关闭，以免降低日常错误检测能力。

## 构建可分析的 Release 版本

```sh
./tools/benchmark_png_resources.sh
```

脚本同时构建 C 数组和 PNG Release 版本，关闭断言，并启用
`-g -fno-omit-frame-pointer`。优化级别仍为 `-O3`，调试符号只用于解析 perf
热点，不会将程序变成 Debug 构建。

## perf 权限

先检查：

```sh
cat /proc/sys/kernel/perf_event_paranoid
```

当前系统值为 `4` 时，普通用户不能使用 perf。可在测试期间临时调整：

```sh
sudo sysctl -w kernel.perf_event_paranoid=1
```

测试结束后恢复原值：

```sh
sudo sysctl -w kernel.perf_event_paranoid=4
```

该设置影响整台机器的性能观测权限，应只在可信的测试环境中调整。

## 运行详细对比

```sh
./tools/perf_image_runtime.sh /tmp/dashboard-image-benchmark 120 30
```

第二个参数是每个模式执行 `perf stat` 的秒数；第三个参数是每个模式执行
采样热点分析的秒数。每个模式会运行两次，因此上述测试总计约 300 秒。

采集内容包括：

- `task-clock`、上下文切换、CPU 迁移和缺页；
- CPU cycles、instructions，以及 IPC；
- branches、branch misses 和分支失败率；
- cache references、cache misses 和缓存失败率；
- 99 Hz 调用栈采样及函数 self-time/children-time。

结果目录：

```text
/tmp/dashboard-image-benchmark/perf/summary.txt
/tmp/dashboard-image-benchmark/perf/c_array/stat.csv
/tmp/dashboard-image-benchmark/perf/c_array/perf.data
/tmp/dashboard-image-benchmark/perf/c_array/report.txt
/tmp/dashboard-image-benchmark/perf/c_array/report-children.txt
/tmp/dashboard-image-benchmark/perf/png/stat.csv
/tmp/dashboard-image-benchmark/perf/png/perf.data
/tmp/dashboard-image-benchmark/perf/png/report.txt
/tmp/dashboard-image-benchmark/perf/png/report-children.txt
```

交互查看调用栈：

```sh
perf report -i /tmp/dashboard-image-benchmark/perf/png/perf.data
```

PNG 报告中应重点关注 `lodepng`、`decode_png_data`、图像缓存、内存分配和
`lv_draw_image` 相关热点。为了得到可复现结果，应使用相同的数据回放文件、
关闭逐条日志，并至少执行五轮后比较中位数。

## 详细内存对比

`perf stat` 的 `page-faults` 不是内存占用。使用同一批 Release、关闭断言的
二进制执行：

```sh
./tools/memory_image_runtime.sh /tmp/dashboard-image-benchmark 120 200
```

参数分别是每个模式的运行秒数和采样间隔毫秒数。脚本读取进程的
`smaps_rollup`，记录：

- RSS：进程当前驻留的物理页总量；
- PSS：共享页按使用者比例分摊后的物理内存；
- Private：只属于当前进程的内存；
- Anonymous：堆、栈以及 LVGL 解码缓存等匿名内存；
- VmSize 和 Swap。

输出文件：

```text
/tmp/dashboard-image-benchmark/memory/summary.txt
/tmp/dashboard-image-benchmark/memory/c_array.csv
/tmp/dashboard-image-benchmark/memory/png.csv
```

PSS 和 Private 通常比 RSS 更适合判断图片方案自身的增量，因为 SDL、Mesa
等共享库的页面会完整计入 RSS。
