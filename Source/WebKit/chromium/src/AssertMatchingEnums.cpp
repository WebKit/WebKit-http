/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
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

// Use this file to assert that various WebKit API enum values continue
// matching WebCore defined enum values.

#include "config.h"

#include "AXObjectCache.h"
#include "AccessibilityObject.h"
#include "ApplicationCacheHost.h"
#include "ContentSecurityPolicy.h"
#include "DocumentMarker.h"
#include "EditorInsertAction.h"
#include "ExceptionCode.h"
#include "FileError.h"
#include "FileMetadata.h"
#include "FileSystemType.h"
#include "FilterOperation.h"
#include "FontDescription.h"
#include "FontSmoothingMode.h"
#include "GeolocationError.h"
#include "GeolocationPosition.h"
#include "HTMLInputElement.h"
#include "IDBCursor.h"
#include "IDBDatabaseException.h"
#include "IDBFactoryBackendInterface.h"
#include "IDBKey.h"
#include "IDBKeyPath.h"
#include "IDBMetadata.h"
#include "IDBTransactionBackendInterface.h"
#include "IceOptions.h"
#include "IconURL.h"
#include "MediaPlayer.h"
#include "MediaStreamSource.h"
#include "NotificationClient.h"
#include "PageVisibilityState.h"
#include "PeerConnection00.h"
#include "PlatformCursor.h"
#include "RTCPeerConnectionHandlerClient.h"
#include "ReferrerPolicy.h"
#include "ResourceResponse.h"
#include "Settings.h"
#include "SpeechRecognitionError.h"
#include "StorageInfo.h"
#include "TextAffinity.h"
#include "TextChecking.h"
#include "TextControlInnerElements.h"
#include "UserContentTypes.h"
#include "UserScriptTypes.h"
#include "UserStyleSheetTypes.h"
#include "WebAccessibilityNotification.h"
#include "WebAccessibilityObject.h"
#include "WebApplicationCacheHost.h"
#include "WebContentSecurityPolicy.h"
#include "WebCursorInfo.h"
#include "WebEditingAction.h"
#include "WebFileError.h"
#include "WebFileInfo.h"
#include "WebFontDescription.h"
#include "WebGeolocationError.h"
#include "WebGeolocationPosition.h"
#include "WebIDBCursor.h"
#include "WebIDBDatabaseException.h"
#include "WebIDBFactory.h"
#include "WebIDBKey.h"
#include "WebIDBKeyPath.h"
#include "WebIDBMetadata.h"
#include "WebIDBTransaction.h"
#include "WebIconURL.h"
#include "WebInputElement.h"
#include "WebMediaPlayer.h"
#include "WebMediaPlayerClient.h"
#include "WebNotificationPresenter.h"
#include "WebPageVisibilityState.h"
#include "WebSettings.h"
#include "WebSpeechRecognizerClient.h"
#include "WebStorageQuotaError.h"
#include "WebStorageQuotaType.h"
#include "WebTextAffinity.h"
#include "WebTextCaseSensitivity.h"
#include "WebTextCheckingResult.h"
#include "WebTextCheckingType.h"
#include "WebView.h"
#include <public/WebClipboard.h>
#include <public/WebFileSystem.h>
#include <public/WebFilterOperation.h>
#include <public/WebICEOptions.h>
#include <public/WebMediaStreamSource.h>
#include <public/WebPeerConnection00Handler.h>
#include <public/WebPeerConnection00HandlerClient.h>
#include <public/WebRTCPeerConnectionHandler.h>
#include <public/WebRTCPeerConnectionHandlerClient.h>
#include <public/WebReferrerPolicy.h>
#include <public/WebScrollbar.h>
#include <public/WebURLResponse.h>
#include <wtf/Assertions.h>
#include <wtf/text/StringImpl.h>

#if OS(DARWIN)
#include "PlatformSupport.h"
#include <public/mac/WebThemeEngine.h>
#endif

#define COMPILE_ASSERT_MATCHING_ENUM(webkit_name, webcore_name) \
    COMPILE_ASSERT(int(WebKit::webkit_name) == int(WebCore::webcore_name), mismatching_enums)

// These constants are in WTF, bring them into WebCore so the ASSERT still works for them!
namespace WebCore {
    using WTF::TextCaseSensitive;
    using WTF::TextCaseInsensitive;
};

COMPILE_ASSERT_MATCHING_ENUM(WebAccessibilityNotificationActiveDescendantChanged, AXObjectCache::AXActiveDescendantChanged);
COMPILE_ASSERT_MATCHING_ENUM(WebAccessibilityNotificationAutocorrectionOccured, AXObjectCache::AXAutocorrectionOccured);
COMPILE_ASSERT_MATCHING_ENUM(WebAccessibilityNotificationCheckedStateChanged, AXObjectCache::AXCheckedStateChanged);
COMPILE_ASSERT_MATCHING_ENUM(WebAccessibilityNotificationChildrenChanged, AXObjectCache::AXChildrenChanged);
COMPILE_ASSERT_MATCHING_ENUM(WebAccessibilityNotificationFocusedUIElementChanged, AXObjectCache::AXFocusedUIElementChanged);
COMPILE_ASSERT_MATCHING_ENUM(WebAccessibilityNotificationLayoutComplete, AXObjectCache::AXLayoutComplete);
COMPILE_ASSERT_MATCHING_ENUM(WebAccessibilityNotificationLoadComplete, AXObjectCache::AXLoadComplete);
COMPILE_ASSERT_MATCHING_ENUM(WebAccessibilityNotificationSelectedChildrenChanged, AXObjectCache::AXSelectedChildrenChanged);
COMPILE_ASSERT_MATCHING_ENUM(WebAccessibilityNotificationSelectedTextChanged, AXObjectCache::AXSelectedTextChanged);
COMPILE_ASSERT_MATCHING_ENUM(WebAccessibilityNotificationValueChanged, AXObjectCache::AXValueChanged);
COMPILE_ASSERT_MATCHING_ENUM(WebAccessibilityNotificationScrolledToAnchor, AXObjectCache::AXScrolledToAnchor);
COMPILE_ASSERT_MATCHING_ENUM(WebAccessibilityNotificationLiveRegionChanged, AXObjectCache::AXLiveRegionChanged);
COMPILE_ASSERT_MATCHING_ENUM(WebAccessibilityNotificationMenuListItemSelected, AXObjectCache::AXMenuListItemSelected);
COMPILE_ASSERT_MATCHING_ENUM(WebAccessibilityNotificationMenuListValueChanged, AXObjectCache::AXMenuListValueChanged);
COMPILE_ASSERT_MATCHING_ENUM(WebAccessibilityNotificationRowCountChanged, AXObjectCache::AXRowCountChanged);
COMPILE_ASSERT_MATCHING_ENUM(WebAccessibilityNotificationRowCollapsed, AXObjectCache::AXRowCollapsed);
COMPILE_ASSERT_MATCHING_ENUM(WebAccessibilityNotificationRowExpanded, AXObjectCache::AXRowExpanded);
COMPILE_ASSERT_MATCHING_ENUM(WebAccessibilityNotificationInvalidStatusChanged, AXObjectCache::AXInvalidStatusChanged);

