/*
 * Copyright (C) 2011, 2012 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef CSSCalculationValue_h
#define CSSCalculationValue_h

#include "CSSParserValues.h"
#include "CSSValue.h"
#include "CalculationValue.h"
#include <wtf/PassOwnPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/RefPtr.h>

namespace WebCore {

class CSSParserValueList;
class CSSValueList;
class RenderStyle;
class CalcValue;
class CalcExpressionNode;

enum CalculationCategory {
    CalcNumber = 0,
    CalcLength,
    CalcPercent,
    CalcPercentNumber,
    CalcPercentLength,
    CalcOther
};
    
class CSSCalcExpressionNode : public RefCounted<CSSCalcExpressionNode> {
public:
    
    virtual ~CSSCalcExpressionNode() = 0;  
    virtual bool isZero() const = 0;
    virtual double doubleValue() const = 0;  
    virtual double computeLengthPx(RenderStyle* currentStyle, RenderStyle* rootStyle, double multiplier = 1.0, bool computingFontSize = false) const = 0;
    
    CalculationCategory category() const { return m_category; }    
    bool isInteger() const { return m_isInteger; }
    
protected:
    CSSCalcExpressionNode(CalculationCategory category, bool isInteger)
        : m_category(category)
        , m_isInteger(isInteger)
    {
    }
    
    CalculationCategory m_category;
    bool m_isInteger;
};
        
class CSSCalcValue : public CSSValue {
public:
    static PassRefPtr<CSSCalcValue> create(CSSParserString name, CSSParserValueList*);

    CalculationCategory category() const { return m_expression->category(); }
    bool isInt() const { return m_expression->isInteger(); }    
    double doubleValue() const;
    double computeLengthPx(RenderStyle* currentStyle, RenderStyle* rootStyle, double multiplier = 1.0, bool computingFontSize = false) const;
        
    String customCssText() const;
    
private:    
    CSSCalcValue(PassRefPtr<CSSCalcExpressionNode> expression)
        : CSSValue(CalculationClass)
        , m_expression(expression)
    {
    }
    
    const RefPtr<CSSCalcExpressionNode> m_expression;
};
    
} // namespace WebCore

#endif // CSSCalculationValue_h
