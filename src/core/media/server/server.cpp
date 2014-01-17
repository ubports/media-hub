#include <core/media/service.h>
#include <core/media/player.h>
#include <core/media/property.h>
#include <core/media/track_list.h>

#include "core/media/service_implementation.h"

namespace media = core::ubuntu::media;

int main()
{
    auto service = std::make_shared<media::ServiceImplementation>();
    service->run();

    return 0;
}
