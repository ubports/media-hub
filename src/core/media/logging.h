/*
 * Copyright © 2021 UBports Foundation.
 *
 * Contact: Alberto Mardegan <mardy@users.sourceforge.net>
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef LOGGING_H_
#define LOGGING_H_

#include <QLoggingCategory>

namespace core {
namespace ubuntu {
namespace media {

// Log returns the core::ubuntu::media-wide configured logger instance.
// Save to call before/after main.
const QLoggingCategory &Log();

} // namespace media
} // namespace ubuntu
} // namespace core

#define MH_TRACE(...) qCDebug(core::ubuntu::media::Log(), __VA_ARGS__)
#define MH_DEBUG(...) qCDebug(core::ubuntu::media::Log(), __VA_ARGS__)
#define MH_INFO(...) qCInfo(core::ubuntu::media::Log(), __VA_ARGS__)
#define MH_WARNING(...) qCWarning(core::ubuntu::media::Log(), __VA_ARGS__)
#define MH_ERROR(...) qCCritical(core::ubuntu::media::Log(), __VA_ARGS__)
#define MH_FATAL(...) qCCritical(core::ubuntu::media::Log(), __VA_ARGS__)

#endif
