/*
    Copyright (C) 2012 Samsung Electronics

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

#include "UnitTestUtils/EWKTestBase.h"
#include <getopt.h>
#include <gtest/gtest.h>

static void parseCustomArguments(int argc, char** argv)
{
    static const option options[] = {
        {"useX11Window", no_argument, &EWKUnitTests::EWKTestBase::useX11Window, true},
        {0, 0, 0, 0}
    };

    int option;
    while ((option = getopt_long(argc, argv, "", options, 0)) != -1) { }
}

int main(int argc, char** argv)
{
    atexit(EWKUnitTests::EWKTestBase::shutdownAll);
    parseCustomArguments(argc, argv);
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
