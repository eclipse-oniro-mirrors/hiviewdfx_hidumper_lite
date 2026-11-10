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

#include "data_inventory.h"
#include <algorithm>
#include <mutex>

namespace OHOS {
namespace HiviewDFX {
bool DataInventory::InputToData(DataId dataId, BaseTypePtr ptr)
{
    if (ptr == nullptr) {
        return false;
    }
    
    std::unique_lock<std::shared_mutex> lock(rwMutex_);
    auto ret = data_.emplace(dataId, ptr);
    return ret.second;
}

BaseTypePtr DataInventory::GetPtr(DataId dataId) const
{
    std::shared_lock<std::shared_mutex> lock(rwMutex_);
    auto it = data_.find(dataId);
    if (it == data_.end()) {
        return {};
    }
    return it->second;
}

std::set<DataId> DataInventory::RemoveRestData(const std::set<DataId>& keepingDataType)
{
    std::set<DataId> removedTypes;
    
    std::unique_lock<std::shared_mutex> lock(rwMutex_);
    for (auto it = data_.begin(); it != data_.end();) {
        if (std::find(keepingDataType.begin(), keepingDataType.end(), it->first) == keepingDataType.end()) {
            removedTypes.insert(it->first);
            it = data_.erase(it);
        } else {
            ++it;
        }
    }
    
    return removedTypes;
}

std::size_t DataInventory::Size() const
{
    std::shared_lock<std::shared_mutex> lock(rwMutex_);
    return data_.size();
}

} // namespace HiviewDFX
} // namespace OHOS
