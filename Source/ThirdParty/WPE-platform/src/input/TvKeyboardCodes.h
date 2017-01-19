/*
 * Copyright (C) 2016 SoftAtHome. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef TvKeyboardCodes_h
#define TvKeyboardCodes_h

// From DASE, also used in CE-HTML
// http://atsc.org/wp-content/uploads/2015/03/a_100_2.pdf
#define VK_DASE_CANCEL 3

// From HAVi, used in DASE and OCAP
#define VK_HAVI_COLORED_KEY_0 403
#define VK_HAVI_COLORED_KEY_1 404
#define VK_HAVI_COLORED_KEY_2 405
#define VK_HAVI_COLORED_KEY_3 406
#define VK_HAVI_POWER 409
#define VK_HAVI_RECORD 416
#define VK_HAVI_DISPLAY_SWAP 444
#define VK_HAVI_SUBTITLE 460

// OCAP
// http://www.cablelabs.com/wp-content/uploads/specdocs/OC-SP-OCAP1.3.1-130530.pdf
#define VK_OCAP_ON_DEMAND 623

#endif // TvKeyboardCodes_h
