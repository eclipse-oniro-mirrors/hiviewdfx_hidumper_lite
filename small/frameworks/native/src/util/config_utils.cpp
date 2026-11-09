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
#include "util/config_utils.h"
#include "hilog_wrapper.h"
#include "dump_common_utils.h"
namespace OHOS {
namespace HiviewDFX {
namespace {
constexpr int ROOT_UID = 0;
constexpr int BMS_UID = 1000;
constexpr int APP_FIRST_UID = 10000;
} // namespace

ConfigUtils::ConfigUtils(const std::shared_ptr<DumperParameter> &param) : dumperParam_(param)
{
}

ConfigUtils::~ConfigUtils()
{
    pidInfos_.clear();
    cpuInfos_.clear();
    currentPidInfos_.clear();
    currentPidInfo_.Reset();
}

DumpStatus ConfigUtils::GetDumperConfigs(const std::shared_ptr<DumperParameter> &param)
{
    DUMPER_HILOGD(MODULE_COMMON, "enter|");
    DumpStatus ret = DumpStatus::DUMP_FAIL;

    if (param == nullptr) {
        return ret;
    }
    ConfigUtils configUtils(param);
    ret = configUtils.GetDumperConfigs();

    DUMPER_HILOGD(MODULE_COMMON, "leave|ret=%{public}d", ret);
    return ret;
}

DumpStatus ConfigUtils::GetDumperConfigs()
{
    DUMPER_HILOGD(MODULE_COMMON, "enter|");

    DumpCommonUtils::GetCpuInfos(cpuInfos_);
    DUMPER_HILOGD(MODULE_COMMON, "debug|cpuInfos=%{public}zu", cpuInfos_.size());

    std::vector<std::shared_ptr<DumpCfg>> dumpCfgs;

    currentPidInfo_.Reset();
    currentPidInfos_.clear();

    HandleDumpCpuUsage(dumpCfgs); // pid
    HandleDumpMem(dumpCfgs);
    HandleDumpProcesses(dumpCfgs);
    DUMPER_HILOGD(MODULE_COMMON, "debug|dumpCfgs=%{public}zu", dumpCfgs.size());
    dumperParam_->SetExecutorConfigList(dumpCfgs);
    DUMPER_HILOGD(MODULE_COMMON, "leave|");
    return DumpStatus::DUMP_OK;
}

bool ConfigUtils::MergePidInfos(std::vector<DumpCommonUtils::PidInfo> &pidInfos, int pid)
{
    pidInfos.clear();
    if (pid < 0) {
        pidInfos_.clear();
        currentPidInfo_.pid_ = pid;
        currentPidInfo_.uid_ = -1;
        DumpCommonUtils::GetPidInfos(pidInfos_);
        DUMPER_HILOGI(MODULE_COMMON, "info|pidInfos=%{public}zu", pidInfos_.size());
        pidInfos.assign(pidInfos_.begin(), pidInfos_.end());
    } else {
        if (DumpCommonUtils::GetProcessInfo(pid, currentPidInfo_)) {
            pidInfos.push_back(currentPidInfo_);
        }
    }
    return true;
}

bool ConfigUtils::HandleDumpCpuUsage(std::vector<std::shared_ptr<DumpCfg>> &dumpCfgs)
{
    const DumperOpts &dumperOpts = dumperParam_->GetOpts();
    if (!dumperOpts.isDumpCpuUsage_) {
        return false;
    }

    DUMPER_HILOGD(MODULE_COMMON, "debug|cpu usage, cpuUsagePid=%{public}d", dumperOpts.cpuUsagePid_);
    currentPidInfo_.Reset();
    currentPidInfos_.clear();
    MergePidInfos(currentPidInfos_, dumperOpts.cpuUsagePid_);

    std::shared_ptr<OptionArgs> args;
    GetConfig(CONFIG_GROUP_CPU_USAGE, dumpCfgs, args);

    currentPidInfos_.clear();
    currentPidInfo_.Reset();
    return true;
}

bool ConfigUtils::HandleDumpMem(std::vector<std::shared_ptr<DumpCfg>> &dumpCfgs)
{
    const DumperOpts &dumperOpts = dumperParam_->GetOpts();
    if (!dumperOpts.isDumpMem_) {
        return false;
    }

    DUMPER_HILOGD(MODULE_COMMON, "debug|mem, memPid=%{public}d", dumperOpts.memPid_);
    HandleDumpMemCommon(dumperParam_, dumpCfgs);
    return true;
}

bool ConfigUtils::HandleDumpProcesses(std::vector<std::shared_ptr<DumpCfg>> &dumpCfgs)
{
    const DumperOpts &dumperOpts = dumperParam_->GetOpts();
    if (!dumperOpts.isDumpProcesses_) {
        return false;
    }

    DUMPER_HILOGD(MODULE_COMMON, "debug|processes");
    currentPidInfo_.Reset();
    currentPidInfos_.clear();
    MergePidInfos(currentPidInfos_, dumperOpts.processPid_);

    std::shared_ptr<OptionArgs> args;
    if (dumperOpts.processPid_ < 0) {
        GetConfig(CONFIG_GROUP_PROCESSES_ENG, dumpCfgs, args);
    } else {
        GetConfig(CONFIG_GROUP_PROCESSES_PID_ENG, dumpCfgs, args);
    }

    currentPidInfos_.clear();
    currentPidInfo_.Reset();
    return true;
}

void ConfigUtils::HandleDumpMemCommon(std::shared_ptr<DumperParameter> dumpParam,
    std::vector<std::shared_ptr<DumpCfg>> &dumpCfgs)
{
    currentPidInfo_.Reset();
    currentPidInfos_.clear();
    MergePidInfos(currentPidInfos_, dumpParam->GetOpts().memPid_);
    std::shared_ptr<OptionArgs> args;
    GetConfig(CONFIG_GROUP_MEMORY, dumpCfgs, args);
    currentPidInfos_.clear();
    currentPidInfo_.Reset();
}

int ConfigUtils::GetDumpLevelByPid(int uid, const DumpCommonUtils::PidInfo &pidInfo)
{
    int ret = DumperConstant::LEVEL_NONE;
    if (uid == ROOT_UID) {
        ret = DumperConstant::LEVEL_HIGH;
    } else if (uid < APP_FIRST_UID) {
        ret = DumperConstant::LEVEL_MIDDLE;
    } else {
        if (uid == pidInfo.uid_) {
            ret = DumperConstant::LEVEL_MIDDLE;
        }
    }
    return ret;
}

DumpStatus ConfigUtils::GetConfig(const std::string &name, std::vector<std::shared_ptr<DumpCfg>> &result,
                                  std::shared_ptr<OptionArgs> args)
{
    DumpStatus ret = DumpStatus::DUMP_FAIL;
    if (name.find(CONFIG_DUMPER_) == 0) {
        DUMPER_HILOGD(MODULE_COMMON, "debug|dumper, name=%{public}s", name.c_str());
        ret = GetDumper(name, result, args);
    } else if (name.find(CONFIG_GROUP_) == 0) {
        DUMPER_HILOGD(MODULE_COMMON, "debug|group, name=%{public}s", name.c_str());
        ret = GetGroup(name, result, args);
    } else {
        DUMPER_HILOGE(MODULE_COMMON, "error|name=%{public}s", name.c_str());
    }
    return ret;
}

DumpStatus ConfigUtils::GetDumper(int index, std::vector<std::shared_ptr<DumpCfg>> &result,
                                  std::shared_ptr<OptionArgs> args, int level)
{
    if ((index < 0) || (index >= dumperSum_)) {
        return DumpStatus::DUMP_INVALID_ARG;
    }
    auto itemlist = dumpers_[index].list_;
    auto itemsize = dumpers_[index].size_;
    for (int i = 0; i < itemsize; i++) {
        if (DumpCfg::IsFilter(itemlist[i].class_) && DumpCfg::IsLevel(level)) {
            if ((itemlist[i].level_ != DumperConstant::LEVEL_ALL) && (itemlist[i].level_ != level)) {
                continue;
            }
        }
        auto dumpCfg = DumpCfg::Create();
        dumpCfg->name_ = itemlist[i].name_;
        dumpCfg->desc_ = itemlist[i].desc_;
        dumpCfg->target_ = itemlist[i].target_;
        dumpCfg->section_ = itemlist[i].section_;
        dumpCfg->class_ = itemlist[i].class_;
        dumpCfg->level_ = itemlist[i].level_;
        dumpCfg->loop_ = itemlist[i].loop_;
        dumpCfg->filterCfg_ = itemlist[i].filterCfg_;
        dumpCfg->args_ = dumpCfg->IsDumper() ? args : nullptr;
        result.push_back(dumpCfg);
    }
    return DumpStatus::DUMP_OK;
}

DumpStatus ConfigUtils::GetDumper(const std::string &name, std::vector<std::shared_ptr<DumpCfg>> &result,
                                  std::shared_ptr<OptionArgs> args, int level)
{
    DumpStatus ret = DumpStatus::DUMP_FAIL;
    int index = -1;
    for (int i = 0; i < dumperSum_; i++) {
        if (dumpers_[i].name_.empty()) {
            continue;
        }
        if (name != dumpers_[i].name_) {
            continue;
        }
        index = i;
        break;
    }
    if (index > -1) {
        ret = GetDumper(index, result, args, level);
    }
    return ret;
}

DumpStatus ConfigUtils::GetGroupSimple(const GroupCfg &groupCfg, std::vector<std::shared_ptr<DumpCfg>> &result,
                                       std::shared_ptr<OptionArgs> args, int level, int nest)
{
    if (nest > NEST_MAX) {
        return DumpStatus::DUMP_INVALID_ARG;
    }
    if ((groupCfg.list_ == nullptr) || (groupCfg.size_ < 1)) {
        return DumpStatus::DUMP_OK;
    }

    auto dumpGroup = DumpCfg::Create();
    if (groupCfg.expand_) {
        dumpGroup->class_ = DumperConstant::GROUP;
        dumpGroup->name_ = groupCfg.name_;
        dumpGroup->desc_ = groupCfg.desc_;
        dumpGroup->type_ = groupCfg.type_;
        dumpGroup->expand_ = groupCfg.expand_;
        result.push_back(dumpGroup);
    }
    auto &outlist = (groupCfg.expand_) ? dumpGroup->childs_ : result;

    for (int i = 0; i < groupCfg.size_; i++) {
        if (groupCfg.list_[i].empty()) {
            continue;
        }
        if (DumpCommonUtils::StartWith(groupCfg.list_[i], CONFIG_DUMPER_)) {
            GetDumper(groupCfg.list_[i], outlist, args, level);
        } else if (DumpCommonUtils::StartWith(groupCfg.list_[i], CONFIG_MINIGROUP_)) {
            GetGroup(groupCfg.list_[i], outlist, args, level, nest + 1);
        } else {
            DUMPER_HILOGE(MODULE_COMMON, "error|name=%{public}s", groupCfg.name_.c_str());
            return DumpStatus::DUMP_INVALID_ARG;
        }
    }

    return DumpStatus::DUMP_OK;
}

DumpStatus ConfigUtils::GetGroup(int index, std::vector<std::shared_ptr<DumpCfg>> &result,
                                 std::shared_ptr<OptionArgs> args, int level, int nest)
{
    if (nest > NEST_MAX) {
        return DumpStatus::DUMP_INVALID_ARG;
    }
    auto dumpGroup = DumpCfg::Create();
    dumpGroup->class_ = DumperConstant::GROUP;
    dumpGroup->name_ = groups_[index].name_;
    dumpGroup->desc_ = groups_[index].desc_;
    dumpGroup->type_ = groups_[index].type_;
    dumpGroup->expand_ = groups_[index].expand_;
    result.push_back(dumpGroup);
    if (dumpGroup->expand_ && (dumpGroup->type_ == DumperConstant::GROUPTYPE_PID)) {
        for (auto pidInfo : currentPidInfos_) {
            int newLevel = GetDumpLevelByPid(dumperParam_->GetUid(), pidInfo);
            if (newLevel == DumperConstant::LEVEL_NONE) {
                continue;
            }
            auto newArgs = OptionArgs::Clone(args);
            newArgs->SetPid(pidInfo.pid_, pidInfo.uid_);
            GetGroupSimple(groups_[index], dumpGroup->childs_, newArgs, newLevel, nest);
        }
    } else if (dumpGroup->expand_ && (dumpGroup->type_ == DumperConstant::GROUPTYPE_CPUID)) {
        for (auto cpuInfo : cpuInfos_) {
            auto newArgs = OptionArgs::Clone(args);
            newArgs->SetCpuId(cpuInfo.id_);
            GetGroupSimple(groups_[index], dumpGroup->childs_, newArgs, level, nest);
        }
    } else if (dumpGroup->type_ == DumperConstant::GROUPTYPE_PID) {
        int newLevel = GetDumpLevelByPid(dumperParam_->GetUid(), currentPidInfo_);
        if (newLevel != DumperConstant::LEVEL_NONE) {
            auto newArgs = OptionArgs::Clone(args);
            newArgs->SetPid(currentPidInfo_.pid_, currentPidInfo_.uid_);
            GetGroupSimple(groups_[index], dumpGroup->childs_, newArgs, level, nest);
        }
    } else if (dumpGroup->type_ == DumperConstant::GROUPTYPE_CPUID) {
        auto newArgs = OptionArgs::Clone(args);
        newArgs->SetCpuId(-1);
        GetGroupSimple(groups_[index], dumpGroup->childs_, newArgs, level, nest);
    } else if (dumpGroup->type_ == DumperConstant::NONE) {
        GetGroupSimple(groups_[index], dumpGroup->childs_, args, level, nest);
    } else {
        DUMPER_HILOGE(MODULE_COMMON, "error|type=%{public}d", dumpGroup->type_);
        return DumpStatus::DUMP_INVALID_ARG;
    }
    return DumpStatus::DUMP_OK;
}

DumpStatus ConfigUtils::GetGroup(const std::string &name, std::vector<std::shared_ptr<DumpCfg>> &result,
                                 std::shared_ptr<OptionArgs> args, int level, int nest)
{
    if (nest > NEST_MAX) {
        return DumpStatus::DUMP_INVALID_ARG;
    }
    DumpStatus ret = DumpStatus::DUMP_FAIL;
    int index = -1;
    // find group
    for (int i = 0; i < groupSum_; i++) {
        if (groups_[i].name_.empty()) {
            continue;
        }
        if (name != groups_[i].name_) {
            continue;
        }
        index = i;
        break;
    }

    // add dump config to tmpUse
    std::vector<std::shared_ptr<DumpCfg>> tmpUse;
    if (index > -1) {
        ret = GetGroup(index, tmpUse, args, level);
    }

    if (nest) {
        result.insert(result.end(), tmpUse.begin(), tmpUse.end());
    } else {
        // add section & add config to result
        SetSection(tmpUse, GetSectionName(name));
        ConvertTreeToList(tmpUse, result);
    }

    return ret;
}

std::string ConfigUtils::GetSectionName(const std::string &name)
{
    std::string ret;
    std::size_t pos = name.find_last_of(CONFIG_NAME_SPLIT);
    if (pos != std::string::npos) {
        ret = name.substr(pos + 1);
    }
    return ret;
}

void ConfigUtils::ConvertTreeToList(std::vector<std::shared_ptr<DumpCfg>> &tree,
                                    std::vector<std::shared_ptr<DumpCfg>> &list, int nest)
{
    if (nest > NEST_MAX) {
        return;
    }

    std::vector<std::shared_ptr<DumpCfg>> tmpUsed;
    for (auto item : tree) {
        if (item == nullptr) {
            continue;
        }
        tmpUsed.push_back(item);
        if (item->childs_.empty()) {
            continue;
        }
        for (auto child : item->childs_) {
            child->parent_ = item; // after point to parent, childs must be cleared.
        }
        ConvertTreeToList(item->childs_, tmpUsed, nest + 1);
        item->childs_.clear(); // must clear
    }

    list.insert(list.end(), tmpUsed.begin(), tmpUsed.end());
}

void ConfigUtils::SetSection(std::vector<std::shared_ptr<DumpCfg>> &dumpCfgs, const std::string &section, int nest)
{
    if (nest > NEST_MAX) {
        return;
    }

    for (auto dumpCfg : dumpCfgs) {
        if (dumpCfg == nullptr) {
            continue;
        }
        if (dumpCfg->IsDumper()) {
            dumpCfg->section_ = section;
        }
        if (dumpCfg->childs_.empty()) {
            continue;
        }
        SetSection(dumpCfg->childs_, section, nest + 1);
    }
}
} // namespace HiviewDFX
} // namespace OHOS
