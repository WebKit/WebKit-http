/*
 * Copyright (C) 2014 Igalia S.L.
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

#include "config.h"
#include "LocalizedStrings.h"

#include "NotImplemented.h"
#include <wtf/text/WTFString.h>
#include <wtf/gobject/GUniquePtr.h>

// FIXME: Lots of unnecessarily defined functions here.

namespace WebCore {

String submitButtonDefaultLabel()
{
    return String::fromUTF8("Submit");
}

String inputElementAltText()
{
    return String::fromUTF8("Submit");
}

String resetButtonDefaultLabel()
{
    return String::fromUTF8("Reset");
}

String defaultDetailsSummaryText()
{
    return String::fromUTF8("Details");
}

String searchableIndexIntroduction()
{
    return String::fromUTF8("This is a searchable index. Enter search keywords: ");
}

String fileButtonChooseFileLabel()
{
    return String::fromUTF8("Choose File");
}

String fileButtonChooseMultipleFilesLabel()
{
    return String::fromUTF8("Choose Files");
}

String fileButtonNoFileSelectedLabel()
{
    return String::fromUTF8("(None)");
}

String fileButtonNoFilesSelectedLabel()
{
    return String::fromUTF8("(None)");
}

String contextMenuItemTagOpenLinkInNewWindow()
{
    return String::fromUTF8("Open Link in New _Window");
}

String contextMenuItemTagDownloadLinkToDisk()
{
    return String::fromUTF8("_Download Linked File");
}

String contextMenuItemTagCopyLinkToClipboard()
{
    return String::fromUTF8("Copy Link Loc_ation");
}

String contextMenuItemTagOpenImageInNewWindow()
{
    return String::fromUTF8("Open _Image in New Window");
}

String contextMenuItemTagDownloadImageToDisk()
{
    return String::fromUTF8("Sa_ve Image As");
}

String contextMenuItemTagCopyImageToClipboard()
{
    return String::fromUTF8("Cop_y Image");
}

String contextMenuItemTagCopyImageUrlToClipboard()
{
    return String::fromUTF8("Copy Image _Address");
}

String contextMenuItemTagOpenVideoInNewWindow()
{
    return String::fromUTF8("Open _Video in New Window");
}

String contextMenuItemTagOpenAudioInNewWindow()
{
    return String::fromUTF8("Open _Audio in New Window");
}

String contextMenuItemTagDownloadVideoToDisk()
{
    return String::fromUTF8("Download _Video");
}

String contextMenuItemTagDownloadAudioToDisk()
{
    return String::fromUTF8("Download _Audio");
}

String contextMenuItemTagCopyVideoLinkToClipboard()
{
    return String::fromUTF8("Cop_y Video Link Location");
}

String contextMenuItemTagCopyAudioLinkToClipboard()
{
    return String::fromUTF8("Cop_y Audio Link Location");
}

String contextMenuItemTagToggleMediaControls()
{
    return String::fromUTF8("_Toggle Media Controls");
}

String contextMenuItemTagShowMediaControls()
{
    return String::fromUTF8("_Show Media Controls");
}

String contextMenuItemTagHideMediaControls()
{
    return String::fromUTF8("_Hide Media Controls");
}

String contextMenuItemTagToggleMediaLoop()
{
    return String::fromUTF8("Toggle Media _Loop Playback");
}

String contextMenuItemTagEnterVideoFullscreen()
{
    return String::fromUTF8("Switch Video to _Fullscreen");
}

String contextMenuItemTagMediaPlay()
{
    return String::fromUTF8("_Play");
}

String contextMenuItemTagMediaPause()
{
    return String::fromUTF8("_Pause");
}

String contextMenuItemTagMediaMute()
{
    return String::fromUTF8("_Mute");
}

String contextMenuItemTagOpenFrameInNewWindow()
{
    return String::fromUTF8("Open _Frame in New Window");
}

String contextMenuItemTagCopy()
{
    return String::fromUTF8("Copy");
}

String contextMenuItemTagDelete()
{
    return String::fromUTF8("Delete");
}

String contextMenuItemTagSelectAll()
{
    return String::fromUTF8("Select All");
}

String contextMenuItemTagUnicode()
{
    return String::fromUTF8("_Insert Unicode Control Character");
}

String contextMenuItemTagInputMethods()
{
    return String::fromUTF8("Input _Methods");
}

String contextMenuItemTagGoBack()
{
    return String::fromUTF8("Go Back");
}

String contextMenuItemTagGoForward()
{
    return String::fromUTF8("Go Forward");
}

String contextMenuItemTagStop()
{
    return String::fromUTF8("Stop");
}

String contextMenuItemTagReload()
{
    return String::fromUTF8("_Reload");
}

String contextMenuItemTagCut()
{
    return String::fromUTF8("Cut");
}

String contextMenuItemTagPaste()
{
    return String::fromUTF8("Paste");
}

String contextMenuItemTagNoGuessesFound()
{
    return String::fromUTF8("No Guesses Found");
}

String contextMenuItemTagIgnoreSpelling()
{
    return String::fromUTF8("_Ignore Spelling");
}

String contextMenuItemTagLearnSpelling()
{
    return String::fromUTF8("_Learn Spelling");
}

String contextMenuItemTagSearchWeb()
{
    return String::fromUTF8("_Search the Web");
}

String contextMenuItemTagLookUpInDictionary(const String&)
{
    return String::fromUTF8("_Look Up in Dictionary");
}

String contextMenuItemTagOpenLink()
{
    return String::fromUTF8("_Open Link");
}

String contextMenuItemTagIgnoreGrammar()
{
    return String::fromUTF8("Ignore _Grammar");
}

String contextMenuItemTagSpellingMenu()
{
    return String::fromUTF8("Spelling and _Grammar");
}

String contextMenuItemTagShowSpellingPanel(bool show)
{
    return String::fromUTF8(show ? "_Show Spelling and Grammar" : "_Hide Spelling and Grammar");
}

String contextMenuItemTagCheckSpelling()
{
    return String::fromUTF8("_Check Document Now");
}

String contextMenuItemTagCheckSpellingWhileTyping()
{
    return String::fromUTF8("Check Spelling While _Typing");
}

String contextMenuItemTagCheckGrammarWithSpelling()
{
    return String::fromUTF8("Check _Grammar With Spelling");
}

String contextMenuItemTagFontMenu()
{
    return String::fromUTF8("_Font");
}

String contextMenuItemTagBold()
{
    return String::fromUTF8("Bold");
}

String contextMenuItemTagItalic()
{
    return String::fromUTF8("Italic");
}

String contextMenuItemTagUnderline()
{
    return String::fromUTF8("Underline");
}

String contextMenuItemTagOutline()
{
    return String::fromUTF8("_Outline");
}

String contextMenuItemTagInspectElement()
{
    return String::fromUTF8("Inspect _Element");
}

String contextMenuItemTagUnicodeInsertLRMMark()
{
    return String::fromUTF8("LRM _Left-to-right mark");
}

String contextMenuItemTagUnicodeInsertRLMMark()
{
    return String::fromUTF8("RLM _Right-to-left mark");
}

String contextMenuItemTagUnicodeInsertLREMark()
{
    return String::fromUTF8("LRE Left-to-right _embedding");
}

String contextMenuItemTagUnicodeInsertRLEMark()
{
    return String::fromUTF8("RLE Right-to-left e_mbedding");
}

String contextMenuItemTagUnicodeInsertLROMark()
{
    return String::fromUTF8("LRO Left-to-right _override");
}

String contextMenuItemTagUnicodeInsertRLOMark()
{
    return String::fromUTF8("RLO Right-to-left o_verride");
}

String contextMenuItemTagUnicodeInsertPDFMark()
{
    return String::fromUTF8("PDF _Pop directional formatting");
}

String contextMenuItemTagUnicodeInsertZWSMark()
{
    return String::fromUTF8("ZWS _Zero width space");
}

String contextMenuItemTagUnicodeInsertZWJMark()
{
    return String::fromUTF8("ZWJ Zero width _joiner");
}

String contextMenuItemTagUnicodeInsertZWNJMark()
{
    return String::fromUTF8("ZWNJ Zero width _non-joiner");
}

String contextMenuItemTagWritingDirectionMenu()
{
    return String::fromUTF8("Writing Direction Menu");
}

String contextMenuItemTagTextDirectionMenu()
{
    return String::fromUTF8("Text Direction Menu");
}

String contextMenuItemTagDefaultDirection()
{
    return String::fromUTF8("Default Direction");
}

String contextMenuItemTagLeftToRight()
{
    return String::fromUTF8("Left-To-Right");
}

String contextMenuItemTagRightToLeft()
{
    return String::fromUTF8("Right-To-Left");
}

String searchMenuNoRecentSearchesText()
{
    return String::fromUTF8("No recent searches");
}

String searchMenuRecentSearchesText()
{
    return String::fromUTF8("Recent searches");
}

String searchMenuClearRecentSearchesText()
{
    return String::fromUTF8("_Clear recent searches");
}

String AXDefinitionText()
{
    return String::fromUTF8("definition");
}

String AXDescriptionListText()
{
    return String::fromUTF8("description list");
}

String AXDescriptionListTermText()
{
    return String::fromUTF8("term");
}

String AXDescriptionListDetailText()
{
    return String::fromUTF8("description");
}

String AXFooterRoleDescriptionText()
{
    return String::fromUTF8("footer");
}

String AXSearchFieldCancelButtonText()
{
    return String::fromUTF8("cancel");
}

String AXButtonActionVerb()
{
    return String::fromUTF8("press");
}

String AXRadioButtonActionVerb()
{
    return String::fromUTF8("select");
}

String AXTextFieldActionVerb()
{
    return String::fromUTF8("activate");
}

String AXCheckedCheckBoxActionVerb()
{
    return String::fromUTF8("uncheck");
}

String AXUncheckedCheckBoxActionVerb()
{
    return String::fromUTF8("check");
}

String AXLinkActionVerb()
{
    return String::fromUTF8("jump");
}

String AXMenuListPopupActionVerb()
{
    return String();
}

String AXMenuListActionVerb()
{
    return String();
}

String AXListItemActionVerb()
{
    return String();
}
    
String missingPluginText()
{
    return String::fromUTF8("Missing Plug-in");
}

String crashedPluginText()
{
    notImplemented();
    return String::fromUTF8("Plug-in Failure");
}

String blockedPluginByContentSecurityPolicyText()
{
    notImplemented();
    return String();
}

String insecurePluginVersionText()
{
    notImplemented();
    return String();
}

String inactivePluginText()
{
    notImplemented();
    return String();
}

String multipleFileUploadText(unsigned numberOfFiles)
{
    // FIXME: If this file gets localized, this should really be localized as one string with a wildcard for the number.
    return String::number(numberOfFiles) + String::fromUTF8(" files");
}

String unknownFileSizeText()
{
    return String::fromUTF8("Unknown");
}

String imageTitle(const String& filename, const IntSize&)
{
    notImplemented();
    return filename;
}


#if ENABLE(VIDEO)

String mediaElementLoadingStateText()
{
    return String::fromUTF8("Loading...");
}

String mediaElementLiveBroadcastStateText()
{
    return String::fromUTF8("Live Broadcast");
}

String localizedMediaControlElementString(const String& name)
{
    if (name == "AudioElement")
        return String::fromUTF8("audio playback");
    if (name == "VideoElement")
        return String::fromUTF8("video playback");
    if (name == "MuteButton")
        return String::fromUTF8("mute");
    if (name == "UnMuteButton")
        return String::fromUTF8("unmute");
    if (name == "PlayButton")
        return String::fromUTF8("play");
    if (name == "PauseButton")
        return String::fromUTF8("pause");
    if (name == "Slider")
        return String::fromUTF8("movie time");
    if (name == "SliderThumb")
        return String::fromUTF8("timeline slider thumb");
    if (name == "RewindButton")
        return String::fromUTF8("back 30 seconds");
    if (name == "ReturnToRealtimeButton")
        return String::fromUTF8("return to realtime");
    if (name == "CurrentTimeDisplay")
        return String::fromUTF8("elapsed time");
    if (name == "TimeRemainingDisplay")
        return String::fromUTF8("remaining time");
    if (name == "StatusDisplay")
        return String::fromUTF8("status");
    if (name == "EnterFullscreenButton")
        return String::fromUTF8("enter fullscreen");
    if (name == "ExitFullscreenButton")
        return String::fromUTF8("exit fullscreen");
    if (name == "SeekForwardButton")
        return String::fromUTF8("fast forward");
    if (name == "SeekBackButton")
        return String::fromUTF8("fast reverse");
    if (name == "ShowClosedCaptionsButton")
        return String::fromUTF8("show closed captions");
    if (name == "HideClosedCaptionsButton")
        return String::fromUTF8("hide closed captions");
    if (name == "ControlsPanel")
        return String::fromUTF8("media controls");

    ASSERT_NOT_REACHED();
    return String();
}

String localizedMediaControlElementHelpText(const String& name)
{
    if (name == "AudioElement")
        return String::fromUTF8("audio element playback controls and status display");
    if (name == "VideoElement")
        return String::fromUTF8("video element playback controls and status display");
    if (name == "MuteButton")
        return String::fromUTF8("mute audio tracks");
    if (name == "UnMuteButton")
        return String::fromUTF8("unmute audio tracks");
    if (name == "PlayButton")
        return String::fromUTF8("begin playback");
    if (name == "PauseButton")
        return String::fromUTF8("pause playback");
    if (name == "Slider")
        return String::fromUTF8("movie time scrubber");
    if (name == "SliderThumb")
        return String::fromUTF8("movie time scrubber thumb");
    if (name == "RewindButton")
        return String::fromUTF8("seek movie back 30 seconds");
    if (name == "ReturnToRealtimeButton")
        return String::fromUTF8("return streaming movie to real time");
    if (name == "CurrentTimeDisplay")
        return String::fromUTF8("current movie time in seconds");
    if (name == "TimeRemainingDisplay")
        return String::fromUTF8("number of seconds of movie remaining");
    if (name == "StatusDisplay")
        return String::fromUTF8("current movie status");
    if (name == "SeekBackButton")
        return String::fromUTF8("seek quickly back");
    if (name == "SeekForwardButton")
        return String::fromUTF8("seek quickly forward");
    if (name == "EnterFullscreenButton")
        return String::fromUTF8("Play movie in fullscreen mode");
    if (name == "EnterFullscreenButton")
        return String::fromUTF8("Exit fullscreen mode");
    if (name == "ShowClosedCaptionsButton")
        return String::fromUTF8("start displaying closed captions");
    if (name == "HideClosedCaptionsButton")
        return String::fromUTF8("stop displaying closed captions");

    ASSERT_NOT_REACHED();
    return String();
}

String localizedMediaTimeDescription(float time)
{
    if (!std::isfinite(time))
        return String::fromUTF8("indefinite time");

    int seconds = static_cast<int>(std::abs(time));
    int days = seconds / (60 * 60 * 24);
    int hours = seconds / (60 * 60);
    int minutes = (seconds / 60) % 60;
    seconds %= 60;

    if (days) {
        GUniquePtr<gchar> string(g_strdup_printf("%d days %d hours %d minutes %d seconds", days, hours, minutes, seconds));
        return String::fromUTF8(string.get());
    }

    if (hours) {
        GUniquePtr<gchar> string(g_strdup_printf("%d hours %d minutes %d seconds", hours, minutes, seconds));
        return String::fromUTF8(string.get());
    }

    if (minutes) {
        GUniquePtr<gchar> string(g_strdup_printf("%d minutes %d seconds", minutes, seconds));
        return String::fromUTF8(string.get());
    }

    GUniquePtr<gchar> string(g_strdup_printf("%d seconds", seconds));
    return String::fromUTF8(string.get());
}
#endif  // ENABLE(VIDEO)

String validationMessageValueMissingText()
{
    return String::fromUTF8("value missing");
}

String validationMessageValueMissingForCheckboxText()
{
    notImplemented();
    return validationMessageValueMissingText();
}

String validationMessageValueMissingForFileText()
{
    notImplemented();
    return validationMessageValueMissingText();
}

String validationMessageValueMissingForMultipleFileText()
{
    notImplemented();
    return validationMessageValueMissingText();
}

String validationMessageValueMissingForRadioText()
{
    notImplemented();
    return validationMessageValueMissingText();
}

String validationMessageValueMissingForSelectText()
{
    notImplemented();
    return validationMessageValueMissingText();
}

String validationMessageTypeMismatchText()
{
    notImplemented();
    return String::fromUTF8("type mismatch");
}

String validationMessageTypeMismatchForEmailText()
{
    notImplemented();
    return validationMessageTypeMismatchText();
}

String validationMessageTypeMismatchForMultipleEmailText()
{
    notImplemented();
    return validationMessageTypeMismatchText();
}

String validationMessageTypeMismatchForURLText()
{
    notImplemented();
    return validationMessageTypeMismatchText();
}

String validationMessagePatternMismatchText()
{
    return String::fromUTF8("pattern mismatch");
}

String validationMessageTooLongText(int, int)
{
    return String::fromUTF8("too long");
}

String validationMessageRangeUnderflowText(const String&)
{
    return String::fromUTF8("range underflow");
}

String validationMessageRangeOverflowText(const String&)
{
    return String::fromUTF8("range overflow");
}

String validationMessageStepMismatchText(const String&, const String&)
{
    return String::fromUTF8("step mismatch");
}

String unacceptableTLSCertificate()
{
    return String::fromUTF8("Unacceptable TLS certificate");
}

String localizedString(const char* key)
{
    return String::fromUTF8(key, strlen(key));
}

String validationMessageBadInputForNumberText()
{
    notImplemented();
    return validationMessageTypeMismatchText();
}

#if ENABLE(VIDEO_TRACK)
String textTrackClosedCaptionsText()
{
    return String::fromUTF8("Closed Captions");
}

String textTrackSubtitlesText()
{
    return String::fromUTF8("Subtitles");
}

String textTrackOffMenuItemText()
{
    return String::fromUTF8("Off");
}

String textTrackAutomaticMenuItemText()
{
    return String::fromUTF8("Auto");
}

String textTrackNoLabelText()
{
    return String::fromUTF8("No label");
}

String audioTrackNoLabelText()
{
    return String::fromUTF8("No label");
}
#endif

String snapshottedPlugInLabelTitle()
{
    return String::fromUTF8("Snapshotted Plug-In");
}

String snapshottedPlugInLabelSubtitle()
{
    return String::fromUTF8("Click to restart");
}

}