COMPILE_ASSERT_MATCHING_ENUM(WebAccessibilityRoleUnknown, UnknownRole);
COMPILE_ASSERT_MATCHING_ENUM(WebAccessibilityRoleButton, ButtonRole);
COMPILE_ASSERT_MATCHING_ENUM(WebAccessibilityRoleRadioButton, RadioButtonRole);
COMPILE_ASSERT_MATCHING_ENUM(WebAccessibilityRoleCheckBox, CheckBoxRole);
COMPILE_ASSERT_MATCHING_ENUM(WebAccessibilityRoleSlider, SliderRole);
COMPILE_ASSERT_MATCHING_ENUM(WebAccessibilityRoleTabGroup, TabGroupRole);
COMPILE_ASSERT_MATCHING_ENUM(WebAccessibilityRoleTextField, TextFieldRole);
COMPILE_ASSERT_MATCHING_ENUM(WebAccessibilityRoleStaticText, StaticTextRole);
COMPILE_ASSERT_MATCHING_ENUM(WebAccessibilityRoleTextArea, TextAreaRole);
COMPILE_ASSERT_MATCHING_ENUM(WebAccessibilityRoleScrollArea, ScrollAreaRole);
COMPILE_ASSERT_MATCHING_ENUM(WebAccessibilityRolePopUpButton, PopUpButtonRole);
COMPILE_ASSERT_MATCHING_ENUM(WebAccessibilityRoleMenuButton, MenuButtonRole);
COMPILE_ASSERT_MATCHING_ENUM(WebAccessibilityRoleTable, TableRole);
COMPILE_ASSERT_MATCHING_ENUM(WebAccessibilityRoleApplication, ApplicationRole);
COMPILE_ASSERT_MATCHING_ENUM(WebAccessibilityRoleGroup, GroupRole);
COMPILE_ASSERT_MATCHING_ENUM(WebAccessibilityRoleRadioGroup, RadioGroupRole);
COMPILE_ASSERT_MATCHING_ENUM(WebAccessibilityRoleList, ListRole);
COMPILE_ASSERT_MATCHING_ENUM(WebAccessibilityRoleScrollBar, ScrollBarRole);
COMPILE_ASSERT_MATCHING_ENUM(WebAccessibilityRoleValueIndicator, ValueIndicatorRole);
COMPILE_ASSERT_MATCHING_ENUM(WebAccessibilityRoleImage, ImageRole);
COMPILE_ASSERT_MATCHING_ENUM(WebAccessibilityRoleMenuBar, MenuBarRole);
COMPILE_ASSERT_MATCHING_ENUM(WebAccessibilityRoleMenu, MenuRole);
COMPILE_ASSERT_MATCHING_ENUM(WebAccessibilityRoleMenuItem, MenuItemRole);
COMPILE_ASSERT_MATCHING_ENUM(WebAccessibilityRoleColumn, ColumnRole);
COMPILE_ASSERT_MATCHING_ENUM(WebAccessibilityRoleRow, RowRole);
COMPILE_ASSERT_MATCHING_ENUM(WebAccessibilityRoleToolbar, ToolbarRole);
COMPILE_ASSERT_MATCHING_ENUM(WebAccessibilityRoleBusyIndicator, BusyIndicatorRole);
COMPILE_ASSERT_MATCHING_ENUM(WebAccessibilityRoleProgressIndicator, ProgressIndicatorRole);
COMPILE_ASSERT_MATCHING_ENUM(WebAccessibilityRoleWindow, WindowRole);
COMPILE_ASSERT_MATCHING_ENUM(WebAccessibilityRoleDrawer, DrawerRole);
COMPILE_ASSERT_MATCHING_ENUM(WebAccessibilityRoleSystemWide, SystemWideRole);
COMPILE_ASSERT_MATCHING_ENUM(WebAccessibilityRoleOutline, OutlineRole);
COMPILE_ASSERT_MATCHING_ENUM(WebAccessibilityRoleIncrementor, IncrementorRole);
COMPILE_ASSERT_MATCHING_ENUM(WebAccessibilityRoleBrowser, BrowserRole);
COMPILE_ASSERT_MATCHING_ENUM(WebAccessibilityRoleComboBox, ComboBoxRole);
COMPILE_ASSERT_MATCHING_ENUM(WebAccessibilityRoleSplitGroup, SplitGroupRole);
COMPILE_ASSERT_MATCHING_ENUM(WebAccessibilityRoleSplitter, SplitterRole);
COMPILE_ASSERT_MATCHING_ENUM(WebAccessibilityRoleColorWell, ColorWellRole);
COMPILE_ASSERT_MATCHING_ENUM(WebAccessibilityRoleGrowArea, GrowAreaRole);
COMPILE_ASSERT_MATCHING_ENUM(WebAccessibilityRoleSheet, SheetRole);
COMPILE_ASSERT_MATCHING_ENUM(WebAccessibilityRoleHelpTag, HelpTagRole);
COMPILE_ASSERT_MATCHING_ENUM(WebAccessibilityRoleMatte, MatteRole);
COMPILE_ASSERT_MATCHING_ENUM(WebAccessibilityRoleRuler, RulerRole);
COMPILE_ASSERT_MATCHING_ENUM(WebAccessibilityRoleRulerMarker, RulerMarkerRole);
COMPILE_ASSERT_MATCHING_ENUM(WebAccessibilityRoleLink, LinkRole);
COMPILE_ASSERT_MATCHING_ENUM(WebAccessibilityRoleDisclosureTriangle, DisclosureTriangleRole);
COMPILE_ASSERT_MATCHING_ENUM(WebAccessibilityRoleGrid, GridRole);
COMPILE_ASSERT_MATCHING_ENUM(WebAccessibilityRoleCell, CellRole);
COMPILE_ASSERT_MATCHING_ENUM(WebAccessibilityRoleColumnHeader, ColumnHeaderRole);
COMPILE_ASSERT_MATCHING_ENUM(WebAccessibilityRoleRowHeader, RowHeaderRole);
COMPILE_ASSERT_MATCHING_ENUM(WebAccessibilityRoleWebCoreLink, WebCoreLinkRole);
COMPILE_ASSERT_MATCHING_ENUM(WebAccessibilityRoleImageMapLink, ImageMapLinkRole);
COMPILE_ASSERT_MATCHING_ENUM(WebAccessibilityRoleImageMap, ImageMapRole);
COMPILE_ASSERT_MATCHING_ENUM(WebAccessibilityRoleListMarker, ListMarkerRole);
COMPILE_ASSERT_MATCHING_ENUM(WebAccessibilityRoleWebArea, WebAreaRole);
COMPILE_ASSERT_MATCHING_ENUM(WebAccessibilityRoleHeading, HeadingRole);
COMPILE_ASSERT_MATCHING_ENUM(WebAccessibilityRoleListBox, ListBoxRole);
COMPILE_ASSERT_MATCHING_ENUM(WebAccessibilityRoleListBoxOption, ListBoxOptionRole);
COMPILE_ASSERT_MATCHING_ENUM(WebAccessibilityRoleMenuListOption, MenuListOptionRole);
COMPILE_ASSERT_MATCHING_ENUM(WebAccessibilityRoleMenuListPopup, MenuListPopupRole);
COMPILE_ASSERT_MATCHING_ENUM(WebAccessibilityRoleTableHeaderContainer, TableHeaderContainerRole);
COMPILE_ASSERT_MATCHING_ENUM(WebAccessibilityRoleDefinitionListTerm, DefinitionListTermRole);
COMPILE_ASSERT_MATCHING_ENUM(WebAccessibilityRoleDefinitionListDefinition, DefinitionListDefinitionRole);
COMPILE_ASSERT_MATCHING_ENUM(WebAccessibilityRoleAnnotation, AnnotationRole);
COMPILE_ASSERT_MATCHING_ENUM(WebAccessibilityRoleSliderThumb, SliderThumbRole);
COMPILE_ASSERT_MATCHING_ENUM(WebAccessibilityRoleSpinButton, SpinButtonRole);
COMPILE_ASSERT_MATCHING_ENUM(WebAccessibilityRoleSpinButtonPart, SpinButtonPartRole);
COMPILE_ASSERT_MATCHING_ENUM(WebAccessibilityRoleIgnored, IgnoredRole);
COMPILE_ASSERT_MATCHING_ENUM(WebAccessibilityRolePresentational, PresentationalRole);
COMPILE_ASSERT_MATCHING_ENUM(WebAccessibilityRoleTab, TabRole);
COMPILE_ASSERT_MATCHING_ENUM(WebAccessibilityRoleTabList, TabListRole);
COMPILE_ASSERT_MATCHING_ENUM(WebAccessibilityRoleTabPanel, TabPanelRole);
COMPILE_ASSERT_MATCHING_ENUM(WebAccessibilityRoleTreeRole, TreeRole);
COMPILE_ASSERT_MATCHING_ENUM(WebAccessibilityRoleTreeGrid, TreeGridRole);
COMPILE_ASSERT_MATCHING_ENUM(WebAccessibilityRoleTreeItemRole, TreeItemRole);
COMPILE_ASSERT_MATCHING_ENUM(WebAccessibilityRoleDirectory, DirectoryRole);
COMPILE_ASSERT_MATCHING_ENUM(WebAccessibilityRoleEditableText, EditableTextRole);
COMPILE_ASSERT_MATCHING_ENUM(WebAccessibilityRoleFooter, FooterRole);
COMPILE_ASSERT_MATCHING_ENUM(WebAccessibilityRoleListItem, ListItemRole);
COMPILE_ASSERT_MATCHING_ENUM(WebAccessibilityRoleParagraph, ParagraphRole);
COMPILE_ASSERT_MATCHING_ENUM(WebAccessibilityRoleLabel, LabelRole);
COMPILE_ASSERT_MATCHING_ENUM(WebAccessibilityRoleDiv, DivRole);
COMPILE_ASSERT_MATCHING_ENUM(WebAccessibilityRoleForm, FormRole);
COMPILE_ASSERT_MATCHING_ENUM(WebAccessibilityRoleLandmarkApplication, LandmarkApplicationRole);
COMPILE_ASSERT_MATCHING_ENUM(WebAccessibilityRoleLandmarkBanner, LandmarkBannerRole);
COMPILE_ASSERT_MATCHING_ENUM(WebAccessibilityRoleLandmarkComplementary, LandmarkComplementaryRole);
COMPILE_ASSERT_MATCHING_ENUM(WebAccessibilityRoleLandmarkContentInfo, LandmarkContentInfoRole);
COMPILE_ASSERT_MATCHING_ENUM(WebAccessibilityRoleLandmarkMain, LandmarkMainRole);
COMPILE_ASSERT_MATCHING_ENUM(WebAccessibilityRoleLandmarkNavigation, LandmarkNavigationRole);
COMPILE_ASSERT_MATCHING_ENUM(WebAccessibilityRoleLandmarkSearch, LandmarkSearchRole);
COMPILE_ASSERT_MATCHING_ENUM(WebAccessibilityRoleApplicationAlert, ApplicationAlertRole);
COMPILE_ASSERT_MATCHING_ENUM(WebAccessibilityRoleApplicationAlertDialog, ApplicationAlertDialogRole);
COMPILE_ASSERT_MATCHING_ENUM(WebAccessibilityRoleApplicationDialog, ApplicationDialogRole);
COMPILE_ASSERT_MATCHING_ENUM(WebAccessibilityRoleApplicationLog, ApplicationLogRole);
COMPILE_ASSERT_MATCHING_ENUM(WebAccessibilityRoleApplicationMarquee, ApplicationMarqueeRole);
COMPILE_ASSERT_MATCHING_ENUM(WebAccessibilityRoleApplicationStatus, ApplicationStatusRole);
COMPILE_ASSERT_MATCHING_ENUM(WebAccessibilityRoleApplicationTimer, ApplicationTimerRole);
COMPILE_ASSERT_MATCHING_ENUM(WebAccessibilityRoleDocument, DocumentRole);
COMPILE_ASSERT_MATCHING_ENUM(WebAccessibilityRoleDocumentArticle, DocumentArticleRole);
COMPILE_ASSERT_MATCHING_ENUM(WebAccessibilityRoleDocumentMath, DocumentMathRole);
COMPILE_ASSERT_MATCHING_ENUM(WebAccessibilityRoleDocumentNote, DocumentNoteRole);
COMPILE_ASSERT_MATCHING_ENUM(WebAccessibilityRoleDocumentRegion, DocumentRegionRole);
COMPILE_ASSERT_MATCHING_ENUM(WebAccessibilityRoleUserInterfaceTooltip, UserInterfaceTooltipRole);
COMPILE_ASSERT_MATCHING_ENUM(WebAccessibilityRoleToggleButton, ToggleButtonRole);
COMPILE_ASSERT_MATCHING_ENUM(WebAccessibilityRoleCanvas, CanvasRole);
COMPILE_ASSERT_MATCHING_ENUM(WebAccessibilityRoleLegend, LegendRole);

