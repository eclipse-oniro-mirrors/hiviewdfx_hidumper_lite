# AGENTS.md

本文件为 `hidumper_lite` 仓库定义 Agent 工作规则。

## 1. 范围与优先级

- 本文件适用于 `OpenHarmony/base/hiviewdfx/hidumper_lite`。
- 用户直接指令优先级高于本文件。
- 核心原则：**代码优先、证据优先、不臆造**。

## 2. 项目定位

HiDumper Lite 是 OpenHarmony 面向 LiteOS-A / LiteOS-M 内核的轻量系统信息导出工具，为开发者提供 CPU、内存、任务、故障日志的 dump 能力。主要组成：

- **lite/（LiteOS-A）**：用户态客户端，通过 `/dev/hidumper` 设备节点 + ioctl 与内核交互。
- **mini/（LiteOS-M）**：AT 命令处理核心 + 适配层，直接在内核态运行，通过 `HiDumperAdapter` 弱函数对接平台实现。
- **BUILD.gn**：按 `ohos_kernel_type`（`liteos_a` / `liteos_m`）选择编译目标。

### 2.1 按任务类型定位代码

| 任务类型 | 推荐目录 | 关键文件 |
|---|---|---|
| LiteOS-A 客户端逻辑（ioctl/设备节点） | `lite/` | `hidumper.c`（`main:205`、`ExecAction:85`、ioctl 命令 `:56-62`） |
| LiteOS-M AT 命令处理 | `mini/` | `hidumper_core.c`（`at_hidumper:155`、`ParameterMatching:105`、`Usage:38`） |
| LiteOS-M 平台适配（弱函数） | `mini/` | `hidumper_adapter.c`（`DumpSysInfo:33`、`DumpCpuUsage:39`、`DumpMemUsage:45`、`DumpTaskInfo:52`、`DumpFaultLog:58`、`DumpMemRegion:64`、`DumpAllMem:72`、`PlatformHiDumperIinit:78`、`HiDumperAdapterInit:84`） |
| 公共接口 | `mini/interfaces/native/` | 头文件 |
| 构建配置 | 根目录 | `BUILD.gn`、`bundle.json` |

### 2.2 嵌套指引

本仓库无目录级别的嵌套 Agent 指引。详细说明见 `README.md`。

## 3. 构建与验证

本仓为 C 语言轻量组件，无独立 Python/JS 工具链，构建依赖 OpenHarmony lite 构建。

### 3.1 构建

```bash
# 构建 hidumper_lite（从 OpenHarmony 根目录）
./build.sh --product-name rk3568 --build-target //base/hiviewdfx/hidumper_lite:hidumper

# liteos_m 产品构建（自动选择 mini/ 目标）
# 编译产物：静态库或内核内置
```

- 组件名 `hidumper_lite`，子系统 `hiviewdfx`，ROM 26KB，RAM ~10KB。
- 依赖：`liteos_m`、`utils_lite`、`bounds_checking_function`。
- `BUILD.gn` 按 `ohos_kernel_type` 分发：`liteos_a` → `lite:hidumper_lite`，`liteos_m` → `mini:hidumper_mini`。

### 3.2 测试

- 本仓无独立单元测试目标；验证通过设备 AT 命令实际调用完成。
- 测试方式：在 LiteOS-M 设备串口输入 `AT+HIDUMPER=-h` 确认帮助输出。

### 3.3 完成标准

1. 代码改动可编译——`./build.sh` 通过，无新增告警。
2. 设备验证（如适用）——AT 命令输出正确。
3. 公共接口（`at_hidumper`、`HiDumperRegisterAdapter`）签名不变。
4. 弱函数适配接口不减少已有函数。

### 3.4 无法运行验证时

明确说明无法运行的原因，列出推荐验证步骤供人工执行。

### 3.5 完成报告格式

报告应包含：改动摘要（文件列表、改动点）、验证结果（编译输出 / AT 命令输出）、风险评估（公共接口兼容性、内存占用、设备安全）、未完成事项。

### 3.6 最小验证与静态检查

小改动不必全量构建，按以下最小路径验证：

```bash
# 最小验证：只编译 hidumper_lite 目标
./build.sh --product-name rk3568 --build-target //base/hiviewdfx/hidumper_lite:hidumper

# 确认组件依赖未变更
grep -n "sub_component\|deps\|components" bundle.json
```

