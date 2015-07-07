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
#include "url_lib.h"
#include "dial_data.h"

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

char* smartstrcat(char* dest, char* src, size_t max_chars) {
    size_t copied = 0;
    while ((*(dest++) = *(src++)) && copied++ < max_chars) {
    }
    // In case we over-stepped the size, end the string nicely.
    *(--dest) = '\0';
    return dest;
}

static int append_char_from_hex(char* dest, char a, char b) {
    if ('a' <= a && a <= 'f')
        a = 10 + a - 'a';
    else if ('A' <= a && a <= 'F')
        a = 10 + a - 'A';
    else if ('0' <= a && a <= '9')
        a = a - '0';
    else
        return 0;

    if ('a' <= b && b <= 'f')
        b = 10 + b - 'a';
    else if ('A' <= b && b <= 'F')
        b = 10 + b - 'A';
    else if ('0' <= b && b <= '9')
        b = b - '0';
    else
        return 0;

    *dest = (char) (16 * a) + b;
    return 1;
}

int urldecode(char *dst, const char *src, size_t max_size) {
    size_t len = 0;

    while (len < max_size && *src) {
        if (*src == '+') {
            *dst = ' ';
        } else if (*src == '%') {
            if (!*(++src) || !*(++src) || !append_char_from_hex(dst, *(src - 1), *src)) {
                *dst = '\0';
                return 0;
            }
        } else {
            *dst = *src;
        }
        ++dst;
        ++src;
        ++len;
    }
    *dst = '\0';
    return len;
}

void xmlencode(char *dst, const char *src, size_t max_size) {
    size_t current_size = 0;
    while (*src && current_size < max_size) {
        switch (*src) {
            case '&':
                dst = smartstrcat(dst, "&amp;", max_size - current_size);
                current_size += 5;
                break;
            case '\"':
                dst = smartstrcat(dst, "&quot;", max_size - current_size);
                current_size += 6;
                break;
            case '\'':
                dst = smartstrcat(dst, "&apos;", max_size - current_size);
                current_size += 6;
                break;
            case '<':
                dst = smartstrcat(dst, "&lt;", max_size - current_size);
                current_size += 4;
                break;
            case '>':
                dst = smartstrcat(dst, "&gt;", max_size - current_size);
                current_size += 4;
                break;
            default:
                *dst++ = *src;
                current_size++;
                break;
        }
        src++;
    }
    *dst = '\0';
}

char *parse_app_name(const char *uri) {
    if (uri == NULL) {
        return "unknown";
    }
    char *slash = strrchr(uri, '/');
    if (slash == NULL || slash == uri) {
        return "unknown";
    }
    char *begin = slash;
    while ((begin != uri) && (*--begin != '/'))
        ;
    begin++;  // skip the slash
    char *result = (char *) calloc(1, slash - begin);
    strncpy(result, begin, slash - begin);
    return result;
}

char *parse_param(char *query_string, char *param_name) {
    if (query_string == NULL) {
        return NULL;
    }
    char * start;
    if ((start = strstr(query_string, param_name)) == NULL) {
        return NULL;
    }
    while (*start && (*start++ != '='))
        ;
    char *end = start;
    while (*end && (*end != '&'))
        end++;
    int result_size = end - start;
    char *result = malloc(result_size + 1);
    result[0] = '\0';
    strncpy(result, start, result_size);
    result[result_size] = '\0';
    return result;
}

DIALData *parse_params(char * query_string) {
    if (query_string == NULL || strlen(query_string) <= 2) {
        return NULL;
    }
    if (query_string[0] == '?') {
        query_string++;  // skip leading question mark
    }
    DIALData *result = NULL;
    char *query_string_dup = strdup(query_string);
    char * name_value = strtok(query_string_dup, "&");
    while (name_value != NULL) {
        DIALData *tmp = (DIALData *) malloc(sizeof(DIALData));
        size_t name_value_length = strlen(name_value);
        tmp->key = (char *) malloc(name_value_length);
        tmp->value = (char *) malloc(name_value_length);
        sscanf(name_value, "%[^=]=%s", tmp->key, tmp->value);
        tmp->next = result;
        result = tmp;

        name_value = strtok(NULL, "&");  // read next token
    }
    free(query_string_dup);
    return result;
}
