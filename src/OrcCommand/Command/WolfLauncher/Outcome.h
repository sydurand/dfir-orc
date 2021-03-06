//
// SPDX-License-Identifier: LGPL-2.1-or-later
//
// Copyright � 2020 ANSSI. All Rights Reserved.
//
// Author(s): fabienfl
//

#pragma once

#include <chrono>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include <windows.h>

#include "StructuredOutputWriter.h"
#include "Utils/Result.h"

namespace Orc::Command::Wolf::Outcome {

using Timestamp = std::chrono::time_point<std::chrono::system_clock>;

class Archive
{
public:
    using FileSize = Traits::ByteQuantity<uint64_t>;

    class Item
    {
    public:
        const std::wstring& GetName() const { return m_name; }
        void SetName(const std::wstring& name) { m_name = name; }

        const std::optional<FileSize>& GetSize() const { return m_size; }
        void SetSize(const uint64_t size) { m_size = size; }
        void SetSize(const std::optional<FileSize>& size) { m_size = size; }

    private:
        std::wstring m_name;
        std::optional<FileSize> m_size;
    };

    void Add(Item item) { m_items.emplace_back(std::move(item)); }
    const std::vector<Item>& GetItems() const { return m_items; }

    const std::wstring& GetName() const { return m_name; }
    void SetName(const std::wstring& name) { m_name = name; }

    const std::optional<FileSize>& GetSize() const { return m_size; }
    void SetSize(const uint64_t size) { m_size = size; }
    void SetSize(const std::optional<FileSize>& size) { m_size = size; }

private:
    std::wstring m_name;
    std::optional<FileSize> m_size;
    std::vector<Item> m_items;
};

class Command
{
public:
    const std::wstring& GetKeyword() const { return m_keyword; }
    void SetKeyword(const std::wstring& keyword) { m_keyword = keyword; }

    const std::wstring& GetCommandLineValue() const { return m_commandLine; }
    void SetCommandLineValue(const std::wstring& commandLine) { m_commandLine = commandLine; }

    Timestamp GetCreationTime() const { return m_creationTime; }
    void SetCreationTime(const Timestamp& creationTime) { m_creationTime = creationTime; }

    Timestamp GetExitTime() const { return m_exitTime; }
    void SetExitTime(const Timestamp& exitTime) { m_exitTime = exitTime; }

    std::optional<std::chrono::seconds> GetUserTime() const { return m_userTime; }
    void SetUserTime(const std::chrono::seconds& userTime) { m_userTime = userTime; }

    std::optional<std::chrono::seconds> GetKernelTime() const { return m_kernelTime; }
    void SetKernelTime(const std::chrono::seconds& kernelTime) { m_kernelTime = kernelTime; }

    int32_t GetExitCode() const { return m_exitCode; }
    void SetExitCode(int32_t code) { m_exitCode = code; }

    uint32_t GetPid() const { return m_pid; }
    void SetPid(uint32_t pid) { m_pid = pid; }

    const std::optional<IO_COUNTERS>& GetIOCounters() const { return m_ioCounters; }
    void SetIOCounters(const IO_COUNTERS& counters) { m_ioCounters = counters; }

private:
    std::wstring m_keyword;
    std::wstring m_commandLine;
    Timestamp m_creationTime;
    Timestamp m_exitTime;
    std::optional<std::chrono::seconds> m_userTime;
    std::optional<std::chrono::seconds> m_kernelTime;
    std::optional<IO_COUNTERS> m_ioCounters;
    int32_t m_exitCode;
    uint32_t m_pid;
};

class JobStatistics
{
public:
    uint64_t GetPageFaultCount() const { return m_pageFaultCount; }
    void SetPageFaultCount(uint64_t count) { m_pageFaultCount = count; }

    uint64_t GetProcessCount() const { return m_processCount; }
    void SetProcessCount(uint64_t count) { m_processCount = count; }

    uint64_t GetActiveProcessCount() const { return m_activeProcessCount; }
    void SetActiveProcessCount(uint64_t count) { m_activeProcessCount = count; }

    uint64_t GetTerminatedProcessCount() const { return m_terminatedProcessCount; }
    void SetTerminatedProcessCount(uint64_t count) { m_terminatedProcessCount = count; }

    uint64_t GetPeakProcessMemory() const { return m_peakProcessMemory; }
    void SetPeakProcessMemory(uint64_t size) { m_peakProcessMemory = size; }

    uint64_t GetPeakJobMemory() const { return m_peakJobMemory; }
    void SetPeakJobMemory(uint64_t size) { m_peakJobMemory = size; }

