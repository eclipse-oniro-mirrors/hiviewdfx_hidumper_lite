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
#ifndef HIDUMPER_LITE_FILE_EX_H
#define HIDUMPER_LITE_FILE_EX_H

#include <cerrno>
#include <fcntl.h>
#include <fstream>
#include <sstream>
#include <string>
#include <unistd.h>

namespace OHOS {

inline bool LoadStringFromFile(const std::string& filePath, std::string& content)
{
    std::ifstream file(filePath, std::ifstream::binary);
    if (!file.is_open()) {
        return false;
    }
    std::ostringstream ss;
    ss << file.rdbuf();
    content = ss.str();
    return true;
}

inline bool SaveStringToFd(int fd, const std::string& content)
{
    if (fd < 0) {
        return false;
    }
    size_t total = 0;
    while (total < content.size()) {
        ssize_t ret = write(fd, content.data() + total, content.size() - total);
        if (ret < 0) {
            if (errno == EINTR) {
                continue;
            }
            return false;
        }
        if (ret == 0) {
            return false;
        }
        total += static_cast<size_t>(ret);
    }
    return true;
}
} // namespace OHOS

#endif // HIDUMPER_LITE_FILE_EX_H
