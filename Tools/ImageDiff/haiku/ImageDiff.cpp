/*
 * Copyright (C) 2009 Zan Dobersek <zandobersek@gmail.com>
 * Copyright (C) 2010 Igalia S.L.
 * Copyright (C) 2011 ProFUSION Embedded Systems
 * Copyright (C) 2011 Samsung Electronics
 * Copyright (C) 2013 Haiku, Inc.
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
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
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

#include "config.h"

#include <Application.h>
#include <Bitmap.h>
#include <BitmapStream.h>
#include <File.h>
#include <FindDirectory.h>
#include <TranslationUtils.h>
#include <TranslatorRoster.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <wtf/PassRefPtr.h>
#include <wtf/StdLibExtras.h>
#include <wtf/RefPtr.h>

enum PixelComponent {
    Red,
    Green,
    Blue,
    Alpha
};

static double gTolerance = 0;

static void abortWithErrorMessage(const char* errorMessage);

static unsigned char* pixelFromImageData(unsigned char* imageData, int rowStride, int x, int y)
{
    return imageData + (y * rowStride) + (x << 2);
}

static BBitmap* differenceImageFromDifferenceBuffer(unsigned char* buffer, int width, int height)
{
    BBitmap* image = new BBitmap(BRect(0, 0, width - 1, height - 1), B_RGBA32);
    if (!image)
        abortWithErrorMessage("could not create difference image");

    unsigned char* diffPixels = static_cast<unsigned char*>(image->Bits());
    const int rowStride = image->BytesPerRow();
    unsigned char* diffPixel;
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            diffPixel = pixelFromImageData(diffPixels, rowStride, x, y);
            diffPixel[Red] = diffPixel[Green] = diffPixel[Blue] = *buffer++;
            diffPixel[Alpha] = 0xff;
        }
    }

    return image;
}

static float computeDistanceBetweenPixelComponents(unsigned char actualComponent, unsigned char baseComponent)
{
    return (actualComponent - baseComponent) / std::max<float>(255 - baseComponent, baseComponent);
}

static float computeDistanceBetweenPixelComponents(unsigned char* actualPixel, unsigned char* basePixel, PixelComponent component)
{
    return computeDistanceBetweenPixelComponents(actualPixel[component], basePixel[component]);
}

static float calculatePixelDifference(unsigned char* basePixel, unsigned char* actualPixel)
{
    const float red = computeDistanceBetweenPixelComponents(actualPixel, basePixel, Red);
    const float green = computeDistanceBetweenPixelComponents(actualPixel, basePixel, Green);
    const float blue = computeDistanceBetweenPixelComponents(actualPixel, basePixel, Blue);
    const float alpha = computeDistanceBetweenPixelComponents(actualPixel, basePixel, Alpha);
    return sqrtf(red * red + green * green + blue * blue + alpha * alpha) / 2.0f;
}

static float calculateDifference(BBitmap* baselineImage, BBitmap* actualImage, BBitmap*& differenceImage)
{
    if (!baselineImage || !actualImage)
        abortWithErrorMessage("One of the images is missing.");
    BRect bounds = actualImage->Bounds();
    BRect baselineBounds = baselineImage->Bounds();

    if (bounds != baselineBounds) {
        printf("Error, test and reference image have different sizes.\n");
        return 100; // Completely different.
    }

    unsigned char* diffBuffer = new unsigned char[(int)((bounds.Width() + 1) * (bounds.Height() + 1))];
    if (!diffBuffer)
        abortWithErrorMessage("could not create difference buffer");

    const int actualRowStride = actualImage->BytesPerRow();
    const int baseRowStride = baselineImage->BytesPerRow();
    unsigned numberOfDifferentPixels = 0;
    float totalDistance = 0;
    float maxDistance = 0;
    unsigned char* actualPixels = static_cast<unsigned char*>(actualImage->Bits());
    unsigned char* basePixels = static_cast<unsigned char*>(baselineImage->Bits());
    unsigned char* currentDiffPixel = diffBuffer;

    for (int y = 0; y <= bounds.Height(); y++) {
        for (int x = 0; x <= bounds.Width(); x++) {
            unsigned char* actualPixel = pixelFromImageData(actualPixels, actualRowStride, x, y);
            unsigned char* basePixel = pixelFromImageData(basePixels, baseRowStride, x, y);

            const float distance = calculatePixelDifference(basePixel, actualPixel);
            *currentDiffPixel++ = static_cast<unsigned char>(distance * 255.0f);

            if (distance >= 1.0f / 255.0f) {
                ++numberOfDifferentPixels;
                totalDistance += distance;
                maxDistance = std::max<float>(maxDistance, distance);
            }
        }
    }

    // Compute the difference as a percentage combining both the number of
    // different pixels and their difference amount i.e. the average distance
    // over the entire image
    float difference = 0;
    if (numberOfDifferentPixels)
        difference = 100.0f * totalDistance / ((bounds.Height() + 1) * (bounds.Width() + 1));
    if (difference <= gTolerance)
        difference = 0;
    else {
        difference = roundf(difference * 100.0f) / 100.0f;
        difference = std::max(difference, 0.01f); // round to 2 decimal places

        differenceImage = differenceImageFromDifferenceBuffer(diffBuffer, bounds.Width() + 1, bounds.Height() + 1);
    }

    delete[] diffBuffer;
    return difference;
}

static void printImage(BBitmap* image)
{
    BMallocIO imageData;
    BBitmapStream input(image);
        // Will delete the image!
    if (BTranslatorRoster::Default()->Translate(&input, NULL, NULL,
        &imageData, B_PNG_FORMAT) == B_OK) {
        printf("Content-Length: %ld\n", imageData.BufferLength());
        fflush(stdout);

        write(1, imageData.Buffer(), imageData.BufferLength());
    }
}

static void printImageDifferences(BBitmap* baselineImage, BBitmap* actualImage)
{
    BBitmap* differenceImage = NULL;
    const float difference = calculateDifference(baselineImage, actualImage, differenceImage);

    if (difference > 0.0f) {
        if (differenceImage)
            printImage(differenceImage);

        printf("diff: %01.2f%% failed\n", difference);
    } else
        printf("diff: %01.2f%% passed\n", difference);
}

static BBitmap* readImageFromStdin(long imageSize)
{
    auto imageBuffer = new unsigned char[imageSize];
    if (!imageBuffer)
        abortWithErrorMessage("cannot allocate image");

    const size_t bytesRead = fread(imageBuffer, 1, imageSize, stdin);
    if (bytesRead <= 0) {
        delete[] imageBuffer;
        abortWithErrorMessage("cannot read image");
    }

    BMemoryIO imageData(imageBuffer, imageSize);
    BBitmap* image = BTranslationUtils::GetBitmap(&imageData);

    delete[] imageBuffer;

    if (image == NULL)
        abortWithErrorMessage("cannot decode image");

    return image;
}

static bool parseCommandLineOptions(int argc, char** argv)
{
    static const option options[] = {
        { "tolerance", required_argument, 0, 't' },
        { 0, 0, 0, 0 }
    };
    int option;

    while ((option = getopt_long(argc, (char* const*)argv, "t:", options, 0)) != -1) {
        switch (option) {
        case 't':
            gTolerance = atof(optarg);
            break;
        case '?':
        case ':':
            return false;
        }
    }

    return true;
}

static void abortWithErrorMessage(const char* errorMessage)
{
    printf("Error, %s.\n", errorMessage);
    exit(EXIT_FAILURE);
}

int32 worker(void*)
{
    BBitmap* actualImage = NULL;
    BBitmap* baselineImage = NULL;

    char buffer[2048];
    while (fgets(buffer, sizeof(buffer), stdin)) {
        char* contentLengthStart = strstr(buffer, "Content-Length: ");
        if (!contentLengthStart)
            continue;
        long imageSize;
        if (sscanf(contentLengthStart, "Content-Length: %ld", &imageSize) == 1) {
            if (imageSize <= 0)
                abortWithErrorMessage("image size must be specified");

            if (!actualImage)
                actualImage = readImageFromStdin(imageSize);
            else if (!baselineImage) {
                baselineImage = readImageFromStdin(imageSize);

                printImageDifferences(baselineImage, actualImage);

                delete actualImage;
                actualImage = NULL;
                delete baselineImage;
                baselineImage = NULL;
            }
        }

        fflush(stdout);
    }

    be_app->PostMessage(B_QUIT_REQUESTED);
    return B_OK;
}

int main(int argc, char* argv[])
{
    BApplication app("application/x-vnd.webkit-imagediff");
        // We need an app_server link for the BBitmaps.
    if (!parseCommandLineOptions(argc, argv))
        return EXIT_FAILURE;

    resume_thread(spawn_thread(worker, "worker", B_NORMAL_PRIORITY, NULL));
    app.Run();

    return EXIT_SUCCESS;
}
