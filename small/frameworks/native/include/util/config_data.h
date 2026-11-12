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
#ifndef HIDUMPER_SERVICES_CONFIG_DATA_H
#define HIDUMPER_SERVICES_CONFIG_DATA_H
#include <string>
namespace OHOS {
namespace HiviewDFX {
class ConfigData {
public:
    ConfigData();
    ~ConfigData();
    static const std::string CONFIG_NAME_SPLIT;
    static const std::string CONFIG_GROUP;
    static const std::string CONFIG_GROUP_;
    static const std::string CONFIG_MINIGROUP;
    static const std::string CONFIG_MINIGROUP_;
    static const std::string CONFIG_DUMPER;
    static const std::string CONFIG_DUMPER_;
    static const std::string CONFIG_GROUP_CPU_USAGE;
    static const std::string CONFIG_GROUP_CPU_FREQ;
    static const std::string CONFIG_GROUP_MEMORY;
    static const std::string CONFIG_GROUP_PROCESSES;
    static const std::string CONFIG_GROUP_PROCESSES_PID;
    static const std::string CONFIG_GROUP_PROCESSES_ENG;
    static const std::string CONFIG_GROUP_PROCESSES_PID_ENG;
protected:
    struct ItemCfg {
        const std::string &name_;
        const std::string &desc_;
        const std::string &target_;
        const std::string &section_;
        const int &class_;
        const int &level_;
        const int &loop_;
        const std::string &filterCfg_;
    };
    struct DumperCfg {
        const std::string &name_;
        const std::string &desc_;
        const ItemCfg * const &list_;
        const int &size_;
    };
    struct GroupCfg {
        const std::string &name_;
        const std::string &desc_;
        const std::string * const &list_;
        const int &size_;
        const int type_; // DumperConstant::NONE, GROUPTYPE_PID, GROUPTYPE_CPUID
        const bool expand_; // true: expand; false: non-expand
    };
    static const GroupCfg groups_[];
    static const int groupSum_;
    static const DumperCfg dumpers_[];
    static const int dumperSum_;
    static const int NEST_MAX;
private:
    static const ItemCfg cpuUsageDumper_[];
    static const ItemCfg memDumper_[];
    static const ItemCfg threadsDumper_[];
    static const ItemCfg threadsPidDumper_[];
    static const ItemCfg blockChannelDumper_[];
    static const ItemCfg excuteTimeDumper_[];
    static const ItemCfg mountInfoDumper_[];
    static const ItemCfg mapDumper_[];
    static const ItemCfg cpuFreqDumper_[];
    static const std::string cpuUsageGroup_[];
    static const std::string cpuFreqGroup_[];
    static const std::string memoryGroup_[];
    static const std::string processesGroup_[];
    static const std::string processesPidGroup_[];
    static const std::string processesGroupMini_[];
    static const std::string processesPidGroupMini_[];
    static const std::string processesGroup_eng_[];
    static const std::string processesPidGroup_eng_[];
    static const std::string processesGroupMini_eng_[];
    static const std::string processesPidGroupMini_eng_[];
};
} // namespace HiviewDFX
} // namespace OHOS
#endif // HIDUMPER_SERVICES_CONFIG_DATA_H
