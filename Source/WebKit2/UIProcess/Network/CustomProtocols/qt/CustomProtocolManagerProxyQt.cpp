/*
 * Copyright (C) 2017 Konstantin Tokarev <annulen@yandex.ru>
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
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "config.h"
#include "CustomProtocolManagerProxy.h"

namespace WebKit {

CustomProtocolManagerProxy::CustomProtocolManagerProxy(ChildProcessProxy* childProcessProxy, WebProcessPool& processPool)
    : m_childProcessProxy(childProcessProxy)
    , m_processPool(processPool)
{
}

CustomProtocolManagerProxy::~CustomProtocolManagerProxy()
{
}

void CustomProtocolManagerProxy::startLoading(uint64_t customProtocolID, const WebCore::ResourceRequest& request)
{
    ASSERT_NOT_REACHED();
}

void CustomProtocolManagerProxy::stopLoading(uint64_t customProtocolID)
{
}

} // namespace WebKit
