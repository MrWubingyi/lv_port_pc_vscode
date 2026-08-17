# UI 图像资源基准测试

## 详细内存采样

记录于 2026-08-17，使用关闭断言的 Release/perf 二进制。每种模式运行
120 秒，并每隔 200 ms 读取一次 `/proc/<pid>/smaps_rollup`。

| 峰值指标 | C 数组 | PNG | PNG 差异 |
|---|---:|---:|---:|
| RSS | 115,876 KiB | 118,456 KiB | +2,580 KiB (+2.23%) |
| PSS | 75,392 KiB | 77,918 KiB | +2,526 KiB (+3.35%) |
| Private | 63,020 KiB | 65,540 KiB | +2,520 KiB (+4.00%) |
| Anonymous | 46,012 KiB | 50,164 KiB | +4,152 KiB (+9.02%) |
| VmSize | 1,608,632 KiB | 1,614,220 KiB | +5,588 KiB (+0.35%) |
| Swap | 0 KiB | 0 KiB | 0 |

两个进程约在启动后的 1–3 秒达到稳定内存平台，10 秒后的数值与峰值基本
一致。PSS 和 Private 表明，当前配置下 PNG 实际增加约 2.5 MiB 物理进程
内存。匿名内存增加 4.05 MiB，反映了更大的 LVGL 内存池中已触碰的页面、
解码数据和图片缓存。两种模式均未使用 Swap。

完整时间序列和 SVG 曲线存放在
[`benchmarks/memory-2026-08-17`](benchmarks/memory-2026-08-17)，并已加入
[`image-resource-benchmark.html`](image-resource-benchmark.html)。

## perf 详细性能画像

记录于 2026-08-17，已关闭断言，并采用 Release 优化结合调试符号/帧指针（symbols/frame pointers）。每种模式下 `perf stat` 运行 120 秒，随后单独进行 30 秒的 `perf record` 采样运行。

| 指标计数器 | C 数组 | PNG | PNG 差异 |
|---|---:|---:|---:|
| 任务时钟 (Task clock) | 12,467.28 ms | 13,177.87 ms | +710.59 ms (+5.70%) |
| 平均 CPU 利用率 | 0.104 | 0.110 | +5.77% |
| 上下文切换 | 73,495 | 73,783 | +288 (+0.39%) |
| CPU 迁移 | 871 | 935 | +64 (+7.35%) |
| 缺页异常 (Page faults) | 13,809 | 16,373 | +2,564 (+18.57%) |
| 采样数 (Samples) | 280 | 299 | +19 |
| 丢失采样数 | 0 | 0 | 0 |

两种模式下的硬件 PMU 事件（`cycles`、`instructions`、分支及缓存事件）均报告为 `<not supported>`。因此本次测试无法计算 IPC、分支预测失败率（branch-miss rate）和缓存缺失率（cache-miss rate）。现有的软件计数器表明，PNG 模式的任务时钟耗时增加了 5.70%，缺页异常增加了 18.57%。

LVGL 中自身耗时（self-time）占比最高的具名函数为软件图像/颜色混合。PNG 模式下 `lv_draw_sw_blend_image_to_rgb888` 占比 2.34%，`lv_draw_sw_blend_color_to_rgb888` 占比 2.01%；C 数组模式下两者均为 1.79%。其他大部分高频采样主要集中在内核调度/内存页分配以及 Mesa llvmpipe 软件渲染。采样到的稳态热点中未出现 lodepng 相关函数，这与“已解码图像缓存将 PNG 解码工作限制在启动/首次使用时”的表现一致。

导出的文本数据保存在 [`benchmarks/perf-2026-08-17`](benchmarks/perf-2026-08-17)，图表包含在 [`image-resource-benchmark.html`](image-resource-benchmark.html) 中。

## Release 构建，关闭解析日志（120 秒）

这是推荐的低噪基准线。记录于 2026-08-17，基于关闭了逐条 `Parsed:` 日志输出的 Release 构建。TCP 客户端在两种模式下均已连接但未持续发送数据；接收通道在 3 秒后超时，因此几乎整个运行过程都代表空闲/静态 UI 状态。

| 指标 | C 数组 | PNG | PNG 差异 |
|---|---:|---:|---:|
| 用户态 CPU 时间 | 11.64 s | 11.85 s | +0.21 s (+1.80%) |
| 系统态 CPU 时间 | 2.63 s | 2.80 s | +0.17 s (+6.46%) |
| 总 CPU 时间 | 14.27 s | 14.65 s | +0.38 s (+2.66%) |
| CPU 利用率 | 11% | 12% | +1 个百分点 |
| 真实运行时间 (Wall time) | 120.05 s | 120.02 s | -0.03 s |
| 最大常驻内存 (Maximum RSS) | 116,352 KiB | 118,784 KiB | +2,432 KiB (+2.09%) |
| 主缺页异常 (Major page faults) | 0 | 0 | 0 |
| 次缺页异常 (Minor page faults) | 15,172 | 15,047 | -125 (-0.82%) |

