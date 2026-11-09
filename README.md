# hiviewdfx_hidumper_lite

-   [Introduction](#introduction)
-   [Directory Structure](#directory-structure)
-   [Use of hidumper_lite (mini system)](#use-of-hidumper_lite-mini-system)
-   [Use of hidumper (small system)](#use-of-hidumper-small-system)
-   [Interface](#interface)
-   [Related warehouses](#related-warehouses)

## Introduction

hidumper_lite provides system information export capabilities for lightweight devices in two forms:

-   mini form: a kernel-mode implementation on the LiteOS-M kernel. It exports CPU, memory, and task information through LOS_ kernel interfaces (LOS_TaskInfoGet, LOS_MemInfoGet, LOS_History*CpuUsage, etc.), and applies to mini-system products that provide a LiteOS kernel (for example, the LiteOS kernel in the ws63v100 SDK).
-   small form: the hidumper command line tool on small systems with the Linux kernel. The small system removes the SA/IPC framework: hidumper is a single-process command line program that exports CPU, memory, and process information, with output formats aligned with the standard-system hidumper.

## Directory Structure

The source code directory structure is as follows:

```
/base/hiviewdfx/hidumper_lite
├── mini                                    #mini system (LiteOS-M kernel) implementation
│   ├── hidumper.c                          #kernel-mode implementation (CPU/memory/task info export)
│   └── hidumper.h                          #public entry declaration
└── small                                   #small system (Linux kernel) implementation
    ├── client                              #command line entry
    ├── frameworks                          #dump framework (executor chain and task framework)
    ├── interfaces                          #inner interface files
    ├── services                            #service layer (single process, no SA/IPC)
    ├── test                                #unit test cases
    └── utils                               #common utilities (including lite-system dependency shims)
```

## Use of hidumper_lite (mini system)

On the mini form, data is queried through AT commands. The commands are registered in the product-side AT command table, which invokes the entry function `DealHidumperCmd` (see [Interface](#interface)).

- Parameter:
  | Format | Meaning |
  | -------- | -------- |
  | AT+HIDUMPER= | Displays the CPU, memory, and all task usage information |
  | AT+HIDUMPER=-dc | Displays CPU usage information (system and task usage in each time window) |
  | AT+HIDUMPER=-dm | Displays memory usage information (pool usage, free size, water line, etc.) |
  | AT+HIDUMPER=-dt | Displays all task information (priority, stack usage, event, semaphore, etc.) |
  | AT+HIDUMPER=-h | Print the help information |

Long aliases are also accepted: `-cpuusage` is equivalent to `-dc`, `--mem` to `-dm`, `-p` to `-dt`, and `-help` to `-h`.

## Use of hidumper (small system)

On small systems with the Linux kernel, HiDumper provides a unified system information collection tool for development and test personnel to analyze and locate faults. The small system removes the SA/IPC framework: hidumper is a single-process command line program that executes the dump flow locally and writes to standard output. HiDumper consists of the following modules:

-   hidumper, the executable program of the HiDumper tool, which receives user input and executes the dump flow directly in a single process.
-   DumpManager/DumpImplement, the manager of HiDumper, which provides functions such as information classification, command request distribution, and information collection.
-   Dumpers, exporters of various information, including process information export, CPU usage export, CPU frequency export, and memory usage export.
-   Output, content output, mainly FdOutput which outputs to standard output or a file descriptor.

**Figure 1**  Architecture of hidumper_lite

![](figures/en-us_image_0000001116226343.PNG)

Running hidumper without arguments prints the help text only (full-system dump is not supported).

The procedure is as follows:

1.  Access the device CLI, and run the **hidumper -h** command to obtain the help information, which includes basic information and function syntax.

    ```
    hidumper -h
    ```

2.  Run the **hidumper -p** command to obtain process information, including the list and information of processes and threads.

    ```
    hidumper -p
    ```

3.  Run the **hidumper -p 1024** command to obtain information about the process whose PID is 1024.

    ```
    hidumper -p 1024
    ```

4.  Run the **hidumper --cpuusage** command to obtain the CPU usage information of the system. The output includes the load average, the sampling time window, the total CPU usage (Total), and the CPU usage details of each process (Details of Processes).

    ```
    hidumper --cpuusage
    ```

5.  Run the **hidumper --cpuusage 1024** command to obtain the CPU usage information of the process whose PID is 1024.

    ```
    hidumper --cpuusage 1024
    ```

6.  Run the **hidumper --cpufreq** command to obtain the actual operating frequency of each CPU core. This command depends on the kernel cpufreq framework and a cpufreq driver (it reads nodes under `/sys/devices/system/cpu/cpu<N>/cpufreq/`); if the kernel does not provide them, the command exits gracefully.

    ```
    hidumper --cpufreq
    ```

7.  Run the **hidumper --mem** command to obtain the memory usage of the system.

    ```
    hidumper --mem
    ```

8.  Run the **hidumper --mem 1024** command to obtain the memory usage of the process whose PID is 1024.

    ```
    hidumper --mem 1024
    ```

9.  Run the **hidumper --mem --prune** command to export simplified memory information quickly.

    ```
    hidumper --mem --prune
    ```

10. Run the **hidumper --mem 1 -t 1** command to collect and print the memory change information of the PID at intervals. Press Ctrl+C to stop the export, and the detail information is stored in /data/log/hidumper/record_mem.txt (a writable /data partition is required).

    ```
    hidumper --mem 1 -t 1
    ```

**Figure 2**  Unit test result on the small-system board (13 tests passed)

![](figures/en-us_image_0000001121989181.PNG)

## Interface

### DealHidumperCmd

```
int DealHidumperCmd(int argc, char *args[]);
```

Command processing entry of the mini form, registered and invoked by the product-side AT command table or the kernel shell.

- Parameter:
  | Member | Type | Required | Description |
  | -------- | -------- | -------- | -------- |
  | argc | int | Y | number of parameters |
  | args | char*[] | Y | content of parameters; a leading '-' prefix of each parameter is ignored |

- Return value: 0 on success; -1 with the help text printed when the command is not recognized; when no valid parameter is present, all information (CPU, memory, tasks) is exported.

## Related warehouses

[DFX subsystem](https://gitee.com/openharmony/docs/blob/master/zh-cn/readme/DFX%E5%AD%90%E7%B3%BB%E7%BB%9F.md)

**hiviewdfx\_hidumper\_lite**

[hiviewdfx\_blackbox](https://gitee.com/openharmony/hiviewdfx_blackbox/blob/master/README_zh.md)

[hiviewdfx\_hilog\_lite](https://gitee.com/openharmony/hiviewdfx_hilog_lite/blob/master/README_zh.md)

[hiviewdfx\_hievent\_lite](https://gitee.com/openharmony/hiviewdfx_hievent_lite/blob/master/README_zh.md)

[hiviewdfx\_hiview\_lite](https://gitee.com/openharmony/hiviewdfx_hiview_lite/blob/master/README_zh.md)
