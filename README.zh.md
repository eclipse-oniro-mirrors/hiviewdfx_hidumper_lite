# hiviewdfx_hidumper_lite

-   [简介](#简介)
-   [目录](#目录)
-   [hidumper_lite的使用](#hidumper_lite的使用)
-   [hidumper（small系统）的使用](#hidumper（small系统）的使用)
-   [系统调用接口定义](#系统调用接口定义)
-   [重写接口](#重写接口)
-   [相关仓](#相关仓)

## 简介

hidumper_lite提供liteOS_M系统下dump接口，帮助开发者提供故障日志、内存、CPU等信息的导出功能。

同时在Linux内核的小型系统上，hidumper_lite提供hidumper命令行工具：小型系统hidumper为单进程命令行程序，帮助开发者导出CPU、内存、进程等信息，输出格式与标准系统hidumper保持一致。

## 目录

Hiview源代码目录结构如下：

```
/base/hiviewdfx/hidumper_lite.
├── lite
    └── hidumper.c                          #AT命令处理过程
├── mini
    ├── hidumper_adapter.c                  #hidumper功能函数适配层
    ├── hidumper_core.c                     #hidumper核心层
    └── interfaces                          #公共接口文件
└── small                                   #small系统（Linux内核）实现
    ├── client                              #命令行入口
    ├── frameworks                          #dump框架（config配置链 + executor执行链）
    ├── interfaces                          #内部接口文件
    ├── services                            #服务层
    ├── test                                #单元测试用例
    └── utils                               #公共能力（含轻量系统依赖替代）
```

## hidumper_lite的使用

hidumper_lite在liteOS_M下面是通过AT命令进行数据查询的，命令由产品侧AT命令表注册并调用入口函数`at_hidumper`（见[系统调用接口定义](#系统调用接口定义)）。mini形态自带平台适配层：`hidumper_adapter.c`提供一组WEAK默认实现并在系统启动时自动注册（`CORE_INIT_PRI`），产品侧按[重写接口](#重写接口)覆写对应函数即可接入真实平台数据。

- 参数：
  | 命令格式 | 命令含义 |
  | -------- | -------- |
  | AT+HIDUMPER= | 打印出CPU、内存和全部的task使用信息 |
  | AT+HIDUMPER=-dc | 打印出CPU使用信息 |
  | AT+HIDUMPER=-dm | 打印出内存使用信息 |
  | AT+HIDUMPER=-df | 打印出最后的故障日志信息 |
  | AT+HIDUMPER=-dt | 打印出全部task信息 |
  | AT+HIDUMPER=-h | 打印出帮助信息 |
  | AT+HIDUMPER=-ikc | 向内核注入故障 |
  | AT+HIDUMPER=-m | 以16进制形式打印出全部内存数据 |
  | AT+HIDUMPER=-m,memstart,memsize | 以16进制形式打印出特定内存区域数据。两个参数，memstart：内存起始地址，memsize：内存大小 |

说明：`-ikc`、`-m`、`-m,memstart,memsize`仅OHOS_DEBUG编译版本支持，否则打印Unsupported；`-m,filepath`与`-m,memstart,memsize,filepath`为预留格式，当前版本打印Unsupported；无法识别的参数打印帮助信息；适配层未注册时`at_hidumper`返回1（正常启动时适配层已自动注册，不会出现）。

另外，mini形态还提供shell风格入口`OH_Hidumper_StartCmd`（见[系统调用接口定义](#系统调用接口定义)），支持`-dc/-dm/-dt/-h`及长格式别名`-cpuusage/--mem/-p/--help`。

## hidumper（small系统）的使用

Linux内核的小型系统上，hidumper为开发、测试人员提供统一的系统信息获取工具，帮助使用者分析、定位问题。小型系统hidumper为单进程命令行程序，直接在本地执行导出流程并输出到标准输出。主要包括以下几个模块：

-   hidumper，hidumper工具的可执行程序，接收用户输入，单进程直接执行导出流程。
-   DumpManager/DumpImplement，系统信息导出工具管理者，提供信息分类、命令请求分发和信息获取方法的管理。
-   Dumpers，各类信息的导出器。主要包括进程信息导出、CPU使用率导出、CPU频率导出、内存使用导出等。
-   Output，内容输出，主要包括向标准输出或文件描述符输出的FdOutput。

**图1**  hidumper_lite（small系统）架构图

![](figures/zh-cn_image_0000001116226343.PNG)

不带参数执行hidumper时仅打印帮助信息（不支持全量dump）。

具体步骤如下：

1.  进入设备命令行，输入hidumper -h获取基本信息介绍，功能语法介绍。

    ```
    hidumper -h
    ```

**图2**  hidumper -h帮助信息输出

![](figures/zh-cn_image_0000001121989181.PNG)

2.  运行  **hidumper -p**  命令获取进程信息，包括进程、线程的列表和信息。

    ```
    hidumper -p
    ```

3.  运行  **hidumper -p 1024**  命令获取pid为1024的进程信息。

    ```
    hidumper -p 1024
    ```

4.  运行  **hidumper --cpuusage**  命令获取系统CPU的使用信息，输出包括负载均值（Load average）、采样时间窗、整机CPU使用率（Total）以及各进程CPU使用明细（Details of Processes）。

    ```
    hidumper --cpuusage
    ```

5.  运行  **hidumper --cpuusage 1024**  命令获取pid为1024进程的CPU使用信息。

    ```
    hidumper --cpuusage 1024
    ```

6.  运行  **hidumper --cpufreq**  命令获取每一个CPU核实际运行的频率。该命令依赖内核cpufreq框架及cpufreq驱动（读取`/sys/devices/system/cpu/cpu<N>/cpufreq/`下的节点）；内核未提供时命令优雅退出。

    ```
    hidumper --cpufreq
    ```

7.  运行  **hidumper --mem**  命令获取系统整体内存使用信息。

    ```
    hidumper --mem
    ```

8.  运行  **hidumper --mem 1024**  命令获取pid为1024进程的内存使用信息。

    ```
    hidumper --mem 1024
    ```

9.  运行  **hidumper --mem --prune**  命令快速导出精简的内存信息。

    ```
    hidumper --mem --prune
    ```

10. 运行  **hidumper --mem 1 -t 1**  命令按时间间隔统计并打印pid的内存变化信息；按Ctrl+C停止导出，明细信息存储在/data/log/hidumper/record_mem.txt（需要设备存在可写的/data分区）。

    ```
    hidumper --mem 1 -t 1
    ```

## 系统调用接口定义

### at_hidumper

```
unsigned int at_hidumper(unsigned int argc, const char **argv);
```

注入到系统的AT指令处理函数

- 参数：
  | 成员 | 类型 | 必填 | 说明 |
  | -------- | -------- | -------- | -------- |
  | argc | unsigned int | 是 | 输入参数的个数。 |
  | argv | const char** | 是 | 输入参数的内容 |

- 返回值：适配层已注册返回0；未注册时打印提示并返回1。

### OH_Hidumper_StartCmd

```
int OH_Hidumper_StartCmd(int argc, const char *args[], char **out);
```

mini形态shell风格命令入口，直接调用LOS_*内核接口实现，不依赖适配层；参数前的'-'前缀会被自动忽略，支持-dc/-dm/-dt/-h及长格式别名-cpuusage/--mem/-p/--help；无有效参数时导出全部信息（CPU、内存、任务）。

- 参数：
  | 成员 | 类型 | 必填 | 说明 |
  | -------- | -------- | -------- | -------- |
  | argc | int | 是 | 输入参数的个数。 |
  | args | const char*[] | 是 | 输入参数的内容。 |
  | out | char** | 否 | 预留输出指针，当前未使用。 |

- 返回值：命令执行成功返回0；命令无法识别时打印帮助信息并返回-1。

## 重写接口

实例可以参考hi3861芯片的重写，对应位置为device/soc/hisilicon/hi3861v100/sdk_liteos/components/at/src/hidumper_adapter_impl.c

以下的函数都是需要在不同的芯片下重写的：

### DumpSysInfo

```
int DumpSysInfo(void);
```

重写打印系统信息函数。该函数打印当前设备系统信息。

### DumpCpuUsage

```
int DumpCpuUsage(void);
```

重写打印CPU使用信息函数。该函数打印当前设备CPU使用信息。

### DumpMemUsage

```
int DumpMemUsage(void);
```

重写打印内存使用信息函数。该函数打印当前设备内存使用信息。

### DumpTaskInfo

```
int DumpTaskInfo(void);
```

重写打印系统全部Task信息函数。该函数打印系统中全部的task信息。

### DumpFaultLog

```
int DumpFaultLog(void);
```

重写打印异常日志信息函数。该函数打印最后保存的日志信息。

### DumpMemRegion

```
int DumpMemRegion(unsigned long long addr, unsigned long long size)
```

重写打印指定区域内存数据函数。该函数打印从addr开始一共size这大小的内存区域的数据。

- 参数：
  | 参数名 | 类型 | 必填 | 说明 |
  | -------- | -------- | -------- | -------- |
  | addr | unsigned long long | 是 | 需要查询的内存起始地址 |
  | size | unsigned long long | 是 | 需要查询的内大小 |

### DumpAllMem

```
int DumpAllMem(void); 
```

重写打印全部内存数据函数。该函数能打印全部内存数据。

### PlatformHiDumperIinit

```
int PlatformHiDumperIinit(void); 
```

重写平台hidumper初始化函数。该函数根据不同平台初始化HiDumper的at命令等特性。

## 相关仓

[DFX子系统](https://gitee.com/openharmony/docs/blob/master/zh-cn/readme/DFX%E5%AD%90%E7%B3%BB%E7%BB%9F.md)

**hiviewdfx\_hidumper_lite**

[hiviewdfx\_blackbox](https://gitee.com/openharmony/hiviewdfx_blackbox/blob/master/README_zh.md)

[hiviewdfx\_hilog\_lite](https://gitee.com/openharmony/hiviewdfx_hilog_lite/blob/master/README_zh.md)

[hiviewdfx\_hievent\_lite](https://gitee.com/openharmony/hiviewdfx_hievent_lite/blob/master/README_zh.md)

[hiviewdfx\_hiview\_lite](https://gitee.com/openharmony/hiviewdfx_hiview_lite/blob/master/README_zh.md)