COMPILE_ASSERT_MATCHING_ENUM(WebApplicationCacheHost::Uncached, ApplicationCacheHost::UNCACHED);
COMPILE_ASSERT_MATCHING_ENUM(WebApplicationCacheHost::Idle, ApplicationCacheHost::IDLE);
COMPILE_ASSERT_MATCHING_ENUM(WebApplicationCacheHost::Checking, ApplicationCacheHost::CHECKING);
COMPILE_ASSERT_MATCHING_ENUM(WebApplicationCacheHost::Downloading, ApplicationCacheHost::DOWNLOADING);
COMPILE_ASSERT_MATCHING_ENUM(WebApplicationCacheHost::UpdateReady, ApplicationCacheHost::UPDATEREADY);
COMPILE_ASSERT_MATCHING_ENUM(WebApplicationCacheHost::Obsolete, ApplicationCacheHost::OBSOLETE);
COMPILE_ASSERT_MATCHING_ENUM(WebApplicationCacheHost::CheckingEvent, ApplicationCacheHost::CHECKING_EVENT);
COMPILE_ASSERT_MATCHING_ENUM(WebApplicationCacheHost::ErrorEvent, ApplicationCacheHost::ERROR_EVENT);
COMPILE_ASSERT_MATCHING_ENUM(WebApplicationCacheHost::NoUpdateEvent, ApplicationCacheHost::NOUPDATE_EVENT);
COMPILE_ASSERT_MATCHING_ENUM(WebApplicationCacheHost::DownloadingEvent, ApplicationCacheHost::DOWNLOADING_EVENT);
COMPILE_ASSERT_MATCHING_ENUM(WebApplicationCacheHost::ProgressEvent, ApplicationCacheHost::PROGRESS_EVENT);
COMPILE_ASSERT_MATCHING_ENUM(WebApplicationCacheHost::UpdateReadyEvent, ApplicationCacheHost::UPDATEREADY_EVENT);
COMPILE_ASSERT_MATCHING_ENUM(WebApplicationCacheHost::CachedEvent, ApplicationCacheHost::CACHED_EVENT);
COMPILE_ASSERT_MATCHING_ENUM(WebApplicationCacheHost::ObsoleteEvent, ApplicationCacheHost::OBSOLETE_EVENT);

