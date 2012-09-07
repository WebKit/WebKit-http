/*
    Copyright (C) 2012 Intel Corporation. All rights reserved.

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this library; if not, write to the Free Software Foundation,
    Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*/

#ifndef EWK2UnitTestEnvironment_h
#define EWK2UnitTestEnvironment_h

#include <Eina.h>
#include <gtest/gtest.h>
#include <wtf/text/CString.h>

namespace EWK2UnitTest {

class EWK2UnitTestEnvironment : public ::testing::Environment {
public:
    EWK2UnitTestEnvironment(bool useX11Window);

    bool useX11Window() const { return m_useX11Window; }
    const char* defaultTestPageUrl() const;
    const char* defaultTheme() const;
    CString urlForResource(const char* resource);
    CString pathForResource(const char* resource);

    virtual unsigned int defaultWidth() const { return m_defaultWidth; }
    virtual unsigned int defaultHeight() const { return m_defaultHeight; }

private:
    unsigned int m_defaultWidth;
    unsigned int m_defaultHeight;

    bool m_useX11Window;
};

} // namespace EWK2UnitTest

#endif // EWK2UnitTestEnvironment_h