    const IO_COUNTERS& GetIOCounters() const { return m_ioCounters; }
    void SetIOCounters(const IO_COUNTERS& counters) { m_ioCounters = counters; }

private:
    uint64_t m_pageFaultCount;
    uint64_t m_processCount;
    uint64_t m_activeProcessCount;
    uint64_t m_terminatedProcessCount;
    uint64_t m_peakProcessMemory;
    uint64_t m_peakJobMemory;
    IO_COUNTERS m_ioCounters;
};

class CommandSet
{
public:
    Command& GetCommand(const std::wstring& keyword) { return m_commands[keyword]; }

    Archive& GetArchive() { return m_archive; }
    const Archive& GetArchive() const { return m_archive; }

    std::vector<std::reference_wrapper<const Command>> GetCommands() const
    {
        std::vector<std::reference_wrapper<const Command>> values;
        for (const auto& [key, value] : m_commands)
        {
            values.push_back(std::cref<Command>(value));
        }

        return values;
    }

    const std::wstring& GetKeyword() const { return m_keyword; }
    void SetKeyword(const std::wstring& keyword) { m_keyword = keyword; }

    Timestamp GetStart() const { return m_start; }
    void SetStart(const Timestamp& start) { m_start = start; }

    Timestamp GetEnd() const { return m_end; }
    void SetEnd(const Timestamp& end) { m_end = end; }

    const std::optional<JobStatistics>& GetJobStatistics() const { return m_jobStatistics; }
    void SetJobStatistics(const JobStatistics& statistics) { m_jobStatistics = statistics; }

private:
    std::wstring m_keyword;
    std::chrono::time_point<std::chrono::system_clock> m_start;
    std::chrono::time_point<std::chrono::system_clock> m_end;
    std::unordered_map<std::wstring, Command> m_commands;
    Archive m_archive;

    std::chrono::seconds m_userTime;
    std::chrono::seconds m_kernelTime;

    std::optional<JobStatistics> m_jobStatistics;
};

class Mothership
{
public:
    const std::wstring& GetCommandLineValue() const { return m_commandLine; }
    void SetCommandLineValue(const std::wstring& commandLine) { m_commandLine = commandLine; }

    void SetSha256(const std::wstring& sha256) { m_sha256 = sha256; }
    const std::wstring& GetSha256() const { return m_sha256; }

private:
    std::wstring m_commandLine;
    std::wstring m_sha256;
};

class WolfLauncher
{
public:
    void SetSha256(const std::wstring& sha256) { m_sha256 = sha256; }
    const std::wstring& GetSha256() const { return m_sha256; }

    void SetVersion(const std::string& version) { m_version = version; }
    const std::string& GetVersion() const { return m_version; }

    const std::wstring& GetCommandLineValue() const { return m_commandLine; }
    void SetCommandLineValue(const std::wstring& commandLine) { m_commandLine = commandLine; }

private:
    std::wstring m_sha256;
    std::string m_version;
    std::wstring m_commandLine;
};

class Outcome
{
public:
    std::lock_guard<std::mutex> Lock() const { return std::lock_guard<std::mutex>(m_mutex); }

    CommandSet& GetCommandSet(const std::wstring& keyword) { return m_commandSets[keyword]; }

    std::vector<std::reference_wrapper<const CommandSet>> GetCommandSets() const
    {
        std::vector<std::reference_wrapper<const CommandSet>> values;
        for (const auto& [key, value] : m_commandSets)
        {
            values.push_back(std::cref<CommandSet>(value));
        }

        return values;
    }

    const std::wstring& GetComputerNameValue() const { return m_computerName; }
    void SetComputerNameValue(const std::wstring& name) { m_computerName = name; }

    Timestamp GetTimestamp() const { return m_timestamp; }
    void SetTimestamp(const Timestamp& timestamp) { m_timestamp = timestamp; }

    WolfLauncher& GetWolfLauncher() { return m_wolfLauncher; }
    const WolfLauncher& GetWolfLauncher() const { return m_wolfLauncher; }

    Mothership& GetMothership() { return m_mothership; }
    const Mothership& GetMothership() const { return m_mothership; }

private:
    mutable std::mutex m_mutex;
    std::wstring m_computerName;
    Mothership m_mothership;
    WolfLauncher m_wolfLauncher;
    std::chrono::time_point<std::chrono::system_clock> m_timestamp;
    std::unordered_map<std::wstring, CommandSet> m_commandSets;
};

Orc::Result<void> Write(const Outcome& outcome, StructuredOutputWriter::IWriter::Ptr& writer);

}  // namespace Orc::Command::Wolf::Outcome
