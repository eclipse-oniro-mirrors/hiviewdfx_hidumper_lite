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
    if (str.empty()) {
        return false;
    }
    errno = 0;
    char* end = nullptr;
    long long result = strtoll(str.c_str(), &end, 10);
    if (errno != 0 || end == nullptr || end == str.c_str() || *end != '\0') {
        return false;
    }
    if (result < INT_MIN || result > INT_MAX) {
        return false;
    }
    value = static_cast<int>(result);
    return true;
}

inline std::string TrimStr(const std::string& str, const char cTrim = ' ')
{
    if (str.empty()) {
        return str;
    }
    size_t start = 0;
    size_t end = str.size() - 1;
    while (start <= end && str[start] == cTrim) {
        start++;
    }
    while (end >= start && str[end] == cTrim) {
        end--;
    }
    if (start > end) {
        return "";
    }
    return str.substr(start, end - start + 1);
}

inline void SplitStr(const std::string& str, const std::string& sep, std::vector<std::string>& strs,
                     bool canEmpty = false, bool needTrim = true)
{
    strs.clear();
    std::string strTmp = needTrim ? TrimStr(str) : str;
    std::string strPart;
    while (true) {
        std::string::size_type pos = strTmp.find(sep);
        if (std::string::npos == pos || sep.empty()) {
            strPart = needTrim ? TrimStr(strTmp) : strTmp;
            if (!strPart.empty() || canEmpty) {
                strs.push_back(strPart);
            }
            break;
        } else {
            strPart = needTrim ? TrimStr(strTmp.substr(0, pos)) : strTmp.substr(0, pos);
            if (!strPart.empty() || canEmpty) {
                strs.push_back(strPart);
            }
            strTmp = strTmp.substr(sep.size() + pos, strTmp.size() - sep.size() - pos);
        }
    }
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