COMPILE_ASSERT_MATCHING_ENUM(WebCursorInfo::TypePointer, PlatformCursor::TypePointer);
COMPILE_ASSERT_MATCHING_ENUM(WebCursorInfo::TypeCross, PlatformCursor::TypeCross);
COMPILE_ASSERT_MATCHING_ENUM(WebCursorInfo::TypeHand, PlatformCursor::TypeHand);
COMPILE_ASSERT_MATCHING_ENUM(WebCursorInfo::TypeIBeam, PlatformCursor::TypeIBeam);
COMPILE_ASSERT_MATCHING_ENUM(WebCursorInfo::TypeWait, PlatformCursor::TypeWait);
COMPILE_ASSERT_MATCHING_ENUM(WebCursorInfo::TypeHelp, PlatformCursor::TypeHelp);
COMPILE_ASSERT_MATCHING_ENUM(WebCursorInfo::TypeEastResize, PlatformCursor::TypeEastResize);
COMPILE_ASSERT_MATCHING_ENUM(WebCursorInfo::TypeNorthResize, PlatformCursor::TypeNorthResize);
COMPILE_ASSERT_MATCHING_ENUM(WebCursorInfo::TypeNorthEastResize, PlatformCursor::TypeNorthEastResize);
COMPILE_ASSERT_MATCHING_ENUM(WebCursorInfo::TypeNorthWestResize, PlatformCursor::TypeNorthWestResize);
COMPILE_ASSERT_MATCHING_ENUM(WebCursorInfo::TypeSouthResize, PlatformCursor::TypeSouthResize);
COMPILE_ASSERT_MATCHING_ENUM(WebCursorInfo::TypeSouthEastResize, PlatformCursor::TypeSouthEastResize);
COMPILE_ASSERT_MATCHING_ENUM(WebCursorInfo::TypeSouthWestResize, PlatformCursor::TypeSouthWestResize);
COMPILE_ASSERT_MATCHING_ENUM(WebCursorInfo::TypeWestResize, PlatformCursor::TypeWestResize);
COMPILE_ASSERT_MATCHING_ENUM(WebCursorInfo::TypeNorthSouthResize, PlatformCursor::TypeNorthSouthResize);
COMPILE_ASSERT_MATCHING_ENUM(WebCursorInfo::TypeEastWestResize, PlatformCursor::TypeEastWestResize);
COMPILE_ASSERT_MATCHING_ENUM(WebCursorInfo::TypeNorthEastSouthWestResize, PlatformCursor::TypeNorthEastSouthWestResize);
COMPILE_ASSERT_MATCHING_ENUM(WebCursorInfo::TypeNorthWestSouthEastResize, PlatformCursor::TypeNorthWestSouthEastResize);
COMPILE_ASSERT_MATCHING_ENUM(WebCursorInfo::TypeColumnResize, PlatformCursor::TypeColumnResize);
COMPILE_ASSERT_MATCHING_ENUM(WebCursorInfo::TypeRowResize, PlatformCursor::TypeRowResize);
COMPILE_ASSERT_MATCHING_ENUM(WebCursorInfo::TypeMiddlePanning, PlatformCursor::TypeMiddlePanning);
COMPILE_ASSERT_MATCHING_ENUM(WebCursorInfo::TypeEastPanning, PlatformCursor::TypeEastPanning);
COMPILE_ASSERT_MATCHING_ENUM(WebCursorInfo::TypeNorthPanning, PlatformCursor::TypeNorthPanning);
COMPILE_ASSERT_MATCHING_ENUM(WebCursorInfo::TypeNorthEastPanning, PlatformCursor::TypeNorthEastPanning);
COMPILE_ASSERT_MATCHING_ENUM(WebCursorInfo::TypeNorthWestPanning, PlatformCursor::TypeNorthWestPanning);
COMPILE_ASSERT_MATCHING_ENUM(WebCursorInfo::TypeSouthPanning, PlatformCursor::TypeSouthPanning);
COMPILE_ASSERT_MATCHING_ENUM(WebCursorInfo::TypeSouthEastPanning, PlatformCursor::TypeSouthEastPanning);
COMPILE_ASSERT_MATCHING_ENUM(WebCursorInfo::TypeSouthWestPanning, PlatformCursor::TypeSouthWestPanning);
COMPILE_ASSERT_MATCHING_ENUM(WebCursorInfo::TypeWestPanning, PlatformCursor::TypeWestPanning);
COMPILE_ASSERT_MATCHING_ENUM(WebCursorInfo::TypeMove, PlatformCursor::TypeMove);
COMPILE_ASSERT_MATCHING_ENUM(WebCursorInfo::TypeVerticalText, PlatformCursor::TypeVerticalText);
COMPILE_ASSERT_MATCHING_ENUM(WebCursorInfo::TypeCell, PlatformCursor::TypeCell);
COMPILE_ASSERT_MATCHING_ENUM(WebCursorInfo::TypeContextMenu, PlatformCursor::TypeContextMenu);
COMPILE_ASSERT_MATCHING_ENUM(WebCursorInfo::TypeAlias, PlatformCursor::TypeAlias);
COMPILE_ASSERT_MATCHING_ENUM(WebCursorInfo::TypeProgress, PlatformCursor::TypeProgress);
COMPILE_ASSERT_MATCHING_ENUM(WebCursorInfo::TypeNoDrop, PlatformCursor::TypeNoDrop);
COMPILE_ASSERT_MATCHING_ENUM(WebCursorInfo::TypeCopy, PlatformCursor::TypeCopy);
COMPILE_ASSERT_MATCHING_ENUM(WebCursorInfo::TypeNone, PlatformCursor::TypeNone);
COMPILE_ASSERT_MATCHING_ENUM(WebCursorInfo::TypeNotAllowed, PlatformCursor::TypeNotAllowed);
COMPILE_ASSERT_MATCHING_ENUM(WebCursorInfo::TypeZoomIn, PlatformCursor::TypeZoomIn);
COMPILE_ASSERT_MATCHING_ENUM(WebCursorInfo::TypeZoomOut, PlatformCursor::TypeZoomOut);
COMPILE_ASSERT_MATCHING_ENUM(WebCursorInfo::TypeGrab, PlatformCursor::TypeGrab);
COMPILE_ASSERT_MATCHING_ENUM(WebCursorInfo::TypeGrabbing, PlatformCursor::TypeGrabbing);
COMPILE_ASSERT_MATCHING_ENUM(WebCursorInfo::TypeCustom, PlatformCursor::TypeCustom);

COMPILE_ASSERT_MATCHING_ENUM(WebEditingActionTyped, EditorInsertActionTyped);
COMPILE_ASSERT_MATCHING_ENUM(WebEditingActionPasted, EditorInsertActionPasted);
COMPILE_ASSERT_MATCHING_ENUM(WebEditingActionDropped, EditorInsertActionDropped);

COMPILE_ASSERT_MATCHING_ENUM(WebFontDescription::GenericFamilyNone, FontDescription::NoFamily);
COMPILE_ASSERT_MATCHING_ENUM(WebFontDescription::GenericFamilyStandard, FontDescription::StandardFamily);
COMPILE_ASSERT_MATCHING_ENUM(WebFontDescription::GenericFamilySerif, FontDescription::SerifFamily);
COMPILE_ASSERT_MATCHING_ENUM(WebFontDescription::GenericFamilySansSerif, FontDescription::SansSerifFamily);
COMPILE_ASSERT_MATCHING_ENUM(WebFontDescription::GenericFamilyMonospace, FontDescription::MonospaceFamily);
COMPILE_ASSERT_MATCHING_ENUM(WebFontDescription::GenericFamilyCursive, FontDescription::CursiveFamily);
COMPILE_ASSERT_MATCHING_ENUM(WebFontDescription::GenericFamilyFantasy, FontDescription::FantasyFamily);

COMPILE_ASSERT_MATCHING_ENUM(WebFontDescription::SmoothingAuto, AutoSmoothing);
COMPILE_ASSERT_MATCHING_ENUM(WebFontDescription::SmoothingNone, NoSmoothing);
COMPILE_ASSERT_MATCHING_ENUM(WebFontDescription::SmoothingGrayscale, Antialiased);
COMPILE_ASSERT_MATCHING_ENUM(WebFontDescription::SmoothingSubpixel, SubpixelAntialiased);

COMPILE_ASSERT_MATCHING_ENUM(WebFontDescription::Weight100, FontWeight100);
COMPILE_ASSERT_MATCHING_ENUM(WebFontDescription::Weight200, FontWeight200);
COMPILE_ASSERT_MATCHING_ENUM(WebFontDescription::Weight300, FontWeight300);
COMPILE_ASSERT_MATCHING_ENUM(WebFontDescription::Weight400, FontWeight400);
COMPILE_ASSERT_MATCHING_ENUM(WebFontDescription::Weight500, FontWeight500);
COMPILE_ASSERT_MATCHING_ENUM(WebFontDescription::Weight600, FontWeight600);
COMPILE_ASSERT_MATCHING_ENUM(WebFontDescription::Weight700, FontWeight700);
COMPILE_ASSERT_MATCHING_ENUM(WebFontDescription::Weight800, FontWeight800);
COMPILE_ASSERT_MATCHING_ENUM(WebFontDescription::Weight900, FontWeight900);
COMPILE_ASSERT_MATCHING_ENUM(WebFontDescription::WeightNormal, FontWeightNormal);
COMPILE_ASSERT_MATCHING_ENUM(WebFontDescription::WeightBold, FontWeightBold);

