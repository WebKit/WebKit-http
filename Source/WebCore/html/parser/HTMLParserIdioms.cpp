/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "HTMLParserIdioms.h"

#include "Decimal.h"
#include "HTMLIdentifier.h"
#include "URL.h"
#include "QualifiedName.h"
#include <limits>
#include <wtf/MathExtras.h>
#include <wtf/text/AtomicString.h>
#include <wtf/text/StringBuilder.h>

namespace WebCore {

template <typename CharType>
static String stripLeadingAndTrailingHTMLSpaces(String string, CharType characters, unsigned length)
{
    unsigned numLeadingSpaces = 0;
    unsigned numTrailingSpaces = 0;

    for (; numLeadingSpaces < length; ++numLeadingSpaces) {
        if (isNotHTMLSpace(characters[numLeadingSpaces]))
            break;
    }

    if (numLeadingSpaces == length)
        return string.isNull() ? string : emptyAtom.string();

    for (; numTrailingSpaces < length; ++numTrailingSpaces) {
        if (isNotHTMLSpace(characters[length - numTrailingSpaces - 1]))
            break;
    }

    ASSERT(numLeadingSpaces + numTrailingSpaces < length);

    if (!(numLeadingSpaces | numTrailingSpaces))
        return string;

    return string.substring(numLeadingSpaces, length - (numLeadingSpaces + numTrailingSpaces));
}

String stripLeadingAndTrailingHTMLSpaces(const String& string)
{
    unsigned length = string.length();

    if (!length)
        return string.isNull() ? string : emptyAtom.string();

    if (string.is8Bit())
        return stripLeadingAndTrailingHTMLSpaces(string, string.characters8(), length);

    return stripLeadingAndTrailingHTMLSpaces(string, string.characters(), length);
}

String serializeForNumberType(const Decimal& number)
{
    if (number.isZero()) {
        // Decimal::toString appends exponent, e.g. "0e-18"
        return number.isNegative() ? "-0" : "0";
    }
    return number.toString();
}

String serializeForNumberType(double number)
{
    // According to HTML5, "the best representation of the number n as a floating
    // point number" is a string produced by applying ToString() to n.
    return String::numberToStringECMAScript(number);
}

Decimal parseToDecimalForNumberType(const String& string, const Decimal& fallbackValue)
{
    // See HTML5 2.5.4.3 `Real numbers.' and parseToDoubleForNumberType

    // String::toDouble() accepts leading + and whitespace characters, which are not valid here.
    const UChar firstCharacter = string[0];
    if (firstCharacter != '-' && firstCharacter != '.' && !isASCIIDigit(firstCharacter))
        return fallbackValue;

    const Decimal value = Decimal::fromString(string);
    if (!value.isFinite())
        return fallbackValue;

    // Numbers are considered finite IEEE 754 single-precision floating point values.
    // See HTML5 2.5.4.3 `Real numbers.'
    // FIXME: We should use numeric_limits<double>::max for number input type.
    const Decimal floatMax = Decimal::fromDouble(std::numeric_limits<float>::max());
    if (value < -floatMax || value > floatMax)
        return fallbackValue;

    // We return +0 for -0 case.
    return value.isZero() ? Decimal(0) : value;
}

Decimal parseToDecimalForNumberType(const String& string)
{
    return parseToDecimalForNumberType(string, Decimal::nan());
}

double parseToDoubleForNumberType(const String& string, double fallbackValue)
{
    // See HTML5 2.5.4.3 `Real numbers.'

    // String::toDouble() accepts leading + and whitespace characters, which are not valid here.
    UChar firstCharacter = string[0];
    if (firstCharacter != '-' && firstCharacter != '.' && !isASCIIDigit(firstCharacter))
        return fallbackValue;

    bool valid = false;
    double value = string.toDouble(&valid);
    if (!valid)
        return fallbackValue;

    // NaN and infinity are considered valid by String::toDouble, but not valid here.
    if (!std::isfinite(value))
        return fallbackValue;

    // Numbers are considered finite IEEE 754 single-precision floating point values.
    // See HTML5 2.5.4.3 `Real numbers.'
    if (-std::numeric_limits<float>::max() > value || value > std::numeric_limits<float>::max())
        return fallbackValue;

    // The following expression converts -0 to +0.
    return value ? value : 0;
}

double parseToDoubleForNumberType(const String& string)
{
    return parseToDoubleForNumberType(string, std::numeric_limits<double>::quiet_NaN());
}

template <typename CharacterType>
static bool parseHTMLIntegerInternal(const CharacterType* position, const CharacterType* end, int& value)
{
    // Step 3
    int sign = 1;

    // Step 4
    while (position < end) {
        if (!isHTMLSpace(*position))
            break;
        ++position;
    }

    // Step 5
    if (position == end)
        return false;
    ASSERT_WITH_SECURITY_IMPLICATION(position < end);

    // Step 6
    if (*position == '-') {
        sign = -1;
        ++position;
    } else if (*position == '+')
        ++position;
    if (position == end)
        return false;
    ASSERT_WITH_SECURITY_IMPLICATION(position < end);

    // Step 7
    if (!isASCIIDigit(*position))
        return false;

    // Step 8
    StringBuilder digits;
    while (position < end) {
        if (!isASCIIDigit(*position))
            break;
        digits.append(*position++);
    }

    // Step 9
    bool ok;
    if (digits.is8Bit())
        value = sign * charactersToIntStrict(digits.characters8(), digits.length(), &ok);
    else
        value = sign * charactersToIntStrict(digits.characters16(), digits.length(), &ok);
    return ok;
}

// http://www.whatwg.org/specs/web-apps/current-work/#rules-for-parsing-integers
bool parseHTMLInteger(const String& input, int& value)
{
    // Step 1
    // Step 2
    unsigned length = input.length();
    if (!length || input.is8Bit()) {
        const LChar* start = input.characters8();
        return parseHTMLIntegerInternal(start, start + length, value);
    }

    const UChar* start = input.characters16();
    return parseHTMLIntegerInternal(start, start + length, value);
}

template <typename CharacterType>
static bool parseHTMLNonNegativeIntegerInternal(const CharacterType* position, const CharacterType* end, unsigned& value)
{
    // Step 3
    while (position < end) {
        if (!isHTMLSpace(*position))
            break;
        ++position;
    }

    // Step 4
    if (position == end)
        return false;
    ASSERT_WITH_SECURITY_IMPLICATION(position < end);

    // Step 5
    if (*position == '+')
        ++position;

    // Step 6
    if (position == end)
        return false;
    ASSERT_WITH_SECURITY_IMPLICATION(position < end);

    // Step 7
    if (!isASCIIDigit(*position))
        return false;

    // Step 8
    StringBuilder digits;
    while (position < end) {
        if (!isASCIIDigit(*position))
            break;
        digits.append(*position++);
    }

    // Step 9
    bool ok;
    if (digits.is8Bit())
        value = charactersToUIntStrict(digits.characters8(), digits.length(), &ok);
    else
        value = charactersToUIntStrict(digits.characters16(), digits.length(), &ok);
    return ok;
}


// http://www.whatwg.org/specs/web-apps/current-work/#rules-for-parsing-non-negative-integers
bool parseHTMLNonNegativeInteger(const String& input, unsigned& value)
{
    // Step 1
    // Step 2
    unsigned length = input.length();
    if (length && input.is8Bit()) {
        const LChar* start = input.characters8();
        return parseHTMLNonNegativeIntegerInternal(start, start + length, value);
    }
    
    const UChar* start = input.characters();
    return parseHTMLNonNegativeIntegerInternal(start, start + length, value);
}

static bool threadSafeEqual(const StringImpl* a, const StringImpl* b)
{
    if (a == b)
        return true;
    if (a->hash() != b->hash())
        return false;
    return equalNonNull(a, b);
}

bool threadSafeMatch(const QualifiedName& a, const QualifiedName& b)
{
    return threadSafeEqual(a.localName().impl(), b.localName().impl());
}

#if ENABLE(THREADED_HTML_PARSER)
bool threadSafeMatch(const HTMLIdentifier& localName, const QualifiedName& qName)
{
    return threadSafeEqual(localName.asStringImpl(), qName.localName().impl());
}
#endif

struct ImageWithScale {
    unsigned imageURLStart;
    unsigned imageURLLength;
    float scaleFactor;

