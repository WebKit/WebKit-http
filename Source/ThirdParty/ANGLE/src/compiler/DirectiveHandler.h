//
// Copyright (c) 2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#ifndef COMPILER_DIRECTIVE_HANDLER_H_
#define COMPILER_DIRECTIVE_HANDLER_H_

#include "compiler/ExtensionBehavior.h"
#include "compiler/Pragma.h"
#include "compiler/preprocessor/new/DirectiveHandler.h"

class TDiagnostics;

class TDirectiveHandler : public pp::DirectiveHandler
{
  public:
    TDirectiveHandler(TExtensionBehavior& extBehavior,
                      TDiagnostics& diagnostics);
    virtual ~TDirectiveHandler();

    const TPragma& pragma() const { return mPragma; }
    const TExtensionBehavior& extensionBehavior() const { return mExtensionBehavior; }

    virtual void handleError(const pp::SourceLocation& loc,
                             const std::string& msg);

    virtual void handlePragma(const pp::SourceLocation& loc,
                              const std::string& name,
                              const std::string& value);

    virtual void handleExtension(const pp::SourceLocation& loc,
                                 const std::string& name,
                                 const std::string& behavior);

    virtual void handleVersion(const pp::SourceLocation& loc,
                               int version);

  private:
    TPragma mPragma;
    TExtensionBehavior& mExtensionBehavior;
    TDiagnostics& mDiagnostics;
};

#endif  // COMPILER_DIRECTIVE_HANDLER_H_
