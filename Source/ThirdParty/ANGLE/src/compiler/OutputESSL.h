//
// Copyright (c) 2002-2011 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#ifndef CROSSCOMPILERGLSL_OUTPUTESSL_H_
#define CROSSCOMPILERGLSL_OUTPUTESSL_H_

#include "compiler/OutputGLSLBase.h"

class TOutputESSL : public TOutputGLSLBase
{
public:
    TOutputESSL(TInfoSinkBase& objSink);

protected:
    virtual bool writeVariablePrecision(TPrecision precision);
};

#endif  // CROSSCOMPILERGLSL_OUTPUTESSL_H_