COMPILE_ASSERT_MATCHING_ENUM(WebIconURL::TypeInvalid, InvalidIcon);
COMPILE_ASSERT_MATCHING_ENUM(WebIconURL::TypeFavicon, Favicon);
COMPILE_ASSERT_MATCHING_ENUM(WebIconURL::TypeTouch, TouchIcon);
COMPILE_ASSERT_MATCHING_ENUM(WebIconURL::TypeTouchPrecomposed, TouchPrecomposedIcon);

#if ENABLE(INPUT_SPEECH)
COMPILE_ASSERT_MATCHING_ENUM(WebInputElement::Idle, InputFieldSpeechButtonElement::Idle);
COMPILE_ASSERT_MATCHING_ENUM(WebInputElement::Recording, InputFieldSpeechButtonElement::Recording);
COMPILE_ASSERT_MATCHING_ENUM(WebInputElement::Recognizing, InputFieldSpeechButtonElement::Recognizing);
#endif

COMPILE_ASSERT_MATCHING_ENUM(WebNode::ElementNode, Node::ELEMENT_NODE);
COMPILE_ASSERT_MATCHING_ENUM(WebNode::AttributeNode, Node::ATTRIBUTE_NODE);
COMPILE_ASSERT_MATCHING_ENUM(WebNode::TextNode, Node::TEXT_NODE);
COMPILE_ASSERT_MATCHING_ENUM(WebNode::CDataSectionNode, Node::CDATA_SECTION_NODE);
COMPILE_ASSERT_MATCHING_ENUM(WebNode::EntityReferenceNode, Node::ENTITY_REFERENCE_NODE);
COMPILE_ASSERT_MATCHING_ENUM(WebNode::EntityNode, Node::ENTITY_NODE);
COMPILE_ASSERT_MATCHING_ENUM(WebNode::ProcessingInstructionsNode, Node::PROCESSING_INSTRUCTION_NODE);
COMPILE_ASSERT_MATCHING_ENUM(WebNode::CommentNode, Node::COMMENT_NODE);
COMPILE_ASSERT_MATCHING_ENUM(WebNode::DocumentNode, Node::DOCUMENT_NODE);
COMPILE_ASSERT_MATCHING_ENUM(WebNode::DocumentTypeNode, Node::DOCUMENT_TYPE_NODE);
COMPILE_ASSERT_MATCHING_ENUM(WebNode::DocumentFragmentNode, Node::DOCUMENT_FRAGMENT_NODE);
COMPILE_ASSERT_MATCHING_ENUM(WebNode::NotationNode, Node::NOTATION_NODE);
COMPILE_ASSERT_MATCHING_ENUM(WebNode::XPathNamespaceNode, Node::XPATH_NAMESPACE_NODE);

#if ENABLE(VIDEO)
COMPILE_ASSERT_MATCHING_ENUM(WebMediaPlayer::NetworkStateEmpty, MediaPlayer::Empty);
COMPILE_ASSERT_MATCHING_ENUM(WebMediaPlayer::NetworkStateIdle, MediaPlayer::Idle);
COMPILE_ASSERT_MATCHING_ENUM(WebMediaPlayer::NetworkStateLoading, MediaPlayer::Loading);
COMPILE_ASSERT_MATCHING_ENUM(WebMediaPlayer::NetworkStateLoaded, MediaPlayer::Loaded);
COMPILE_ASSERT_MATCHING_ENUM(WebMediaPlayer::NetworkStateFormatError, MediaPlayer::FormatError);
COMPILE_ASSERT_MATCHING_ENUM(WebMediaPlayer::NetworkStateNetworkError, MediaPlayer::NetworkError);
COMPILE_ASSERT_MATCHING_ENUM(WebMediaPlayer::NetworkStateDecodeError, MediaPlayer::DecodeError);

COMPILE_ASSERT_MATCHING_ENUM(WebMediaPlayer::ReadyStateHaveNothing, MediaPlayer::HaveNothing);
COMPILE_ASSERT_MATCHING_ENUM(WebMediaPlayer::ReadyStateHaveMetadata, MediaPlayer::HaveMetadata);
COMPILE_ASSERT_MATCHING_ENUM(WebMediaPlayer::ReadyStateHaveCurrentData, MediaPlayer::HaveCurrentData);
COMPILE_ASSERT_MATCHING_ENUM(WebMediaPlayer::ReadyStateHaveFutureData, MediaPlayer::HaveFutureData);
COMPILE_ASSERT_MATCHING_ENUM(WebMediaPlayer::ReadyStateHaveEnoughData, MediaPlayer::HaveEnoughData);

COMPILE_ASSERT_MATCHING_ENUM(WebMediaPlayer::MovieLoadTypeUnknown, MediaPlayer::Unknown);
COMPILE_ASSERT_MATCHING_ENUM(WebMediaPlayer::MovieLoadTypeDownload, MediaPlayer::Download);
COMPILE_ASSERT_MATCHING_ENUM(WebMediaPlayer::MovieLoadTypeStoredStream, MediaPlayer::StoredStream);
COMPILE_ASSERT_MATCHING_ENUM(WebMediaPlayer::MovieLoadTypeLiveStream, MediaPlayer::LiveStream);

COMPILE_ASSERT_MATCHING_ENUM(WebMediaPlayer::PreloadNone, MediaPlayer::None);
COMPILE_ASSERT_MATCHING_ENUM(WebMediaPlayer::PreloadMetaData, MediaPlayer::MetaData);
COMPILE_ASSERT_MATCHING_ENUM(WebMediaPlayer::PreloadAuto, MediaPlayer::Auto);

#if ENABLE(MEDIA_SOURCE)
COMPILE_ASSERT_MATCHING_ENUM(WebMediaPlayer::AddIdStatusOk, MediaPlayer::Ok);
COMPILE_ASSERT_MATCHING_ENUM(WebMediaPlayer::AddIdStatusNotSupported, MediaPlayer::NotSupported);
COMPILE_ASSERT_MATCHING_ENUM(WebMediaPlayer::AddIdStatusReachedIdLimit, MediaPlayer::ReachedIdLimit);
#endif

#if ENABLE(ENCRYPTED_MEDIA)
COMPILE_ASSERT_MATCHING_ENUM(WebMediaPlayer::EndOfStreamStatusNoError, MediaPlayer::EosNoError);
COMPILE_ASSERT_MATCHING_ENUM(WebMediaPlayer::EndOfStreamStatusNetworkError, MediaPlayer::EosNetworkError);
COMPILE_ASSERT_MATCHING_ENUM(WebMediaPlayer::EndOfStreamStatusDecodeError, MediaPlayer::EosDecodeError);
#endif

#if ENABLE(ENCRYPTED_MEDIA)
COMPILE_ASSERT_MATCHING_ENUM(WebMediaPlayer::MediaKeyExceptionNoError, MediaPlayer::NoError);
COMPILE_ASSERT_MATCHING_ENUM(WebMediaPlayer::MediaKeyExceptionInvalidPlayerState, MediaPlayer::InvalidPlayerState);
COMPILE_ASSERT_MATCHING_ENUM(WebMediaPlayer::MediaKeyExceptionKeySystemNotSupported, MediaPlayer::KeySystemNotSupported);

COMPILE_ASSERT_MATCHING_ENUM(WebMediaPlayerClient::MediaKeyErrorCodeUnknown, MediaPlayerClient::UnknownError);
COMPILE_ASSERT_MATCHING_ENUM(WebMediaPlayerClient::MediaKeyErrorCodeClient, MediaPlayerClient::ClientError);
COMPILE_ASSERT_MATCHING_ENUM(WebMediaPlayerClient::MediaKeyErrorCodeService, MediaPlayerClient::ServiceError);
COMPILE_ASSERT_MATCHING_ENUM(WebMediaPlayerClient::MediaKeyErrorCodeOutput, MediaPlayerClient::OutputError);
COMPILE_ASSERT_MATCHING_ENUM(WebMediaPlayerClient::MediaKeyErrorCodeHardwareChange, MediaPlayerClient::HardwareChangeError);
COMPILE_ASSERT_MATCHING_ENUM(WebMediaPlayerClient::MediaKeyErrorCodeDomain, MediaPlayerClient::DomainError);
#endif

