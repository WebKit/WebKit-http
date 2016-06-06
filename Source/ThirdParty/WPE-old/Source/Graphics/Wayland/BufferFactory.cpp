/*
 * Copyright (C) 2015 Igalia S.L.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "Config.h"
#include "BufferFactory.h"

#if WPE_BUFFER_MANAGEMENT(GBM)
#include "BufferFactoryWLDRM.h"
#endif

#if WPE_BUFFER_MANAGEMENT(BCM_RPI)
#include "BufferFactoryBCMRPi.h"
#endif

#if WPE_BUFFER_MANAGEMENT(BCM_NEXUS)
#include "BufferFactoryBCMNexus.h"
#endif

namespace WPE {

namespace Graphics {

std::unique_ptr<BufferFactory> BufferFactory::create()
{
#if WPE_BUFFER_MANAGEMENT(GBM)
    return std::unique_ptr<BufferFactory>(new BufferFactoryWLDRM);
#endif
#if WPE_BUFFER_MANAGEMENT(BCM_RPI)
    return std::unique_ptr<BufferFactory>(new BufferFactoryBCMRPi);
#endif
#if WPE_BUFFER_MANAGEMENT(BCM_NEXUS)
    return std::unique_ptr<BufferFactory>(new BufferFactoryBCMNexus);
#endif
    return nullptr;
}

} // namespace Graphics

} // namespace WPE
