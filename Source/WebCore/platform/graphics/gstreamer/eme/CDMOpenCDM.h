/* GStreamer OpenCDM decryptor
 *
 * Copyright (C) 2016-2017 TATA ELXSI
 * Copyright (C) 2016-2017 Metrological
 * Copyright (C) 2016-2017 Igalia S.L
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Suite 500,
 * Boston, MA 02110-1335, USA.
 */

#pragma once

#if ENABLE(ENCRYPTED_MEDIA) && USE(OPENCDM)

#include "CDM.h"
#include "CDMFactory.h"
#include "CDMInstance.h"
#include "MediaKeyStatus.h"
#include <wtf/HashMap.h>
#include <open_cdm.h>
#include <map>

namespace WebCore {

class CDMFactoryOpenCDM : public CDMFactory {
private:
    CDMFactoryOpenCDM();
    CDMFactoryOpenCDM(const CDMFactoryOpenCDM&) = delete;
    CDMFactoryOpenCDM& operator= (const CDMFactoryOpenCDM&) = delete;

public:
    static CDMFactoryOpenCDM& singleton();

    virtual ~CDMFactoryOpenCDM();

    virtual std::unique_ptr<CDMPrivate> createCDM(const String&) override;
    virtual bool supportsKeySystem(const String&) override;
};

class CDMInstanceOpenCDM final : public CDMInstance {
private:
    CDMInstanceOpenCDM() = delete;
    CDMInstanceOpenCDM(const CDMInstanceOpenCDM&) = delete;
    CDMInstanceOpenCDM& operator= (const CDMInstanceOpenCDM&) = delete;

    class Session {
    private:
        Session() = delete;
        Session(const Session&) = delete;
        Session& operator= (const Session&) = delete;

    public:
        Session(const media::OpenCdm& source, Ref<WebCore::SharedBuffer>&& initData);
        ~Session();

    public:
        inline bool IsValid() const {
            return (m_url.empty() == false);
        }
        inline const std::string& URL() const {
            return (m_url);
        }
        inline const std::string& Message() const {
            return (m_message);
        }
        inline bool NeedIndividualization() const {
            return (m_needIndividualisation);
        }
        inline Ref<WebCore::SharedBuffer>& SharedBuffer() {
           return (m_buffer);
        }
        inline media::OpenCdm::KeyStatus Update(const uint8_t* data, const uint16_t length, std::string& response) {
            return (m_session.Update(data, length, response));
        }
        inline int Load(std::string& response) {
            return (m_session.Load(response));
        }
        inline int Remove(std::string& response) {
            return (m_session.Remove(response));
        }
        inline int Close() {
            return (m_session.Close());
        }
        inline bool operator== (const String& data) const {
            return ( (m_buffer->size() == data.sizeInBytes()) &&
                     (memcmp(m_buffer->data(), data.latin1().data(), m_buffer->size()) == 0) );
        }
        inline bool operator!= (const String& data) const {
            return (!operator== (data));
        }

    private:
        media::OpenCdm m_session;
        std::string m_message;
        std::string m_url;
        bool m_needIndividualisation;
        Ref<WebCore::SharedBuffer> m_buffer;
    };

public:
    CDMInstanceOpenCDM(media::OpenCdm&, const String&);
    virtual ~CDMInstanceOpenCDM();

    // -----------------------------------------------------------------------------------------
    // Metadata getters. Just for some DRM characteristics
    // -----------------------------------------------------------------------------------------
    virtual const String& keySystem() const override { 
        return m_keySystem;
    }
    virtual ImplementationType implementationType() const override { 
        return  ImplementationType::OpenCDM; 
    }
    virtual SuccessValue initializeWithConfiguration(const MediaKeySystemConfiguration&) override {
        return Succeeded;
    }
    virtual SuccessValue setDistinctiveIdentifiersAllowed(bool) override {
        return Succeeded;
    }
    virtual SuccessValue setPersistentStateAllowed(bool) override {
        return Succeeded;
    }

    // -----------------------------------------------------------------------------------------
    // Operations on the DRM system
    // -----------------------------------------------------------------------------------------
    virtual SuccessValue setServerCertificate(Ref<SharedBuffer>&&) override; 
    
    // Request Licnese will automagically create a Session. The session is later on referred to with
    // its SessionId
    virtual void requestLicense(LicenseType, const AtomicString&, Ref<SharedBuffer>&&, LicenseCallback) override;

    // -----------------------------------------------------------------------------------------
    // Operations on the DRM system -> Session
    // -----------------------------------------------------------------------------------------
    void updateLicense(const String&, LicenseType, const SharedBuffer&, LicenseUpdateCallback) override;
    void loadSession(LicenseType, const String&, const String&, LoadSessionCallback) override;
    void closeSession(const String&, CloseSessionCallback) override;
    void removeSessionData(const String&, LicenseType, RemoveSessionDataCallback) override;
    void storeRecordOfKeyUsage(const String&) override;

    // The initial Data, is the only way to find a proper SessionId.
    String sessionIdByInitData(const String&, const bool firstInLine) const;

private:
    media::OpenCdm m_openCdm;
    std::string m_mimeType;
    std::map<std::string, Session> m_sessionIdMap;
    String m_keySystem;
};

} // namespace WebCore

SPECIALIZE_TYPE_TRAITS_CDM_INSTANCE(WebCore::CDMInstanceOpenCDM, WebCore::CDMInstance::ImplementationType::OpenCDM);

#endif // ENABLE(ENCRYPTED_MEDIA) && USE(OPENCDM)
