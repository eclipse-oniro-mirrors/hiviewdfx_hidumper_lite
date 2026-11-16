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

#include "dump_common_utils.h"
#include "dump_manager_service.h"
#include "file_ex.h"
#include "string_ex.h"
#include "unique_fd.h"

using namespace OHOS::HiviewDFX;
using OHOS::IsNumericStr;
using OHOS::LoadStringFromFile;
using OHOS::StrToInt;
using OHOS::TrimStr;
using OHOS::IsSubStr;
using OHOS::ReplaceStr;
using OHOS::SaveStringToFd;

namespace {

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
    EXPECT_EQ(0, remove(capturePath));
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
    EXPECT_FALSE(StrToInt("2147483648", value));
    EXPECT_TRUE(StrToInt("-45", value));
    EXPECT_EQ(value, -45);
    EXPECT_TRUE(StrToInt("2147483647", value));
    EXPECT_FALSE(StrToInt("-2147483649", value));
    EXPECT_TRUE(StrToInt("+5", value));
    EXPECT_EQ(value, 5);
    EXPECT_TRUE(StrToInt(" 7", value));
    EXPECT_EQ(value, 7);
    EXPECT_TRUE(IsSubStr("abcdef", "cd"));
    EXPECT_FALSE(IsSubStr("abc", "abcd"));
    EXPECT_EQ(ReplaceStr("a-b-c", "-", "+"), "a+b+c");
    EXPECT_EQ(ReplaceStr("aaa", "a", "bb"), "bbbbbb");
}

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
    EXPECT_EQ(0, remove(path.c_str()));
}

/**
 * @tc.name: LiteUtilsTrimStr001
 * @tc.type: FUNC
 */
TEST(LiteUtilsTest, TrimStrBranches)
{
    EXPECT_EQ(TrimStr("  x  "), "x");
    EXPECT_EQ(TrimStr("   "), "");
    EXPECT_EQ(TrimStr(""), "");
    EXPECT_EQ(TrimStr("y"), "y");
    EXPECT_EQ(TrimStr("xxaxx", 'x'), "a");
    EXPECT_EQ(TrimStr("xxx", 'x'), "");
}

/**
 * @tc.name: LiteUtilsSplitStr001
 * @tc.type: FUNC
 */
