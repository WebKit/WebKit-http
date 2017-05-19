/* Playready Session management
 *
 * Copyright (C) 2016 Igalia S.L
 * Copyright (C) 2016 Metrological
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

#include "config.h"
#include "PlayreadySession.h"

#if USE(PLAYREADY)
#include "MediaPlayerPrivateGStreamer.h"
#include <wtf/UUID.h>

#if ENABLE(LEGACY_ENCRYPTED_MEDIA_V1) || ENABLE(LEGACY_ENCRYPTED_MEDIA)
#include "WebKitMediaKeyError.h"
#endif

#include <runtime/JSCInlines.h>
#include <runtime/TypedArrayInlines.h>
#include <wtf/text/CString.h>

GST_DEBUG_CATEGORY_EXTERN(webkit_media_playready_decrypt_debug_category);
#define GST_CAT_DEFAULT webkit_media_playready_decrypt_debug_category

G_LOCK_DEFINE_STATIC (pr_decoder_lock);

namespace WebCore {

// The default location of CDM DRM store.
// /tmp/drmstore.dat
const DRM_WCHAR g_rgwchCDMDrmStoreName[] = {WCHAR_CAST('/'), WCHAR_CAST('t'), WCHAR_CAST('m'), WCHAR_CAST('p'), WCHAR_CAST('/'),
                                            WCHAR_CAST('d'), WCHAR_CAST('r'), WCHAR_CAST('m'), WCHAR_CAST('s'), WCHAR_CAST('t'),
                                            WCHAR_CAST('o'), WCHAR_CAST('r'), WCHAR_CAST('e'), WCHAR_CAST('.'), WCHAR_CAST('d'),
                                            WCHAR_CAST('a'), WCHAR_CAST('t'), WCHAR_CAST('\0')};

const DRM_CONST_STRING g_dstrCDMDrmStoreName = CREATE_DRM_STRING(g_rgwchCDMDrmStoreName);

const DRM_CONST_STRING* g_rgpdstrRights[1] = {&g_dstrWMDRM_RIGHT_PLAYBACK};

PlayreadySession::PlayreadySession(const Vector<uint8_t> &initData, const void* pipeline)
    : m_key()
    , m_poAppContext(nullptr)
    , m_pbOpaqueBuffer(nullptr)
    , m_pbRevocationBuffer(nullptr)
    , m_eKeyState(KEY_INIT)
    , m_fCommit(FALSE)
    , m_sessionId(createCanonicalUUIDString())
    , m_initData(initData)
    , m_pipeline(pipeline)
{
    DRM_RESULT dr = DRM_SUCCESS;
    DRM_ID oSessionID;
    DRM_DWORD cchEncodedSessionID = SIZEOF(m_rgchSessionID);

    ChkMem(m_pbOpaqueBuffer = (DRM_BYTE *)Oem_MemAlloc(MINIMUM_APPCONTEXT_OPAQUE_BUFFER_SIZE));
    m_cbOpaqueBuffer = MINIMUM_APPCONTEXT_OPAQUE_BUFFER_SIZE;

    ChkMem(m_poAppContext = (DRM_APP_CONTEXT *)Oem_MemAlloc(SIZEOF(DRM_APP_CONTEXT)));
    ZEROMEM(m_poAppContext, SIZEOF(DRM_APP_CONTEXT));

    // Initialize DRM app context.
    ChkDR(Drm_Initialize(m_poAppContext,
                         NULL,
                         m_pbOpaqueBuffer,
                         m_cbOpaqueBuffer,
                         &g_dstrCDMDrmStoreName));

    if (DRM_REVOCATION_IsRevocationSupported()) {
        ChkMem(m_pbRevocationBuffer = (DRM_BYTE *)Oem_MemAlloc(REVOCATION_BUFFER_SIZE));

        ChkDR(Drm_Revocation_SetBuffer(m_poAppContext,
                                       m_pbRevocationBuffer,
                                       REVOCATION_BUFFER_SIZE));
    }

    // Generate a random media session ID.
    ChkDR(Oem_Random_GetBytes(NULL, (DRM_BYTE *)&oSessionID, SIZEOF(oSessionID)));

    ZEROMEM(m_rgchSessionID, SIZEOF(m_rgchSessionID));
    // Store the generated media session ID in base64 encoded form.
    ChkDR(DRM_B64_EncodeA((DRM_BYTE *)&oSessionID,
                          SIZEOF(oSessionID),
                          m_rgchSessionID,
                          &cchEncodedSessionID,
                          0));

    GST_DEBUG("Playready initialized");

 ErrorExit:
    if (DRM_FAILED(dr)) {
        m_eKeyState = KEY_ERROR;
        GST_ERROR("Playready initialization failed");
    }
}

PlayreadySession::~PlayreadySession()
{
    GST_DEBUG("Releasing resources");

    if (DRM_REVOCATION_IsRevocationSupported())
        SAFE_OEM_FREE(m_pbRevocationBuffer);

    SAFE_OEM_FREE(m_pbOpaqueBuffer);
    SAFE_OEM_FREE(m_poAppContext);
}

// PlayReady license policy callback which should be
// customized for platform/environment that hosts the CDM.
// It is currently implemented as a place holder that
// does nothing.
DRM_RESULT DRM_CALL PlayreadySession::_PolicyCallback(const DRM_VOID *, DRM_POLICY_CALLBACK_TYPE, const DRM_VOID *)
{
    return DRM_SUCCESS;
}

//
// Expected synchronisation from caller. This method is not thread-safe!
//
RefPtr<Uint8Array> PlayreadySession::playreadyGenerateKeyRequest(Uint8Array* initData, const String& customData, String& destinationURL, unsigned short& errorCode, uint32_t& systemCode)
{
    RefPtr<Uint8Array> result;
    DRM_RESULT dr = DRM_SUCCESS;
    DRM_BYTE *pbChallenge = NULL;
    DRM_DWORD cbChallenge = 0;
    DRM_CHAR *pchSilentURL = NULL;
    DRM_DWORD cchSilentURL = 0;

    GST_DEBUG("generating key request");
    GST_MEMDUMP("init data", initData->data(), initData->byteLength());

    // The current state MUST be KEY_INIT otherwise error out.
    ChkBOOL(m_eKeyState == KEY_INIT, DRM_E_INVALIDARG);

    ChkDR(Drm_Content_SetProperty(m_poAppContext,
                                  DRM_CSP_AUTODETECT_HEADER,
                                  initData->data(),
                                  initData->byteLength()));
    GST_DEBUG("init data set on DRM context");

    // FIXME: Revert once re-use of key is fixed
    Drm_Reader_Bind(m_poAppContext, g_rgpdstrRights, NO_OF(g_rgpdstrRights), _PolicyCallback, NULL, &m_oDecryptContext);
    GST_DEBUG("DRM reader bound");

    // Try to figure out the size of the license acquisition
    // challenge to be returned.
    dr = Drm_LicenseAcq_GenerateChallenge(m_poAppContext,
                                          g_rgpdstrRights,
                                          sizeof(g_rgpdstrRights) / sizeof(DRM_CONST_STRING*),
                                          NULL,
                                          customData.isEmpty() ? NULL: customData.utf8().data(),
                                          customData.isEmpty() ? 0 : customData.length(),
                                          NULL,
                                          &cchSilentURL,
                                          NULL,
                                          NULL,
                                          NULL,
                                          &cbChallenge);
 
    if (dr == DRM_E_BUFFERTOOSMALL) {
        if (cchSilentURL > 0) {
            ChkMem(pchSilentURL = (DRM_CHAR *)Oem_MemAlloc(cchSilentURL + 1));
            ZEROMEM(pchSilentURL, cchSilentURL + 1);
            GST_TRACE("allocated silent url size %d", cchSilentURL);
        }

        // Allocate buffer that is sufficient to store the license acquisition
        // challenge.
        if (cbChallenge > 0) {
            ChkMem(pbChallenge = (DRM_BYTE *)Oem_MemAlloc(cbChallenge));
            GST_TRACE("allocated challenge size %d", cbChallenge);
        }

        dr = DRM_SUCCESS;
    } else {
         ChkDR(dr);
         GST_DEBUG("challenge generated");
    }

    // Supply a buffer to receive the license acquisition challenge.
    ChkDR(Drm_LicenseAcq_GenerateChallenge(m_poAppContext,
                                           g_rgpdstrRights,
                                           sizeof(g_rgpdstrRights) / sizeof(DRM_CONST_STRING*),
                                           NULL,
                                           customData.isEmpty() ? NULL: customData.utf8().data(),
                                           customData.isEmpty() ? 0 : customData.length(),
                                           pchSilentURL,
                                           &cchSilentURL,
                                           NULL,
                                           NULL,
                                           pbChallenge,
                                           &cbChallenge));

    GST_TRACE("generated license request of size %d", cbChallenge);
    GST_MEMDUMP("generated license request :", pbChallenge, cbChallenge);

    result = Uint8Array::create(pbChallenge, cbChallenge);
    destinationURL = static_cast<const char *>(pchSilentURL);
    GST_DEBUG("destination URL : %s", destinationURL.utf8().data());

    m_eKeyState = KEY_PENDING;
    systemCode = dr;
    errorCode = 0;
    SAFE_OEM_FREE(pbChallenge);
    SAFE_OEM_FREE(pchSilentURL);

    return result;

ErrorExit:
    if (DRM_FAILED(dr)) {
        GST_DEBUG("DRM key generation failed");
#if ENABLE(LEGACY_ENCRYPTED_MEDIA_V1) || ENABLE(LEGACY_ENCRYPTED_MEDIA)
        errorCode = WebKitMediaKeyError::MEDIA_KEYERR_CLIENT;
#elif ENABLE(ENCRYPTED_MEDIA)
        systemCode = dr;
        errorCode = 1;
#endif
    }
    return result;
}

//
// Expected synchronisation from caller. This method is not thread-safe!
//
bool PlayreadySession::playreadyProcessKey(Uint8Array* key, RefPtr<Uint8Array>&, unsigned short& errorCode, uint32_t& systemCode)
{
    DRM_RESULT dr = DRM_SUCCESS;
    DRM_LICENSE_RESPONSE oLicenseResponse = {eUnknownProtocol, 0};
    uint8_t *m_pbKeyMessageResponse = key->data();
    uint32_t m_cbKeyMessageResponse = key->byteLength();

    GST_MEMDUMP("response received :", key->data(), key->byteLength());

    // The current state MUST be KEY_PENDING otherwise error out.
    ChkBOOL(m_eKeyState == KEY_PENDING, DRM_E_INVALIDARG);

    ChkArg(m_pbKeyMessageResponse != NULL && m_cbKeyMessageResponse > 0);

    ChkDR(Drm_LicenseAcq_ProcessResponse(m_poAppContext,
                                         DRM_PROCESS_LIC_RESPONSE_SIGNATURE_NOT_REQUIRED,
                                         NULL,
                                         NULL,
                                         const_cast<DRM_BYTE *>(m_pbKeyMessageResponse),
                                         m_cbKeyMessageResponse,
                                         &oLicenseResponse));

    ChkDR(Drm_Reader_Bind(m_poAppContext,
                          g_rgpdstrRights,
                          NO_OF(g_rgpdstrRights),
                          _PolicyCallback,
                          NULL,
                          &m_oDecryptContext));

    m_key = key->possiblySharedBuffer();
    errorCode = 0;
    m_eKeyState = KEY_READY;
    GST_DEBUG("key processed, now ready for content decryption");
    systemCode = dr;
    return true;

ErrorExit:
    if (DRM_FAILED(dr)) {
        GST_ERROR("failed processing license response");
#if ENABLE(LEGACY_ENCRYPTED_MEDIA_V1) || ENABLE(LEGACY_ENCRYPTED_MEDIA)
        errorCode = WebKitMediaKeyError::MEDIA_KEYERR_CLIENT;
#endif
        m_eKeyState = KEY_ERROR;
    }
    return false;
}

int PlayreadySession::processPayload(const void* iv, uint32_t ivSize, void* payloadData, uint32_t payloadDataSize)
{
    DRM_RESULT dr = DRM_SUCCESS;
    DRM_AES_COUNTER_MODE_CONTEXT oAESContext = {0};

    uint8_t* ivData = (uint8_t*) iv;

    G_LOCK (pr_decoder_lock);
    ChkDR(Drm_Reader_InitDecrypt(&m_oDecryptContext, NULL, 0));

    // FIXME: IV bytes need to be swapped ???
    uint8_t temp;
    for (uint32_t i = 0; i < ivSize / 2; i++) {
        temp = ivData[i];
        ivData[i] = ivData[ivSize - i - 1];
        ivData[ivSize - i - 1] = temp;
    }

    MEMCPY(&oAESContext.qwInitializationVector, ivData, ivSize);
    ChkDR(Drm_Reader_Decrypt(&m_oDecryptContext, &oAESContext, (DRM_BYTE*) payloadData,  payloadDataSize));

    // Call commit during the decryption of the first sample.
    if (!m_fCommit) {
        ChkDR(Drm_Reader_Commit(m_poAppContext, _PolicyCallback, NULL));
        m_fCommit = TRUE;
    }

    G_UNLOCK (pr_decoder_lock);

    return 0;

ErrorExit:
    G_UNLOCK (pr_decoder_lock);
    return 1;
}
}

#endif
