#include <core/media/service.h>
#include <core/media/player.h>
#include <core/media/track_list.h>

#include <media/decoding_service.h>
#include <media/media_codec_layer.h>

#include "core/media/service_implementation.h"

#include <iostream>

namespace media = core::ubuntu::media;

using namespace std;

int main()
{
    // Init hybris-level DecodingService
    decoding_service_init();
    cout << "Starting DecodingService..." << endl;

    auto service = std::make_shared<media::ServiceImplementation>();
    service->run();

    return 0;
}
