/*
 * Project: provision (BLE Provisioning for Raspberry Pi)
 *
 * Description:
 *   BLE-based provisioning daemon for Raspberry Pi devices.
 *
 * Website:
 *   https://pidevelop.com
 *
 * Contact:
 *   james@pidevelop.com
 *
 * License:
 *   MIT License
 *
 * Copyright (c) 2026 PiDevelop
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 */

// File: src/util/log.cpp
// Purpose:
//   Implementation of the simple file-based logger used by provision-ble.
//
// Behaviour:
//   - Appends log lines to the configured file.
//   - Prepends ISO-like timestamps and log level.
//   - Thread-safe via a single mutex.
//
// Failure handling:
//   - If logging is not initialised or file open fails, messages are dropped.
//   - Logging must never crash or block the daemon.

/*
 * Website:
 *   https://pidevelop.com
 *
 * Contact:
 *   james@pidevelop.com
 *
 * License:
 *   MIT License (see LICENSE file at repo root)
 *
 * Copyright (c) 2026 PiDevelop
 */

#include "util/log.hpp"

#include <ctime>
#include <fstream>
#include <mutex>

namespace provision::log {

static std::string g_log_path;
static std::mutex  g_mutex;

static void write_line(const char* level, const std::string& message)
{
    if (g_log_path.empty()) {
        return;
    }

    std::lock_guard<std::mutex> lock(g_mutex);

    std::ofstream file(g_log_path, std::ios::app);
    if (!file.is_open()) {
        return;
    }

    std::time_t now = std::time(nullptr);
    char ts[32]{};
    std::strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S", std::localtime(&now));

    file << ts << " [" << level << "] " << message << "\n";
}

void init(const std::string& logfile_path)
{
    g_log_path = logfile_path;
}

void info(const std::string& message)
{
    write_line("INFO", message);
}

void warn(const std::string& message)
{
    write_line("WARN", message);
}

void error(const std::string& message)
{
    write_line("ERROR", message);
}

} // namespace provision::log