    ImageWithScale()
        : imageURLStart(0)
        , imageURLLength(0)
        , scaleFactor(1)
    { 
    }

    bool hasImageURL() const
    {
        return imageURLLength;
    }
};
typedef Vector<ImageWithScale> ImageCandidates;

static inline bool compareByScaleFactor(const ImageWithScale& first, const ImageWithScale& second)
{
    return first.scaleFactor < second.scaleFactor;
}

static inline bool isHTMLSpaceOrComma(UChar character)
{
    return isHTMLSpace(character) || character == ',';
}

// See the specifications for more details about the algorithm to follow.
// http://www.w3.org/TR/2013/WD-html-srcset-20130228/#processing-the-image-candidates.
static void parseImagesWithScaleFromSrcsetAttribute(const String& srcsetAttribute, ImageCandidates& imageCandidates)
{
    ASSERT(imageCandidates.isEmpty());

    size_t imageCandidateStart = 0;
    unsigned srcsetAttributeLength = srcsetAttribute.length();

    while (imageCandidateStart < srcsetAttributeLength) {
        float imageScaleFactor = 1;
        size_t separator;

        // 4. Splitting loop: Skip whitespace.
        size_t imageURLStart = srcsetAttribute.find(isNotHTMLSpace, imageCandidateStart);
        if (imageURLStart == notFound)
            break;
        // If The current candidate is either totally empty or only contains space, skipping.
        if (srcsetAttribute[imageURLStart] == ',') {
            imageCandidateStart = imageURLStart + 1;
            continue;
        }
        // 5. Collect a sequence of characters that are not space characters, and let that be url.
        size_t imageURLEnd = srcsetAttribute.find(isHTMLSpace, imageURLStart + 1);
        if (imageURLEnd == notFound) {
            imageURLEnd = srcsetAttributeLength;
            separator = srcsetAttributeLength;
        } else if (srcsetAttribute[imageURLEnd - 1] == ',') {
            --imageURLEnd;
            separator = imageURLEnd;
        } else {
            // 7. Collect a sequence of characters that are not "," (U+002C) characters, and let that be descriptors.
            size_t imageScaleStart = srcsetAttribute.find(isNotHTMLSpace, imageURLEnd + 1);
            if (imageScaleStart == notFound)
                separator = srcsetAttributeLength;
            else if (srcsetAttribute[imageScaleStart] == ',')
                separator = imageScaleStart;
            else {
                // This part differs from the spec as the current implementation only supports pixel density descriptors for now.
                size_t imageScaleEnd = srcsetAttribute.find(isHTMLSpaceOrComma, imageScaleStart + 1);
                imageScaleEnd = (imageScaleEnd == notFound) ? srcsetAttributeLength : imageScaleEnd;
                size_t commaPosition = imageScaleEnd;
                // Make sure there are no other descriptors.
                while ((commaPosition < srcsetAttributeLength - 1) && isHTMLSpace(srcsetAttribute[commaPosition]))
                    ++commaPosition;
                // If the first not html space character after the scale modifier is not a comma,
                // the current candidate is an invalid input.
                if ((commaPosition < srcsetAttributeLength - 1) && srcsetAttribute[commaPosition] != ',') {
                    // Find the nearest comma and skip the input.
                    commaPosition = srcsetAttribute.find(',', commaPosition + 1);
                    if (commaPosition == notFound)
                        break;
                    imageCandidateStart = commaPosition + 1;
                    continue;
                }
                separator = commaPosition;
                if (srcsetAttribute[imageScaleEnd - 1] != 'x') {
                    imageCandidateStart = separator + 1;
                    continue;
                }
                bool validScaleFactor = false;
                size_t scaleFactorLengthWithoutUnit = imageScaleEnd - imageScaleStart - 1;
                imageScaleFactor = charactersToFloat(srcsetAttribute.characters() + imageScaleStart, scaleFactorLengthWithoutUnit, &validScaleFactor);

                if (!validScaleFactor) {
                    imageCandidateStart = separator + 1;
                    continue;
                }
            }
        }
        ImageWithScale image;
        image.imageURLStart = imageURLStart;
        image.imageURLLength = imageURLEnd - imageURLStart;
        image.scaleFactor = imageScaleFactor;

        imageCandidates.append(image);
        // 11. Return to the step labeled splitting loop.
        imageCandidateStart = separator + 1;
    }
}

String bestFitSourceForImageAttributes(float deviceScaleFactor, const String& srcAttribute, const String& srcsetAttribute)
{
    ImageCandidates imageCandidates;

    parseImagesWithScaleFromSrcsetAttribute(srcsetAttribute, imageCandidates);

    if (!srcAttribute.isEmpty()) {
        ImageWithScale srcPlaceholderImage;
        imageCandidates.append(srcPlaceholderImage);
    }

    if (imageCandidates.isEmpty())
        return String();

    std::stable_sort(imageCandidates.begin(), imageCandidates.end(), compareByScaleFactor);

    for (size_t i = 0; i < imageCandidates.size() - 1; ++i) {
        if (imageCandidates[i].scaleFactor >= deviceScaleFactor)
            return imageCandidates[i].hasImageURL() ? srcsetAttribute.substringSharingImpl(imageCandidates[i].imageURLStart, imageCandidates[i].imageURLLength) : srcAttribute;
    }
    const ImageWithScale& lastCandidate = imageCandidates.last();
    return lastCandidate.hasImageURL() ? srcsetAttribute.substringSharingImpl(lastCandidate.imageURLStart, lastCandidate.imageURLLength) : srcAttribute;
}

}
