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
#ifndef HIDUMPER_LITE_SINGLETON_H
#define HIDUMPER_LITE_SINGLETON_H

namespace OHOS {
namespace HiviewDFX {

template<typename T>
class Singleton {
public:
    static T& GetInstance()
    {
        static T instance;
        return instance;
    }

    Singleton(const Singleton&) = delete;
    Singleton& operator=(const Singleton&) = delete;
    Singleton(Singleton&&) = delete;
    Singleton& operator=(Singleton&&) = delete;

protected:
    Singleton() = default;
    ~Singleton() = default;
};

template<typename T>
class DelayedRefSingleton {
public:
    static T& GetInstance()
    {
        static T instance;
        return instance;
    }

    DelayedRefSingleton(const DelayedRefSingleton&) = delete;
    DelayedRefSingleton& operator=(const DelayedRefSingleton&) = delete;
    DelayedRefSingleton(DelayedRefSingleton&&) = delete;
    DelayedRefSingleton& operator=(DelayedRefSingleton&&) = delete;

protected:
    DelayedRefSingleton() = default;
    ~DelayedRefSingleton() = default;
};
} // namespace HiviewDFX
} // namespace OHOS

#endif // HIDUMPER_LITE_SINGLETON_H
