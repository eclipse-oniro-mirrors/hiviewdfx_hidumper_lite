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
#ifndef HIDUMPER_LITE_UNIQUE_FD_H
#define HIDUMPER_LITE_UNIQUE_FD_H

#include <unistd.h>

namespace OHOS {
namespace HiviewDFX {

class UniqueFd {
public:
    explicit UniqueFd(int fd = -1) : fd_(fd) {}
    ~UniqueFd()
    {
        Reset();
    }

    UniqueFd(const UniqueFd&) = delete;
    UniqueFd& operator=(const UniqueFd&) = delete;

    UniqueFd(UniqueFd&& other) noexcept : fd_(other.fd_)
    {
        other.fd_ = -1;
    }

    UniqueFd& operator=(UniqueFd&& other) noexcept
    {
        if (this != &other) {
            Reset();
            fd_ = other.fd_;
            other.fd_ = -1;
        }
        return *this;
    }

    int Get() const
    {
        return fd_;
    }

    int Release()
    {
        int fd = fd_;
        fd_ = -1;
        return fd;
    }

    void Reset(int fd = -1)
    {
        if (fd_ >= 0) {
            close(fd_);
        }
        fd_ = fd;
    }

    explicit operator bool() const
    {
        return fd_ >= 0;
    }

private:
    int fd_ = -1;
};
} // namespace HiviewDFX
} // namespace OHOS

#endif // HIDUMPER_LITE_UNIQUE_FD_H
