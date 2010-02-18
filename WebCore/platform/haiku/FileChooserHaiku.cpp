/*
 * Copyright (C) 2006 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2007 Ryan Leavengood <leavengood@gmail.com>
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
 * the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */

#include "config.h"
#include "CString.h"
#include "FileChooser.h"
#include "Font.h"
#include "TextEncoding.h"

#include <Font.h>
#include <String.h>

namespace WebCore {

String FileChooser::basenameForWidth(const Font& font, int width) const
{
	String result;
	if (width <= 0 || m_filenames.isEmpty()) {
    	result = "No files selected";
	} else if (m_filenames.size() == 1) { 
        const BFont *currentFont = font.primaryFont()->platformData().font();
        CString data = UTF8Encoding().encode(m_filenames[0].characters(), m_filenames[0].length(), URLEncodedEntitiesForUnencodables);
        BString tempData(data.data());
        float currentWidth = currentFont->StringWidth(tempData);
        if (currentWidth > width)
    	    currentFont->TruncateString(&tempData, B_TRUNCATE_BEGINNING, width);
    
        result = UTF8Encoding().decode(tempData.String(), tempData.Length());
    } else {
    	char data[50];
    	int size = snprintf(data, sizeof(data), "%ld file(s) selected", m_filenames.size());
    	result = UTF8Encoding().decode(data, size);
    }
    
    return result;
}

} // namespace WebCore

