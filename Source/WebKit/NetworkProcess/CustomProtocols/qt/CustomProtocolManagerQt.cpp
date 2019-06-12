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
#include "CustomProtocolManager.h"

namespace WebKit {

const char* CustomProtocolManager::supplementName()
{
    return "";
}

CustomProtocolManager::CustomProtocolManager(ChildProcess*)
    : m_impl(nullptr)
{
}

void CustomProtocolManager::initializeConnection(IPC::Connection*)
{
}

void CustomProtocolManager::initialize(const NetworkProcessCreationParameters&)
{
}

void CustomProtocolManager::registerScheme(const String&)
{
    ASSERT_NOT_REACHED();
}

void CustomProtocolManager::unregisterScheme(const String&)
{
}

bool CustomProtocolManager::supportsScheme(const String&)
{
    return false;
}

void CustomProtocolManager::didFailWithError(uint64_t, const WebCore::ResourceError&)
{
    ASSERT_NOT_REACHED();
}

void CustomProtocolManager::didLoadData(uint64_t, const IPC::DataReference&)
{
}

void CustomProtocolManager::didReceiveResponse(uint64_t, const WebCore::ResourceResponse&, uint32_t)
{
}

void CustomProtocolManager::didFinishLoading(uint64_t)
{
}

void CustomProtocolManager::wasRedirectedToRequest(uint64_t, const WebCore::ResourceRequest&, const WebCore::ResourceResponse&)
{
}

} // namespace WebKit
