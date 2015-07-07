/*
 * Copyright (c) 2014 Netflix, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY NETFLIX, INC. AND CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL NETFLIX OR CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*
 * Defines functions for persisting and retrieving DIAL data.
 */

#ifndef SRC_SERVER_DIAL_DATA_H_
#define SRC_SERVER_DIAL_DATA_H_

/*
 * Slash-terminated directory of where to persist the DIAL data.
 */
#define DIAL_DATA_DIR "/tmp/"

/*
 * The maximum DIAL data payload accepted per the 'DIAL extension for smooth
 * pairing' specification'.
 */
#define DIAL_DATA_MAX_PAYLOAD (4096) /* 4 KB */

/*
 * Url path where DIAL data should be posted according to the 'DIAL extension
 * for smooth pairing' specification.
 */
#define DIAL_DATA_URI "/dial_data"

struct DIALData_ {
    struct DIALData_ *next;
    char *key;
    char *value;
};

typedef struct DIALData_ DIALData;

void store_dial_data(char *app_name, DIALData *data);

DIALData *retrieve_dial_data(char *app_name);

#endif /* SRC_SERVER_DIAL_DATA_H_ */
