/*
 * Copyright (C) 2009 Zan Dobersek <zandobersek@gmail.com>
 * Copyright (C) 2016 Igalia S.L.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <algorithm>
#include <cairo.h>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <glib.h>

static double tolerance = 0;
static GOptionEntry commandLineOptionEntries[] = 
{
    { "tolerance", 0, 0, G_OPTION_ARG_DOUBLE, &tolerance, "Percentage difference between images before considering them different", "T" },
    { 0, 0, 0, G_OPTION_ARG_NONE, 0, 0, 0 },
};

cairo_surface_t* readPNGFromStdin(unsigned long contentSize)
{
    struct ReadContext {
        char buffer[2048];
        unsigned long incomingBytes;
        unsigned long readBytes;
    } context{ { }, contentSize, 0 };

    cairo_surface_t* surface = cairo_image_surface_create_from_png_stream(
        [](void* closure, unsigned char* data, unsigned int length) -> cairo_status_t
        {
            auto& context = *static_cast<ReadContext*>(closure);
            context.readBytes += length;

            fread(data, 1, length, stdin);
            return CAIRO_STATUS_SUCCESS;
        }, &context);

    if (cairo_surface_status(surface) != CAIRO_STATUS_SUCCESS) {
        cairo_surface_destroy(surface);
        return nullptr;
    }

    return surface;
}

void writePNGToStdout(cairo_surface_t* surface)
{
    struct WriteContext {
        unsigned long writtenBytes;
    } context{ 0 };

    // First we sum up the bytes that are to be written.
    cairo_surface_write_to_png_stream(surface,
        [](void* closure, const unsigned char*, unsigned int length) -> cairo_status_t
        {
            auto& context = *static_cast<WriteContext*>(closure);
            context.writtenBytes += length;
            return CAIRO_STATUS_SUCCESS;
        }, &context);

    printf("Content-Length: %" G_GSIZE_FORMAT "\n", context.writtenBytes);
    cairo_surface_write_to_png_stream(surface,
        [](void*, const unsigned char* data, unsigned int length) -> cairo_status_t
        {
            fwrite(data, 1, length, stdout);
            return CAIRO_STATUS_SUCCESS;
        }, nullptr);
}

float calculateDifference(cairo_surface_t* baselineImage, cairo_surface_t* actualImage, cairo_surface_t* differenceImage)
{
    bool formatCheck = true;
    formatCheck &= (cairo_image_surface_get_format(baselineImage) == CAIRO_FORMAT_ARGB32
        || cairo_image_surface_get_format(baselineImage) == CAIRO_FORMAT_RGB24);
    formatCheck &= cairo_image_surface_get_width(baselineImage) == cairo_image_surface_get_width(actualImage);
    formatCheck &= cairo_image_surface_get_height(baselineImage) == cairo_image_surface_get_height(actualImage);
    formatCheck &= cairo_image_surface_get_stride(baselineImage) == cairo_image_surface_get_stride(actualImage);
    formatCheck &= cairo_image_surface_get_format(baselineImage) == cairo_image_surface_get_format(actualImage);
    if (!formatCheck)
        return 100.0;

    unsigned char* baselinePixels = cairo_image_surface_get_data(baselineImage);
    unsigned char* actualPixels = cairo_image_surface_get_data(actualImage);
    unsigned char* differencePixels = cairo_image_surface_get_data(differenceImage);

    int width = cairo_image_surface_get_width(baselineImage);
    int height = cairo_image_surface_get_height(baselineImage);
    int stride = cairo_image_surface_get_stride(baselineImage);
    unsigned numberOfChannels = 3;
    if (cairo_image_surface_get_format(baselineImage) == CAIRO_FORMAT_ARGB32)
        numberOfChannels = 4;

    float distanceSum = 0;
    float maxDistance = 0;

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            unsigned char* actualPixel = actualPixels + (y * stride) + (x * numberOfChannels);
            unsigned char* baselinePixel = baselinePixels + (y * stride) + (x * numberOfChannels);
            unsigned char* differencePixel = differencePixels + (y * stride) + (x * numberOfChannels);

            float red = (actualPixel[0] - baselinePixel[0]) / std::max<float>(255.0f - baselinePixel[0], baselinePixel[0]);
            float green = (actualPixel[1] - baselinePixel[1]) / std::max<float>(255.0f - baselinePixel[1], baselinePixel[1]);
            float blue = (actualPixel[2] - baselinePixel[2]) / std::max<float>(255.0f - baselinePixel[2], baselinePixel[2]);

            float alpha = 0;
            if (numberOfChannels == 4)
                alpha = (actualPixel[3] - baselinePixel[3]) / std::max<float>(255.0f - baselinePixel[3], baselinePixel[3]);

            float distance = sqrtf(red * red + green * green + blue * blue + alpha * alpha) / 2.0f;

            differencePixel[0] = differencePixel[1] = differencePixel[2] = distance * 255.0f;

            if (distance >= 1.0f / 255.0f) {
                distanceSum += distance;
                maxDistance = std::max<float>(maxDistance, distance);
            }
        }
    }

    if (maxDistance > 0 && maxDistance <= 1) {
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                unsigned char* differencePixel = differencePixels + (y * stride) + (x * numberOfChannels);

                differencePixel[0] /= maxDistance;
                differencePixel[1] /= maxDistance;
                differencePixel[2] /= maxDistance;
            }
        }
    }

    float difference = 0;
    if (distanceSum > 0)
        difference = 100.0f * distanceSum / (height * width);
    if (difference > tolerance) {
        difference = std::round(difference * 100.0f) / 100.0f;
        difference = std::max<float>(difference, 0.01f);
    } else
        difference = 0;

    return difference;
}

void printImageDifferences(cairo_surface_t* baselineImage, cairo_surface_t* actualImage)
{
    cairo_surface_t* differenceImage = cairo_image_surface_create(CAIRO_FORMAT_RGB24,
        cairo_image_surface_get_width(actualImage), cairo_image_surface_get_height(actualImage));
    memset(cairo_image_surface_get_data(differenceImage), 0,
        cairo_image_surface_get_stride(differenceImage) * cairo_image_surface_get_height(actualImage));
    cairo_surface_mark_dirty(differenceImage);

    float difference = calculateDifference(baselineImage, actualImage, differenceImage);
    if (difference > 0) {
        writePNGToStdout(differenceImage);
        printf("diff: %01.2f%% failed\n", difference);
    } else
        printf("diff: %01.2f%% passed\n", difference);

    cairo_surface_destroy(differenceImage);
}

int main(int argc, char* argv[])
{
    GError* error = 0;
    GOptionContext* context = g_option_context_new("- compare two image files, printing their percentage difference and the difference image to stdout");
    g_option_context_add_main_entries(context, commandLineOptionEntries, 0);
    if (!g_option_context_parse(context, &argc, &argv, &error)) {
        printf("Option parsing failed: %s\n", error->message);
        g_error_free(error);
        return 1;
    }

    cairo_surface_t* actualImage = nullptr;
    cairo_surface_t* baselineImage = nullptr;

    char buffer[2048];
    while (fgets(buffer, sizeof(buffer), stdin)) {
        // Convert the first newline into a NUL character so that strtok doesn't produce it.
        char* newLineCharacter = strchr(buffer, '\n');
        if (newLineCharacter)
            *newLineCharacter = '\0';

        if (!strncmp("Content-Length: ", buffer, 16)) {
            gchar** tokens = g_strsplit(buffer, " ", 0);
            if (!tokens[1]) {
                g_strfreev(tokens);
                printf("Error, image size must be specified..\n");
                return 1;
            }

            long imageSize = strtol(tokens[1], 0, 10);
            g_strfreev(tokens);

            if (imageSize > 0 && !actualImage) {
                if (!(actualImage = readPNGFromStdin(imageSize))) {
                    printf("Error, could not read actual image.\n");
                    return 1;
                }
            } else if (imageSize > 0 && !baselineImage) {
                if (!(baselineImage = readPNGFromStdin(imageSize))) {
                    printf("Error, could not read actual image.\n");
                    return 1;
                }
            } else {
                printf("Error, image size must be specified..\n");
                return 1;
            }
        }

        if (actualImage && baselineImage) {
            printImageDifferences(baselineImage, actualImage);

            cairo_surface_destroy(actualImage);
            actualImage = nullptr;
            cairo_surface_destroy(baselineImage);
            baselineImage = nullptr;
        }

        fflush(stdout);
    }

    return 0;
}
