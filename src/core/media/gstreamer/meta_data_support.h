/*
 * Copyright © 2020 UBports Foundation
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


#ifndef GSTREAMER_META_DATA_SUPPORT_H_
#define GSTREAMER_META_DATA_SUPPORT_H_

#include <core/media/track.h>
#include <gst/gst.h>

namespace gstreamer
{

class MetaDataSupport {
public:
    MetaDataSupport();

    static bool checkForImageData(const gchar *tagName, 
                                  guint index,
                                  const GstTagList *list,
                                  core::ubuntu::media::Track::MetaData& md);
    static bool createTempAlbumArtFile(core::ubuntu::media::Track::MetaData& md, const GstMapInfo& mapInfo);
    static bool cleanupTempAlbumArtFile(const core::ubuntu::media::Track::MetaData& md);

    static unsigned short computeBufferCRC(const void *buffer, unsigned int length);

protected:
    static bool initStatic();

    static bool crc16_table_init;
    static std::string tempDirName;
};

}
#endif