#endif

#if ENABLE(NOTIFICATIONS) || ENABLE(LEGACY_NOTIFICATIONS)
COMPILE_ASSERT_MATCHING_ENUM(WebNotificationPresenter::PermissionAllowed, NotificationClient::PermissionAllowed);
COMPILE_ASSERT_MATCHING_ENUM(WebNotificationPresenter::PermissionNotAllowed, NotificationClient::PermissionNotAllowed);
COMPILE_ASSERT_MATCHING_ENUM(WebNotificationPresenter::PermissionDenied, NotificationClient::PermissionDenied);
#endif

COMPILE_ASSERT_MATCHING_ENUM(WebScrollbar::Horizontal, HorizontalScrollbar);
COMPILE_ASSERT_MATCHING_ENUM(WebScrollbar::Vertical, VerticalScrollbar);

COMPILE_ASSERT_MATCHING_ENUM(WebScrollbar::ScrollByLine, ScrollByLine);
COMPILE_ASSERT_MATCHING_ENUM(WebScrollbar::ScrollByPage, ScrollByPage);
COMPILE_ASSERT_MATCHING_ENUM(WebScrollbar::ScrollByDocument, ScrollByDocument);
COMPILE_ASSERT_MATCHING_ENUM(WebScrollbar::ScrollByPixel, ScrollByPixel);

COMPILE_ASSERT_MATCHING_ENUM(WebScrollbar::RegularScrollbar, RegularScrollbar);
COMPILE_ASSERT_MATCHING_ENUM(WebScrollbar::SmallScrollbar, SmallScrollbar);
COMPILE_ASSERT_MATCHING_ENUM(WebScrollbar::NoPart, NoPart);
COMPILE_ASSERT_MATCHING_ENUM(WebScrollbar::BackButtonStartPart, BackButtonStartPart);
COMPILE_ASSERT_MATCHING_ENUM(WebScrollbar::ForwardButtonStartPart, ForwardButtonStartPart);
COMPILE_ASSERT_MATCHING_ENUM(WebScrollbar::BackTrackPart, BackTrackPart);
COMPILE_ASSERT_MATCHING_ENUM(WebScrollbar::ThumbPart, ThumbPart);
COMPILE_ASSERT_MATCHING_ENUM(WebScrollbar::ForwardTrackPart, ForwardTrackPart);
COMPILE_ASSERT_MATCHING_ENUM(WebScrollbar::BackButtonEndPart, BackButtonEndPart);
COMPILE_ASSERT_MATCHING_ENUM(WebScrollbar::ForwardButtonEndPart, ForwardButtonEndPart);
COMPILE_ASSERT_MATCHING_ENUM(WebScrollbar::ScrollbarBGPart, ScrollbarBGPart);
COMPILE_ASSERT_MATCHING_ENUM(WebScrollbar::TrackBGPart, TrackBGPart);
COMPILE_ASSERT_MATCHING_ENUM(WebScrollbar::AllParts, AllParts);
COMPILE_ASSERT_MATCHING_ENUM(WebScrollbar::ScrollbarOverlayStyleDefault, ScrollbarOverlayStyleDefault);
COMPILE_ASSERT_MATCHING_ENUM(WebScrollbar::ScrollbarOverlayStyleDark, ScrollbarOverlayStyleDark);
COMPILE_ASSERT_MATCHING_ENUM(WebScrollbar::ScrollbarOverlayStyleLight, ScrollbarOverlayStyleLight);

COMPILE_ASSERT_MATCHING_ENUM(WebSettings::EditingBehaviorMac, EditingMacBehavior);
COMPILE_ASSERT_MATCHING_ENUM(WebSettings::EditingBehaviorWin, EditingWindowsBehavior);
COMPILE_ASSERT_MATCHING_ENUM(WebSettings::EditingBehaviorUnix, EditingUnixBehavior);

COMPILE_ASSERT_MATCHING_ENUM(WebTextAffinityUpstream, UPSTREAM);
COMPILE_ASSERT_MATCHING_ENUM(WebTextAffinityDownstream, DOWNSTREAM);

COMPILE_ASSERT_MATCHING_ENUM(WebTextCaseSensitive, TextCaseSensitive);
COMPILE_ASSERT_MATCHING_ENUM(WebTextCaseInsensitive, TextCaseInsensitive);

COMPILE_ASSERT_MATCHING_ENUM(WebView::UserScriptInjectAtDocumentStart, InjectAtDocumentStart);
COMPILE_ASSERT_MATCHING_ENUM(WebView::UserScriptInjectAtDocumentEnd, InjectAtDocumentEnd);
COMPILE_ASSERT_MATCHING_ENUM(WebView::UserContentInjectInAllFrames, InjectInAllFrames);
COMPILE_ASSERT_MATCHING_ENUM(WebView::UserContentInjectInTopFrameOnly, InjectInTopFrameOnly);
COMPILE_ASSERT_MATCHING_ENUM(WebView::UserStyleInjectInExistingDocuments, InjectInExistingDocuments);
COMPILE_ASSERT_MATCHING_ENUM(WebView::UserStyleInjectInSubsequentDocuments, InjectInSubsequentDocuments);

COMPILE_ASSERT_MATCHING_ENUM(WebIDBDatabaseExceptionDataError, IDBDatabaseException::DATA_ERR);
COMPILE_ASSERT_MATCHING_ENUM(WebIDBDatabaseExceptionQuotaError, IDBDatabaseException::IDB_QUOTA_EXCEEDED_ERR);

#if ENABLE(INDEXED_DATABASE)
COMPILE_ASSERT_MATCHING_ENUM(WebIDBKey::InvalidType, IDBKey::InvalidType);
COMPILE_ASSERT_MATCHING_ENUM(WebIDBKey::ArrayType, IDBKey::ArrayType);
COMPILE_ASSERT_MATCHING_ENUM(WebIDBKey::StringType, IDBKey::StringType);
COMPILE_ASSERT_MATCHING_ENUM(WebIDBKey::DateType, IDBKey::DateType);
COMPILE_ASSERT_MATCHING_ENUM(WebIDBKey::NumberType, IDBKey::NumberType);

COMPILE_ASSERT_MATCHING_ENUM(WebIDBKeyPath::NullType, IDBKeyPath::NullType);
COMPILE_ASSERT_MATCHING_ENUM(WebIDBKeyPath::StringType, IDBKeyPath::StringType);
COMPILE_ASSERT_MATCHING_ENUM(WebIDBKeyPath::ArrayType, IDBKeyPath::ArrayType);

COMPILE_ASSERT_MATCHING_ENUM(WebIDBMetadata::NoIntVersion, IDBDatabaseMetadata::NoIntVersion);

COMPILE_ASSERT_MATCHING_ENUM(WebIDBCursor::Next, IDBCursor::NEXT);
COMPILE_ASSERT_MATCHING_ENUM(WebIDBCursor::NextNoDuplicate, IDBCursor::NEXT_NO_DUPLICATE);
COMPILE_ASSERT_MATCHING_ENUM(WebIDBCursor::Prev, IDBCursor::PREV);
COMPILE_ASSERT_MATCHING_ENUM(WebIDBCursor::PrevNoDuplicate, IDBCursor::PREV_NO_DUPLICATE);

COMPILE_ASSERT_MATCHING_ENUM(WebIDBTransaction::PreemptiveTask, IDBTransactionBackendInterface::PreemptiveTask);
COMPILE_ASSERT_MATCHING_ENUM(WebIDBTransaction::NormalTask, IDBTransactionBackendInterface::NormalTask);
#endif

#if ENABLE(FILE_SYSTEM)
COMPILE_ASSERT_MATCHING_ENUM(WebFileSystem::TypeTemporary, FileSystemTypeTemporary);
COMPILE_ASSERT_MATCHING_ENUM(WebFileSystem::TypePersistent, FileSystemTypePersistent);
COMPILE_ASSERT_MATCHING_ENUM(WebFileSystem::TypeExternal, FileSystemTypeExternal);
COMPILE_ASSERT_MATCHING_ENUM(WebFileSystem::TypeIsolated, FileSystemTypeIsolated);
COMPILE_ASSERT_MATCHING_ENUM(WebFileInfo::TypeUnknown, FileMetadata::TypeUnknown);
COMPILE_ASSERT_MATCHING_ENUM(WebFileInfo::TypeFile, FileMetadata::TypeFile);
COMPILE_ASSERT_MATCHING_ENUM(WebFileInfo::TypeDirectory, FileMetadata::TypeDirectory);
#endif

