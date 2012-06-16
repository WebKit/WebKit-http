/*
 * Copyright 2010 Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "BaseURL.h"


BString
baseURL(const BString string)
{
	int32 baseURLStart = string.FindFirst("://") + 3;
	int32 baseURLEnd = string.FindFirst("/", baseURLStart + 1);
	BString result;
	result.SetTo(string.String() + baseURLStart, baseURLEnd - baseURLStart);
	return result;
}

