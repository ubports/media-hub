/*
 * Copyright (C) 2016 Canonical Ltd
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

#include "backend.h"
#include "core/media/logger/logger.h"

#include <gst/gst.h>

namespace media = core::ubuntu::media;

media::AVBackend::Backend media::AVBackend::get_backend_type()
{
    GstRegistry *registry;
    GstPlugin *plugin;

    registry = gst_registry_get();
    if (not registry)
        return media::AVBackend::Backend::none;

    plugin = gst_registry_lookup(registry, "libgstandroidmedia.so");
    if (plugin)
    {
        gst_object_unref(plugin);
        return media::AVBackend::Backend::hybris;
    }

    return media::AVBackend::Backend::none;
}