TEST(LiteUtilsTest, SplitStrBranches)
{
    std::vector<std::string> out;
    OHOS::SplitStr("a, b,,c", ",", out);
    EXPECT_EQ(out.size(), 3U);
    EXPECT_EQ(out[0], "a");
    EXPECT_EQ(out[1], "b");
    EXPECT_EQ(out[2], "c");

    OHOS::SplitStr("a,,b", ",", out, true);
    EXPECT_EQ(out.size(), 3U);
    EXPECT_TRUE(out[1].empty());

    OHOS::SplitStr("abc", "", out, false);
    EXPECT_EQ(out.size(), 1U);
    EXPECT_EQ(out[0], "abc");

    OHOS::SplitStr("", ",", out, false);
    EXPECT_TRUE(out.empty());

    OHOS::SplitStr(",", ",", out, true);
    EXPECT_EQ(out.size(), 2U);
    EXPECT_TRUE(out[0].empty());
    EXPECT_TRUE(out[1].empty());
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
 * @tc.name: HidumperCmdOverlongArg001
 * @tc.type: FUNC
 */
TEST(HidumperCmdTest, OverlongArg)
{
    std::string overlongArg(SINGLE_ARG_MAXLEN + 1, '1');
    std::string out;
    int ret = RunHidumperCmd({"hidumper", "--mem", overlongArg.c_str()}, out);
    EXPECT_EQ(ret, static_cast<int>(DumpStatus::DUMP_INVALID_ARG));
    EXPECT_EQ(out.find("Total Memory Usage by PID"), std::string::npos);
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
    EXPECT_NE(out.find("Pss"), std::string::npos);
    EXPECT_NE(out.find("Total"), std::string::npos);
    EXPECT_NE(out.find("Heap"), std::string::npos);
    EXPECT_NE(out.find("Free"), std::string::npos);
    EXPECT_NE(out.find("Shared"), std::string::npos);
    EXPECT_NE(out.find("Clean"), std::string::npos);
    EXPECT_NE(out.find("( kB )"), std::string::npos);
    EXPECT_NE(out.find("native heap"), std::string::npos);
    EXPECT_NE(out.find("Purgeable:"), std::string::npos);
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

/**
 * @tc.name: HidumperCmdProcessesPid001
 * @tc.type: FUNC
 */
TEST(HidumperCmdTest, ProcessesListByPid)
{
    if (access("/bin/ps", X_OK) != 0) {
        SUCCEED() << "skip: ps not available";
        return;
    }
    std::string pidStr = std::to_string(getpid());
    std::string out;
    int ret = RunHidumperCmd({"hidumper", "-p", pidStr.c_str()}, out);
    EXPECT_EQ(ret, static_cast<int>(DumpStatus::DUMP_OK));
    EXPECT_NE(out.find("cmd is: ps -efT -p " + pidStr), std::string::npos);
    EXPECT_NE(out.find("/proc/" + pidStr + "/wchan"), std::string::npos);
    EXPECT_NE(out.find("/proc/" + pidStr + "/mountinfo"), std::string::npos);
}

/**
 * @tc.name: HidumperCmdCpuUsagePid001
 * @tc.type: FUNC
 */
TEST(HidumperCmdTest, CpuUsageByPid)
{
    std::string pidStr = std::to_string(getpid());
    std::string out;
    int ret = RunHidumperCmd({"hidumper", "--cpuusage", pidStr.c_str()}, out);
    EXPECT_EQ(ret, static_cast<int>(DumpStatus::DUMP_OK));
    EXPECT_NE(out.find("Load average:"), std::string::npos);
    EXPECT_NE(out.find("Details of Processes:"), std::string::npos);
}

/**
 * @tc.name: HidumperCmdMemOverview001
 * @tc.type: FUNC
 */
TEST(HidumperCmdTest, MemOverview)
{
    std::string out;
    int ret = RunHidumperCmd({"hidumper", "--mem"}, out);
    EXPECT_EQ(ret, static_cast<int>(DumpStatus::DUMP_OK));
    EXPECT_NE(out.find("Total Memory Usage by PID:"), std::string::npos);
    EXPECT_NE(out.find("Total RAM:"), std::string::npos);
}

/**
 * @tc.name: HidumperCmdMemTableConsistency001
 * @tc.type: FUNC
 */
TEST(HidumperCmdTest, MemTableConsistency)
{
    std::string pidStr = std::to_string(getpid());
    std::string out;
    int ret = RunHidumperCmd({"hidumper", "--mem", pidStr.c_str()}, out);
    EXPECT_EQ(ret, static_cast<int>(DumpStatus::DUMP_OK));
    EXPECT_NE(out.find("Pss"), std::string::npos);
    EXPECT_NE(out.find("( kB )"), std::string::npos);
    EXPECT_NE(out.find("Heap"), std::string::npos);
    EXPECT_NE(out.find("SwapPss"), std::string::npos);
    EXPECT_NE(out.find("DMA:"), std::string::npos);
    EXPECT_NE(out.find("Total"), std::string::npos);
    EXPECT_NE(out.find("FilePage other"), std::string::npos);
}

/**
 * @tc.name: HidumperCmdHelpContent001
 * @tc.type: FUNC
 */
TEST(HidumperCmdTest, HelpContent)
{
    std::string out;
    RunHidumperCmd({"hidumper", "-h"}, out);
    EXPECT_NE(out.find("|help text for the tool"), std::string::npos);
    EXPECT_NE(out.find("--cpufreq"), std::string::npos);
    EXPECT_NE(out.find("--cpuusage"), std::string::npos);
    EXPECT_NE(out.find("--mem"), std::string::npos);
    EXPECT_NE(out.find("-p"), std::string::npos);
}

/**
 * @tc.name: HidumperCmdInvalidMemArg001
 * @tc.type: FUNC
 */
TEST(HidumperCmdTest, InvalidMemArg)
{
    std::string out;
    int ret = RunHidumperCmd({"hidumper", "--mem", "not-a-pid"}, out);
    EXPECT_EQ(ret, static_cast<int>(DumpStatus::DUMP_INVALID_ARG));
    EXPECT_NE(out.find("hidumper:"), std::string::npos);
}

/**
 * @tc.name: HidumperCmdInvalidPid001
 * @tc.type: FUNC
 */
TEST(HidumperCmdTest, InvalidPidArg)
{
    std::string out;
    int ret = RunHidumperCmd({"hidumper", "-p", "999999"}, out);
    EXPECT_NE(ret, static_cast<int>(DumpStatus::DUMP_OK));
}

/**
 * @tc.name: HidumperCmdMemNonexistentPid001
 * @tc.type: FUNC
 */
TEST(HidumperCmdTest, MemNonexistentPid)
{
    std::string out;
    int ret = RunHidumperCmd({"hidumper", "--mem", "999999"}, out);
    EXPECT_EQ(ret, static_cast<int>(DumpStatus::DUMP_FAIL));
}