COMPILE_ASSERT_MATCHING_ENUM(WebFileErrorNotFound, FileError::NOT_FOUND_ERR);
COMPILE_ASSERT_MATCHING_ENUM(WebFileErrorSecurity, FileError::SECURITY_ERR);
COMPILE_ASSERT_MATCHING_ENUM(WebFileErrorAbort, FileError::ABORT_ERR);
COMPILE_ASSERT_MATCHING_ENUM(WebFileErrorNotReadable, FileError::NOT_READABLE_ERR);
COMPILE_ASSERT_MATCHING_ENUM(WebFileErrorEncoding, FileError::ENCODING_ERR);
COMPILE_ASSERT_MATCHING_ENUM(WebFileErrorNoModificationAllowed, FileError::NO_MODIFICATION_ALLOWED_ERR);
COMPILE_ASSERT_MATCHING_ENUM(WebFileErrorInvalidState, FileError::INVALID_STATE_ERR);
COMPILE_ASSERT_MATCHING_ENUM(WebFileErrorSyntax, FileError::SYNTAX_ERR);
COMPILE_ASSERT_MATCHING_ENUM(WebFileErrorInvalidModification, FileError::INVALID_MODIFICATION_ERR);
COMPILE_ASSERT_MATCHING_ENUM(WebFileErrorQuotaExceeded, FileError::QUOTA_EXCEEDED_ERR);
COMPILE_ASSERT_MATCHING_ENUM(WebFileErrorTypeMismatch, FileError::TYPE_MISMATCH_ERR);
COMPILE_ASSERT_MATCHING_ENUM(WebFileErrorPathExists, FileError::PATH_EXISTS_ERR);

COMPILE_ASSERT_MATCHING_ENUM(WebGeolocationError::ErrorPermissionDenied, GeolocationError::PermissionDenied);
COMPILE_ASSERT_MATCHING_ENUM(WebGeolocationError::ErrorPositionUnavailable, GeolocationError::PositionUnavailable);

COMPILE_ASSERT_MATCHING_ENUM(WebTextCheckingTypeSpelling, TextCheckingTypeSpelling);
COMPILE_ASSERT_MATCHING_ENUM(WebTextCheckingTypeGrammar, TextCheckingTypeGrammar);
COMPILE_ASSERT_MATCHING_ENUM(WebTextCheckingTypeLink, TextCheckingTypeLink);
COMPILE_ASSERT_MATCHING_ENUM(WebTextCheckingTypeQuote, TextCheckingTypeQuote);
COMPILE_ASSERT_MATCHING_ENUM(WebTextCheckingTypeDash, TextCheckingTypeDash);
COMPILE_ASSERT_MATCHING_ENUM(WebTextCheckingTypeReplacement, TextCheckingTypeReplacement);
COMPILE_ASSERT_MATCHING_ENUM(WebTextCheckingTypeCorrection, TextCheckingTypeCorrection);
COMPILE_ASSERT_MATCHING_ENUM(WebTextCheckingTypeShowCorrectionPanel, TextCheckingTypeShowCorrectionPanel);

#if ENABLE(QUOTA)
COMPILE_ASSERT_MATCHING_ENUM(WebStorageQuotaErrorNotSupported, NOT_SUPPORTED_ERR);
COMPILE_ASSERT_MATCHING_ENUM(WebStorageQuotaErrorAbort, ABORT_ERR);

COMPILE_ASSERT_MATCHING_ENUM(WebStorageQuotaTypeTemporary, StorageInfo::TEMPORARY);
COMPILE_ASSERT_MATCHING_ENUM(WebStorageQuotaTypePersistent, StorageInfo::PERSISTENT);

COMPILE_ASSERT_MATCHING_ENUM(WebStorageQuotaErrorNotSupported, NOT_SUPPORTED_ERR);
COMPILE_ASSERT_MATCHING_ENUM(WebStorageQuotaErrorAbort, ABORT_ERR);
#endif

#if OS(DARWIN)
COMPILE_ASSERT_MATCHING_ENUM(WebThemeEngine::StateDisabled, PlatformSupport::StateDisabled);
COMPILE_ASSERT_MATCHING_ENUM(WebThemeEngine::StateInactive, PlatformSupport::StateInactive);
COMPILE_ASSERT_MATCHING_ENUM(WebThemeEngine::StateActive, PlatformSupport::StateActive);
COMPILE_ASSERT_MATCHING_ENUM(WebThemeEngine::StatePressed, PlatformSupport::StatePressed);

COMPILE_ASSERT_MATCHING_ENUM(WebThemeEngine::SizeRegular, PlatformSupport::SizeRegular);
COMPILE_ASSERT_MATCHING_ENUM(WebThemeEngine::SizeSmall, PlatformSupport::SizeSmall);

COMPILE_ASSERT_MATCHING_ENUM(WebThemeEngine::ScrollbarOrientationHorizontal, PlatformSupport::ScrollbarOrientationHorizontal);
COMPILE_ASSERT_MATCHING_ENUM(WebThemeEngine::ScrollbarOrientationVertical, PlatformSupport::ScrollbarOrientationVertical);

COMPILE_ASSERT_MATCHING_ENUM(WebThemeEngine::ScrollbarParentScrollView, PlatformSupport::ScrollbarParentScrollView);
COMPILE_ASSERT_MATCHING_ENUM(WebThemeEngine::ScrollbarParentRenderLayer, PlatformSupport::ScrollbarParentRenderLayer);
#endif

COMPILE_ASSERT_MATCHING_ENUM(WebPageVisibilityStateVisible, PageVisibilityStateVisible);
COMPILE_ASSERT_MATCHING_ENUM(WebPageVisibilityStateHidden, PageVisibilityStateHidden);
COMPILE_ASSERT_MATCHING_ENUM(WebPageVisibilityStatePrerender, PageVisibilityStatePrerender);
COMPILE_ASSERT_MATCHING_ENUM(WebPageVisibilityStatePreview, PageVisibilityStatePreview);

#if ENABLE(MEDIA_STREAM)
COMPILE_ASSERT_MATCHING_ENUM(WebMediaStreamSource::TypeAudio, MediaStreamSource::TypeAudio);
COMPILE_ASSERT_MATCHING_ENUM(WebMediaStreamSource::TypeVideo, MediaStreamSource::TypeVideo);
COMPILE_ASSERT_MATCHING_ENUM(WebMediaStreamSource::ReadyStateLive, MediaStreamSource::ReadyStateLive);
COMPILE_ASSERT_MATCHING_ENUM(WebMediaStreamSource::ReadyStateMuted, MediaStreamSource::ReadyStateMuted);
COMPILE_ASSERT_MATCHING_ENUM(WebMediaStreamSource::ReadyStateEnded, MediaStreamSource::ReadyStateEnded);

COMPILE_ASSERT_MATCHING_ENUM(WebICEOptions::CandidateTypeAll, IceOptions::ALL);
COMPILE_ASSERT_MATCHING_ENUM(WebICEOptions::CandidateTypeNoRelay, IceOptions::NO_RELAY);
COMPILE_ASSERT_MATCHING_ENUM(WebICEOptions::CandidateTypeOnlyRelay, IceOptions::ONLY_RELAY);

COMPILE_ASSERT_MATCHING_ENUM(WebPeerConnection00Handler::ActionSDPOffer, PeerConnection00::SDP_OFFER);
COMPILE_ASSERT_MATCHING_ENUM(WebPeerConnection00Handler::ActionSDPPRanswer, PeerConnection00::SDP_PRANSWER);
COMPILE_ASSERT_MATCHING_ENUM(WebPeerConnection00Handler::ActionSDPAnswer, PeerConnection00::SDP_ANSWER);

