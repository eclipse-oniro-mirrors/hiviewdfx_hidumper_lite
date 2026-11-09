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
#include "executor/fd_thread_dumper.h"
#include <fstream>
#include <tuple>

using namespace std;
namespace OHOS {
namespace HiviewDFX {

FdThreadDumper::FdThreadDumper()
{
}

FdThreadDumper::~FdThreadDumper()
{
}

DumpStatus FdThreadDumper::PreExecute(const shared_ptr<DumperParameter> &parameter, StringMatrix dumpDatas)
{
    isDumpFd_ = parameter->GetOpts().isDumpFd_;
    isDumpThread_ = parameter->GetOpts().isDumpThread_;
    isDumpFdThreadAll_ = parameter->GetOpts().isShowSmapsInfo_;
    processPid_ = parameter->GetOpts().processPid_;
    dumpDatas_ = dumpDatas;
    return DumpStatus::DUMP_OK;
}

DumpStatus FdThreadDumper::Execute()
{
    if (isDumpFd_) {
        DumpFdInfo();
    }
    if (isDumpThread_) {
        DumpThreadInfo();
    }
    return DumpStatus::DUMP_OK;
}

DumpStatus FdThreadDumper::AfterExecute()
{
    return DumpStatus::DUMP_OK;
}

bool FdThreadDumper::DumpFdInfo()
{
    auto links = GetFdLinks(processPid_);
    if (links.empty()) {
        return false;
    }
    auto linkCounts = CountPaths(links);
    auto topLinks = TopN(linkCounts, FD_TOP_CNT);

    map<string, unordered_map<string, int>> typePaths;
    for (const auto& [path, count] : linkCounts) {
        string type(path, 0, FindFdClusterStartIndex(path));
        if (type != path) {
            typePaths[type][path] = count;
        }
    }
    unordered_map<string, int> typeTotal;
    for (const auto& [type, paths] : typePaths) {
        int total = 0;
        for (const auto& [path, count] : paths) {
            total += count;
        }
        typeTotal[type] = total;
    }
    auto topTypes = TopN(typeTotal, FD_TOP_CNT);

    uint32_t fdNums = links.size();

    vector<string> tempResult;
    tempResult.push_back("fd num: " + to_string(fdNums));
    dumpDatas_->push_back(tempResult);

    DumpFdSummary(topLinks, topTypes);
    DumpFdTopInfo(topLinks);
    DumpFdDirInfo(topTypes, typePaths);
    DumpFdLinkCounts(linkCounts);

    return true;
}

void FdThreadDumper::DumpFdSummary(const vector<pair<string, int>>& topLinks,
                                   const vector<pair<string, int>>& topTypes)
{
    vector<string> tempResult;
    tempResult = {"Summary:"};
    dumpDatas_->push_back(tempResult);

    string summary = GetSummary(topLinks, topTypes);
    if (!summary.empty()) {
        tempResult = {summary};
        dumpDatas_->push_back(tempResult);
    }

    tempResult = {""};
    dumpDatas_->push_back(tempResult);
}

void FdThreadDumper::DumpFdTopInfo(const vector<pair<string, int>>& topLinks)
{
    vector<string> tempResult;
    tempResult = {"Leaked fd Top " + to_string(FD_TOP_CNT) + ":"};
    dumpDatas_->push_back(tempResult);

    string topFdInfo = GetTopFdInfo(topLinks);
    stringstream ss(topFdInfo);
    string line;
    while (getline(ss, line)) {
        tempResult = {line};
        dumpDatas_->push_back(tempResult);
    }
}

void FdThreadDumper::DumpFdDirInfo(const vector<pair<string, int>>& topTypes,
                                   const map<string, unordered_map<string, int>>& typePaths)
{
    vector<string> tempResult;
    tempResult = {"Top Dir " + to_string(FD_TOP_CNT) + ":"};
    dumpDatas_->push_back(tempResult);

    string topDirInfo = GetTopDirInfo(topTypes, typePaths);
    stringstream ss(topDirInfo);
    string line;
    while (getline(ss, line)) {
        tempResult = {line};
        dumpDatas_->push_back(tempResult);
    }
}

void FdThreadDumper::DumpFdLinkCounts(const unordered_map<string, int>& linkCounts)
{
    if (!isDumpFdThreadAll_) {
        return;
    }

    vector<string> tempResult;
    tempResult = {"Fd link counts:"};
    dumpDatas_->push_back(tempResult);

    for (const auto& [key, value] : linkCounts) {
        tempResult = {key + ":" + to_string(value)};
        dumpDatas_->push_back(tempResult);
    }
}

void FdThreadDumper::DumpThreadInfo()
{
    string taskPath = "/proc/" + to_string(processPid_) + "/task/";
    auto threadIds = GetSubNodes(taskPath, true);
    map<string, int64_t> nameCntMap;

    vector<tuple<string, string, string>> threadInfos;
    string startTime;
    string threadName;
    for (const auto& threadId : threadIds) {
        startTime = GetThreadStartTime(processPid_, threadId);
        threadName = GetThreadName(processPid_, threadId);
        threadInfos.push_back({threadId, threadName, startTime});
        nameCntMap[threadName]++;
    }

    vector<string> tempResult;
    tempResult = {"Thread num: " + to_string(threadIds.size())};
    dumpDatas_->push_back(tempResult);

    vector<pair<string, int>> threadCnt;
    for (const auto& [name, cnt] : nameCntMap) {
        threadCnt.push_back({name, cnt});
    }
    sort(threadCnt.begin(), threadCnt.end(), [](const pair<string, int>& a, const pair<string, int>& b) {
            return a.second > b.second;
        });

    tempResult = {"Top 10 Thread Names:"};
    dumpDatas_->push_back(tempResult);

    for (size_t i = 0; i < min(threadCnt.size(), FD_TOP_CNT); i++) {
        tempResult = {to_string(threadCnt[i].second) + "\t" + threadCnt[i].first};
        dumpDatas_->push_back(tempResult);
    }

    if (isDumpFdThreadAll_) {
        tempResult = {"tid\tthread_name\tstart_time"};
        dumpDatas_->push_back(tempResult);
        for (const auto& [tid, name, time] : threadInfos) {
            tempResult = {tid + "\t" + name + "\t" +time};
            dumpDatas_->push_back(tempResult);
        }
    }
}

string FdThreadDumper::GetFdLink(const string &linkPath)
{
    char linkDest[FD_PATH_MAX] = {0};
    ssize_t linkDestSize = readlink(linkPath.c_str(), linkDest, sizeof(linkDest) - 1);
    if (linkDestSize < 0) {
        return "unknown";
    }
    linkDest[linkDestSize] = '\0';
    return linkDest;
}

vector<string> FdThreadDumper::GetFdLinks(int pid)
{
    string fdPath = "/proc/" + to_string(pid) + "/fd/";
    vector<string> nodes = GetSubNodes(fdPath, true);
    vector<string> links;
    for (const auto &node : nodes) {
        string linkPath = fdPath + node;
        links.push_back(GetFdLink(linkPath));
    }
    return links;
}

string FdThreadDumper::MaybeKnownType(const string &link)
{
    const set<string> knownTypes{"eventfd", "eventpoll", "sync_file", "dmabuf", "socket", "pipe", "ashmem"};
    for (const auto &type : knownTypes) {
        if (link.find(type) != string::npos) {
            return type;
        }
    }
    return "unknown";
}

unordered_map<string, int> FdThreadDumper::CountPaths(const vector<string>& links)
{
    unordered_map<string, int> counter;
    for (const auto& link : links) {
        string type = MaybeKnownType(link);
        if (type != "unknown") {
            ++counter[type];
        } else {
            ++counter[link];
        }
    }
    return counter;
}

vector<pair<string, int>> FdThreadDumper::TopN(const unordered_map<string, int>& counter, size_t n)
{
    using Entry = pair<string, int>;

    auto cmp = [](const Entry& a, const Entry& b) {
        return a.second > b.second;
    };
    priority_queue<Entry, vector<Entry>, decltype(cmp)> minHeap(cmp);

    for (const auto& kv : counter) {
        if (minHeap.size() < n) {
            minHeap.push(kv);
        } else if (kv.second > minHeap.top().second) {
            minHeap.pop();
            minHeap.push(kv);
        }
    }

    vector<Entry> result;
    while (!minHeap.empty()) {
        result.push_back(minHeap.top());
        minHeap.pop();
    }
    sort(result.begin(), result.end(), [](const Entry& a, const Entry& b) {
            return (a.second == b.second && a.first < b.first) || (a.second > b.second);
        });
    return result;
}

string FdThreadDumper::GetSummary(const vector<pair<string, int>>& topLinks,
                                  const vector<pair<string, int>>& topTypes)
{
    if (topLinks.size() == 0 && topTypes.size() == 0) {
        return "";
    }
    if (topLinks.size() == 0) {
        return "Leaked dir:" + topTypes[0].first;
    }
    if (topTypes.size() == 0) {
        return "Leaked fd:" + topLinks[0].first;
    }
    if (topTypes[0].second > topLinks[0].second) {
        return "Leaked dir:" + topTypes[0].first;
    }
    return "Leaked fd:" + topLinks[0].first;
}

string FdThreadDumper::GetTopFdInfo(const vector<pair<string, int>>& topLinks)
{
    stringstream rtn;
    for (const auto &[name, count] : topLinks) {
        rtn << to_string(count) << "\t" << name << "\n";
    }
    return rtn.str();
}

string FdThreadDumper::GetTopDirInfo(const vector<pair<string, int>>& topTypes,
                                     const map<string, unordered_map<string, int>>& typePaths)
{
    stringstream rtn;
    for (size_t i = 0; i < topTypes.size(); ++i) {
        const auto &[type, total] = topTypes[i];
        rtn << to_string(total) << "\t" << type << "\n";
        auto it = typePaths.find(type);
        if (it == typePaths.end()) {
            continue;
        }
        auto paths = TopN(it->second, FD_TOP_CNT);
        for (const auto &[path, count] : paths) {
            rtn << "0" << to_string(count) << "\t" << path << "\n";
        }
    }
    return rtn.str();
}

vector<string> FdThreadDumper::GetSubNodes(const string &path, bool digit)
{
    vector<string> subNodes;
    auto dir = opendir(path.c_str());
    if (dir == nullptr) {
        return subNodes;
    }
    for (struct dirent *ent = readdir(dir); ent != nullptr; ent = readdir(dir)) {
        string childNode = ent->d_name;
        if (childNode == "." || childNode == "..") {
            continue;
        }
        if (digit && !IsNumericStr(childNode)) {
            continue;
        }
        subNodes.push_back(childNode);
    }
    closedir(dir);
    return subNodes;
}

bool FdThreadDumper::IsNumericStr(const string &str)
{
    if (str.empty()) {
        return false;
    }
    for (char c : str) {
        if (!isdigit(c)) {
            return false;
        }
    }
    return true;
}

size_t FdThreadDumper::FindFdClusterStartIndex(const string &fullFileName)
{
    size_t fileNameSize = fullFileName.size();
    if (fullFileName.compare(0, STORAGE_PATH_SIZE, STORAGE_PATH_PREFIX) != 0 ||
        !std::isdigit(fullFileName[STORAGE_PATH_SIZE])) {
        for (size_t i = 0; i < fileNameSize; i++) {
            if (std::isdigit(fullFileName[i])) {
                return i;
            }
        }
        return fileNameSize;
    }
    for (size_t i = STORAGE_PATH_SIZE + 1; i < fileNameSize; i++) {
        if (std::isdigit(fullFileName[i])) {
            return i;
        }
    }
    return fileNameSize;
}

string FdThreadDumper::GetThreadStartTime(int pid, const string &tid)
{
    string statPath = "/proc/" + to_string(pid) + "/task/" + tid + "/stat";
    ifstream file(statPath);
    if (!file.is_open()) {
        return "";
    }
    string line;
    if (!getline(file, line)) {
        file.close();
        return "";
    }
    file.close();
    size_t parenPos = line.find(')');
    if (parenPos == string::npos) {
        return "";
    }

    string fieldsStr = line.substr(parenPos + 1);
    vector<string> fields;
    stringstream ss(fieldsStr);
    string field;
    while (ss >> field) {
        fields.push_back(field);
    }
    if (fields.size() <= START_TIME_FIELD_INDEX) {
        return "";
    }
    const string& startTimeStr = fields[START_TIME_FIELD_INDEX];
    if (startTimeStr.empty()) {
        return "";
    }
    bool allDigits = true;
    for (char c : startTimeStr) {
        if (!isdigit(c)) {
            allDigits = false;
            break;
        }
    }
    if (!allDigits) {
        return "";
    }
    char* endPtr = nullptr;
    unsigned long long result = strtoull(startTimeStr.c_str(), &endPtr, 10);
    if (endPtr != startTimeStr.c_str() + startTimeStr.size()) {
        return "";
    }
    return to_string(result);
}

string FdThreadDumper::GetThreadName(int pid, const string &tid)
{
    string commPath = "/proc/" + to_string(pid) + "/task/" + tid + "/comm";
    ifstream file(commPath);
    if (!file.is_open()) {
        return "";
    }
    string line;
    if (getline(file, line)) {
        file.close();
        return line;
    }
    file.close();
    return "";
}
} // namespace HiviewDFX
} // namespace OHOS
