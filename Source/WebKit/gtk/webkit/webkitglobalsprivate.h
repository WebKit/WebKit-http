/*
 * Copyright (C) 2007, 2008, 2009 Holger Hans Peter Freyther
 * Copyright (C) 2008 Jan Michael C. Alonzo
 * Copyright (C) 2008 Collabora Ltd.
 * Copyright (C) 2010 Igalia S.L.
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

#ifndef webkitglobalsprivate_h
#define webkitglobalsprivate_h

#include <glib.h>

#define WEBKIT_PARAM_READABLE ((GParamFlags)(G_PARAM_READABLE|G_PARAM_STATIC_NAME|G_PARAM_STATIC_NICK|G_PARAM_STATIC_BLURB))
#define WEBKIT_PARAM_READWRITE ((GParamFlags)(G_PARAM_READWRITE|G_PARAM_STATIC_NAME|G_PARAM_STATIC_NICK|G_PARAM_STATIC_BLURB))

extern "C" {

void webkitInit();

}

#endif