COMPILE_ASSERT_MATCHING_ENUM(WebPeerConnection00HandlerClient::ReadyStateNew, PeerConnection00::NEW);
COMPILE_ASSERT_MATCHING_ENUM(WebPeerConnection00HandlerClient::ReadyStateOpening, PeerConnection00::OPENING);
COMPILE_ASSERT_MATCHING_ENUM(WebPeerConnection00HandlerClient::ReadyStateNegotiating, PeerConnection00::OPENING);
COMPILE_ASSERT_MATCHING_ENUM(WebPeerConnection00HandlerClient::ReadyStateActive, PeerConnection00::ACTIVE);
COMPILE_ASSERT_MATCHING_ENUM(WebPeerConnection00HandlerClient::ReadyStateClosed, PeerConnection00::CLOSED);

COMPILE_ASSERT_MATCHING_ENUM(WebPeerConnection00HandlerClient::ICEStateGathering, PeerConnection00::ICE_GATHERING);
COMPILE_ASSERT_MATCHING_ENUM(WebPeerConnection00HandlerClient::ICEStateWaiting, PeerConnection00::ICE_WAITING);
COMPILE_ASSERT_MATCHING_ENUM(WebPeerConnection00HandlerClient::ICEStateChecking, PeerConnection00::ICE_CHECKING);
COMPILE_ASSERT_MATCHING_ENUM(WebPeerConnection00HandlerClient::ICEStateConnected, PeerConnection00::ICE_CONNECTED);
COMPILE_ASSERT_MATCHING_ENUM(WebPeerConnection00HandlerClient::ICEStateCompleted, PeerConnection00::ICE_COMPLETED);
COMPILE_ASSERT_MATCHING_ENUM(WebPeerConnection00HandlerClient::ICEStateFailed, PeerConnection00::ICE_FAILED);
COMPILE_ASSERT_MATCHING_ENUM(WebPeerConnection00HandlerClient::ICEStateClosed, PeerConnection00::ICE_CLOSED);

COMPILE_ASSERT_MATCHING_ENUM(WebRTCPeerConnectionHandlerClient::ReadyStateNew, RTCPeerConnectionHandlerClient::ReadyStateNew);
COMPILE_ASSERT_MATCHING_ENUM(WebRTCPeerConnectionHandlerClient::ReadyStateOpening, RTCPeerConnectionHandlerClient::ReadyStateOpening);
COMPILE_ASSERT_MATCHING_ENUM(WebRTCPeerConnectionHandlerClient::ReadyStateActive, RTCPeerConnectionHandlerClient::ReadyStateActive);
COMPILE_ASSERT_MATCHING_ENUM(WebRTCPeerConnectionHandlerClient::ReadyStateClosing, RTCPeerConnectionHandlerClient::ReadyStateClosing);
COMPILE_ASSERT_MATCHING_ENUM(WebRTCPeerConnectionHandlerClient::ReadyStateClosed, RTCPeerConnectionHandlerClient::ReadyStateClosed);

COMPILE_ASSERT_MATCHING_ENUM(WebRTCPeerConnectionHandlerClient::ICEStateNew, RTCPeerConnectionHandlerClient::IceStateNew);
COMPILE_ASSERT_MATCHING_ENUM(WebRTCPeerConnectionHandlerClient::ICEStateGathering, RTCPeerConnectionHandlerClient::IceStateGathering);
COMPILE_ASSERT_MATCHING_ENUM(WebRTCPeerConnectionHandlerClient::ICEStateWaiting, RTCPeerConnectionHandlerClient::IceStateWaiting);
COMPILE_ASSERT_MATCHING_ENUM(WebRTCPeerConnectionHandlerClient::ICEStateChecking, RTCPeerConnectionHandlerClient::IceStateChecking);
COMPILE_ASSERT_MATCHING_ENUM(WebRTCPeerConnectionHandlerClient::ICEStateConnected, RTCPeerConnectionHandlerClient::IceStateConnected);
COMPILE_ASSERT_MATCHING_ENUM(WebRTCPeerConnectionHandlerClient::ICEStateCompleted, RTCPeerConnectionHandlerClient::IceStateCompleted);
COMPILE_ASSERT_MATCHING_ENUM(WebRTCPeerConnectionHandlerClient::ICEStateFailed, RTCPeerConnectionHandlerClient::IceStateFailed);
COMPILE_ASSERT_MATCHING_ENUM(WebRTCPeerConnectionHandlerClient::ICEStateClosed, RTCPeerConnectionHandlerClient::IceStateClosed);
#endif

#if ENABLE(SCRIPTED_SPEECH)
COMPILE_ASSERT_MATCHING_ENUM(WebSpeechRecognizerClient::OtherError, SpeechRecognitionError::OTHER);
COMPILE_ASSERT_MATCHING_ENUM(WebSpeechRecognizerClient::NoSpeechError, SpeechRecognitionError::NO_SPEECH);
COMPILE_ASSERT_MATCHING_ENUM(WebSpeechRecognizerClient::AbortedError, SpeechRecognitionError::ABORTED);
COMPILE_ASSERT_MATCHING_ENUM(WebSpeechRecognizerClient::AudioCaptureError, SpeechRecognitionError::AUDIO_CAPTURE);
COMPILE_ASSERT_MATCHING_ENUM(WebSpeechRecognizerClient::NetworkError, SpeechRecognitionError::NETWORK);
COMPILE_ASSERT_MATCHING_ENUM(WebSpeechRecognizerClient::NotAllowedError, SpeechRecognitionError::NOT_ALLOWED);
COMPILE_ASSERT_MATCHING_ENUM(WebSpeechRecognizerClient::ServiceNotAllowedError, SpeechRecognitionError::SERVICE_NOT_ALLOWED);
COMPILE_ASSERT_MATCHING_ENUM(WebSpeechRecognizerClient::BadGrammarError, SpeechRecognitionError::BAD_GRAMMAR);
COMPILE_ASSERT_MATCHING_ENUM(WebSpeechRecognizerClient::LanguageNotSupportedError, SpeechRecognitionError::LANGUAGE_NOT_SUPPORTED);
#endif

COMPILE_ASSERT_MATCHING_ENUM(WebReferrerPolicyAlways, ReferrerPolicyAlways);
COMPILE_ASSERT_MATCHING_ENUM(WebReferrerPolicyDefault, ReferrerPolicyDefault);
COMPILE_ASSERT_MATCHING_ENUM(WebReferrerPolicyNever, ReferrerPolicyNever);
COMPILE_ASSERT_MATCHING_ENUM(WebReferrerPolicyOrigin, ReferrerPolicyOrigin);

COMPILE_ASSERT_MATCHING_ENUM(WebContentSecurityPolicyTypeReportOnly, ContentSecurityPolicy::ReportOnly);
COMPILE_ASSERT_MATCHING_ENUM(WebContentSecurityPolicyTypeEnforcePolicy, ContentSecurityPolicy::EnforcePolicy);

COMPILE_ASSERT_MATCHING_ENUM(WebURLResponse::Unknown, ResourceResponse::Unknown);
COMPILE_ASSERT_MATCHING_ENUM(WebURLResponse::HTTP_0_9, ResourceResponse::HTTP_0_9);
COMPILE_ASSERT_MATCHING_ENUM(WebURLResponse::HTTP_1_0, ResourceResponse::HTTP_1_0);
COMPILE_ASSERT_MATCHING_ENUM(WebURLResponse::HTTP_1_1, ResourceResponse::HTTP_1_1);

COMPILE_ASSERT_MATCHING_ENUM(WebMediaPlayer::CORSModeUnspecified, MediaPlayerClient::Unspecified);
COMPILE_ASSERT_MATCHING_ENUM(WebMediaPlayer::CORSModeAnonymous, MediaPlayerClient::Anonymous);
COMPILE_ASSERT_MATCHING_ENUM(WebMediaPlayer::CORSModeUseCredentials, MediaPlayerClient::UseCredentials);
