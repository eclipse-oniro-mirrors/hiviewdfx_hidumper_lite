/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include "task/base/task_enable_config.h"

#include <cerrno>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <mutex>

namespace OHOS {
namespace HiviewDFX {
namespace {
struct MiniJson {
    enum class Type { NUL, BOOL, NUMBER, STRING, OBJECT, ARRAY };
    Type type = Type::NUL;
    bool boolVal = false;
    double numVal = 0;
    std::string strVal;
    std::vector<std::pair<std::string, MiniJson>> items;
    const MiniJson* Find(const std::string& key) const
    {
        if (type != Type::OBJECT) {
            return nullptr;
        }
        for (const auto& item : items) {
            if (item.first == key) {
                return &item.second;
            }
        }
        return nullptr;
    }
};

class MiniJsonParser {
public:
    explicit MiniJsonParser(const std::string& text) : text_(text) {}

    bool Parse(MiniJson& value)
    {
        pos_ = 0;
        SkipSpace();
        if (!ParseValue(value)) {
            return false;
        }
        SkipSpace();
        return pos_ == text_.size();
    }

private:
    void SkipSpace()
    {
        while (pos_ < text_.size() && isspace(static_cast<unsigned char>(text_[pos_]))) {
            pos_++;
        }
    }

    bool Match(char expected)
    {
        if (pos_ < text_.size() && text_[pos_] == expected) {
            pos_++;
            return true;
        }
        return false;
    }

    bool ParseValue(MiniJson& value)
    {
        SkipSpace();
        if (pos_ >= text_.size()) {
            return false;
        }
        char c = text_[pos_];
        if (c == '{') {
            return ParseObject(value);
        }
        if (c == '[') {
            return ParseArray(value);
        }
        if (c == '"') {
            value.type = MiniJson::Type::STRING;
            return ParseString(value.strVal);
        }
        if (c == 't' || c == 'f') {
            value.type = MiniJson::Type::BOOL;
            return ParseBool(value.boolVal);
        }
        if (c == 'n') {
            value.type = MiniJson::Type::NUL;
            return MatchWord("null");
        }
        return ParseNumber(value);
    }

    bool ParseObject(MiniJson& value)
    {
        value.type = MiniJson::Type::OBJECT;
        pos_++; // '{'
        SkipSpace();
        if (Match('}')) {
            return true;
        }
        while (true) {
            SkipSpace();
            std::string key;
            if (!ParseString(key)) {
                return false;
            }
            SkipSpace();
            if (!Match(':')) {
                return false;
            }
            MiniJson child;
            if (!ParseValue(child)) {
                return false;
            }
            value.items.emplace_back(key, std::move(child));
            SkipSpace();
            if (Match(',')) {
                continue;
            }
            return Match('}');
        }
    }

    bool ParseArray(MiniJson& value)
    {
        value.type = MiniJson::Type::ARRAY;
        pos_++; // '['
        SkipSpace();
        if (Match(']')) {
            return true;
        }
        while (true) {
            MiniJson child;
            if (!ParseValue(child)) {
                return false;
            }
            value.items.emplace_back("", std::move(child));
            SkipSpace();
            if (Match(',')) {
                continue;
            }
            return Match(']');
        }
    }

    bool ParseString(std::string& value)
    {
        if (!Match('"')) {
            return false;
        }
        value.clear();
        while (pos_ < text_.size()) {
            char c = text_[pos_++];
            if (c == '"') {
                return true;
            }
            if (c != '\\') {
                value.push_back(c);
                continue;
            }
            if (pos_ >= text_.size()) {
                return false;
            }
            char esc = text_[pos_++];
            switch (esc) {
                case '"': value.push_back('"'); break;
                case '\\': value.push_back('\\'); break;
                case '/': value.push_back('/'); break;
                case 'b': value.push_back('\b'); break;
                case 'f': value.push_back('\f'); break;
                case 'n': value.push_back('\n'); break;
                case 'r': value.push_back('\r'); break;
                case 't': value.push_back('\t'); break;
                default:
                    return false;
            }
        }
        return false;
    }

    bool ParseBool(bool& value)
    {
        if (MatchWord("true")) {
            value = true;
            return true;
        }
        if (MatchWord("false")) {
            value = false;
            return true;
        }
        return false;
    }

    bool MatchWord(const char* word)
    {
        size_t len = strlen(word);
        if (text_.compare(pos_, len, word) == 0) {
            pos_ += len;
            return true;
        }
        return false;
    }

