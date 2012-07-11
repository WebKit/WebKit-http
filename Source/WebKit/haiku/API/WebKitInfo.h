/*
 * Copyright 2012, Alexandre Deckner, alexandre.deckner@uzzl.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef WEBKIT_INFO_H_
#define WEBKIT_INFO_H_

#include <String.h>


class WebKitInfo {
public:
	static	BString				HaikuWebKitVersion();
	static	BString				HaikuWebKitRevision();
	static	BString				WebKitVersion();
	static	int					WebKitMajorVersion();
	static	int					WebKitMinorVersion();
	static	BString				WebKitRevision();
};


#endif	// WEBKIT_INFO_H_
