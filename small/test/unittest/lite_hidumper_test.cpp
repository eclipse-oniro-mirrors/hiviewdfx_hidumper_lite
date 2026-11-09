/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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
#include <gtest/gtest.h>

#include <cstdio>
#include <fcntl.h>
#include <fstream>
#include <set>
#include <string>
#include <unistd.h>

#include "data_inventory.h"
#include "dump_common_utils.h"
#include "dump_manager_service.h"
#include "file_ex.h"
#include "string_ex.h"
#include "task/base/task_enable_config.h"
#include "unique_fd.h"

using namespace OHOS::HiviewDFX;
using OHOS::IsNumericStr;
using OHOS::StrToInt;
using OHOS::SaveStringToFd;

namespace {
const std::string TEST_CONFIG_PATH = "/storage/hidumper_lite_task_enable_test.json";

int RunHidumperCmd(std::initializer_list<const char *> args, std::string &out)
{
    const char *capturePath = "/storage/hidumper_lite_ut_output.txt";
    int fd = open(capturePath, O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        return -1;
    }
    int argc = static_cast<int>(args.size());
    int index = 0;
    char *argv[16] = {nullptr};
    for (const char *arg : args) {
        argv[index++] = const_cast<char *>(arg);
    }
    int32_t ret = DumpManagerService::GetInstance().Request(argc, argv, fd);
    out.clear();
    char buffer[4096];
    lseek(fd, 0, SEEK_SET);
    ssize_t readSize;
    while ((readSize = read(fd, buffer, sizeof(buffer))) > 0) {
        out.append(buffer, static_cast<size_t>(readSize));
    }
    close(fd);
    remove(capturePath);
    return static_cast<int>(ret);
}
} // namespace

/**
 * @tc.name: LiteUtilsStringEx001
 * @tc.type: FUNC
 */
TEST(LiteUtilsTest, StringEx)
{
    EXPECT_FALSE(IsNumericStr(""));
    EXPECT_TRUE(IsNumericStr("0"));
    EXPECT_TRUE(IsNumericStr("12345"));
    EXPECT_FALSE(IsNumericStr("-1"));
    EXPECT_FALSE(IsNumericStr("1a"));
    EXPECT_FALSE(IsNumericStr("1.5"));

    int value = 0;
    EXPECT_TRUE(StrToInt("123", value));
    EXPECT_EQ(value, 123);
    EXPECT_FALSE(StrToInt("", value));
    EXPECT_FALSE(StrToInt("12a", value));
    EXPECT_FALSE(StrToInt("2147483648", value));}

/**
 * @tc.name: LiteUtilsUniqueFd001
 * @tc.type: FUNC
 */
TEST(LiteUtilsTest, UniqueFd)
{
    int fd = dup(STDOUT_FILENO);
    ASSERT_GE(fd, 0);
    UniqueFd unique(fd);
    EXPECT_EQ(unique.Get(), fd);
    EXPECT_EQ(unique.Release(), fd);
    EXPECT_EQ(unique.Get(), -1);
    close(fd);
    UniqueFd moved(UniqueFd(dup(STDOUT_FILENO)));
    EXPECT_GE(moved.Get(), 0);
    moved.Reset();
    EXPECT_EQ(moved.Get(), -1);
}

/**
 * @tc.name: LiteUtilsFileEx001
 * @tc.type: FUNC
 */
TEST(LiteUtilsTest, FileEx)
{
    const std::string path = "/storage/hidumper_lite_file_ex_test.txt";
    const std::string content = "hidumper lite test";
    {
        std::ofstream file(path);
        file << content;
    }
    std::string loaded;
    EXPECT_TRUE(LoadStringFromFile(path, loaded));
    EXPECT_EQ(loaded, content);
    EXPECT_FALSE(LoadStringFromFile("/storage/not_exist_file_hidumper", loaded));

    int fd = open(path.c_str(), O_WRONLY | O_TRUNC | O_CREAT, 0644);
    ASSERT_GE(fd, 0);
    EXPECT_TRUE(SaveStringToFd(fd, "written-by-fd"));
    close(fd);
    EXPECT_TRUE(LoadStringFromFile(path, loaded));
    EXPECT_EQ(loaded, "written-by-fd");
    remove(path.c_str());
}

/**
 * @tc.name: DumpCommonUtils001
 * @tc.type: FUNC
 */
TEST(DumpCommonUtilsTest, PidsAndTime)
{
    std::vector<int32_t> pids = DumpCommonUtils::GetAllPids();
    bool hasSelf = false;
    for (int32_t pid : pids) {
        if (pid == static_cast<int32_t>(getpid())) {
            hasSelf = true;
            break;
        }
    }
    EXPECT_TRUE(hasSelf);

    std::string dateTime;
    DumpCommonUtils::GetDateAndTime(DumpCommonUtils::GetMilliseconds() / 1000, dateTime);
    EXPECT_FALSE(dateTime.empty());
}

/**
 * @tc.name: DataInventory001
 * @tc.type: FUNC
 */
TEST(DataInventoryTest, InjectGetRemove)
{
    DataInventory inventory;
    auto data = std::make_shared<std::vector<std::string>>(std::vector<std::string>{"a", "b"});
    EXPECT_TRUE(inventory.Inject(DataId::CPU_FREQ_INFO, data));
    auto ptr = inventory.GetPtr<std::vector<std::string>>(DataId::CPU_FREQ_INFO);
    ASSERT_NE(ptr, nullptr);
    EXPECT_EQ(ptr->size(), 2U);
    EXPECT_EQ((*ptr)[0], "a");

    EXPECT_FALSE(inventory.Inject(DataId::CPU_FREQ_INFO, data));
    EXPECT_EQ(inventory.Size(), 1U);

    inventory.RemoveRestData(std::set<DataId>{DataId::CPU_FREQ_INFO});
    EXPECT_EQ(inventory.Size(), 1U);
    inventory.RemoveRestData(std::set<DataId>{});
    EXPECT_EQ(inventory.Size(), 0U);
    EXPECT_EQ(inventory.GetPtr<std::vector<std::string>>(DataId::CPU_FREQ_INFO), nullptr);
}

