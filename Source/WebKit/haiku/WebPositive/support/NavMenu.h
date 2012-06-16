/*
Open Tracker License

Terms and Conditions

Copyright (c) 1991-2000, Be Incorporated. All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice applies to all licensees
and shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF TITLE, MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
BE INCORPORATED BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF, OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of Be Incorporated shall not be
used in advertising or otherwise to promote the sale, use or other dealings in
this Software without prior written authorization from Be Incorporated.

Tracker(TM), Be(R), BeOS(R), and BeIA(TM) are trademarks or registered trademarks
of Be Incorporated in the United States and other countries. Other brand product
names are registered trademarks or trademarks of their respective holders.
All rights reserved.
*/

//	NavMenu is a hierarchical menu of volumes, folders, files and queries
//	displays icons, uses the SlowMenu API for full interruptability

#ifndef NAV_MENU_H
#define NAV_MENU_H


#include <Messenger.h>
#include <StorageDefs.h>
#include <Entry.h>

#include "SlowMenu.h"


template<class T> class BObjectList;
class BMenuItem;

namespace BPrivate {

class Model;
class BContainerWindow;
class ModelMenuItem;
class EntryListBase;


class TrackingHookData {
	public:
		TrackingHookData()
			:
			fTrackingHook(NULL),
			fDragMessage(NULL)
		{
		}

		bool (*fTrackingHook)(BMenu *, void *);
		BMessenger fTarget;
		const BMessage *fDragMessage;
};


class BNavMenu : public BSlowMenu {
	public:
		BNavMenu(const char* title, uint32 message, const BHandler *,
			BWindow *parentWindow = NULL, const BObjectList<BString> *list = NULL);
		BNavMenu(const char* title, uint32 message, const BMessenger &,
			BWindow *parentWindow = NULL, const BObjectList<BString> *list = NULL);
			// parentWindow, if specified, will be closed if nav menu item invoked
			// with option held down
							
		virtual ~BNavMenu();
	
		virtual	void AttachedToWindow();
		virtual	void DetachedFromWindow();
				
		void SetNavDir(const entry_ref *);
		void ForceRebuild();
		bool NeedsToRebuild() const;
			// will cause menu to get rebuilt next time it is shown

		virtual	void ResetTargets();
		void SetTarget(const BMessenger &);
		BMessenger Target();

		void SetTypesList(const BObjectList<BString> *list);
		const BObjectList<BString> *TypesList() const;	

		void AddNavDir(const Model *model, uint32 what, BHandler *target,
			bool populateSubmenu);

		void AddNavParentDir(const char *name, const Model *model, uint32 what, BHandler *target);
		void AddNavParentDir(const Model *model, uint32 what, BHandler *target);
		void SetShowParent(bool show);

		static int32 GetMaxMenuWidth();

		static int CompareFolderNamesFirstOne(const BMenuItem *, const BMenuItem *);
		static int CompareOne(const BMenuItem *, const BMenuItem *);

		static ModelMenuItem *NewModelItem(Model *, const BMessage *, const BMessenger &,
			bool suppressFolderHierarchy=false, BContainerWindow * = NULL,
			const BObjectList<BString> *typeslist = NULL,
			TrackingHookData *hook = NULL);

		TrackingHookData *InitTrackingHook(bool (*hookfunction)(BMenu *, void *),
			const BMessenger *target, const BMessage *dragMessage);

	protected:
		virtual bool StartBuildingItemList();
		virtual bool AddNextItem();
		virtual void DoneBuildingItemList();	
		virtual void  ClearMenuBuildingState();

		void BuildVolumeMenu();

		void AddOneItem(Model *);
		void AddRootItemsIfNeeded();
		void AddTrashItem();
		static void SetTrackingHookDeep(BMenu *, bool (*)(BMenu *, void *), void *);

		entry_ref	fNavDir;
		BMessage	fMessage;
		BMessenger	fMessenger;
		BWindow		*fParentWindow;

		// menu building state
		uint8		fFlags;
		BObjectList<BMenuItem> *fItemList;
		EntryListBase *fContainer;
		bool		fIteratingDesktop;

		const BObjectList<BString> *fTypesList;

		TrackingHookData fTrackingHook;
};

//	Spring Loaded Folder convenience routines
//		used in both Tracker and Deskbar
#ifndef _IMPEXP_TRACKER
#	define _IMPEXP_TRACKER
#endif
_IMPEXP_TRACKER bool SpringLoadedFolderCompareMessages(const BMessage *incoming,
	const BMessage *dragmessage);
_IMPEXP_TRACKER void SpringLoadedFolderSetMenuStates(const BMenu *menu,
	const BObjectList<BString> *typeslist);
_IMPEXP_TRACKER void SpringLoadedFolderAddUniqueTypeToList(entry_ref *ref,
	BObjectList<BString> *typeslist);
_IMPEXP_TRACKER void SpringLoadedFolderCacheDragData(const BMessage *incoming,
	BMessage **, BObjectList<BString> **typeslist);

} // namespace BPrivate

using namespace BPrivate;

#endif	// NAV_MENU_H
