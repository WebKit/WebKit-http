/*
 * Copyright (C) 2008, 2009, 2010 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#if HAVE(ACCESSIBILITY)

#include "AXObjectCache.h"

#include "AccessibilityARIAGrid.h"
#include "AccessibilityARIAGridCell.h"
#include "AccessibilityARIAGridRow.h"
#include "AccessibilityImageMapLink.h"
#include "AccessibilityList.h"
#include "AccessibilityListBox.h"
#include "AccessibilityListBoxOption.h"
#include "AccessibilityMediaControls.h"
#include "AccessibilityMenuList.h"
#include "AccessibilityMenuListOption.h"
#include "AccessibilityMenuListPopup.h"
#include "AccessibilityProgressIndicator.h"
#include "AccessibilityRenderObject.h"
#include "AccessibilitySVGRoot.h"
#include "AccessibilityScrollView.h"
#include "AccessibilityScrollbar.h"
#include "AccessibilitySearchFieldButtons.h"
#include "AccessibilitySlider.h"
#include "AccessibilitySpinButton.h"
#include "AccessibilityTable.h"
#include "AccessibilityTableCell.h"
#include "AccessibilityTableColumn.h"
#include "AccessibilityTableHeaderContainer.h"
#include "AccessibilityTableRow.h"
#include "Document.h"
#include "Editor.h"
#include "FocusController.h"
#include "Frame.h"
#include "HTMLAreaElement.h"
#include "HTMLImageElement.h"
#include "HTMLInputElement.h"
#include "HTMLLabelElement.h"
#include "HTMLMeterElement.h"
#include "HTMLNames.h"
#include "Page.h"
#include "RenderListBox.h"
#include "RenderMenuList.h"
#include "RenderMeter.h"
#include "RenderProgress.h"
#include "RenderSlider.h"
#include "RenderTable.h"
#include "RenderTableCell.h"
#include "RenderTableRow.h"
#include "RenderView.h"
#include "ScrollView.h"
#include <wtf/PassRefPtr.h>

#if ENABLE(VIDEO)
#include "MediaControlElements.h"
#endif

namespace WebCore {

using namespace HTMLNames;

AccessibilityObjectInclusion AXComputedObjectAttributeCache::getIgnored(AXID id) const
{
    HashMap<AXID, CachedAXObjectAttributes>::const_iterator it = m_idMapping.find(id);
    return it != m_idMapping.end() ? it->value.ignored : DefaultBehavior;
}

void AXComputedObjectAttributeCache::setIgnored(AXID id, AccessibilityObjectInclusion inclusion)
{
    HashMap<AXID, CachedAXObjectAttributes>::iterator it = m_idMapping.find(id);
    if (it != m_idMapping.end())
        it->value.ignored = inclusion;
    else {
        CachedAXObjectAttributes attributes;
        attributes.ignored = inclusion;
        m_idMapping.set(id, attributes);
    }
}
    
bool AXObjectCache::gAccessibilityEnabled = false;
bool AXObjectCache::gAccessibilityEnhancedUserInterfaceEnabled = false;

void AXObjectCache::enableAccessibility()
{
    gAccessibilityEnabled = true;
}

void AXObjectCache::disableAccessibility()
{
    gAccessibilityEnabled = false;
}

void AXObjectCache::setEnhancedUserInterfaceAccessibility(bool flag)
{
    gAccessibilityEnhancedUserInterfaceEnabled = flag;
}

AXObjectCache::AXObjectCache(Document& document)
    : m_document(document)
    , m_notificationPostTimer(this, &AXObjectCache::notificationPostTimerFired)
{
}

AXObjectCache::~AXObjectCache()
{
    m_notificationPostTimer.stop();

    for (const auto& object : m_objects.values()) {
        detachWrapper(object.get(), CacheDestroyed);
        object->detach(CacheDestroyed);
        removeAXID(object.get());
    }
}

AccessibilityObject* AXObjectCache::focusedImageMapUIElement(HTMLAreaElement* areaElement)
{
    // Find the corresponding accessibility object for the HTMLAreaElement. This should be
    // in the list of children for its corresponding image.
    if (!areaElement)
        return nullptr;
    
    HTMLImageElement* imageElement = areaElement->imageElement();
    if (!imageElement)
        return nullptr;
    
    AccessibilityObject* axRenderImage = areaElement->document().axObjectCache()->getOrCreate(imageElement);
    if (!axRenderImage)
        return nullptr;
    
    for (const auto& child : axRenderImage->children()) {
        if (!child->isImageMapLink())
            continue;
        
        if (toAccessibilityImageMapLink(child.get())->areaElement() == areaElement)
            return child.get();
    }    
    
    return nullptr;
}
    
AccessibilityObject* AXObjectCache::focusedUIElementForPage(const Page* page)
{
    if (!gAccessibilityEnabled)
        return nullptr;

    // get the focused node in the page
    Document* focusedDocument = page->focusController().focusedOrMainFrame().document();
    Element* focusedElement = focusedDocument->focusedElement();
    if (focusedElement && isHTMLAreaElement(focusedElement))
        return focusedImageMapUIElement(toHTMLAreaElement(focusedElement));

    AccessibilityObject* obj = focusedDocument->axObjectCache()->getOrCreate(focusedElement ? static_cast<Node*>(focusedElement) : focusedDocument);
    if (!obj)
        return nullptr;

    if (obj->shouldFocusActiveDescendant()) {
        if (AccessibilityObject* descendant = obj->activeDescendant())
            obj = descendant;
    }

    // the HTML element, for example, is focusable but has an AX object that is ignored
    if (obj->accessibilityIsIgnored())
        obj = obj->parentObjectUnignored();

    return obj;
}

AccessibilityObject* AXObjectCache::get(Widget* widget)
{
    if (!widget)
        return nullptr;
        
    AXID axID = m_widgetObjectMapping.get(widget);
    ASSERT(!HashTraits<AXID>::isDeletedValue(axID));
    if (!axID)
        return nullptr;
    
    return m_objects.get(axID);    
}
    
AccessibilityObject* AXObjectCache::get(RenderObject* renderer)
{
    if (!renderer)
        return nullptr;
    
    AXID axID = m_renderObjectMapping.get(renderer);
    ASSERT(!HashTraits<AXID>::isDeletedValue(axID));
    if (!axID)
        return nullptr;

    return m_objects.get(axID);    
}

AccessibilityObject* AXObjectCache::get(Node* node)
{
    if (!node)
        return nullptr;

    AXID renderID = node->renderer() ? m_renderObjectMapping.get(node->renderer()) : 0;
    ASSERT(!HashTraits<AXID>::isDeletedValue(renderID));

    AXID nodeID = m_nodeObjectMapping.get(node);
    ASSERT(!HashTraits<AXID>::isDeletedValue(nodeID));

    if (node->renderer() && nodeID && !renderID) {
        // This can happen if an AccessibilityNodeObject is created for a node that's not
        // rendered, but later something changes and it gets a renderer (like if it's
        // reparented).
        remove(nodeID);
        return nullptr;
    }

    if (renderID)
        return m_objects.get(renderID);

    if (!nodeID)
        return nullptr;

    return m_objects.get(nodeID);
}

// FIXME: This probably belongs on Node.
// FIXME: This should take a const char*, but one caller passes nullAtom.
bool nodeHasRole(Node* node, const String& role)
{
    if (!node || !node->isElementNode())
        return false;

    auto& roleValue = toElement(node)->fastGetAttribute(roleAttr);
    if (role.isNull())
        return roleValue.isEmpty();
    if (roleValue.isEmpty())
        return false;

    return SpaceSplitString(roleValue, true).contains(role);
}

static PassRefPtr<AccessibilityObject> createFromRenderer(RenderObject* renderer)
{
    // FIXME: How could renderer->node() ever not be an Element?
    Node* node = renderer->node();

    // If the node is aria role="list" or the aria role is empty and its a
    // ul/ol/dl type (it shouldn't be a list if aria says otherwise).
    if (node && ((nodeHasRole(node, "list") || nodeHasRole(node, "directory"))
                      || (nodeHasRole(node, nullAtom) && (node->hasTagName(ulTag) || node->hasTagName(olTag) || node->hasTagName(dlTag)))))
        return AccessibilityList::create(renderer);

    // aria tables
    if (nodeHasRole(node, "grid") || nodeHasRole(node, "treegrid"))
        return AccessibilityARIAGrid::create(renderer);
    if (nodeHasRole(node, "row"))
        return AccessibilityARIAGridRow::create(renderer);
    if (nodeHasRole(node, "gridcell") || nodeHasRole(node, "columnheader") || nodeHasRole(node, "rowheader"))
        return AccessibilityARIAGridCell::create(renderer);

#if ENABLE(VIDEO)
    // media controls
    if (node && node->isMediaControlElement())
        return AccessibilityMediaControl::create(renderer);
#endif

    if (renderer->isSVGRoot())
        return AccessibilitySVGRoot::create(renderer);
    
    // Search field buttons
    if (node && node->isElementNode() && toElement(node)->isSearchFieldCancelButtonElement())
        return AccessibilitySearchFieldCancelButton::create(renderer);
    
    if (renderer->isBoxModelObject()) {
        RenderBoxModelObject* cssBox = toRenderBoxModelObject(renderer);
        if (cssBox->isListBox())
            return AccessibilityListBox::create(toRenderListBox(cssBox));
        if (cssBox->isMenuList())
            return AccessibilityMenuList::create(toRenderMenuList(cssBox));

        // standard tables
        if (cssBox->isTable())
            return AccessibilityTable::create(toRenderTable(cssBox));
        if (cssBox->isTableRow())
            return AccessibilityTableRow::create(toRenderTableRow(cssBox));
        if (cssBox->isTableCell())
            return AccessibilityTableCell::create(toRenderTableCell(cssBox));

        // progress bar
        if (cssBox->isProgress())
            return AccessibilityProgressIndicator::create(toRenderProgress(cssBox));

#if ENABLE(METER_ELEMENT)
        if (cssBox->isMeter())
            return AccessibilityProgressIndicator::create(toRenderMeter(cssBox));
#endif

        // input type=range
        if (cssBox->isSlider())
            return AccessibilitySlider::create(toRenderSlider(cssBox));
    }

    return AccessibilityRenderObject::create(renderer);
}

static PassRefPtr<AccessibilityObject> createFromNode(Node* node)
{
    return AccessibilityNodeObject::create(node);
}

AccessibilityObject* AXObjectCache::getOrCreate(Widget* widget)
{
    if (!widget)
        return nullptr;

    if (AccessibilityObject* obj = get(widget))
        return obj;
    
    RefPtr<AccessibilityObject> newObj = nullptr;
    if (widget->isFrameView())
        newObj = AccessibilityScrollView::create(toScrollView(widget));
    else if (widget->isScrollbar())
        newObj = AccessibilityScrollbar::create(toScrollbar(widget));

    // Will crash later if we have two objects for the same widget.
    ASSERT(!get(widget));

    // Catch the case if an (unsupported) widget type is used. Only FrameView and ScrollBar are supported now.
    ASSERT(newObj);
    if (!newObj)
        return nullptr;

    getAXID(newObj.get());
    
    m_widgetObjectMapping.set(widget, newObj->axObjectID());
    m_objects.set(newObj->axObjectID(), newObj);    
    newObj->init();
    attachWrapper(newObj.get());
    return newObj.get();
}

AccessibilityObject* AXObjectCache::getOrCreate(Node* node)
{
    if (!node)
        return nullptr;

    if (AccessibilityObject* obj = get(node))
        return obj;

    if (node->renderer())
        return getOrCreate(node->renderer());

    if (!node->parentElement())
        return nullptr;
    
    // It's only allowed to create an AccessibilityObject from a Node if it's in a canvas subtree.
    // Or if it's a hidden element, but we still want to expose it because of other ARIA attributes.
    bool inCanvasSubtree = node->parentElement()->isInCanvasSubtree();
    bool isHidden = !node->renderer() && isNodeAriaVisible(node);

    bool insideMeterElement = false;
#if ENABLE(METER_ELEMENT)
    insideMeterElement = isHTMLMeterElement(node->parentElement());
#endif
    
    if (!inCanvasSubtree && !isHidden && !insideMeterElement)
        return nullptr;

    RefPtr<AccessibilityObject> newObj = createFromNode(node);

    // Will crash later if we have two objects for the same node.
    ASSERT(!get(node));

    getAXID(newObj.get());

    m_nodeObjectMapping.set(node, newObj->axObjectID());
    m_objects.set(newObj->axObjectID(), newObj);
    newObj->init();
    attachWrapper(newObj.get());
    newObj->setLastKnownIsIgnoredValue(newObj->accessibilityIsIgnored());
    // Sometimes asking accessibilityIsIgnored() will cause the newObject to be deallocated, and then
    // it will disappear when this function is finished, leading to a use-after-free.
    if (newObj->isDetached())
        return nullptr;
    
    return newObj.get();
}

AccessibilityObject* AXObjectCache::getOrCreate(RenderObject* renderer)
{
    if (!renderer)
        return nullptr;

    if (AccessibilityObject* obj = get(renderer))
        return obj;

    RefPtr<AccessibilityObject> newObj = createFromRenderer(renderer);

    // Will crash later if we have two objects for the same renderer.
    ASSERT(!get(renderer));

    getAXID(newObj.get());

    m_renderObjectMapping.set(renderer, newObj->axObjectID());
    m_objects.set(newObj->axObjectID(), newObj);
    newObj->init();
    attachWrapper(newObj.get());
    newObj->setLastKnownIsIgnoredValue(newObj->accessibilityIsIgnored());
    // Sometimes asking accessibilityIsIgnored() will cause the newObject to be deallocated, and then
    // it will disappear when this function is finished, leading to a use-after-free.
    if (newObj->isDetached())
        return nullptr;
    
    return newObj.get();
}
    
AccessibilityObject* AXObjectCache::rootObject()
{
    if (!gAccessibilityEnabled)
        return nullptr;

    return getOrCreate(m_document.view());
}

AccessibilityObject* AXObjectCache::rootObjectForFrame(Frame* frame)
{
    if (!gAccessibilityEnabled)
        return nullptr;

    if (!frame)
        return nullptr;
    return getOrCreate(frame->view());
}    
    
AccessibilityObject* AXObjectCache::getOrCreate(AccessibilityRole role)
{
    RefPtr<AccessibilityObject> obj = nullptr;
    
    // will be filled in...
    switch (role) {
    case ListBoxOptionRole:
        obj = AccessibilityListBoxOption::create();
        break;
    case ImageMapLinkRole:
        obj = AccessibilityImageMapLink::create();
        break;
    case ColumnRole:
        obj = AccessibilityTableColumn::create();
        break;            
    case TableHeaderContainerRole:
        obj = AccessibilityTableHeaderContainer::create();
        break;   
    case SliderThumbRole:
        obj = AccessibilitySliderThumb::create();
        break;
    case MenuListPopupRole:
        obj = AccessibilityMenuListPopup::create();
        break;
    case MenuListOptionRole:
        obj = AccessibilityMenuListOption::create();
        break;
    case SpinButtonRole:
        obj = AccessibilitySpinButton::create();
        break;
    case SpinButtonPartRole:
        obj = AccessibilitySpinButtonPart::create();
        break;
    default:
        obj = nullptr;
    }
    
    if (obj)
        getAXID(obj.get());
    else
        return nullptr;

    m_objects.set(obj->axObjectID(), obj);    
    obj->init();
    attachWrapper(obj.get());
    return obj.get();
}

void AXObjectCache::remove(AXID axID)
{
    if (!axID)
        return;
    
    // first fetch object to operate some cleanup functions on it 
    AccessibilityObject* obj = m_objects.get(axID);
    if (!obj)
        return;
    
    detachWrapper(obj, ElementDestroyed);
    obj->detach(ElementDestroyed, this);
    removeAXID(obj);
    
    // finally remove the object
    if (!m_objects.take(axID))
        return;
    
    ASSERT(m_objects.size() >= m_idsInUse.size());    
}
    
void AXObjectCache::remove(RenderObject* renderer)
{
    if (!renderer)
        return;
    
    AXID axID = m_renderObjectMapping.get(renderer);
    remove(axID);
    m_renderObjectMapping.remove(renderer);
}

void AXObjectCache::remove(Node* node)
{
    if (!node)
        return;

    removeNodeForUse(node);

    // This is all safe even if we didn't have a mapping.
    AXID axID = m_nodeObjectMapping.get(node);
    remove(axID);
    m_nodeObjectMapping.remove(node);

    if (node->renderer()) {
        remove(node->renderer());
        return;
    }
}

void AXObjectCache::remove(Widget* view)
{
    if (!view)
        return;
        
    AXID axID = m_widgetObjectMapping.get(view);
    remove(axID);
    m_widgetObjectMapping.remove(view);
}
    
    
#if !PLATFORM(WIN) || OS(WINCE)
AXID AXObjectCache::platformGenerateAXID() const
{
    static AXID lastUsedID = 0;

    // Generate a new ID.
    AXID objID = lastUsedID;
    do {
        ++objID;
    } while (!objID || HashTraits<AXID>::isDeletedValue(objID) || m_idsInUse.contains(objID));

    lastUsedID = objID;

    return objID;
}
#endif

AXID AXObjectCache::getAXID(AccessibilityObject* obj)
{
    // check for already-assigned ID
    AXID objID = obj->axObjectID();
    if (objID) {
        ASSERT(m_idsInUse.contains(objID));
        return objID;
    }

    objID = platformGenerateAXID();

    m_idsInUse.add(objID);
    obj->setAXObjectID(objID);
    
    return objID;
}

void AXObjectCache::removeAXID(AccessibilityObject* object)
{
    if (!object)
        return;
    
    AXID objID = object->axObjectID();
    if (!objID)
        return;
    ASSERT(!HashTraits<AXID>::isDeletedValue(objID));
    ASSERT(m_idsInUse.contains(objID));
    object->setAXObjectID(0);
    m_idsInUse.remove(objID);
}

void AXObjectCache::textChanged(Node* node)
{
    textChanged(getOrCreate(node));
}

void AXObjectCache::textChanged(RenderObject* renderer)
{
    textChanged(getOrCreate(renderer));
}

void AXObjectCache::textChanged(AccessibilityObject* obj)
{
    if (!obj)
        return;

    bool parentAlreadyExists = obj->parentObjectIfExists();
    obj->textChanged();
    postNotification(obj, obj->document(), AXObjectCache::AXTextChanged);
    if (parentAlreadyExists)
        obj->notifyIfIgnoredValueChanged();
}

void AXObjectCache::updateCacheAfterNodeIsAttached(Node* node)
{
    // Calling get() will update the AX object if we had an AccessibilityNodeObject but now we need
    // an AccessibilityRenderObject, because it was reparented to a location outside of a canvas.
    get(node);
}

void AXObjectCache::handleMenuOpened(Node* node)
{
    if (!node || !node->renderer() || !nodeHasRole(node, "menu"))
        return;
    
    postNotification(getOrCreate(node), &document(), AXMenuOpened);
}
    
void AXObjectCache::handleLiveRegionCreated(Node* node)
{
    if (!node || !node->renderer() || !node->isElementNode())
        return;
    
    Element* element = toElement(node);
    String liveRegionStatus = element->fastGetAttribute(aria_liveAttr);
    if (liveRegionStatus.isEmpty()) {
        const AtomicString& ariaRole = element->fastGetAttribute(roleAttr);
        if (!ariaRole.isEmpty())
            liveRegionStatus = AccessibilityObject::defaultLiveRegionStatusForRole(AccessibilityObject::ariaRoleToWebCoreRole(ariaRole));
    }
    
    if (AccessibilityObject::liveRegionStatusIsEnabled(liveRegionStatus))
        postNotification(getOrCreate(node), &document(), AXLiveRegionCreated);
}
    
void AXObjectCache::childrenChanged(Node* node, Node* newChild)
{
    if (newChild) {
        handleMenuOpened(newChild);
        handleLiveRegionCreated(newChild);
    }
    
    childrenChanged(get(node));
}

void AXObjectCache::childrenChanged(RenderObject* renderer, RenderObject* newChild)
{
    if (!renderer)
        return;
    
    if (newChild) {
        handleMenuOpened(newChild->node());
        handleLiveRegionCreated(newChild->node());
    }
    
    childrenChanged(get(renderer));
}

void AXObjectCache::childrenChanged(AccessibilityObject* obj)
{
    if (!obj)
        return;

    obj->childrenChanged();
}
    
void AXObjectCache::notificationPostTimerFired(Timer<AXObjectCache>&)
{
    Ref<Document> protectorForCacheOwner(m_document);
    m_notificationPostTimer.stop();
    
    // In DRT, posting notifications has a tendency to immediately queue up other notifications, which can lead to unexpected behavior
    // when the notification list is cleared at the end. Instead copy this list at the start.
    auto notifications = WTF::move(m_notificationsToPost);
    
    for (const auto& note : notifications) {
        AccessibilityObject* obj = note.first.get();
        if (!obj->axObjectID())
            continue;

        if (!obj->axObjectCache())
            continue;
        
#ifndef NDEBUG
        // Make sure none of the render views are in the process of being layed out.
        // Notifications should only be sent after the renderer has finished
        if (obj->isAccessibilityRenderObject()) {
            AccessibilityRenderObject* renderObj = toAccessibilityRenderObject(obj);
            RenderObject* renderer = renderObj->renderer();
            if (renderer)
                ASSERT(!renderer->view().layoutState());
        }
#endif

        AXNotification notification = note.second;
        
        // Ensure that this menu really is a menu. We do this check here so that we don't have to create
        // the axChildren when the menu is marked as opening.
        if (notification == AXMenuOpened) {
            obj->updateChildrenIfNecessary();
            if (obj->roleValue() != MenuRole)
                continue;
        }
        
        postPlatformNotification(obj, notification);

        if (notification == AXChildrenChanged && obj->parentObjectIfExists() && obj->lastKnownIsIgnoredValue() != obj->accessibilityIsIgnored())
            childrenChanged(obj->parentObject());
    }
}
    
void AXObjectCache::postNotification(RenderObject* renderer, AXNotification notification, PostTarget postTarget, PostType postType)
{
    if (!renderer)
        return;
    
    stopCachingComputedObjectAttributes();

    // Get an accessibility object that already exists. One should not be created here
    // because a render update may be in progress and creating an AX object can re-trigger a layout
    RefPtr<AccessibilityObject> object = get(renderer);
    while (!object && renderer) {
        renderer = renderer->parent();
        object = get(renderer); 
    }
    
    if (!renderer)
        return;
    
    postNotification(object.get(), &renderer->document(), notification, postTarget, postType);
}

void AXObjectCache::postNotification(Node* node, AXNotification notification, PostTarget postTarget, PostType postType)
{
    if (!node)
        return;
    
    stopCachingComputedObjectAttributes();

    // Get an accessibility object that already exists. One should not be created here
    // because a render update may be in progress and creating an AX object can re-trigger a layout
    RefPtr<AccessibilityObject> object = get(node);
    while (!object && node) {
        node = node->parentNode();
        object = get(node);
    }
    
    if (!node)
        return;
    
    postNotification(object.get(), &node->document(), notification, postTarget, postType);
}

void AXObjectCache::postNotification(AccessibilityObject* object, Document* document, AXNotification notification, PostTarget postTarget, PostType postType)
{
    stopCachingComputedObjectAttributes();

    if (object && postTarget == TargetObservableParent)
        object = object->observableObject();

    if (!object && document)
        object = get(document->renderView());

    if (!object)
        return;

    if (postType == PostAsynchronously) {
        m_notificationsToPost.append(std::make_pair(object, notification));
        if (!m_notificationPostTimer.isActive())
            m_notificationPostTimer.startOneShot(0);
    } else
        postPlatformNotification(object, notification);
}

void AXObjectCache::checkedStateChanged(Node* node)
{
    postNotification(node, AXObjectCache::AXCheckedStateChanged);
}

void AXObjectCache::handleMenuItemSelected(Node* node)
{
    if (!node)
        return;
    
    if (!nodeHasRole(node, "menuitem") && !nodeHasRole(node, "menuitemradio") && !nodeHasRole(node, "menuitemcheckbox"))
        return;
    
    if (!toElement(node)->focused() && !equalIgnoringCase(toElement(node)->fastGetAttribute(aria_selectedAttr), "true"))
        return;
    
    postNotification(getOrCreate(node), &document(), AXMenuListItemSelected);
}
    
void AXObjectCache::handleFocusedUIElementChanged(Node* oldNode, Node* newNode)
{
    handleMenuItemSelected(newNode);
    platformHandleFocusedUIElementChanged(oldNode, newNode);
}
    
void AXObjectCache::selectedChildrenChanged(Node* node)
{
    handleMenuItemSelected(node);
    
    // postTarget is TargetObservableParent so that you can pass in any child of an element and it will go up the parent tree
    // to find the container which should send out the notification.
    postNotification(node, AXSelectedChildrenChanged, TargetObservableParent);
}

void AXObjectCache::selectedChildrenChanged(RenderObject* renderer)
{
    if (renderer)
        handleMenuItemSelected(renderer->node());

    // postTarget is TargetObservableParent so that you can pass in any child of an element and it will go up the parent tree
    // to find the container which should send out the notification.
    postNotification(renderer, AXSelectedChildrenChanged, TargetObservableParent);
}

void AXObjectCache::nodeTextChangeNotification(Node* node, AXTextChange textChange, unsigned offset, const String& text)
{
    if (!node)
        return;

    stopCachingComputedObjectAttributes();

    // Delegate on the right platform
    AccessibilityObject* obj = getOrCreate(node);
    nodeTextChangePlatformNotification(obj, textChange, offset, text);
}

void AXObjectCache::frameLoadingEventNotification(Frame* frame, AXLoadingEvent loadingEvent)
{
    if (!frame)
        return;

    // Delegate on the right platform
    RenderView* contentRenderer = frame->contentRenderer();
    if (!contentRenderer)
        return;

    AccessibilityObject* obj = getOrCreate(contentRenderer);
    frameLoadingEventPlatformNotification(obj, loadingEvent);
}

void AXObjectCache::handleScrollbarUpdate(ScrollView* view)
{
    if (!view)
        return;
    
    // We don't want to create a scroll view from this method, only update an existing one.
    if (AccessibilityObject* scrollViewObject = get(view)) {
        stopCachingComputedObjectAttributes();
        scrollViewObject->updateChildrenIfNecessary();
    }
}
    
void AXObjectCache::handleAriaExpandedChange(Node* node)
{
    if (AccessibilityObject* obj = getOrCreate(node))
        obj->handleAriaExpandedChanged();
}
    
void AXObjectCache::handleActiveDescendantChanged(Node* node)
{
    if (AccessibilityObject* obj = getOrCreate(node))
        obj->handleActiveDescendantChanged();
}

void AXObjectCache::handleAriaRoleChanged(Node* node)
{
    stopCachingComputedObjectAttributes();

    if (AccessibilityObject* obj = getOrCreate(node)) {
        obj->updateAccessibilityRole();
        obj->notifyIfIgnoredValueChanged();
    }
}

void AXObjectCache::handleAttributeChanged(const QualifiedName& attrName, Element* element)
{
    if (attrName == roleAttr)
        handleAriaRoleChanged(element);
    else if (attrName == altAttr || attrName == titleAttr)
        textChanged(element);
    else if (attrName == forAttr && isHTMLLabelElement(element))
        labelChanged(element);

    if (!attrName.localName().string().startsWith("aria-"))
        return;

    if (attrName == aria_activedescendantAttr)
        handleActiveDescendantChanged(element);
    else if (attrName == aria_busyAttr)
        postNotification(element, AXObjectCache::AXElementBusyChanged);
    else if (attrName == aria_valuenowAttr || attrName == aria_valuetextAttr)
        postNotification(element, AXObjectCache::AXValueChanged);
    else if (attrName == aria_labelAttr || attrName == aria_labeledbyAttr || attrName == aria_labelledbyAttr)
        textChanged(element);
    else if (attrName == aria_checkedAttr)
        checkedStateChanged(element);
    else if (attrName == aria_selectedAttr)
        selectedChildrenChanged(element);
    else if (attrName == aria_expandedAttr)
        handleAriaExpandedChange(element);
    else if (attrName == aria_hiddenAttr)
        childrenChanged(element->parentNode(), element);
    else if (attrName == aria_invalidAttr)
        postNotification(element, AXObjectCache::AXInvalidStatusChanged);
    else
        postNotification(element, AXObjectCache::AXAriaAttributeChanged);
}

void AXObjectCache::labelChanged(Element* element)
{
    ASSERT(isHTMLLabelElement(element));
    HTMLElement* correspondingControl = toHTMLLabelElement(element)->control();
    textChanged(correspondingControl);
}

void AXObjectCache::recomputeIsIgnored(RenderObject* renderer)
{
    if (AccessibilityObject* obj = get(renderer))
        obj->notifyIfIgnoredValueChanged();
}

void AXObjectCache::startCachingComputedObjectAttributesUntilTreeMutates()
{
    if (!m_computedObjectAttributeCache)
        m_computedObjectAttributeCache = std::make_unique<AXComputedObjectAttributeCache>();
}

void AXObjectCache::stopCachingComputedObjectAttributes()
{
    m_computedObjectAttributeCache = nullptr;
}

VisiblePosition AXObjectCache::visiblePositionForTextMarkerData(TextMarkerData& textMarkerData)
{
    if (!isNodeInUse(textMarkerData.node))
        return VisiblePosition();
    
    // FIXME: Accessability should make it clear these are DOM-compliant offsets or store Position objects.
    VisiblePosition visiblePos = VisiblePosition(createLegacyEditingPosition(textMarkerData.node, textMarkerData.offset), textMarkerData.affinity);
    Position deepPos = visiblePos.deepEquivalent();
    if (deepPos.isNull())
        return VisiblePosition();
    
    RenderObject* renderer = deepPos.deprecatedNode()->renderer();
    if (!renderer)
        return VisiblePosition();
    
    AXObjectCache* cache = renderer->document().axObjectCache();
    if (!cache->isIDinUse(textMarkerData.axID))
        return VisiblePosition();
    
    if (deepPos.deprecatedNode() != textMarkerData.node || deepPos.deprecatedEditingOffset() != textMarkerData.offset)
        return VisiblePosition();
    
    return visiblePos;
}

void AXObjectCache::textMarkerDataForVisiblePosition(TextMarkerData& textMarkerData, const VisiblePosition& visiblePos)
{
    // This memory must be bzero'd so instances of TextMarkerData can be tested for byte-equivalence.
    // This also allows callers to check for failure by looking at textMarkerData upon return.
    memset(&textMarkerData, 0, sizeof(TextMarkerData));
    
    if (visiblePos.isNull())
        return;
    
    Position deepPos = visiblePos.deepEquivalent();
    Node* domNode = deepPos.deprecatedNode();
    ASSERT(domNode);
    if (!domNode)
        return;
    
    if (domNode->isHTMLElement()) {
        HTMLInputElement* inputElement = domNode->toInputElement();
        if (inputElement && inputElement->isPasswordField())
            return;
    }
    
    // find or create an accessibility object for this node
    AXObjectCache* cache = domNode->document().axObjectCache();
    RefPtr<AccessibilityObject> obj = cache->getOrCreate(domNode);
    
    textMarkerData.axID = obj.get()->axObjectID();
    textMarkerData.node = domNode;
    textMarkerData.offset = deepPos.deprecatedEditingOffset();
    textMarkerData.affinity = visiblePos.affinity();   
    
    cache->setNodeInUse(domNode);
}

const Element* AXObjectCache::rootAXEditableElement(const Node* node)
{
    const Element* result = node->rootEditableElement();
    const Element* element = node->isElementNode() ? toElement(node) : node->parentElement();

    for (; element; element = element->parentElement()) {
        if (nodeIsTextControl(element))
            result = element;
    }

    return result;
}

void AXObjectCache::clearTextMarkerNodesInUse(Document* document)
{
    HashSet<Node*>::iterator it = m_textMarkerNodes.begin();
    HashSet<Node*>::iterator end = m_textMarkerNodes.end();

    // Check each node to see if it's inside the document being deleted.
    HashSet<Node*> nodesToDelete;
    for (; it != end; ++it) {
        if (&(*it)->document() == document)
            nodesToDelete.add(*it);
    }
    
    it = nodesToDelete.begin();
    end = nodesToDelete.end();
    for (; it != end; ++it)
        m_textMarkerNodes.remove(*it);
}
    
bool AXObjectCache::nodeIsTextControl(const Node* node)
{
    if (!node)
        return false;

    const AccessibilityObject* axObject = getOrCreate(const_cast<Node*>(node));
    return axObject && axObject->isTextControl();
}
    
bool isNodeAriaVisible(Node* node)
{
    if (!node)
        return false;
    
    // To determine if a node is ARIA visible, we need to check the parent hierarchy to see if anyone specifies
    // aria-hidden explicitly.
    for (Node* testNode = node; testNode; testNode = testNode->parentNode()) {
        if (testNode->isElementNode()) {
            const AtomicString& ariaHiddenValue = toElement(testNode)->fastGetAttribute(aria_hiddenAttr);
            if (equalIgnoringCase(ariaHiddenValue, "false"))
                return true;
            if (equalIgnoringCase(ariaHiddenValue, "true"))
                return false;
        }
    }
    
    return false;
}

AXAttributeCacheEnabler::AXAttributeCacheEnabler(AXObjectCache* cache)
    : m_cache(cache)
{
    if (m_cache)
        m_cache->startCachingComputedObjectAttributesUntilTreeMutates();
}
    
AXAttributeCacheEnabler::~AXAttributeCacheEnabler()
{
    if (m_cache)
        m_cache->stopCachingComputedObjectAttributes();
}
    
} // namespace WebCore

#endif // HAVE(ACCESSIBILITY)
