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
/* Utility functions for dealing with URLs */

#ifndef URLLIB_H_
#define URLLIB_H_

#include "dial_data.h"
#include <stddef.h>

/**
 * Concatenate a maxim of max_chars characters from src into dest,
 * and return a pointer to the last character in dest.
 */
char* smartstrcat(char* dest, char* src, size_t max_chars);

int urldecode(char *dst, const char *src, size_t max_size);

void xmlencode(char *dst, const char *src, size_t max_size);

/**
 * Parse the value of the parameter with the given name from a query string.
 */
char *parse_param(char *query_string, char *param_name);

/*
 * Parse the application name out of a URI, such as /app/YouTube/dial_data.
 * Note: this parser assumes that the dial_data url is of the form
 * /apps/<app_name>/dial_data. If your DIAL server uses a different url-path,
 * you will need to adapt the method below.
 */
char *parse_app_name(const char *uri);

/*
 * Parse a list of DIALData params out of a query string.
 */
DIALData *parse_params(char * query_string);

#endif  // URLLIB_H_
