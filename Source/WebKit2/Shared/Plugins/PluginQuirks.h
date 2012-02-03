/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef PluginQuirks_h
#define PluginQuirks_h

namespace WebKit {

class PluginQuirks {
public:
    enum PluginQuirk {
        // Mac specific quirks:
#if PLUGIN_ARCHITECTURE(MAC)
        // The plug-in wants the call to getprogname() to return "WebKitPluginHost".
        // Adobe Flash Will not handle key down events otherwise.
        PrognameShouldBeWebKitPluginHost,

        // Supports receiving a paint event, even when using CoreAnimation rendering.
        SupportsSnapshotting,

        // Make the plug-in transparent if it has a "background" attribute set.
        // Microsoft Silverlight doesn't opt into transparency using NPN_SetValue and
        // NPPVpluginTransparentBool, so we'll always force if the plug-in has a "background"
        // attribute specified, regardless of it's value.
        // FIXME: We could get more fancy here and check for specific values that we know are
        // transparent.
        MakeTransparentIfBackgroundAttributeExists,

        // Whether we can short circuit some NPRuntime calls during plug-in initialization.
        // The Flash plug-in uses NPRuntime to figure out the URL of the frame it is in, as well
        // as the URL of the main frame. Since we know the exact NPRuntime calls the plug-in makes,
        // we can return the right values without having to do sync IPC back into the web process.
        CanShortCircuitSomeNPRuntimeCallsDuringInitialization,

        // Whether calling NPP_GetValue with NPPVpluginCoreAnimationLayer returns a retained Core Animation
        // layer or not. According to the NPAPI specifications, plug-in shouldn't return a retained layer but
        // WebKit1 expects a retained plug-in layer. We use this for Flash to avoid leaking OpenGL layers.
        ReturnsRetainedCoreAnimationLayer,

        // Whether NPP_GetValue with NPPVpluginScriptableNPObject returns a non-retained NPObject or not.
        // Versions of Silverlight prior to 4 never retained the returned NPObject.
        ReturnsNonRetainedScriptableNPObject,

        // Whether the plug-in wants parameter names to be lowercase.
        // <rdar://problem/8440903>: AppleConnect has a bug where it does not
        // understand the parameter names specified in the <object> element that
        // embeds its plug-in. 
        WantsLowercaseParameterNames,

        // Whether to append Version/3.2.1 to the user-agent passed to the plugin
        // This is necessary to disable Silverlight's workaround for a Safari 2 leak
        // which is enabled if it doesn't find Version/3 in the user-agent.
        AppendVersion3UserAgent,

#ifndef NP_NO_QUICKDRAW
        // Allow the plug-in to use the QuickDraw drawing model, since we know that the plug-in
        // will never paint or receive events. Used by the AppleConnect plug-in.
        AllowHalfBakedQuickDrawSupport,
#endif

        // X11 specific quirks:
#elif PLUGIN_ARCHITECTURE(X11)
        // Flash and npwrapper ask the browser about which GTK version does it use
        // and refuse to load and work if it is not GTK 2 so we need to fake it in
        // NPN_GetValue even when it is a lie.
        RequiresGTKToolKit,

        // Some version 10 releases of Flash run under nspluginwrapper will completely
        // freeze when sending right click events to them in windowed mode.
        IgnoreRightClickInWindowlessMode,

        // Windows specific quirks:
#elif PLUGIN_ARCHITECTURE(WIN)
        // Whether NPN_UserAgent should always return a Mozilla user agent.
        // Flash on Windows prior to version 10 only requests windowless plugins 
        // if we return a Mozilla user agent.
        WantsMozillaUserAgent,
#endif

        NumPluginQuirks
    };
    
    PluginQuirks()
        : m_quirks(0)
    {
        COMPILE_ASSERT(sizeof(m_quirks) * 8 >= NumPluginQuirks, not_enough_room_for_quirks);
    }
    
    void add(PluginQuirk quirk)
    {
        ASSERT(quirk >= 0);
        ASSERT(quirk < NumPluginQuirks);
        
        m_quirks |= (1 << quirk);
    }
    
    bool contains(PluginQuirk quirk) const
    {
        return m_quirks & (1 << quirk);
    }

private:
    uint32_t m_quirks;
};

} // namespace WebKit

#endif // PluginQuirkSet_h
