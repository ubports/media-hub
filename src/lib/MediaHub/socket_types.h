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
 */

#ifndef LOMIRI_MEDIAHUB_SOCKET_TYPES_H
#define LOMIRI_MEDIAHUB_SOCKET_TYPES_H

namespace lomiri {
namespace MediaHub {

struct BufferMetadata
{
    int width;
    int height;
    int fourcc;
    int stride;
    int offset;
};

struct BufferData
{
    int fd;
    BufferMetadata meta;
};

} // namespace MediaHub
} // namespace lomiri

#endif // LOMIRI_MEDIAHUB_SOCKET_TYPES_H
