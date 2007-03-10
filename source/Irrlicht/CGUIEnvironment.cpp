
// Copyright (C) 2002-2007 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "CGUIEnvironment.h"
#include "IVideoDriver.h"

#include "CGUISkin.h"
#include "CGUIButton.h"
#include "CGUIWindow.h"
#include "CGUIScrollBar.h"
#include "CGUIFont.h"
#include "CGUISpriteBank.h"
#include "CGUIImage.h"
#include "CGUIMeshViewer.h"
#include "CGUICheckBox.h"
#include "CGUIListBox.h"
#include "CGUIFileOpenDialog.h"
#include "CGUIColorSelectDialog.h"
#include "CGUIStaticText.h"
#include "CGUIEditBox.h"
#include "CGUIInOutFader.h"
#include "CGUIMessageBox.h"
#include "CGUIModalScreen.h"
#include "CGUITabControl.h"
#include "CGUIContextMenu.h"
#include "CGUIComboBox.h"
#include "CGUIMenu.h"
#include "CGUIToolBar.h"

#include "CDefaultGUIElementFactory.h"
#include "IWriteFile.h"
#include "IXMLWriter.h"

#include "BuiltInFont.h"
#include "os.h"

namespace irr
{
namespace gui
{

const wchar_t* IRR_XML_FORMAT_GUI_ENV			= L"irr_gui";
const wchar_t* IRR_XML_FORMAT_GUI_ELEMENT		= L"element";
const wchar_t* IRR_XML_FORMAT_GUI_ELEMENT_ATTR_TYPE	= L"type";

//! constructor
CGUIEnvironment::CGUIEnvironment(io::IFileSystem* fs, video::IVideoDriver* driver, IOSOperator* op)
: IGUIElement(EGUIET_ELEMENT, 0, 0, 0, core::rect<s32>(core::position2d<s32>(0,0), driver ? driver->getScreenSize() : core::dimension2d<s32>(0,0))),
	Driver(driver), Hovered(0), Focus(0), CurrentSkin(0),
	FileSystem(fs), UserReceiver(0), Operator(op)
{
	if (Driver)
		Driver->grab();

	if (FileSystem)
		FileSystem->grab();

	if (Operator)
		Operator->grab();

	#ifdef _DEBUG
	IGUIEnvironment::setDebugName("CGUIEnvironment");
	#endif

	// gui factory
	IGUIElementFactory* factory = new CDefaultGUIElementFactory(this);
	registerGUIElementFactory(factory);
	factory->drop();

	loadBuiltInFont();

	IGUISkin* skin = createSkin( gui::EGST_WINDOWS_METALLIC );
	setSkin(skin);
	skin->drop();

	//set tooltip default
	ToolTip.LastTime = 0;
	ToolTip.LaunchTime = 1000;
	ToolTip.Element = 0;
}


//! destructor
CGUIEnvironment::~CGUIEnvironment()
{
	if (Hovered && Hovered != this)
		Hovered->drop();

	if (CurrentSkin)
		CurrentSkin->drop();

	if (Driver)
		Driver->drop();

	if (Focus)
		Focus->drop();

	if (FileSystem)
		FileSystem->drop();

	if (Operator)
		Operator->drop();

	// delete all fonts
	u32 i;

	for (i=0; i<Fonts.size(); ++i)
		Fonts[i].Font->drop();

	// remove all factories
	for (i=0; i<GUIElementFactoryList.size(); ++i)
		GUIElementFactoryList[i]->drop();

}


void CGUIEnvironment::loadBuiltInFont()
{
	const c8* filename = "#DefaultFont";
	io::IReadFile* file = io::createMemoryReadFile(BuiltInFontData, BuiltInFontDataSize, filename, false);

	CGUIFont* font = new CGUIFont(this, "#DefaultFont");
	if (!font->load(file))
	{
		os::Printer::log("Error: Could not load built-in Font.", ELL_ERROR);
		font->drop();
		file->drop();
		return;
	}

	SFont f;
	f.Filename = filename;
	f.Font = font;
	Fonts.push_back(f);

	file->drop();
}



//! draws all gui elements
void CGUIEnvironment::drawAll()
{
	if (Driver)
	{
		core::dimension2d<s32> dim = Driver->getScreenSize();
		if (AbsoluteRect.LowerRightCorner.X != dim.Width ||
			AbsoluteRect.LowerRightCorner.Y != dim.Height)
		{
			// resize gui environment
			RelativeRect.LowerRightCorner.X = Driver->getScreenSize().Width;
			RelativeRect.LowerRightCorner.Y = Driver->getScreenSize().Height;
			AbsoluteClippingRect = RelativeRect;
			AbsoluteRect = RelativeRect;
			updateAbsolutePosition();
		}		
	}

	draw();
	OnPostRender ( os::Timer::getTime () );
}


//! sets the focus to an element
void CGUIEnvironment::setFocus(IGUIElement* element)
{
	if (Focus == element)
		return;

	removeFocus(Focus);

	Focus = element;

	if (Focus)
	{
		Focus->grab();

		// send focused event
		SEvent e;
		e.EventType = EET_GUI_EVENT;
		e.GUIEvent.Caller = Focus;
		e.GUIEvent.EventType = EGET_ELEMENT_FOCUSED;
		Focus->OnEvent(e);
	}
}

//! returns the element with the focus
IGUIElement* CGUIEnvironment::getFocus()
{
	return Focus;
}


//! removes the focus from an element
void CGUIEnvironment::removeFocus(IGUIElement* element)
{
	if (Focus && Focus==element)
	{
		SEvent e;
		e.EventType = EET_GUI_EVENT;
		e.GUIEvent.Caller = Focus;
		e.GUIEvent.EventType = EGET_ELEMENT_FOCUS_LOST;
		Focus->OnEvent(e);
		Focus->drop();
		Focus = 0;
	}
}


//! Returns if the element has focus
bool CGUIEnvironment::hasFocus(IGUIElement* element)
{
	bool ret = (element == Focus); 
	_IRR_IMPLEMENT_MANAGED_MARSHALLING_BUGFIX;
	return ret;
}


//! returns the current video driver
video::IVideoDriver* CGUIEnvironment::getVideoDriver()
{
	return Driver;
}

//! returns the current file system
io::IFileSystem* CGUIEnvironment::getFileSystem()
{
	return FileSystem;
}


//! clear all GUI elements
void CGUIEnvironment::clear()
{
	// get the root's children in case the root changes in future
	const core::list<IGUIElement*>& children = getRootGUIElement()->getChildren();

	while (!children.empty())
		(*children.getLast())->remove();
}


//! called by ui if an event happened.
bool CGUIEnvironment::OnEvent(SEvent event)
{
	if (UserReceiver && event.GUIEvent.Caller != this)
		return UserReceiver->OnEvent(event);

	return false;
}

/*
*/
void CGUIEnvironment::OnPostRender( u32 time )
{
	// check tooltip

	// launch tooltip
	if ( time - ToolTip.LastTime >= ToolTip.LaunchTime &&
		Hovered && Hovered != this &&
		ToolTip.Element == 0 &&
		Hovered != ToolTip.Element &&
		Hovered->getToolTipText().size()
		)
	{
		core::rect<s32> pos;
		pos.UpperLeftCorner = Hovered->getAbsolutePosition().LowerRightCorner;
		pos.LowerRightCorner = pos.UpperLeftCorner + core::position2d<s32> ( 100, 50 );
		if (getSkin() && getSkin()->getFont())
		{
			pos.LowerRightCorner = pos.UpperLeftCorner + 
				getSkin()->getFont()->getDimension(Hovered->getToolTipText().c_str()) + 
				core::position2di(getSkin()->getSize(EGDS_TEXT_DISTANCE_X)*2, getSkin()->getSize(EGDS_TEXT_DISTANCE_Y)*2);
		}

		ToolTip.Element = addStaticText (	Hovered->getToolTipText().c_str(), pos, true, true, this, -1, true );
		ToolTip.Element->setOverrideColor ( getSkin()->getColor ( EGDC_TOOLTIP ) );

		s32 textHeight = ToolTip.Element->getTextHeight();
		pos = ToolTip.Element->getRelativePosition();
		pos.LowerRightCorner.Y = pos.UpperLeftCorner.Y + textHeight;
		ToolTip.Element->setRelativePosition(pos);

	}

	IGUIElement::OnPostRender ( time );
}

/*
*/
void CGUIEnvironment::updateHoveredElement(core::position2d<s32> mousePos)
{
	IGUIElement* lastHovered = Hovered;

	Hovered = getElementFromPoint(mousePos);

	if (Hovered)
	{
		u32 now = os::Timer::getTime ();

		if (Hovered != this)
			Hovered->grab();

		if (Hovered != lastHovered)
		{
			SEvent event; 
			event.EventType = EET_GUI_EVENT;

			if (lastHovered)
			{
				event.GUIEvent.Caller = lastHovered;
				event.GUIEvent.EventType = EGET_ELEMENT_LEFT;
				lastHovered->OnEvent(event);
			}

			if ( ToolTip.Element )
			{
				ToolTip.Element->remove ();
				ToolTip.Element = 0;
			}
			else
			{
				// boost tooltip generation for relaunch
				if ( now - ToolTip.LastTime < ToolTip.LastTime )
				{
					ToolTip.LastTime += 100;
				}
				else
				{
					ToolTip.LastTime = now;
				}
			}


			event.GUIEvent.Caller = Hovered;
			event.GUIEvent.EventType = EGET_ELEMENT_HOVERED;
			Hovered->OnEvent(event);
		}
	}	

	if (lastHovered && lastHovered != this)
		lastHovered->drop();
}


//! This sets a new event receiver for gui events. Usually you do not have to
//! use this method, it is used by the internal engine.
void CGUIEnvironment::setUserEventReceiver(IEventReceiver* evr)
{
	UserReceiver = evr;
}


//! posts an input event to the environment
bool CGUIEnvironment::postEventFromUser(SEvent event)
{
	switch(event.EventType)
	{
	case EET_GUI_EVENT:
		// hey, why is the user sending gui events..?
		break;
	case EET_MOUSE_INPUT_EVENT:

		// sending input to focus, stopping of focus processed input
		
		if (Focus && Focus->OnEvent(event))
			return true;

		if (!Focus) // focus could have died in last call
		{
			// trying to send input to hovered element
			updateHoveredElement(core::position2d<s32>(event.MouseInput.X, event.MouseInput.Y));

			if (Hovered && Hovered != this)
				return Hovered->OnEvent(event);
		}
		break;
	case EET_KEY_INPUT_EVENT:
		if (Focus && Focus != this)
			return Focus->OnEvent(event);
		break;
	} // end switch

	return false;
}



//! returns the current gui skin
IGUISkin* CGUIEnvironment::getSkin()
{
	return CurrentSkin;
}


//! Sets a new GUI Skin
void CGUIEnvironment::setSkin(IGUISkin* skin)
{
	if (CurrentSkin)
		CurrentSkin->drop();

	CurrentSkin = skin;

	if (CurrentSkin)
		CurrentSkin->grab();
}

//! Creates a new GUI Skin based on a template.
/** \return Returns a pointer to the created skin.
If you no longer need the skin, you should call IGUISkin::drop().
See IUnknown::drop() for more information. */
IGUISkin* CGUIEnvironment::createSkin(EGUI_SKIN_TYPE type)
{
	IGUISkin* skin = new CGUISkin(type, Driver);

	IGUIFont* builtinfont = getBuiltInFont();
	IGUIFontBitmap* bitfont = 0;
	if (builtinfont && builtinfont->getType() == EGFT_BITMAP)
		bitfont = (IGUIFontBitmap*)builtinfont;

	IGUISpriteBank* bank = 0;
	skin->setFont(builtinfont);

	if (bitfont)
		bank = bitfont->getSpriteBank();

	skin->setSpriteBank(bank);

	return skin;
}


//! Returns the default element factory which can create all built in elements
IGUIElementFactory* CGUIEnvironment::getDefaultGUIElementFactory()
{
	return getGUIElementFactory(0);
}

//! Adds an element factory to the gui environment.
/** Use this to extend the gui environment with new element types which it should be
able to create automaticly, for example when loading data from xml files. */
void CGUIEnvironment::registerGUIElementFactory(IGUIElementFactory* factoryToAdd)
{
	if (factoryToAdd)
	{
		factoryToAdd->grab();
		GUIElementFactoryList.push_back(factoryToAdd);
	}
}

//! Returns amount of registered scene node factories.
s32 CGUIEnvironment::getRegisteredGUIElementFactoryCount()
{
	return GUIElementFactoryList.size();
}

//! Returns a scene node factory by index
IGUIElementFactory* CGUIEnvironment::getGUIElementFactory(s32 index)
{
	if (index>=0 && index<(int)GUIElementFactoryList.size())
		return GUIElementFactoryList[index];

	return 0;
}

//! adds a GUI Element using its name
IGUIElement* CGUIEnvironment::addGUIElement(const c8* elementName, IGUIElement* parent)
{
	IGUIElement* node=0;

	if (!parent)
		parent = this;

	for (s32 i=0; i<(int)GUIElementFactoryList.size() && !node; ++i)
		node = GUIElementFactoryList[i]->addGUIElement(elementName, parent);

	return node;
}


//! Saves the current gui into a file.
//! \param filename: Name of the file .
bool CGUIEnvironment::saveGUI(const c8* filename)
{
	io::IWriteFile* file = FileSystem->createAndWriteFile(filename);
	if (!file)
		return false;

	bool ret = saveGUI(file);
	file->drop();
	return ret;
}


//! Saves the current gui into a file.
bool CGUIEnvironment::saveGUI(io::IWriteFile* file)
{
	if (!file)
		return false;

	io::IXMLWriter* writer = FileSystem->createXMLWriter(file);
	if (!writer)
		return false;

	writer->writeXMLHeader();
	writeGUIElement(writer, this);
	writer->drop();

	return true;
}


//! Loads the gui. Note that the current gui is not cleared before.
//! \param filename: Name of the file .
bool CGUIEnvironment::loadGUI(const c8* filename)
{
	io::IReadFile* read = FileSystem->createAndOpenFile(filename);
	if (!read)
	{
		os::Printer::log("Unable to open gui file", filename, ELL_ERROR);
		return false;
	}

	bool ret = loadGUI(read);
	read->drop();

	return ret;
}


//! Loads the gui. Note that the current gui is not cleared before.
bool CGUIEnvironment::loadGUI(io::IReadFile* file)
{
	
	if (!file)
	{
		os::Printer::log("Unable to open gui file", ELL_ERROR);
		return false;
	}

	io::IXMLReader* reader = FileSystem->createXMLReader(file);
	if (!reader)
	{
		os::Printer::log("GUI is not a valid XML file", file->getFileName(), ELL_ERROR);
		return false;
	}
	
	// read file
	while(reader->read())
	{
		readGUIElement(reader, 0);
	}

	// finish up

	reader->drop(); 
	return true; 

}



//! reads an element
void CGUIEnvironment::readGUIElement(io::IXMLReader* reader, IGUIElement* parent)
{
	if (!reader)
		return;

	gui::IGUIElement* node = 0;

	if ((!parent && !wcscmp(IRR_XML_FORMAT_GUI_ENV, reader->getNodeName())) ||
		( parent && !wcscmp(IRR_XML_FORMAT_GUI_ELEMENT, reader->getNodeName())))
	{
		if (parent)
		{
			// find node type and create it
			core::stringc attrName = reader->getAttributeValue(IRR_XML_FORMAT_GUI_ELEMENT_ATTR_TYPE);

			node = addGUIElement(attrName.c_str(), parent);

			if (!node)
				os::Printer::log("Could not create GUI element of unknown type", attrName.c_str());
		}
		else
			node = this; // root
	}

	// read attributes

	while(reader->read())
	{
		bool endreached = false;

		switch (reader->getNodeType())
		{
		case io::EXN_ELEMENT_END:
			if (!wcscmp(IRR_XML_FORMAT_GUI_ELEMENT,  reader->getNodeName()) ||
				!wcscmp(IRR_XML_FORMAT_GUI_ENV, reader->getNodeName()))
			{
				endreached = true;
			}
			break;
		case io::EXN_ELEMENT:
			if (!wcscmp(L"attributes", reader->getNodeName()))
			{
				// read attributes
				io::IAttributes* attr = FileSystem->createEmptyAttributes(Driver);
				attr->read(reader, true);

				if (node)
					node->deserializeAttributes(attr);

				attr->drop();
			}
			else
			if (!wcscmp(IRR_XML_FORMAT_GUI_ELEMENT, reader->getNodeName()) ||
				!wcscmp(IRR_XML_FORMAT_GUI_ENV, reader->getNodeName()))
			{
				readGUIElement(reader, node);
			}
			else
			{
				os::Printer::log("Found unknown element in irrlicht GUI file",
								 core::stringc(reader->getNodeName()).c_str());
			}

			break;
		}

		if (endreached)
			break;
	}
}



//! writes an element
void CGUIEnvironment::writeGUIElement(io::IXMLWriter* writer, IGUIElement* node)
{
	if (!writer || !node )
		return;

	const wchar_t* name = 0;

	// write properties

	io::IAttributes* attr = FileSystem->createEmptyAttributes();
	node->serializeAttributes(attr);

	// all gui elements must have at least one attribute
	// if they have nothing then we ignore them.
	if (attr->getAttributeCount() > 0)
	{
		if (node == this)
		{
			name = IRR_XML_FORMAT_GUI_ENV;
			writer->writeElement(name, false);
		}
		else
		{
			name = IRR_XML_FORMAT_GUI_ELEMENT;
			writer->writeElement(name, false, IRR_XML_FORMAT_GUI_ELEMENT_ATTR_TYPE,
				core::stringw(node->getTypeName()).c_str());
		}

		writer->writeLineBreak();
		writer->writeLineBreak();

		attr->write(writer);
		writer->writeLineBreak();
	}

	// write children

	core::list<IGUIElement*>::Iterator it = node->getChildren().begin();
	for (; it != node->getChildren().end(); ++it)
	{
		if (!(*it)->isSubElement())
			writeGUIElement(writer, (*it));
	}

	// write closing brace if required
	if (attr->getAttributeCount() > 0)
	{
		writer->writeClosingTag(name);
		writer->writeLineBreak();
		writer->writeLineBreak();
	}

	attr->drop();

}


//! Writes attributes of the environment
void CGUIEnvironment::serializeAttributes(io::IAttributes* out, io::SAttributeReadWriteOptions* options)
{
	IGUISkin* skin = getSkin();

	if (skin)
	{
		out->addEnum("Skin",getSkin()->getType(),GUISkinTypeNames);
		skin->serializeAttributes(out, options);
	}
}

//! Reads attributes of the environment
void CGUIEnvironment::deserializeAttributes(io::IAttributes* in, io::SAttributeReadWriteOptions* options)
{

	if (in->existsAttribute("Skin"))
	{
		IGUISkin *skin = getSkin();

		EGUI_SKIN_TYPE t = (EGUI_SKIN_TYPE) in->getAttributeAsEnumeration("Skin",GUISkinTypeNames);
		if ( !skin || t != skin->getType())
		{
			skin = createSkin(t);
			setSkin(skin);
			skin->drop();
		}

		skin = getSkin();

		if (skin)
		{
			skin->deserializeAttributes(in, options);
		}

	}

	RelativeRect = AbsoluteRect = 
			core::rect<s32>(core::position2d<s32>(0,0), 
					Driver ? Driver->getScreenSize() : core::dimension2d<s32>(0,0));

}




//! adds an button. The returned pointer must not be dropped.
IGUIButton* CGUIEnvironment::addButton(const core::rect<s32>& rectangle, IGUIElement* parent, s32 id, const wchar_t* text, const wchar_t *tooltiptext)
{
	IGUIButton* button = new CGUIButton(this, parent ? parent : this, id, rectangle);
	if (text)
		button->setText(text);

	if ( tooltiptext )
		button->setToolTipText ( tooltiptext );

	button->drop();
	return button;
}


//! adds a window. The returned pointer must not be dropped.
IGUIWindow* CGUIEnvironment::addWindow(const core::rect<s32>& rectangle, bool modal, 
		const wchar_t* text, IGUIElement* parent, s32 id)
{
	parent = parent ? parent : this;

	if (modal)
	{
		parent = new CGUIModalScreen(this, parent, -1);
		parent->drop();
	}

	IGUIWindow* win = new CGUIWindow(this, parent, id, rectangle);
	if (text)
		win->setText(text);
	win->drop();

	return win;
}
//! adds a modal screen. The returned pointer must not be dropped.
IGUIElement* CGUIEnvironment::addModalScreen(IGUIElement* parent)
{
	parent = parent ? parent : this;

	IGUIElement *win = new CGUIModalScreen(this, parent, -1);
	win->drop();

	return win;
}

//! Adds a message box.
IGUIWindow* CGUIEnvironment::addMessageBox(const wchar_t* caption, const wchar_t* text,
	bool modal, s32 flag, IGUIElement* parent, s32 id)
{
	if (!CurrentSkin)
		return 0;

	parent = parent ? parent : this;

	core::rect<s32> rect;
	core::dimension2d<s32> screenDim, msgBoxDim;

	screenDim.Width = parent->getAbsolutePosition().getWidth();
	screenDim.Height = parent->getAbsolutePosition().getHeight();
	msgBoxDim.Width = CurrentSkin->getSize(gui::EGDS_MESSAGE_BOX_WIDTH);
	msgBoxDim.Height = CurrentSkin->getSize(gui::EGDS_MESSAGE_BOX_HEIGHT);

	rect.UpperLeftCorner.X = (screenDim.Width - msgBoxDim.Width) / 2;
	rect.UpperLeftCorner.Y = (screenDim.Height - msgBoxDim.Height) / 2;
	rect.LowerRightCorner.X = rect.UpperLeftCorner.X + msgBoxDim.Width;
	rect.LowerRightCorner.Y = rect.UpperLeftCorner.Y + msgBoxDim.Height;

	if (modal)
	{
		parent = new CGUIModalScreen(this, parent, -1);
		parent->drop();
	}

	IGUIWindow* win = new CGUIMessageBox(this, caption, text, flag,
		parent, id, rect);

	win->drop();
	return win;
}


//! adds a scrollbar. The returned pointer must not be dropped.
IGUIScrollBar* CGUIEnvironment::addScrollBar(bool horizontal, const core::rect<s32>& rectangle, IGUIElement* parent, s32 id)
{
	IGUIScrollBar* bar = new CGUIScrollBar(horizontal, this, parent ? parent : this, id, rectangle);
	bar->drop();
	return bar;
}


//! Adds an image element. 
IGUIImage* CGUIEnvironment::addImage(video::ITexture* image, core::position2d<s32> pos,
	bool useAlphaChannel, IGUIElement* parent, s32 id, const wchar_t* text)
{
	core::dimension2d<s32> sz(0,0);
	if (image)
		sz = image->getOriginalSize();

	IGUIImage* img = new CGUIImage(this, parent ? parent : this,
		id, core::rect<s32>(pos, sz));

	if (text)
		img->setText(text);

	if (useAlphaChannel)
		img->setUseAlphaChannel(true);

	if (image)
		img->setImage(image);

	img->drop();
	return img;
}


//! adds an image. The returned pointer must not be dropped.
IGUIImage* CGUIEnvironment::addImage(const core::rect<s32>& rectangle, IGUIElement* parent, s32 id, const wchar_t* text)
{
	IGUIImage* img = new CGUIImage(this, parent ? parent : this,
		id, rectangle);

	if (text)
		img->setText(text);

	img->drop();
	return img;
}


//! adds an mesh viewer. The returned pointer must not be dropped.
IGUIMeshViewer* CGUIEnvironment::addMeshViewer(const core::rect<s32>& rectangle, IGUIElement* parent, s32 id, const wchar_t* text)
{
	IGUIMeshViewer* v = new CGUIMeshViewer(this, parent ? parent : this,
		id, rectangle);

	if (text)
		v->setText(text);

	v->drop();
	return v;
}


//! adds a checkbox
IGUICheckBox* CGUIEnvironment::addCheckBox(bool checked, const core::rect<s32>& rectangle, IGUIElement* parent, s32 id, const wchar_t* text)
{
	IGUICheckBox* b = new CGUICheckBox(checked, this, 
		parent ? parent : this , id , rectangle);

	if (text)
		b->setText(text);

	b->drop();
	return b;
}



//! adds a list box
IGUIListBox* CGUIEnvironment::addListBox(const core::rect<s32>& rectangle, 
										 IGUIElement* parent, s32 id,
										 bool drawBackground)
{
	IGUIListBox* b = new CGUIListBox(this, parent ? parent : this, id, rectangle,
		true, drawBackground, false);

	if (getBuiltInFont() && getBuiltInFont()->getType() == EGFT_BITMAP)
		b->setSpriteBank( ((IGUIFontBitmap*)getBuiltInFont())->getSpriteBank());

	b->drop();
	return b;

}



//! adds a file open dialog. The returned pointer must not be dropped.
IGUIFileOpenDialog* CGUIEnvironment::addFileOpenDialog(const wchar_t* title, 
													   bool modal,
													   IGUIElement* parent, s32 id)
{
	parent = parent ? parent : this;

	if (modal)
	{
		parent = new CGUIModalScreen(this, parent, -1);
		parent->drop();
	}

	IGUIFileOpenDialog* d = new CGUIFileOpenDialog(title, this, parent, id);

	d->drop();
	return d;
}


//! adds a color select dialog. The returned pointer must not be dropped.
IGUIColorSelectDialog* CGUIEnvironment::addColorSelectDialog(const wchar_t* title, 
													   bool modal,
													   IGUIElement* parent, s32 id)
{
	parent = parent ? parent : this;

	if (modal)
	{
		parent = new CGUIModalScreen(this, parent, -1);
		parent->drop();
	}

	IGUIColorSelectDialog* d = new CGUIColorSelectDialog( title,
			this, parent, id);

	d->drop();
	return d;
}



//! adds a static text. The returned pointer must not be dropped.
IGUIStaticText* CGUIEnvironment::addStaticText(const wchar_t* text,  
											   const core::rect<s32>& rectangle,
											   bool border,
											   bool wordWrap,
											   IGUIElement* parent, s32 id,
											   bool background)
{
	IGUIStaticText* d = new CGUIStaticText(text, border, this,
			parent ? parent : this, id, rectangle, background);

	d->setWordWrap(wordWrap);
	d->drop();

	return d;
}



//! Adds an edit box. The returned pointer must not be dropped.
IGUIEditBox* CGUIEnvironment::addEditBox(const wchar_t* text, 
										 const core::rect<s32>& rectangle, 
										 bool border, IGUIElement* parent,
										 s32 id)
{
	IGUIEditBox* d = new CGUIEditBox(text, border, this,
			parent ? parent : this, id, rectangle, Operator);

	d->drop();
	return d;
}

//! Adds a tab control to the environment.
IGUITabControl* CGUIEnvironment::addTabControl(const core::rect<s32>& rectangle,
	IGUIElement* parent, bool fillbackground, bool border, s32 id)
{
	IGUITabControl* t = new CGUITabControl(this, parent ? parent : this,
		rectangle, fillbackground, border, id);
	t->drop();
	return t;
}


//! Adds tab to the environment. 
IGUITab* CGUIEnvironment::addTab(const core::rect<s32>& rectangle,
	IGUIElement* parent, s32 id)
{
	IGUITab* t = new CGUITab(-1, this, parent ? parent : this,
		rectangle, id);
	t->drop();
	return t;
}


//! Adds a context menu to the environment.
IGUIContextMenu* CGUIEnvironment::addContextMenu(const core::rect<s32>& rectangle,
	IGUIElement* parent, s32 id)
{
	IGUIContextMenu* c = new CGUIContextMenu(this, 
		parent ? parent : this, id, rectangle, true);
	c->drop();
	return c;
}


//! Adds a menu to the environment.
IGUIContextMenu* CGUIEnvironment::addMenu(IGUIElement* parent, s32 id)
{
	if (!parent)
		parent = this;

	IGUIContextMenu* c = new CGUIMenu(this, 
		parent, id, core::rect<s32>(0,0,
				parent->getAbsolutePosition().getWidth(),
				parent->getAbsolutePosition().getHeight()));

	c->drop();
	return c;
}

//! Adds a toolbar to the environment. It is like a menu is always placed on top
//! in its parent, and contains buttons.
IGUIToolBar* CGUIEnvironment::addToolBar(IGUIElement* parent, s32 id)
{
	if (!parent)
		parent = this;

	IGUIToolBar* b = new CGUIToolBar(this, parent, id, core::rect<s32>(0,0,10,10));
	b->drop();
	return b;
}



//! Adds an element for fading in or out.
IGUIInOutFader* CGUIEnvironment::addInOutFader(const core::rect<s32>* rectangle, IGUIElement* parent, s32 id)
{
	core::rect<s32> rect;

	if (rectangle)
		rect = *rectangle;
	else
		if (Driver)
			rect = core::rect<s32>(core::position2d<s32>(0,0), Driver->getScreenSize());

	if (!parent)
		parent = this;

	IGUIInOutFader* fader = new CGUIInOutFader(this, parent, id, rect);
	fader->drop();
	return fader;
}


//! Adds a combo box to the environment.
IGUIComboBox* CGUIEnvironment::addComboBox(const core::rect<s32>& rectangle,
	IGUIElement* parent, s32 id)
{
	IGUIComboBox* t = new CGUIComboBox(this, parent ? parent : this,
		id, rectangle);
	t->drop();
	return t;

}



//! returns the font
IGUIFont* CGUIEnvironment::getFont(const c8* filename)
{
	// search existing font

	SFont f;
	IGUIFont* ifont=0;
	if (!filename)
		f.Filename = "";
	else
		f.Filename = filename;

	f.Filename.make_lower();

	s32 index = Fonts.binary_search(f);
	if (index != -1)
		return Fonts[index].Font;

	// font doesn't exist, attempt to load it

	// does the file exist?

	if (!FileSystem->existFile(f.Filename.c_str()))
	{
		os::Printer::log("Could not load font because the file does not exist", f.Filename.c_str(), ELL_ERROR);
		return 0;
	}

	io::IXMLReader *xml = FileSystem->createXMLReader(f.Filename.c_str());
	if (xml)
	{
		// this is an XML font, but we need to know what type
		EGUI_FONT_TYPE t = EGFT_CUSTOM;

		bool found=false;
		while(xml->read() && !found)
		{
			if (xml->getNodeType() == io::EXN_ELEMENT)
			{
				if (core::stringw(L"font") == xml->getNodeName())
				{
					if (core::stringw(L"vector") == xml->getAttributeValue(L"type"))
					{
						t = EGFT_VECTOR;
						found=true;
					}
					else if (core::stringw(L"bitmap") == xml->getAttributeValue(L"type"))
					{
						t = EGFT_BITMAP;
						found=true;
					}
					else found=true;
				}
			}
		}

		if (t==EGFT_BITMAP)
		{
			CGUIFont* font = new CGUIFont(this, f.Filename.c_str());
			ifont = (IGUIFont*)font;
			if (!font->load(xml))
			{
				font->drop();
				font  = 0;
				ifont = 0;
			}
		}
		else if (t==EGFT_VECTOR)
		{
			// todo: vector fonts

			//CGUIFontVector* font = new CGUIFontVector(Driver);
			//ifont = (IGUIFont*)font;
			//if (!font->load(xml))
		}
		xml->drop();
	}


	if (!ifont)
	{

		CGUIFont* font = new CGUIFont(this, f.Filename.c_str());
		ifont = (IGUIFont*)font;
		if (!font->load(f.Filename.c_str()))
		{
			font->drop();
			return 0;
		}
	}

	// add to fonts.

	f.Font = ifont;
	Fonts.push_back(f);

	return ifont;
}

IGUISpriteBank* CGUIEnvironment::getSpriteBank(const c8* filename)
{
	// search for the file name

	SSpriteBank b;
	if (!filename)
		b.Filename = "";
	else
		b.Filename = filename;

	b.Filename.make_lower();

	s32 index = Banks.binary_search(b);
	if (index != -1)
		return Banks[index].Bank;

	// we don't have this sprite bank, we should load it

	if (!FileSystem->existFile(b.Filename.c_str()))
	{
		os::Printer::log("Could not load sprite bank because the file does not exist", filename, ELL_ERROR);
		return 0;
	}

	// todo: load it!

	return 0;

}

IGUISpriteBank* CGUIEnvironment::addEmptySpriteBank(const c8 *name)
{
	// no duplicate names allowed

	SSpriteBank b;
	if (!name)
		b.Filename = "";
	else
		b.Filename = name;

	s32 index = Banks.binary_search(b);
	if (index != -1)
		return 0; 


	// create a new sprite bank

	b.Bank = new CGUISpriteBank(this);

	Banks.push_back(b);

	return b.Bank;

}

//! returns default font
IGUIFont* CGUIEnvironment::getBuiltInFont()
{
	if (Fonts.empty())
		return 0;

	return Fonts[0].Font;
}



//! Returns the root gui element. 
IGUIElement* CGUIEnvironment::getRootGUIElement()
{
	return this;
}



//! creates an GUI Environment
IGUIEnvironment* createGUIEnvironment(io::IFileSystem* fs, video::IVideoDriver* Driver,
									  IOSOperator* op)
{
	return new CGUIEnvironment(fs, Driver, op);
}


} // end namespace gui
} // end namespace irr