去除冗长的解析日志输出后，PNG 测得的 CPU 开销从此前动态运行测试的 13.15% 下降至 2.66%。这表明控制台/网络负载以及不均匀的消息流对早期的测试结果产生了实质性影响。峰值 RSS 保持一致：PNG 占用了大约 2.38 MiB 更多的进程内存，同时节省了 1,394,768 字节（1.33 MiB）的可执行文件/图像存储占用。

尽管是以 Release 模式编译，但本次测试并非完全剥离调试开销的最终生产环境基准测试：LVGL 报告内存完整性（memory-integrity）、对象有效性（object-sanity）和样式有效性（style-sanity）断言仍然处于启用状态。这些检查会对 CPU 和 RAM 都产生影响，应在独立的最终生产对比测试中将其关闭。

汇总的原始记录保存在 [`benchmarks/2026-08-17-release-no-parse-log-120s.log`](benchmarks/2026-08-17-release-no-parse-log-120s.log) 中。

## 空闲/静态基准线

记录于 2026-08-17。每个可执行文件运行 10 秒，仪表盘 UI 正常显示，TCP 服务器监听端口 19090，且未接收车辆数据。两个变体均为由 `tools/benchmark_png_resources.sh` 生成的 Release 构建。

| 指标 | C 数组 | PNG | PNG 差异 |
|---|---:|---:|---:|
| 用户态 CPU 时间 | 0.87 s | 0.88 s | +0.01 s |
| 系统态 CPU 时间 | 0.35 s | 1.39 s | +1.04 s |
| CPU 利用率 | 12% | 22% | +10 个百分点 |
| 真实运行时间 (Wall time) | 10.10 s | 10.11 s | +0.01 s |
| 最大常驻内存 (Maximum RSS) | 116,224 KiB | 119,040 KiB | +2,816 KiB (+2.42%) |
| 主缺页异常 (Major page faults) | 0 | 0 | 0 |
| 次缺页异常 (Minor page faults) | 14,262 | 15,927 | +1,665 (+11.67%) |

最大常驻内存（RSS）指的是整个 SDL 模拟器进程，而不仅仅是 LVGL 的图像内存。在此配置下，PNG 构建预留了 8 MiB 的 LVGL 内存池，并使用 2 MiB 的已解码图像缓存。C 数组构建则使用默认的 1 MiB LVGL 内存池。

PNG 构建使可执行文件体积从 5,052,584 字节缩减至 3,427,440 字节。其 30 个外部 PNG 文件总计 230,376 字节，因此可执行文件加上图像文件的总存储空间比 C 数组可执行文件减少了 1,394,768 字节。

## 动态数据基准线（120 秒）

记录于 2026-08-17，TCP 客户端持续变更车速、转速、挡位、转向灯、车门锁、安全带警报以及远光灯状态。

| 指标 | C 数组 | PNG | PNG 差异 |
|---|---:|---:|---:|
| 解析的车辆报文数 | 781 | 761 | -20 (-2.56%) |
| 用户态 CPU 时间 | 11.92 s | 13.13 s | +1.21 s (+10.15%) |
| 系统态 CPU 时间 | 2.68 s | 3.39 s | +0.71 s (+26.49%) |
| 总 CPU 时间 | 14.60 s | 16.52 s | +1.92 s (+13.15%) |
| CPU 利用率 | 12% | 13% | +1 个百分点 |
| 每条解析报文的 CPU 耗时 | 18.69 ms | 21.71 ms | +3.01 ms (+16.13%) |
| 真实运行时间 (Wall time) | 120.06 s | 120.03 s | -0.03 s |
| 最大常驻内存 (Maximum RSS) | 116,608 KiB | 119,296 KiB | +2,688 KiB (+2.31%) |
| 主缺页异常 (Major page faults) | 0 | 0 | 0 |
| 次缺页异常 (Minor page faults) | 15,001 | 16,758 | +1,757 (+11.71%) |

PNG 节省了 1,394,768 字节的总可执行文件/图像存储数据，代价是在本次运行中增加了约 2.63 MiB 的进程峰值 RSS 和 13.15% 的总 CPU 时间。按不等的消息数量归一化后，每条解析报文的 CPU 耗时高出 16.13%。由于两个变体是先后针对持续运行的数据源进行测试的，因此报文数量和确切的状态序列并不完全相同；需要使用回放固定输入流进行重复测试，以获得具备统计控制意义的结果。

完整的捕获控制台日志保存在 [`benchmarks/2026-08-17-runtime-120s.log`](benchmarks/2026-08-17-runtime-120s.log) 中。可视化报告为 [`image-resource-benchmark.html`](image-resource-benchmark.html)。

## 复现测试步骤

```sh
./tools/benchmark_png_resources.sh
./tools/measure_image_runtime.sh
```

运行时脚本会将合并报告输出到 `/tmp/dashboard-image-benchmark/runtime-comparison.txt`，并在 `c_array` 和 `png` 子目录下保留原始的 `time -v` 报告。
