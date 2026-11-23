/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
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
#ifndef HIDUMPER_SERVICES_DUMPER_OPTS_H
#define HIDUMPER_SERVICES_DUMPER_OPTS_H
#include <string>
namespace OHOS {
namespace HiviewDFX {
struct DumperOpts {
    bool isDumpCpuFreq_;
    bool isDumpCpuUsage_;
    int cpuUsagePid_;
    bool isDumpMem_;
    int memPid_;
    bool dumpMemPrune_;
    int timeInterval_;
    bool isReceivedSigInt_;
    bool isDumpProcesses_;
    int processPid_;

public:
    DumperOpts();
    void Reset();
    DumperOpts& operator = (const DumperOpts& opts);
    bool IsSelectAny() const;
    bool CheckOptions(std::string& errStr) const;
private:
    void ResetCpuOptions();
    void ResetMemOptions();
    void ResetProcessOptions();
    void AssignCpuOptions(const DumperOpts& opts);
    void AssignMemOptions(const DumperOpts& opts);
    void AssignProcessOptions(const DumperOpts& opts);
};
} // namespace HiviewDFX
} // namespace OHOS
#endif // HIDUMPER_SERVICES_DUMPER_OPTS_H