- 本仓为纯 C 轻量组件，未配置独立 lint 工具；以构建无新增告警作为静态检查门槛。
- 涉及 ioctl 命令 / 弱函数签名 / AT 命令格式的改动仍须走 §3.3 完成标准（含设备验证）。

## 4. 核心工作原则（必须遵守）

### 4.1 只以真实代码为准
- 任何结论先读真实代码再下结论，引用 `文件:行号`。
- 若代码缺失，明确说明「此代码在 hidumper_lite 中未找到」。

### 4.2 臆测管理
- 任何未经验证的陈述必须标注「推测」。

### 4.3 代码优先验证
- 出现分歧时，以实现证据为准。

## 5. 项目目录结构

```
/base/hiviewdfx/hidumper_lite
├── lite/
│   ├── BUILD.gn                      # LiteOS-A 编译目标
│   └── hidumper.c                    # LiteOS-A 客户端（ioctl /dev/hidumper）
├── mini/
│   ├── BUILD.gn                      # LiteOS-M 编译目标
│   ├── hidumper_core.c              # LiteOS-M 核心（AT 命令解析 + 适配注册）
│   ├── hidumper_adapter.c           # LiteOS-M 弱函数适配层（平台可重写）
│   └── interfaces/
│       └── native/                   # 公共接口头文件
├── BUILD.gn                           # 根目标（按 kernel_type 分发）
├── bundle.json                        # 组件元数据
├── README.md / README.zh.md
└── LICENSE
```

### 5.1 经常改动的路径

- `mini/hidumper_adapter.c` — 平台适配弱函数（各厂商重写最频繁）。
- `mini/hidumper_core.c` — AT 命令解析与新功能扩展。

### 5.2 任务 → 路径速查

| 任务 | 起点 |
|---|---|
| 新增一种 dump 类型 | `hidumper_core.c` `ParameterMatching`（`:105`）加分支 + `hidumper_adapter.c` 加弱函数 |
| 修改 AT 命令帮助 | `hidumper_core.c` `Usage`（`:38`） |
| 平台适配实现 | 重写 `hidumper_adapter.c` 中对应 `WEAK` 函数 |
| 修改 LiteOS-A ioctl 命令 | `lite/hidumper.c` ioctl 定义（`:56-62`）+ `ExecAction`（`:85`） |
| 注册适配 | `hidumper_core.c` `HiDumperRegisterAdapter`（`:79`） |

## 6. 知识索引

- `README.md` — 组件简介、AT 命令用法表、系统接口、适配接口。
- `bundle.json` — 组件依赖与构建目标。

### 6.1 词汇 / 场景 → 文档触发表

| 触发词 / 场景 | 先读 |
|---|---|
| 任务含 `AT+HIDUMPER` / 命令解析 | `hidumper_core.c:105` `ParameterMatching` + `:38` `Usage` |
| 任务含 `DumpSysInfo` / `DumpCpuUsage` / `DumpMemUsage` / `DumpTaskInfo` | `hidumper_adapter.c` 弱函数 + `hidumper_core.c` 调用点 |
| 任务含 `HiDumperRegisterAdapter` / 适配注册 | `hidumper_core.c:79` + `hidumper_adapter.c:84` `HiDumperAdapterInit` |
| 任务含 `ioctl` / `/dev/hidumper` / `HIDUMPER_IOC` | `lite/hidumper.c:56-62` ioctl 定义 + `:85` `ExecAction` |
| 任务含 `fault log` / `-df` / `InjectKernelCrash` | `hidumper_core.c:69` `InjectKernelCrash` + `hidumper_adapter.c:58` `DumpFaultLog` |
| 任务含 `memstart` / `memsize` / `-m` | `lite/hidumper.c` `DumpMemData:118` + `hidumper_adapter.c:64` `DumpMemRegion` |

### 6.2 术语表

| 术语 | 含义 |
|---|---|
| `AT+HIDUMPER=` | LiteOS-M 串口 AT 命令入口，触发 dump |
| `WEAK` | 弱函数符号，平台可重写覆盖默认空实现 |
| `HiDumperAdapter` | 适配结构体，含 6 个函数指针，由平台注册 |
| `ioctl` | LiteOS-A 通过设备节点 `/dev/hidumper` 与内核交互 |
| `HIDUMPER_IOC_BASE` | ioctl 命令基字符 `'d'`，派生 7 条命令 |
| `ohos_kernel_type` | GN 变量，决定编译 `lite/` 或 `mini/` |
| `bounds_checking_function` | 第三方安全字符串库依赖 |

### 6.3 开始编辑前

