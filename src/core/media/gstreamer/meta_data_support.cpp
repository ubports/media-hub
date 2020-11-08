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

#include <cstring>

#include "core/media/logger/logger.h"

#include "meta_data_support.h"

static unsigned short crc16_table[256];

namespace gstreamer
{

bool gstreamer::MetaDataSupport::crc16_table_init = false;
std::string gstreamer::MetaDataSupport::tempDirName = "";

static void initCRCTable() {
    unsigned short c, i, j;
    unsigned short crc;

    for(i=0; i<256; i++) {
        crc = 0;
        c = i << 8;
        for(j=0;j<8;j++) {
            if((crc ^ c) & 0x8000 )
                crc = (crc << 1) ^ 0x1021; // CCITT
            else                      
                crc = crc << 1;
            c = c << 1;
        }
	crc16_table[i] = crc;
    }
}

#define IMAGE_DIRNAME_TEMPLATE "/tmp/media-hub_images-XXXXXX"
#define IMAGE_FILENAME_TEMPLATE "/image-XXXXXX"

gstreamer::MetaDataSupport::MetaDataSupport() {
}

bool gstreamer::MetaDataSupport::initStatic() {
    bool result = false;

    if(!crc16_table_init) {
        initCRCTable();
        crc16_table_init = true;
    }

    if(tempDirName == "") {
        char dtemplate[] = IMAGE_DIRNAME_TEMPLATE;
        char * dname = mkdtemp(dtemplate);
        if(dname == nullptr) {
            MH_ERROR("error creating temporary directory: %s", strerror(errno));
        } else {
            MH_INFO("created temporary directory for album art: %s", dname);
            tempDirName = dname;
            result = true;
        }
    } else
        result = true;

    return result;
}

bool gstreamer::MetaDataSupport::createTempAlbumArtFile(core::ubuntu::media::Track::MetaData& md, const GstMapInfo& mapInfo) {

    if(!initStatic())
        return false;

    // delete any current one
    cleanupTempAlbumArtFile(md);

    if(tempDirName == "")
        return false;

    char fname[sizeof(IMAGE_DIRNAME_TEMPLATE) + sizeof(IMAGE_FILENAME_TEMPLATE)] = {0};
    strncpy(fname, tempDirName.data(), sizeof(IMAGE_DIRNAME_TEMPLATE));
    strcpy(&fname[sizeof(IMAGE_DIRNAME_TEMPLATE)-1], IMAGE_FILENAME_TEMPLATE);
    //MH_INFO("temporary file template: %s", fname);
    int fd = mkstemp(fname);
    if(fd == -1) {
        MH_ERROR("error creating temporary file: %s", strerror(errno));
        return false;
    }
    bool ok = write(fd, mapInfo.data, mapInfo.size) != -1;
    close(fd);
    if(!ok) {
        MH_ERROR("error writing album art data to temporary file: %s", strerror(errno));
        return false;
    }
    md.embeddedAlbumArtFileName = fname;

    return true;
}

bool gstreamer::MetaDataSupport::cleanupTempAlbumArtFile(const core::ubuntu::media::Track::MetaData& md) {
    if(md.embeddedAlbumArtFileName == "")
        return true;

    if(unlink(md.embeddedAlbumArtFileName.c_str()) == -1) {
        MH_ERROR("error removing album art file %s: %s", 
                 md.embeddedAlbumArtFileName, strerror(errno));
        return false;
    }

    return true;
}

bool gstreamer::MetaDataSupport::checkForImageData(const gchar *tagName, 
                                                   guint index,
                                                   const GstTagList *list,
                                                   core::ubuntu::media::Track::MetaData& md) {
    GstSample * sample;

    if(!gst_tag_list_get_sample_index(list, tagName, index, &sample))
        return false;

    GstBuffer * buffer = gst_sample_get_buffer(sample);
    GstMapInfo mapInfo;
    bool used = false;

    if(gst_buffer_map(buffer, &mapInfo, GST_MAP_READ)) {
        unsigned short crc = computeBufferCRC(mapInfo.data, mapInfo.size);
        if(md.embeddedAlbumArtSize != mapInfo.size
           || md.embeddedAlbumArtCRC != crc) {
            if(MetaDataSupport::createTempAlbumArtFile(md, mapInfo)) {
                md.embeddedAlbumArtSize = mapInfo.size;
                md.embeddedAlbumArtCRC = crc;
                used = true;
            }
        }
        gst_buffer_unmap(buffer, &mapInfo);
    }
    gst_sample_unref(sample);

    return used;
}

unsigned short gstreamer::MetaDataSupport::computeBufferCRC(const void *buffer, unsigned int length) {
    unsigned short crc;
    const unsigned char *ptr;
    unsigned int i;

    if(!crc16_table_init)
        initStatic();

    crc = 0xFFFF; // CCITT
    ptr = (const unsigned char *)buffer;

    if(ptr != nullptr) 
        for(i=0;i<length;i++)
            crc = (crc << 8) ^ crc16_table[((crc >> 8)^(unsigned short)*ptr++) & 0x00FF];

    return crc;
}

}
