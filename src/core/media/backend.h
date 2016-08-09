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
#ifndef CORE_UBUNTU_MEDIA_BACKEND_H_
#define CORE_UBUNTU_MEDIA_BACKEND_H_

namespace core
{
namespace ubuntu
{
namespace media
{
struct AVBackend
{
    enum Backend
    {
        none,
        hybris
    };

    /**
     * @brief Returns the type of audio/video decoding/encoding backend being used.
     * @return Returns the current backend type.
     */
    static Backend get_backend_type();
};
}
}
}

#endif // CORE_UBUNTU_MEDIA_BACKEND_H_
