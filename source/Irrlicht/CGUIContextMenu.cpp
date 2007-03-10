// Copyright (C) 2002-2007 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "CGUIContextMenu.h"
#include "IGUISkin.h"
#include "IGUIEnvironment.h"
#include "IVideoDriver.h"
#include "IGUIFont.h"
#include "IGUISpriteBank.h"
#include "os.h"

namespace irr
{
namespace gui
{



//! constructor
CGUIContextMenu::CGUIContextMenu(IGUIEnvironment* environment,
				 IGUIElement* parent, s32 id,
				 core::rect<s32> rectangle, bool getFocus)
: IGUIContextMenu(environment, parent, id, rectangle), HighLighted(-1), ChangeTime(0)
{
	#ifdef _DEBUG
	setDebugName("CGUIContextMenu");
	#endif

	Pos = rectangle.UpperLeftCorner;
	recalculateSize();

	if (getFocus)
		Environment->setFocus(this);

	setNotClipped(true);
}


//! destructor
CGUIContextMenu::~CGUIContextMenu()
{
	for (s32 i=0; i<(s32)Items.size(); ++i)
		if (Items[i].SubMenu)
			Items[i].SubMenu->drop();
}


//! Returns amount of menu items
s32 CGUIContextMenu::getItemCount() const
{
	return Items.size();
}


//! Adds a menu item.
s32 CGUIContextMenu::addItem(const wchar_t* text, s32 id, bool enabled, bool hasSubMenu, bool checked)
{
	SItem s;
	s.Enabled = enabled;
	s.Checked = checked;
	s.Text = text;
	s.IsSeparator = (text == 0);
	s.SubMenu = 0;
	s.CommandId = id;

	if (hasSubMenu)
	{
		s.SubMenu = new CGUIContextMenu(Environment, this, id,
			core::rect<s32>(0,0,100,100), false);
		s.SubMenu->setVisible(false);
	}

	Items.push_back(s);

	recalculateSize();
	return Items.size() - 1;
}

//! Adds a sub menu from an element that already exists.
void CGUIContextMenu::setSubMenu(s32 index, CGUIContextMenu* menu)
{
	if (index >= (s32)Items.size() || index < 0)
		return;

	if (Items[index].SubMenu)
		Items[index].SubMenu->drop();

	Items[index].SubMenu = menu;
	menu->setVisible(false);
	
	if (Items[index].SubMenu)
		menu->grab();

	recalculateSize();
}


//! Adds a separator item to the menu
void CGUIContextMenu::addSeparator()
{
	addItem(0, -1, true, false, false);
}


//! Returns text of the menu item.
const wchar_t* CGUIContextMenu::getItemText(s32 idx)
{
	if (idx < 0 || idx >= (s32)Items.size())
		return 0;

	return Items[idx].Text.c_str();
}


//! Sets text of the menu item.
void CGUIContextMenu::setItemText(s32 idx, const wchar_t* text)
{
	if (idx < 0 || idx >= (s32)Items.size())
		return;

	Items[idx].Text = text;
	recalculateSize();
}


//! Returns if a menu item is enabled
bool CGUIContextMenu::isItemEnabled(s32 idx)
{
	if (idx < 0 || idx >= (s32)Items.size())
	{
		_IRR_IMPLEMENT_MANAGED_MARSHALLING_BUGFIX;
		return false;
	}

	_IRR_IMPLEMENT_MANAGED_MARSHALLING_BUGFIX;
	return Items[idx].Enabled;
}


//! Sets if the menu item should be enabled.
void CGUIContextMenu::setItemEnabled(s32 idx, bool enabled)
{
	if (idx < 0 || idx >= (s32)Items.size())
		return;

	Items[idx].Enabled = enabled;
}

//! Sets if the menu item should be checked.
void CGUIContextMenu::setItemChecked(s32 idx, bool checked )
{
	if (idx < 0 || idx >= (s32)Items.size())
		return;

	Items[idx].Checked = checked;
}


//! Removes a menu item
void CGUIContextMenu::removeItem(s32 idx)
{
	if (idx < 0 || idx >= (s32)Items.size())
		return;

	if (Items[idx].SubMenu)
	{
		Items[idx].SubMenu->drop();
		Items[idx].SubMenu = 0;
	}

	Items.erase(idx);
	recalculateSize();
}


//! Removes all menu items
void CGUIContextMenu::removeAllItems()
{
	for (s32 i=0; i<(s32)Items.size(); ++i)
		if (Items[i].SubMenu)
			Items[i].SubMenu->drop();

	Items.clear();
	recalculateSize();
}


//! called if an event happened.
bool CGUIContextMenu::OnEvent(SEvent event)
{
	if (!IsEnabled)
		return Parent ? Parent->OnEvent(event) : false;

	switch(event.EventType)
	{
	case EET_GUI_EVENT:
		switch(event.GUIEvent.EventType)
		{
		case gui::EGET_ELEMENT_FOCUS_LOST:
			if (event.GUIEvent.Caller == (IGUIElement*)this)
			{
				remove();
				return true;
			}
			break;
		}
		break;
	case EET_MOUSE_INPUT_EVENT:
		switch(event.MouseInput.Event)
		{
		case EMIE_LMOUSE_LEFT_UP:
			{
				// menu might be removed if it loses focus in sendClick, so grab a reference
				grab();
				s32 t = sendClick(core::position2d<s32>(event.MouseInput.X, event.MouseInput.Y));
				if ((t==0 || t==1) && Environment->hasFocus(this))
					Environment->removeFocus(this);
				drop();
			}
			return true;
		case EMIE_LMOUSE_PRESSED_DOWN:
			return true;
		case EMIE_MOUSE_MOVED:
			if (Environment->hasFocus(this))
				highlight(core::position2d<s32>(event.MouseInput.X,	event.MouseInput.Y));
			return true;
		}
		break;
	}

	return Parent ? Parent->OnEvent(event) : false;
}


//! Sets the visible state of this element.
void CGUIContextMenu::setVisible(bool visible)
{
	HighLighted = -1;
	ChangeTime = os::Timer::getTime();
	for (s32 j=0; j<(s32)Items.size(); ++j)
		if (Items[j].SubMenu)
			Items[j].SubMenu->setVisible(false);

	IGUIElement::setVisible(visible);
}


//! sends a click Returns:
//! 0 if click went outside of the element,
//! 1 if a valid button was clicked,
//! 2 if a nonclickable element was clicked
s32 CGUIContextMenu::sendClick(core::position2d<s32> p)
{
	s32 t = 0;

	// get number of open submenu
	s32 openmenu = -1;
	s32 j;
	for (j=0; j<(s32)Items.size(); ++j)
		if (Items[j].SubMenu && Items[j].SubMenu->isVisible())
		{
			openmenu = j;
			break;
		}

	// delegate click operation to submenu
	if (openmenu != -1)
	{
		t = Items[j].SubMenu->sendClick(p);
		if (t != 0)
			return t; // clicked something
	}

	// check click on myself
	if (AbsoluteClippingRect.isPointInside(p) &&
		HighLighted >= 0 && HighLighted <(s32)Items.size())
	{
		if (!Items[HighLighted].Enabled ||
			Items[HighLighted].IsSeparator ||
			Items[HighLighted].SubMenu)
			return 2;

		SEvent event;
		event.EventType = EET_GUI_EVENT;
		event.GUIEvent.Caller = this;
		event.GUIEvent.EventType = EGET_MENU_ITEM_SELECTED;
		Parent->OnEvent(event);

		return 1;
	}

	return 0;
}


//! returns true, if an element was highligted
bool CGUIContextMenu::highlight(core::position2d<s32> p)
{
	// get number of open submenu
	s32 openmenu = -1;
	s32 i;
	for (i=0; i<(s32)Items.size(); ++i)
		if (Items[i].SubMenu && Items[i].SubMenu->isVisible())
		{
			openmenu = i;
			break;
		}

	// delegate highlight operation to submenu
	if (openmenu != -1)
	{
		if (Items[openmenu].SubMenu->highlight(p))
		{
			HighLighted = openmenu;
			ChangeTime = os::Timer::getTime();
			return true;
		}
	}

	// highlight myself
	for (i=0; i<(s32)Items.size(); ++i)
		if (getHRect(Items[i], AbsoluteRect).isPointInside(p))
		{
			HighLighted = i;
			ChangeTime = os::Timer::getTime();

			// make submenus visible/invisible
			for (s32 j=0; j<(s32)Items.size(); ++j)
				if (Items[j].SubMenu)
					Items[j].SubMenu->setVisible(j == i);
			return true;
		}

	HighLighted = openmenu;
	return false;
}


//! returns the item highlight-area
core::rect<s32> CGUIContextMenu::getHRect(const SItem& i, const core::rect<s32>& absolute)
{
	core::rect<s32> r = absolute;
	r.UpperLeftCorner.Y += i.PosY;
	r.LowerRightCorner.Y = r.UpperLeftCorner.Y + i.Dim.Height;
	return r;
}


//! Gets drawing rect of Item
core::rect<s32> CGUIContextMenu::getRect(const SItem& i, const core::rect<s32>& absolute)
{
	core::rect<s32> r = absolute;
	r.UpperLeftCorner.Y += i.PosY;
	r.LowerRightCorner.Y = r.UpperLeftCorner.Y + i.Dim.Height;
	r.UpperLeftCorner.X += 20;
	return r;
}


//! draws the element and its children
void CGUIContextMenu::draw()
{
	if (!IsVisible)
		return;

	IGUISkin* skin = Environment->getSkin();

	if (!skin)
		return;
	
	IGUIFont* font = skin->getFont();
	IGUISpriteBank* sprites = skin->getSpriteBank();

	video::IVideoDriver* driver = Environment->getVideoDriver();

	core::rect<s32> rect = AbsoluteRect;
	core::rect<s32>* clip = 0;

	// draw frame
	skin->draw3DMenuPane(this, AbsoluteRect, clip);

	// loop through all menu items

	rect = AbsoluteRect;
	s32 y = AbsoluteRect.UpperLeftCorner.Y;

	for (s32 i=0; i<(s32)Items.size(); ++i)
	{
		if (Items[i].IsSeparator)
		{
			// draw separator
			rect = AbsoluteRect;
			rect.UpperLeftCorner.Y += Items[i].PosY + 3;
			rect.LowerRightCorner.Y = rect.UpperLeftCorner.Y + 1;
			rect.UpperLeftCorner.X += 5;
			rect.LowerRightCorner.X -= 5;
			driver->draw2DRectangle(skin->getColor(EGDC_3D_SHADOW), rect, clip);

			rect.LowerRightCorner.Y += 1;
			rect.UpperLeftCorner.Y += 1;
			driver->draw2DRectangle(skin->getColor(EGDC_3D_HIGH_LIGHT), rect, clip);

			y += 10;
		}
		else
		{
			rect = getRect(Items[i], AbsoluteRect);

			// draw highlighted

			if (i == HighLighted && Items[i].Enabled)
			{
				core::rect<s32> r = AbsoluteRect;
				r.LowerRightCorner.Y = rect.LowerRightCorner.Y;
				r.UpperLeftCorner.Y = rect.UpperLeftCorner.Y;
				r.LowerRightCorner.X -= 5;
				r.UpperLeftCorner.X += 5;
				driver->draw2DRectangle(skin->getColor(EGDC_HIGH_LIGHT), r, clip);
			}

			// draw text

			EGUI_DEFAULT_COLOR c = EGDC_BUTTON_TEXT;

			if (i == HighLighted)
				c = EGDC_HIGH_LIGHT_TEXT;

			if (!Items[i].Enabled)
				c = EGDC_GRAY_TEXT;

			if (font)
				font->draw(Items[i].Text.c_str(), rect,
					skin->getColor(c), false, true, clip);

			// draw submenu symbol
			if (Items[i].SubMenu && sprites)
			{
				core::rect<s32> r = rect;
				r.UpperLeftCorner.X = r.LowerRightCorner.X - 15;

				sprites->draw2DSprite(skin->getIcon(EGDI_CURSOR_RIGHT), 
					r.getCenter(), clip, skin->getColor(c), 
					(i == HighLighted) ? ChangeTime : 0,  
					(i == HighLighted) ? os::Timer::getTime() : 0, 
					(i == HighLighted), true); 
			}

			// draw checked symbol
			if (Items[i].Checked && sprites)
			{
				core::rect<s32> r = rect;
				r.UpperLeftCorner.X -= 15;
				sprites->draw2DSprite(skin->getIcon(EGDI_CHECK_BOX_CHECKED), 
					r.getCenter(), clip, skin->getColor(c), 
					(i == HighLighted) ? ChangeTime : 0,  
					(i == HighLighted) ? os::Timer::getTime() : 0, 
					(i == HighLighted), true); 
			}

		}
	}

	IGUIElement::draw();
}


void CGUIContextMenu::recalculateSize()
{
	IGUISkin* skin = Environment->getSkin();
	IGUIFont* font = skin->getFont();

	if (!font)
		return;

	core::rect<s32> rect;
	rect.UpperLeftCorner = RelativeRect.UpperLeftCorner;
	s32 width = 100;
	s32 height = 3;
	s32 i;

	for (i=0; i<(s32)Items.size(); ++i)
	{
		if (Items[i].IsSeparator)
		{
			Items[i].Dim.Width = 100;
			Items[i].Dim.Height = 10;
		}
		else
		{
			Items[i].Dim = font->getDimension(Items[i].Text.c_str());
			Items[i].Dim.Width += 40;

			if (Items[i].Dim.Width > width)
				width = Items[i].Dim.Width;
		}

		Items[i].PosY = height;
		height += Items[i].Dim.Height;
	}

	height += 5;

	if (height < 10)
		height = 10;

	rect.LowerRightCorner.X = RelativeRect.UpperLeftCorner.X + width;
	rect.LowerRightCorner.Y = RelativeRect.UpperLeftCorner.Y + height;

	setRelativePosition(rect);

	// recalculate submenus
	for (i=0; i<(s32)Items.size(); ++i)
		if (Items[i].SubMenu)
		{
			// move submenu
			s32 w = Items[i].SubMenu->getAbsolutePosition().getWidth();
			s32 h = Items[i].SubMenu->getAbsolutePosition().getHeight();

			Items[i].SubMenu->setRelativePosition(
				core::rect<s32>(width-5, Items[i].PosY,
					width+w-5, Items[i].PosY+h));
		}
}


//! Returns the selected item in the menu
s32 CGUIContextMenu::getSelectedItem()
{
	return HighLighted;
}


//! \return Returns a pointer to the submenu of an item.
IGUIContextMenu* CGUIContextMenu::getSubMenu(s32 idx)
{
	if (idx < 0 || idx >= (s32)Items.size())
		return 0;

	return Items[idx].SubMenu;
}


//! Returns command id of a menu item
s32 CGUIContextMenu::getItemCommandId(s32 idx)
{
	if (idx < 0 || idx >= (s32)Items.size())
		return -1;

	return Items[idx].CommandId;
}

//! Sets the command id of a menu item
void CGUIContextMenu::setItemCommandId(s32 idx, s32 id)
{
	if (idx < 0 || idx >= (s32)Items.size())
		return;

	Items[idx].CommandId = id;
}

//! Writes attributes of the element.
void CGUIContextMenu::serializeAttributes(io::IAttributes* out, io::SAttributeReadWriteOptions* options=0)
{
	IGUIElement::serializeAttributes(out,options);
	out->addPosition2d	("Position",		Pos);

	if (Parent->getType() == EGUIET_CONTEXT_MENU || Parent->getType() == EGUIET_MENU )
	{
		IGUIContextMenu* ptr = (IGUIContextMenu*)Parent;
		s32 i=0;
		// find the position of this item in its parent's list
		for (; i<(s32)ptr->getItemCount() && ptr->getSubMenu(i) != this; ++i);

		out->addInt("ParentItem",	i);
	}
	
	// write out the item list
	out->addInt("ItemCount", Items.size());

	core::stringc tmp;

	s32 i=0;
	for (; i < (s32)Items.size(); ++i)
	{
		tmp = "IsSeparator"; tmp += i;
		out->addBool(tmp.c_str(), Items[i].IsSeparator);

		if (!Items[i].IsSeparator)
		{
			tmp = "Text"; tmp += i;
			out->addString(tmp.c_str(), Items[i].Text.c_str());
			tmp = "CommandID"; tmp += i;
			out->addInt(tmp.c_str(), Items[i].CommandId);
			tmp = "Enabled"; tmp += i;
			out->addBool(tmp.c_str(), Items[i].Enabled);
		}
	}
}

//! Reads attributes of the element
void CGUIContextMenu::deserializeAttributes(io::IAttributes* in, io::SAttributeReadWriteOptions* options=0)
{
	IGUIElement::deserializeAttributes(in,options);

	Pos = in->getAttributeAsPosition2d("Position");

	// link to this item's parent
	if (Parent && ( Parent->getType() == EGUIET_CONTEXT_MENU || Parent->getType() == EGUIET_MENU ) )
		((CGUIContextMenu*)Parent)->setSubMenu(in->getAttributeAsInt("ParentItem"),this);


	removeAllItems();

	// read the item list
	s32 count = in->getAttributeAsInt("ItemCount");

	s32 i=0;
	for (; i<(s32)count; ++i)
	{
		core::stringc tmp;
		core::stringw txt;
		s32 commandid;
		bool enabled;
		bool checked;

		tmp = "IsSeparator"; tmp += i;
		if ( in->getAttributeAsBool(tmp.c_str()) )
			addSeparator();
		else
		{
			tmp = "Text"; tmp += i;
			txt = in->getAttributeAsStringW(tmp.c_str());

			tmp = "CommandID"; tmp += i;
			commandid = in->getAttributeAsInt(tmp.c_str());

			tmp = "Enabled"; tmp += i;
			enabled = in->getAttributeAsBool(tmp.c_str());

			tmp = "Checked"; tmp += i;
			checked = in->getAttributeAsBool(tmp.c_str());
			
			addItem(core::stringw(txt.c_str()).c_str(), commandid, enabled, false, checked);
		}
	}


	recalculateSize();

}


} // end namespace
} // end namespace

