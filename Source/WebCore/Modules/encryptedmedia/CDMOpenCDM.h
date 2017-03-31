/* GStreamer OpenCDM decryptor
 *
 * Copyright (C) 2016-2017 TATA ELXSI
 * Copyright (C) 2016-2017 Metrological
 * Copyright (C) 2016-2017 Igalia S.L
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Suite 500,
 * Boston, MA 02110-1335, USA.
 */

#pragma once

#if ENABLE(ENCRYPTED_MEDIA) && USE(OCDM)

#include "CDM.h"

namespace WebCore {

class CDMFactoryOpenCDM : public CDMFactory {
public:
    CDMFactoryOpenCDM();
    virtual ~CDMFactoryOpenCDM();

    std::unique_ptr<CDMPrivate> createCDM(CDM&, const String&) override;
    bool supportsKeySystem(const String&) override;
};

} // namespace WebCore

#endif // ENABLE(ENCRYPTED_MEDIA) && USE(OCDM)
