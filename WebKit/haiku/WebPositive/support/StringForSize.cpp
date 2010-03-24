/*
 * Copyright 2010 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include "StringForSize.h"

#include <stdio.h>


namespace BPrivate {


const char*
string_for_size(double size, char* string, size_t stringSize)
{
	double kib = size / 1024.0;
	if (kib < 1.0) {
		snprintf(string, stringSize, "%d bytes", (int)size);
		return string;
	}
	double mib = kib / 1024.0;
	if (mib < 1.0) {
		snprintf(string, stringSize, "%3.2f KiB", kib);
		return string;
	}
	double gib = mib / 1024.0;
	if (gib < 1.0) {
		snprintf(string, stringSize, "%3.2f MiB", mib);
		return string;
	}
	double tib = gib / 1024.0;
	if (tib < 1.0) {
		snprintf(string, stringSize, "%3.2f GiB", gib);
		return string;
	}
	snprintf(string, stringSize, "%.2f TiB", tib);
	return string;
}


}	// namespace BPrivate

