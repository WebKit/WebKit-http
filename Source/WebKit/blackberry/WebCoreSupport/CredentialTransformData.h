/*
 * Copyright (C) 2012 Research In Motion Limited. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef CredentialTransformData_h
#define CredentialTransformData_h

#include "Credential.h"
#include "HTMLFormElement.h"
#include "KURL.h"
#include "ProtectionSpace.h"

namespace WebCore {

struct CredentialTransformData {
    // If the provided form is suitable for password completion, isValid() will
    // return true;
    CredentialTransformData(HTMLFormElement*);
    CredentialTransformData(const KURL&, const ProtectionSpace&, const Credential&);

    // If creation failed, return false.
    bool isValid() const { return m_isValid; }

    KURL url() const;
    ProtectionSpace protectionSpace() const { return m_protectionSpace; }
    Credential credential() const;
    void setCredential(const Credential&);

private:
    bool findPasswordFormFields(HTMLFormElement*);

    KURL m_url;
    KURL m_action;
    ProtectionSpace m_protectionSpace;
    mutable Credential m_credential;
    HTMLInputElement* m_userNameElement;
    HTMLInputElement* m_passwordElement;
    bool m_isValid;
};

} // namespace WebCore

#endif
