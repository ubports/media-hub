/*
 * Copyright (C) 2014 Canonical Ltd
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Authored by: Jim Hodapp <jim.hodapp@canonical.com>
 */

#include <core/media/service.h>
#include <core/media/player.h>
#include <core/media/track_list.h>

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
