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

#ifndef __SLOW_MENU__
#define __SLOW_MENU__

#include <Menu.h>
#include <MenuItem.h>
#include <Debug.h>

// SlowMenu is a convenience class that makes it easier to
// use the AddDynamicItem callback to implement a menu that can
// checks periodically between creating new items and quits
// early if needed

namespace BPrivate {

class BSlowMenu : public BMenu {
	public:
		BSlowMenu(const char *title, menu_layout layout = B_ITEMS_IN_COLUMN);

	protected:
		virtual bool StartBuildingItemList();
			// set up state to start building the item list
			// returns false if setup failed
		virtual bool AddNextItem() = 0;
			// returns false if done
		virtual void DoneBuildingItemList() = 0;
			// default version adds items from itemList to menu and deletes
			// the list; override to sort items first, etc.

		virtual void ClearMenuBuildingState() = 0;

	protected:
		virtual bool AddDynamicItem(add_state state);
			// this is the callback from BMenu, you shouldn't need to override this

		bool fMenuBuilt;
};

} // namespace BPrivate

using namespace BPrivate;

#endif	/* __SLOW_MENU__ */
