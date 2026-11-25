/*
 * Copyright (C) 2021 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "util/config_data.h"
#include <pubdef.h>
#include "common/dumper_constant.h"
namespace OHOS {
namespace HiviewDFX {
const std::string ConfigData::CONFIG_NAME_SPLIT = "_";
const std::string ConfigData::CONFIG_GROUP = "group";
const std::string ConfigData::CONFIG_GROUP_ = ConfigData::CONFIG_GROUP + ConfigData::CONFIG_NAME_SPLIT;
const std::string ConfigData::CONFIG_MINIGROUP = "groupmini";
const std::string ConfigData::CONFIG_MINIGROUP_ = ConfigData::CONFIG_MINIGROUP + ConfigData::CONFIG_NAME_SPLIT;
const std::string ConfigData::CONFIG_DUMPER = "dumper";
const std::string ConfigData::CONFIG_DUMPER_ = ConfigData::CONFIG_DUMPER + ConfigData::CONFIG_NAME_SPLIT;
const std::string ConfigData::CONFIG_GROUP_CPU_USAGE = ConfigData::CONFIG_GROUP_ + "cpuusage";
const std::string ConfigData::CONFIG_GROUP_CPU_FREQ = ConfigData::CONFIG_GROUP_ + "cpufreq";
const std::string ConfigData::CONFIG_GROUP_MEMORY = ConfigData::CONFIG_GROUP_ + "memory";
const std::string ConfigData::CONFIG_GROUP_PROCESSES = ConfigData::CONFIG_GROUP_ + "processes";
const std::string ConfigData::CONFIG_GROUP_PROCESSES_PID = ConfigData::CONFIG_GROUP_ + "pid_processes";
const std::string ConfigData::CONFIG_GROUP_PROCESSES_ENG = ConfigData::CONFIG_GROUP_ + "eng_processes";
const std::string ConfigData::CONFIG_GROUP_PROCESSES_PID_ENG = ConfigData::CONFIG_GROUP_ + "pid_eng_processes";

const ConfigData::ItemCfg ConfigData::cpuUsageDumper_[] = {
    {
        "dumper_cpu_usage", "CPU Usage", "%pid", "",
        DumperConstant::CPU_DUMPER, DumperConstant::NONE, DumperConstant::NONE, ""
    },
    {
        "", "", "", "",
        DumperConstant::FD_OUTPUT, DumperConstant::NONE, DumperConstant::NONE, ""
    },
};

const ConfigData::ItemCfg ConfigData::memDumper_[] = {
    {
        "dumper_mem", "Memory Information", "%pid", "",
        DumperConstant::MEMORY_DUMPER, DumperConstant::NONE, DumperConstant::LOOP, ""
    },
    {
        "", "", "", "",
        DumperConstant::FD_OUTPUT, DumperConstant::NONE, DumperConstant::LOOP, ""
    },
};

const ConfigData::ItemCfg ConfigData::threadsDumper_[] = {
    {
        "dumper_threads", "Processes/Threads List", "ps -efT", "",
        DumperConstant::CMD_DUMPER, DumperConstant::NONE, DumperConstant::NONE, ""
    },
    {
        "", "", "", "",
        DumperConstant::FD_OUTPUT, DumperConstant::NONE, DumperConstant::NONE, ""
    },
};

const ConfigData::ItemCfg ConfigData::threadsPidDumper_[] = {
    {
        "dumper_threads_pid", "Processes/Threads List", "ps -efT -p %pid", "",
        DumperConstant::CMD_DUMPER, DumperConstant::NONE, DumperConstant::NONE, ""
    },
    {
        "", "", "", "",
        DumperConstant::FD_OUTPUT, DumperConstant::NONE, DumperConstant::NONE, ""
    },
};

const ConfigData::ItemCfg ConfigData::blockChannelDumper_[] = {
    {
        "dumper_block_channel", "Block Channel", "/proc/%pid/wchan", "",
        DumperConstant::FILE_DUMPER, DumperConstant::NONE, DumperConstant::NONE, ""
    },
    {
        "", "", "", "",
        DumperConstant::FD_OUTPUT, DumperConstant::NONE, DumperConstant::NONE, ""
    },
};

const ConfigData::ItemCfg ConfigData::excuteTimeDumper_[] = {
    {
        "dumper_excute_time", "Excute Time", "ps -o \"TIME\" -p %pid", "",
        DumperConstant::CMD_DUMPER, DumperConstant::NONE, DumperConstant::NONE, ""
    },
    {
        "", "", "", "",
        DumperConstant::FD_OUTPUT, DumperConstant::NONE, DumperConstant::NONE, ""
    },
};

const ConfigData::ItemCfg ConfigData::mountInfoDumper_[] = {
    {
        "dumper_mount_info", "Mount Information", "/proc/%pid/mountinfo", "",
        DumperConstant::FILE_DUMPER, DumperConstant::NONE, DumperConstant::LOOP, ""
    },
    {
        "", "", "", "",
        DumperConstant::FD_OUTPUT, DumperConstant::NONE, DumperConstant::LOOP, ""
    },
};

const ConfigData::ItemCfg ConfigData::mapDumper_[] = {
    {
        "dumper_map", "Process MAP Information", "/proc/%pid/maps", "",
        DumperConstant::FILE_DUMPER, DumperConstant::NONE, DumperConstant::LOOP, ""
    },
    {
        "", "", "", "",
        DumperConstant::FD_OUTPUT, DumperConstant::NONE, DumperConstant::LOOP, ""
    },
};

const ConfigData::ItemCfg ConfigData::cpuFreqDumper_[] = {
    {
        "dumper_cpu_freq", "CPU Frequency", "cat /sys/devices/system/cpu/cpu%cpuid/cpufreq/cpuinfo_cur_freq", "",
        DumperConstant::CMD_DUMPER, DumperConstant::NONE, DumperConstant::NONE, ""
    },
    {
        "", "", "", "",
        DumperConstant::FD_OUTPUT, DumperConstant::NONE, DumperConstant::NONE, ""
    },
    {
        "", "", "cat /sys/devices/system/cpu/cpu%cpuid/cpufreq/cpuinfo_max_freq", "",
        DumperConstant::CMD_DUMPER, DumperConstant::NONE, DumperConstant::NONE, ""
    },
    {
        "", "", "", "",
        DumperConstant::FD_OUTPUT, DumperConstant::NONE, DumperConstant::NONE, ""
    },
};

const ConfigData::DumperCfg ConfigData::dumpers_[] = {
    {.name_ = cpuUsageDumper_[0].name_,
     .desc_ = cpuUsageDumper_[0].desc_,
     .list_ = cpuUsageDumper_,
     .size_ = ArraySize(cpuUsageDumper_)},
    {.name_ = memDumper_[0].name_,
     .desc_ = memDumper_[0].desc_,
     .list_ = memDumper_,
     .size_ = ArraySize(memDumper_)},
    {.name_ = threadsDumper_[0].name_,
     .desc_ = threadsDumper_[0].desc_,
     .list_ = threadsDumper_,
     .size_ = ArraySize(threadsDumper_)},
    {.name_ = threadsPidDumper_[0].name_,
     .desc_ = threadsPidDumper_[0].desc_,
     .list_ = threadsPidDumper_,
     .size_ = ArraySize(threadsPidDumper_)},
    {.name_ = blockChannelDumper_[0].name_,
     .desc_ = blockChannelDumper_[0].desc_,
     .list_ = blockChannelDumper_,
     .size_ = ArraySize(blockChannelDumper_)},
    {.name_ = excuteTimeDumper_[0].name_,
     .desc_ = excuteTimeDumper_[0].desc_,
     .list_ = excuteTimeDumper_,
     .size_ = ArraySize(excuteTimeDumper_)},
    {.name_ = mountInfoDumper_[0].name_,
     .desc_ = mountInfoDumper_[0].desc_,
     .list_ = mountInfoDumper_,
     .size_ = ArraySize(mountInfoDumper_)},
    {.name_ = mapDumper_[0].name_,
     .desc_ = mapDumper_[0].desc_,
     .list_ = mapDumper_,
     .size_ = ArraySize(mapDumper_)},
    {.name_ = cpuFreqDumper_[0].name_,
     .desc_ = cpuFreqDumper_[0].desc_,
     .list_ = cpuFreqDumper_,
     .size_ = ArraySize(cpuFreqDumper_)},
};

const std::string ConfigData::cpuUsageGroup_[] = {
    "dumper_cpu_usage",
};

const std::string ConfigData::cpuFreqGroup_[] = {
    "dumper_cpu_freq",
};

const std::string ConfigData::memoryGroup_[] = {
    "dumper_mem",
};

const std::string ConfigData::processesGroup_[] = {
    "dumper_threads",
    "groupmini_expand_processesGroup",
};

const std::string ConfigData::processesPidGroup_[] = {
    "groupmini_pid_expand_processesGroup",
};

const std::string ConfigData::processesGroupMini_[] = {
    "dumper_block_channel",
    "dumper_excute_time",
    "dumper_mount_info",
};

const std::string ConfigData::processesPidGroupMini_[] = {
    "dumper_threads_pid",
    "dumper_block_channel",
    "dumper_excute_time",
    "dumper_mount_info",
};

const std::string ConfigData::processesGroup_eng_[] = {
    "dumper_threads",
    "groupmini_expand_processesGroup_eng",
};

const std::string ConfigData::processesPidGroup_eng_[] = {
    "groupmini_pid_expand_processesGroup_eng",
};

const std::string ConfigData::processesGroupMini_eng_[] = {
    "dumper_map",
    "dumper_block_channel",
    "dumper_excute_time",
    "dumper_mount_info",
};

const std::string ConfigData::processesPidGroupMini_eng_[] = {
    "dumper_threads_pid",
    "dumper_map",
    "dumper_block_channel",
    "dumper_excute_time",
    "dumper_mount_info",
};

const ConfigData::GroupCfg ConfigData::groups_[] = {
    {
        .name_ = ConfigData::CONFIG_GROUP_CPU_FREQ,
        .desc_ = "group of cpu freq dumper",
        .list_ = cpuFreqGroup_,
        .size_ = ArraySize(cpuFreqGroup_),
        .type_ = DumperConstant::GROUPTYPE_CPUID,
        .expand_ = true,
    },
    {
        .name_ = ConfigData::CONFIG_GROUP_CPU_USAGE,
        .desc_ = "group of cpu usage dumper",
        .list_ = cpuUsageGroup_,
        .size_ = ArraySize(cpuUsageGroup_),
        .type_ = DumperConstant::GROUPTYPE_PID,
        .expand_ = false,
    },
    {
        .name_ = ConfigData::CONFIG_GROUP_MEMORY,
        .desc_ = "group of memory dumper",
        .list_ = memoryGroup_,
        .size_ = ArraySize(memoryGroup_),
        .type_ = DumperConstant::GROUPTYPE_PID,
        .expand_ = false,
    },
    {
        .name_ = ConfigData::CONFIG_GROUP_PROCESSES,
        .desc_ = "group of processes dumper",
        .list_ = processesGroup_,
        .size_ = ArraySize(processesGroup_),
        .type_ = DumperConstant::NONE,
        .expand_ = false,
    },
    {
        .name_ = ConfigData::CONFIG_GROUP_PROCESSES_PID,
        .desc_ = "group of processes pid dumper",
        .list_ = processesPidGroup_,
        .size_ = ArraySize(processesPidGroup_),
        .type_ = DumperConstant::NONE,
        .expand_ = false,
    },
    {
        .name_ = "groupmini_expand_processesGroup",
        .desc_ = "mini-group for of processes dumper",
        .list_ = processesGroupMini_,
        .size_ = ArraySize(processesGroupMini_),
        .type_ = DumperConstant::GROUPTYPE_PID,
        .expand_ = true,
    },
    {
        .name_ = "groupmini_pid_expand_processesGroup",
        .desc_ = "mini-group for of processes dumper",
        .list_ = processesPidGroupMini_,
        .size_ = ArraySize(processesPidGroupMini_),
        .type_ = DumperConstant::GROUPTYPE_PID,
        .expand_ = true,
    },
    {
        .name_ = ConfigData::CONFIG_GROUP_PROCESSES_ENG,
        .desc_ = "group of processes dumper by eng ",
        .list_ = processesGroup_eng_,
        .size_ = ArraySize(processesGroup_eng_),
        .type_ = DumperConstant::NONE,
        .expand_ = false,
    },
    {
        .name_ = ConfigData::CONFIG_GROUP_PROCESSES_PID_ENG,
        .desc_ = "group of processes pid dumper by eng",
        .list_ = processesPidGroup_eng_,
        .size_ = ArraySize(processesPidGroup_eng_),
        .type_ = DumperConstant::NONE,
        .expand_ = false,
    },
    {
        .name_ = "groupmini_expand_processesGroup_eng",
        .desc_ = "mini-group for of processes dumper by eng",
        .list_ = processesGroupMini_eng_,
        .size_ = ArraySize(processesGroupMini_eng_),
        .type_ = DumperConstant::GROUPTYPE_PID,
        .expand_ = true,
    },
    {
        .name_ = "groupmini_pid_expand_processesGroup_eng",
        .desc_ = "mini-group for of processes dumper by eng",
        .list_ = processesPidGroupMini_eng_,
        .size_ = ArraySize(processesPidGroupMini_eng_),
        .type_ = DumperConstant::GROUPTYPE_PID,
        .expand_ = true,
    },
};

const int ConfigData::dumperSum_ = ArraySize(dumpers_);
const int ConfigData::groupSum_ = ArraySize(groups_);
const int ConfigData::NEST_MAX = 10;

ConfigData::ConfigData()
{
}

ConfigData::~ConfigData()
{
}
} // namespace HiviewDFX
} // namespace OHOS