按以下顺序确认：

1. 确认任务类别（见 2.1 表）。
2. 确认已阅读相关 README / 头文件（或明确声明「无相关文档」）。
3. 根据「项目约束」确认不违反任何约束。
4. 声明：「将修改 X，已阅读 Y，遵循 Z 约束」。

## 7. 编码约定

### 7.1 语言与命名

- 纯 C 代码（非 C++），无命名空间，函数前缀 `Dump*` / `at_*` / `HiDumper*`。
- 弱函数用 `WEAK` 宏标记，默认返回 0（空实现），平台覆盖。

### 7.2 AT 命令格式

- `AT+HIDUMPER=<参数>` 格式，参数以 `-` 前缀（`-dc`/`-dm`/`-dt`/`-df`/`-h`/`-ikc`/`-m`）。
- 内存 dump 支持逗号分隔子参数（`-m,memstart,memsize[,filepath]`）。

### 7.3 错误处理

- ioctl 失败：`printf("Failed to ...")` + `strerror(errno)`（LiteOS-A）。
- 弱函数默认返回 0（不输出），平台实现返回实际数据量。
- `InjectKernelCrash` 写入 `FAULT_ADDR` 触发内核崩溃（仅 `OHOS_DEBUG` 编译）。

## 8. 项目约束

### 8.1 性能约束

- LiteOS-M 内存受限（RAM ~10KB），dump 输出须流式打印，不要全量缓冲。
- 内存 dump `-m` 输出量大，须分块输出。

### 8.2 架构约束

- `lite/` 与 `mini/` 互斥编译（由 `ohos_kernel_type` 决定），不要交叉引用。
- `hidumper_adapter.c` 的弱函数是平台适配唯一扩展点，核心逻辑不依赖平台实现。
- `HiDumperRegisterAdapter` 须在 `at_hidumper` 调用前完成注册。

### 8.3 公共 API 约束

**禁止：**

- 修改 `at_hidumper(unsigned int argc, const char **argv)` 签名。
- 修改 `HiDumperRegisterAdapter(struct HiDumperAdapter *pAdapter)` 签名。
- 减少已有弱函数（`DumpSysInfo` / `DumpCpuUsage` / `DumpMemUsage` / `DumpTaskInfo` / `DumpFaultLog` / `DumpMemRegion` / `DumpAllMem`）。

**修改前必须确认：**

- 新增 dump 类型须同时加弱函数 + `ParameterMatching` 分支 + `Usage` 帮助。
- 修改 AT 命令格式须向后兼容（已有 `-dc`/`-dm`/`-dt`/`-df`/`-h`/`-ikc`/`-m` 不可改含义）。

### 8.4 安全与设备约束

**禁止：**

- 在非 `OHOS_DEBUG` 编译下保留 `InjectKernelCrash` / `InjectUserCrash` 实际崩溃逻辑（须 `#else` 空实现）。
- 修改 `/dev/hidumper` 设备节点路径或 ioctl 命令编号（与内核驱动契约）。
- 将内存 dump 数据（`-m`）输出到非授权通道。

**修改前必须确认：**

- 内存地址注入（`-m,memstart,memsize`）须校验地址合法性与大小上限。
- 内核崩溃注入（`-ikc`）仅在 debug 构建可用。

### 8.5 第三方依赖

- `bounds_checking_function` 为安全字符串库（`strcat_s`/`sprintf_s` 等），许可证 Apache-2.0。
- 新增或升级第三方依赖须经许可证评审，同步 `bundle.json` 的 `deps.third_party`。
- 不得引入 GPL 等传染性许可证依赖。

## 9. 常见陷阱

- **新增 dump 类型只改 `hidumper_core.c` 忘记加弱函数** — 平台无法实现该 dump，调用返回 0 空输出。
- **在 LiteOS-M 编译中引用 LiteOS-A 的 `lite/hidumper.c`** — 互斥编译目标混淆，编译失败。
- **`HiDumperRegisterAdapter` 未在 `at_hidumper` 前调用** — 适配未注册，所有 dump 返回空。
- **在非 `OHOS_DEBUG` 构建保留 `InjectKernelCrash`** — release 构建引入内核崩溃风险。
- **修改 ioctl 命令编号** — 与内核 `/dev/hidumper` 驱动契约破坏。
- **内存 dump 不做地址校验** — 任意地址读取可能触发内核异常。
- **AT 命令参数格式变更不向后兼容** — 已有设备固件使用旧格式会失效。
