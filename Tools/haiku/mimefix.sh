#!/bin/sh
# This script fixes some known issues with MIME type detection in Haiku. The MIME sniffer does not
# always make the best guess on the type of files, and the test suite relies on the types being
# correct. So, let's fix them!

rmattr BEOS:TYPE /Dev/Haiku/webkit/LayoutTests/webgl/1.0.3/resources/webgl_test_files/conformance/uniforms/*.html
addattr BEOS:TYPE text/html /Dev/Haiku/webkit/LayoutTests/webgl/1.0.3/resources/webgl_test_files/conformance/uniforms/*.html
