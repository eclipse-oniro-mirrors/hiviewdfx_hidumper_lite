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
#ifndef HIDUMPER_LITE_STRING_EX_H
#define HIDUMPER_LITE_STRING_EX_H

#include <cctype>
#include <cerrno>
#include <climits>
#include <cstdlib>
#include <string>
#include <vector>

namespace OHOS {

inline bool IsNumericStr(const std::string& str)
{
    if (str.empty()) {
        return false;
    }
    for (char c : str) {
        if (!isdigit(static_cast<unsigned char>(c))) {
            return false;
        }
    }
    return true;
}

inline bool StrToInt(const std::string& str, int& value)
{
    errno = 0;
    char* end = nullptr;
    long long result = strtoll(str.c_str(), &end, 10);
    if (str.empty() || errno != 0 || end == str.c_str() || *end != '\0' ||
        result < INT_MIN || result > INT_MAX) {
        return false;
    }
    value = static_cast<int>(result);
    return true;
}

inline std::string TrimStr(const std::string& str, const char cTrim = ' ')
{
    size_t start = str.find_first_not_of(cTrim);
    if (start == std::string::npos) {
        return "";
    }
    size_t end = str.find_last_not_of(cTrim);
    return str.substr(start, end - start + 1);
}

inline void PushSplitPart(std::vector<std::string>& strs, const std::string& part, bool canEmpty)
{
    if (!part.empty() || canEmpty) {
        strs.push_back(part);
    }
}

inline void SplitStr(const std::string& str, const std::string& sep, std::vector<std::string>& strs,
                     bool canEmpty = false, bool needTrim = true)
{
    strs.clear();
    std::string strTmp = needTrim ? TrimStr(str) : str;
    std::string::size_type pos = strTmp.find(sep);
    while (pos != std::string::npos && !sep.empty()) {
        PushSplitPart(strs, needTrim ? TrimStr(strTmp.substr(0, pos)) : strTmp.substr(0, pos), canEmpty);
        strTmp = strTmp.substr(sep.size() + pos, strTmp.size() - sep.size() - pos);
        pos = strTmp.find(sep);
    }
    PushSplitPart(strs, needTrim ? TrimStr(strTmp) : strTmp, canEmpty);
}

inline bool IsSubStr(const std::string& str, const std::string& sub)
{
    return str.find(sub) != std::string::npos;
}

inline std::string ReplaceStr(const std::string& str, const std::string& src, const std::string& dst)
{
    if (src.empty() || src == dst) {
        return str;
    }
    std::string result = str;
    std::string::size_type pos = 0;
    while ((pos = result.find(src, pos)) != std::string::npos) {
        result.replace(pos, src.size(), dst);
        pos += dst.size();
    }
    return result;
}
} // namespace OHOS

#endif // HIDUMPER_LITE_STRING_EX_H