    bool ParseNumber(MiniJson& value)
    {
        errno = 0;
        const char* start = text_.c_str() + pos_;
        char* end = nullptr;
        double number = strtod(start, &end);
        if (end == start || errno == ERANGE || std::isnan(number)) {
            return false;
        }
        pos_ = static_cast<size_t>(end - text_.c_str());
        value.type = MiniJson::Type::NUMBER;
        value.numVal = number;
        return true;
    }

    const std::string& text_;
    size_t pos_ = 0;
};
} // namespace

TaskEnableConfig::TaskEnableConfig() = default;
TaskEnableConfig::~TaskEnableConfig() = default;

DumpStatus TaskEnableConfig::LoadFromConfig(const std::string& configPath)
{
    if (configLoaded_) {
        DUMPER_HILOGD(MODULE_COMMON, "Task enable config already loaded");
        return DumpStatus::DUMP_OK;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    if (configLoaded_) {
        DUMPER_HILOGD(MODULE_COMMON, "Task enable config already loaded (double-check)");
        return DumpStatus::DUMP_OK;
    }

    DUMPER_HILOGD(MODULE_COMMON, "Loading task enable config from: %{public}s", configPath.c_str());
    DumpStatus status = ParseConfigFile(configPath);
    if (status == DumpStatus::DUMP_OK) {
        configLoaded_ = true;
        DUMPER_HILOGD(MODULE_COMMON, "Task enable config loaded successfully");
    } else {
        DUMPER_HILOGE(MODULE_COMMON, "Failed to load task enable config");
    }
    return status;
}

DumpStatus TaskEnableConfig::ParseConfigFile(const std::string& configPath)
{
    std::ifstream file(configPath);
    if (!file.is_open()) {
        DUMPER_HILOGW(MODULE_COMMON, "Config file not exist: %{public}s, use default enable", configPath.c_str());
        return DumpStatus::DUMP_OK;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();
    file.close();

    return ParseJsonContent(content);
}

DumpStatus TaskEnableConfig::ParseJsonContent(const std::string& content)
{
    MiniJson root;
    MiniJsonParser parser(content);
    if (!parser.Parse(root)) {
        DUMPER_HILOGW(MODULE_COMMON, "Failed to parse JSON content, use default enable");
        return DumpStatus::DUMP_OK;
    }

    const MiniJson* taskConfigs = root.Find("task_configs");
    if (taskConfigs == nullptr || taskConfigs->type != MiniJson::Type::OBJECT) {
        DUMPER_HILOGW(MODULE_COMMON, "Missing or invalid 'task_configs' field, use default enable");
        return DumpStatus::DUMP_OK;
    }

    for (const auto& taskEntry : taskConfigs->items) {
        const std::string& taskName = taskEntry.first;
        const MiniJson& taskItem = taskEntry.second;
        if (taskItem.type != MiniJson::Type::OBJECT) {
            continue;
        }
        const MiniJson* taskIdItem = taskItem.Find("task_id");
        if (taskIdItem == nullptr || taskIdItem->type != MiniJson::Type::NUMBER) {
            DUMPER_HILOGW(MODULE_COMMON, "Missing or invalid 'task_id' for task: %{public}s", taskName.c_str());
            continue;
        }

        TaskId taskId = static_cast<TaskId>(static_cast<int64_t>(taskIdItem->numVal));
        bool enabled = true;
        const MiniJson* enabledItem = taskItem.Find("enabled");
        if (enabledItem != nullptr && enabledItem->type == MiniJson::Type::BOOL) {
            enabled = enabledItem->boolVal;
        }

        taskEnableMap_[taskId] = enabled;
        DUMPER_HILOGD(MODULE_COMMON, "Task %{public}s (ID: %{public}d) enabled: %{public}s",
                      taskName.c_str(), taskId, enabled ? "true" : "false");
    }

    DUMPER_HILOGD(MODULE_COMMON, "Parsed %{public}zu task enable configs", taskEnableMap_.size());
    return DumpStatus::DUMP_OK;
}

bool TaskEnableConfig::IsTaskEnabled(TaskId taskId) const
{
    std::lock_guard<std::mutex> lock(mutex_);

    if (!configLoaded_) {
        DUMPER_HILOGW(MODULE_COMMON, "Config not loaded yet, defaulting to enabled for task ID: %{public}d", taskId);
        return true;
    }
    auto it = taskEnableMap_.find(taskId);
    if (it != taskEnableMap_.end()) {
        return it->second;
    }
    DUMPER_HILOGW(MODULE_COMMON, "Task ID %{public}d not found in config, defaulting to enabled", taskId);
    return true;
}

} // namespace HiviewDFX
} // namespace OHOS
