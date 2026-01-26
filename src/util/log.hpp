// File: src/util/log.hpp
// Purpose:
//   Minimal, thread-safe logging utility for the provision-ble daemon.
//
// Design:
//   - Writes timestamped lines to a single log file.
//   - No stdout/stderr dependency (daemon-friendly).
//   - Very small surface area: init + info/warn/error.
//
// Notes:
//   - This is intentionally simple; journald integration can be added later
//     if desired, but file logging is predictable and appliance-friendly.
/*
 *
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
#pragma once

#include <string>

namespace provision::log {

/// Initialise logging.
/// Must be called once at startup before any log calls.
void init(const std::string& logfile_path);

/// Informational message.
void info(const std::string& message);

/// Warning message (non-fatal).
void warn(const std::string& message);

/// Error message (fatal or near-fatal).
void error(const std::string& message);

} // namespace provision::log
