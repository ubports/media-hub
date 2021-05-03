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

#include "logging.h"
#include "service_implementation.h"
#include "service_skeleton.h"

#include <QCoreApplication>
#include <QDBusConnection>
#include <QSharedPointer>
#include <QTextStream>

#include <hybris/media/media_codec_layer.h>

namespace media = core::ubuntu::media;

namespace
{
void logger_init()
{
    QTextStream out(stdout);
    QTextStream err(stderr);

    const char *level = ::getenv("MH_LOG_LEVEL");
    // Default level is info
    QString severity = "info";
    if (level)
    {
        if (strcmp(level, "trace") == 0)
            severity = "debug";
        else if (strcmp(level, "info") == 0 ||
                 strcmp(level, "debug") == 0 ||
                 strcmp(level, "warning") == 0 ||
                 strcmp(level, "fatal") == 0)
            severity = level;
        else if (strcmp(level, "error") == 0)
            severity = "critical";
        else
            err << "Invalid log level \"" << level
                << "\", setting to info. Valid options: [trace, debug, info, warning, error, fatal].\n";
    }
    else
        out << "Using default log level: info\n";

    out << "Log level: " << severity << '\n';
}

// All platform-specific initialization routines go here.
void platform_init()
{
    const media::AVBackend::Backend b {media::AVBackend::get_backend_type()};
    switch (b)
    {
        case media::AVBackend::Backend::hybris:
            MH_DEBUG("Found hybris backend");
            decoding_service_init();
            break;
        case media::AVBackend::Backend::mir:
            MH_DEBUG("Found mir backend");
            break;
        case media::AVBackend::Backend::none:
            MH_WARNING("No video backend selected. Video functionality won't work.");
            break;
        default:
            MH_INFO("Invalid or no A/V backend specified, using \"hybris\" as a default.");
            decoding_service_init();
    }
}
}

int main(int argc, char **argv)
{
    logger_init();

    // Init platform-specific functionality.
    platform_init();

    QCoreApplication app(argc, argv);

    auto bus = QDBusConnection::sessionBus();

    media::ServiceImplementation impl;

    auto skeleton = new media::ServiceSkeleton(
            media::ServiceSkeleton::Configuration {
        bus,
        nullptr
    }, &impl, &impl);

    bool ok =
        bus.registerObject(QStringLiteral("/core/ubuntu/media/Service"),
                           skeleton,
                           QDBusConnection::ExportAllSlots |
                           QDBusConnection::ExportScriptableSignals |
                           QDBusConnection::ExportAllProperties);
    if (!ok) {
        MH_ERROR("Failed to register service object");
        return EXIT_FAILURE;
    }

    ok = bus.registerService("core.ubuntu.media.Service");
    if (!ok) {
        MH_ERROR("Failed to register service name");
        return EXIT_FAILURE;
    }

    ok = bus.registerService("org.mpris.MediaPlayer2.MediaHub");
    if (!ok) {
        MH_ERROR("Failed to register MPRIS service name");
        return EXIT_FAILURE;
    }

    app.exec();

    return 0;
}
