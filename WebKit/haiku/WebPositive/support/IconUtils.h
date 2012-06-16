/*
 * Copyright 2006-2008, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _ICON_UTILS_H
#define _ICON_UTILS_H


#include <Mime.h>

class BBitmap;
class BNode;


// This class is a little different from many other classes.
// You don't create an instance of it; you just call its various
// static member functions for utility-like operations.
class BIconUtils {
								BIconUtils();
								~BIconUtils();
								BIconUtils(const BIconUtils&);
			BIconUtils&			operator=(const BIconUtils&);

public:

	// Utility function to import an icon from the node that
	// has either of the provided attribute names. Which icon type
	// is preferred (vector, small or large B_CMAP8 icon) depends
	// on the colorspace of the provided bitmap. If the colorspace
	// is B_CMAP8, B_CMAP8 icons are preferred. In that case, the
	// bitmap size must also match the provided icon_size "size"!
	static	status_t			GetIcon(BNode* node,
									const char* vectorIconAttrName,
									const char* smallIconAttrName,
									const char* largeIconAttrName,
									icon_size size, BBitmap* result);

	// Utility functions to import a vector icon in "flat icon"
	// format from a BNode attribute or from a flat buffer in
	// memory into the preallocated BBitmap "result".
	// The colorspace of result needs to be B_RGBA32 or at
	// least B_RGB32 (though that makes less sense). The icon
	// will be scaled from it's "native" size of 64x64 to the
	// size of the bitmap, the scale is derived from the bitmap
	// width, the bitmap should have square dimension, or the
	// icon will be cut off at the bottom (or have room left).
	static	status_t			GetVectorIcon(BNode* node,
									const char* attrName, BBitmap* result);

	static	status_t			GetVectorIcon(const uint8* buffer,
									size_t size, BBitmap* result);

	// Utility function to import an "old" BeOS icon in B_CMAP8
	// colorspace from either the small icon attribute or the
	// large icon attribute as given in "smallIconAttrName" and
	// "largeIconAttrName". Which icon is loaded depends on
	// the given "size".
	static	status_t			GetCMAP8Icon(BNode* node,
									const char* smallIconAttrName,
									const char* largeIconAttrName,
									icon_size size, BBitmap* icon);

	// Utility functions to convert from old icon colorspace
	// into colorspace of BBitmap "result" (should be B_RGBA32
	// to make any sense).
	static	status_t			ConvertFromCMAP8(BBitmap* source,
									BBitmap* result);
	static	status_t			ConvertToCMAP8(BBitmap* source,
									BBitmap* result);

	static	status_t			ConvertFromCMAP8(const uint8* data,
									uint32 width, uint32 height,
									uint32 bytesPerRow, BBitmap* result);

	static	status_t			ConvertToCMAP8(const uint8* data,
									uint32 width, uint32 height,
									uint32 bytesPerRow, BBitmap* result);
};

#endif	// _ICON_UTILS_H