/**
 * @tc.name: TaskEnableConfig001
 * @tc.type: FUNC
 */
TEST(TaskEnableConfigTest, ParseAndDefault)
{
    const std::string config = "{\"version\":\"1.0.0\",\"task_configs\":{"
        "\"TEST_KEEP_CPU_FREQ\":{\"task_id\":8,\"enabled\":true},"
        "\"TEST_DISABLE_KERNEL_MEM\":{\"task_id\":10004,\"enabled\":false}}}";
    {
        std::ofstream file(TEST_CONFIG_PATH);
        file << config;
    }
    EXPECT_EQ(TaskEnableConfig::GetInstance().LoadFromConfig(TEST_CONFIG_PATH), DumpStatus::DUMP_OK);
    EXPECT_TRUE(TaskEnableConfig::GetInstance().IsTaskEnabled(TaskId::DUMP_CPU_FREQ_INFO));
    EXPECT_FALSE(TaskEnableConfig::GetInstance().IsTaskEnabled(TaskId::WRITE_KERNEL_MEM_INFO));
    EXPECT_TRUE(TaskEnableConfig::GetInstance().IsTaskEnabled(TaskId::WRITE_CPU_FREQ_INFO));
    remove(TEST_CONFIG_PATH.c_str());
}

/**
 * @tc.name: HidumperCmdHelp001
 * @tc.type: FUNC
 */
TEST(HidumperCmdTest, Help)
{
    std::string out;
    int ret = RunHidumperCmd({"hidumper", "-h"}, out);
    EXPECT_EQ(ret, static_cast<int>(DumpStatus::DUMP_HELP));
    EXPECT_NE(out.find("usage:"), std::string::npos);

    ret = RunHidumperCmd({"hidumper"}, out);
    EXPECT_EQ(ret, static_cast<int>(DumpStatus::DUMP_HELP));
    EXPECT_NE(out.find("usage:"), std::string::npos);
}

/**
 * @tc.name: HidumperCmdInvalid001
 * @tc.type: FUNC
 */
TEST(HidumperCmdTest, InvalidOption)
{
    std::string out;
    int ret = RunHidumperCmd({"hidumper", "--no-such-option"}, out);
    EXPECT_NE(ret, static_cast<int>(DumpStatus::DUMP_OK));
    EXPECT_NE(out.find("hidumper:"), std::string::npos);
}

/**
 * @tc.name: HidumperCmdCpuUsage001
 * @tc.type: FUNC
 */
TEST(HidumperCmdTest, CpuUsage)
{
    std::string out;
    int ret = RunHidumperCmd({"hidumper", "--cpuusage"}, out);
    EXPECT_EQ(ret, static_cast<int>(DumpStatus::DUMP_OK));
    EXPECT_NE(out.find("Load average:"), std::string::npos);
    EXPECT_NE(out.find("CPU usage from "), std::string::npos);
    EXPECT_NE(out.find("Total:"), std::string::npos);
    EXPECT_NE(out.find("Details of Processes:"), std::string::npos);
}

/**
 * @tc.name: HidumperCmdMemPid001
 * @tc.type: FUNC
 */
TEST(HidumperCmdTest, MemByPid)
{
    std::string pidStr = std::to_string(getpid());
    std::string out;
    int ret = RunHidumperCmd({"hidumper", "--mem", pidStr.c_str()}, out);
    EXPECT_EQ(ret, static_cast<int>(DumpStatus::DUMP_OK));
    EXPECT_NE(out.find("Process Memory Information :"), std::string::npos);
    EXPECT_NE(out.find("VmPeak:"), std::string::npos);
    EXPECT_NE(out.find("VmRSS:"), std::string::npos);
    EXPECT_NE(out.find("Rss(total):"), std::string::npos);
    EXPECT_NE(out.find("Pss(total):"), std::string::npos);
}

/**
 * @tc.name: HidumperCmdMemPrune001
 * @tc.type: FUNC
 */
TEST(HidumperCmdTest, MemPrune)
{
    std::string out;
    int ret = RunHidumperCmd({"hidumper", "--mem", "--prune"}, out);
    EXPECT_EQ(ret, static_cast<int>(DumpStatus::DUMP_OK));
    EXPECT_NE(out.find("-------------------------------[memory]-------------------------------"), std::string::npos);
}

/**
 * @tc.name: HidumperCmdCpuFreq001
 * @tc.type: FUNC
 */
TEST(HidumperCmdTest, CpuFreq)
{
    std::string out;
    RunHidumperCmd({"hidumper", "--cpufreq"}, out);
    EXPECT_NE(out.find("-------------------------------[cpufreq]-------------------------------"), std::string::npos);
}

/**
 * @tc.name: HidumperCmdProcesses001
 * @tc.type: FUNC
 */
TEST(HidumperCmdTest, ProcessesList)
{
    if (access("/system/bin/ps", X_OK) != 0) {
        SUCCEED() << "skip: /system/bin/ps not available";
        return;
    }
    std::string out;
    int ret = RunHidumperCmd({"hidumper", "-p"}, out);
    EXPECT_EQ(ret, static_cast<int>(DumpStatus::DUMP_OK));
    EXPECT_NE(out.find("cmd is: ps -efT"), std::string::npos);
}
