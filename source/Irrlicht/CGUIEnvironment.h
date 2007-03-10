// Copyright (C) 2002-2007 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __C_GUI_ENVIRNMENT_H_INCLUDED__
#define __C_GUI_ENVIRNMENT_H_INCLUDED__

#include "IGUIEnvironment.h"
#include "IGUIElement.h"
#include "irrArray.h"
#include "IFileSystem.h"
#include "IOSOperator.h"

namespace irr
{
namespace io
{
	class IXMLWriter;
}
namespace gui
{

class CGUIEnvironment : public IGUIEnvironment, public IGUIElement
{
public:

	//! constructor
	CGUIEnvironment(io::IFileSystem* fs, video::IVideoDriver* driver, IOSOperator* op);

	//! destructor
	virtual ~CGUIEnvironment();

	//! draws all gui elements
	virtual void drawAll();

	//! returns the current video driver
	virtual video::IVideoDriver* getVideoDriver();

	//! returns the current video driver
	virtual io::IFileSystem* getFileSystem();

	//! posts an input event to the environment
	virtual bool postEventFromUser(SEvent event);

	//! This sets a new event receiver for gui events. Usually you do not have to
	//! use this method, it is used by the internal engine.
	virtual void setUserEventReceiver(IEventReceiver* evr);

	//! removes all elements from the environment
	virtual void clear();

	//! called if an event happened.
	virtual bool OnEvent(SEvent event);

	//! returns the current gui skin
	virtual IGUISkin* getSkin();

	//! Sets a new GUI Skin
	virtual void setSkin(IGUISkin* skin);

	//! Creates a new GUI Skin based on a template.
	/** \return Returns a pointer to the created skin.
	If you no longer need the skin, you should call IGUISkin::drop().
	See IUnknown::drop() for more information. */
	virtual IGUISkin* createSkin(EGUI_SKIN_TYPE type);

	//! returns the font
	virtual IGUIFont* getFont(const c8* filename);

	//! returns the sprite bank
	virtual IGUISpriteBank* getSpriteBank(const c8* filename);

	//! returns the sprite bank
	virtual IGUISpriteBank* addEmptySpriteBank(const c8* name);

	//! adds an button. The returned pointer must not be dropped.
	virtual IGUIButton* addButton(const core::rect<s32>& rectangle, IGUIElement* parent=0, s32 id=-1, const wchar_t* text=0,const wchar_t* tooltiptext = 0);

	//! adds a window. The returned pointer must not be dropped.
	virtual IGUIWindow* addWindow(const core::rect<s32>& rectangle, bool modal = false, 
		const wchar_t* text=0, IGUIElement* parent=0, s32 id=-1);

	//! adds a modal screen. The returned pointer must not be dropped.
	virtual IGUIElement* addModalScreen(IGUIElement* parent);

	//! Adds a message box.
	virtual IGUIWindow* addMessageBox(const wchar_t* caption, const wchar_t* text=0,
		bool modal = true, s32 flag = EMBF_OK, IGUIElement* parent=0, s32 id=-1);

	//! adds a scrollbar. The returned pointer must not be dropped.
	virtual IGUIScrollBar* addScrollBar(bool horizontal, const core::rect<s32>& rectangle, IGUIElement* parent=0, s32 id=-1);

	//! Adds an image element. 
	virtual IGUIImage* addImage(video::ITexture* image, core::position2d<s32> pos,
		bool useAlphaChannel=true, IGUIElement* parent=0, s32 id=-1, const wchar_t* text=0);

	//! adds an image. The returned pointer must not be dropped.
	virtual IGUIImage* addImage(const core::rect<s32>& rectangle, 
		IGUIElement* parent=0, s32 id=-1, const wchar_t* text=0);

	//! adds a checkbox
	virtual IGUICheckBox* addCheckBox(bool checked, const core::rect<s32>& rectangle, IGUIElement* parent=0, s32 id=-1, const wchar_t* text=0);

	//! adds a list box
	virtual IGUIListBox* addListBox(const core::rect<s32>& rectangle,
		IGUIElement* parent=0, s32 id=-1, bool drawBackground=false);

	//! adds an mesh viewer. The returned pointer must not be dropped.
	virtual IGUIMeshViewer* addMeshViewer(const core::rect<s32>& rectangle, IGUIElement* parent=0, s32 id=-1, const wchar_t* text=0);

	//! Adds a file open dialog.
	virtual IGUIFileOpenDialog* addFileOpenDialog(const wchar_t* title = 0, bool modal=true, IGUIElement* parent=0, s32 id=-1);
	virtual IGUIColorSelectDialog* addColorSelectDialog(const wchar_t* title = 0, bool modal=true, IGUIElement* parent=0, s32 id=-1);

	//! adds a static text. The returned pointer must not be dropped.
	virtual IGUIStaticText* addStaticText(const wchar_t* text, const core::rect<s32>& rectangle,
		bool border=false, bool wordWrap=true, IGUIElement* parent=0, s32 id=-1, bool drawBackground = false);

	//! Adds an edit box. The returned pointer must not be dropped.
	virtual IGUIEditBox* addEditBox(const wchar_t* text, const core::rect<s32>& rectangle, 
		bool border=false, IGUIElement* parent=0, s32 id=-1);

	//! Adds a tab control to the environment.
	virtual IGUITabControl* addTabControl(const core::rect<s32>& rectangle,
		IGUIElement* parent=0, bool fillbackground=false, bool border=true, s32 id=-1);

	//! Adds tab to the environment. 
	virtual IGUITab* addTab(const core::rect<s32>& rectangle,
		IGUIElement* parent=0, s32 id=-1);

	//! Adds a context menu to the environment.
	virtual IGUIContextMenu* addContextMenu(const core::rect<s32>& rectangle,
		IGUIElement* parent=0, s32 id=-1);

	//! Adds a menu to the environment.
	virtual IGUIContextMenu* addMenu(IGUIElement* parent=0, s32 id=-1);

	//! Adds a toolbar to the environment. It is like a menu is always placed on top
	//! in its parent, and contains buttons.
	virtual IGUIToolBar* addToolBar(IGUIElement* parent=0, s32 id=-1);

	//! Adds a combo box to the environment.
	virtual IGUIComboBox* addComboBox(const core::rect<s32>& rectangle,
		IGUIElement* parent=0, s32 id=-1);

	//! sets the focus to an element
	virtual void setFocus(IGUIElement* element);

	//! removes the focus from an element
	virtual void removeFocus(IGUIElement* element);

	//! Returns if the element has focus
	virtual bool hasFocus(IGUIElement* element);

	//! Returns the element with the focus
	virtual IGUIElement* getFocus();

	//! returns default font
	virtual IGUIFont* getBuiltInFont();

	//! Adds an element for fading in or out.
	virtual IGUIInOutFader* addInOutFader(const core::rect<s32>* rectangle=0, IGUIElement* parent=0, s32 id=-1);

	//! Returns the root gui element. 
	virtual IGUIElement* getRootGUIElement();

	virtual void OnPostRender( u32 time );

	//! Returns the default element factory which can create all built in elements
	virtual IGUIElementFactory* getDefaultGUIElementFactory();

	//! Adds an element factory to the gui environment.
	/** Use this to extend the gui environment with new element types which it should be
	able to create automaticly, for example when loading data from xml files. */
	virtual void registerGUIElementFactory(IGUIElementFactory* factoryToAdd);

	//! Returns amount of registered scene node factories.
	virtual s32 getRegisteredGUIElementFactoryCount();

	//! Returns a scene node factory by index
	virtual IGUIElementFactory* getGUIElementFactory(s32 index);

	//! Adds a GUI Element by its name
	virtual IGUIElement* addGUIElement(const c8* elementName, IGUIElement* parent=0);

	//! Saves the current gui into a file.
	//! \param filename: Name of the file.
	virtual bool saveGUI(const c8* filename);

	//! Saves the current gui into a file.
	virtual bool saveGUI(io::IWriteFile* file);

	//! Loads the gui. Note that the current gui is not cleared before.
	//! \param filename: Name of the file .
	virtual bool loadGUI(const c8* filename);

	//! Loads the gui. Note that the current gui is not cleared before.
	virtual bool loadGUI(io::IReadFile* file);	

	//! Writes attributes of the environment
	virtual void serializeAttributes(io::IAttributes* out, io::SAttributeReadWriteOptions* options=0);

	//! Reads attributes of the environment.
	virtual void deserializeAttributes(io::IAttributes* in, io::SAttributeReadWriteOptions* options=0);

	//! writes an element
	virtual void writeGUIElement(io::IXMLWriter* writer, IGUIElement* node);

	//! reads an element
	virtual void readGUIElement(io::IXMLReader* reader, IGUIElement* parent);


private:

	struct SFont
	{
		core::stringc Filename;
		IGUIFont* Font;

		bool operator < (const SFont& other) const
		{
			return (Filename < other.Filename);
		}
	};

	struct SSpriteBank
	{
		core::stringc Filename;
		IGUISpriteBank* Bank;

		bool operator < (const SSpriteBank& other) const
		{
			return (Filename < other.Filename);
		}
	};

	struct SToolTip
	{
		u32 LastTime;
		u32 LaunchTime;
		IGUIStaticText* Element;
	};
	SToolTip ToolTip;
	void updateHoveredElement(core::position2d<s32> mousePos);

	void loadBuiltInFont();

	core::array<IGUIElementFactory*> GUIElementFactoryList;

	core::array<SFont> Fonts;
	core::array<SSpriteBank> Banks;
	video::IVideoDriver* Driver;
	IGUIElement* Hovered;
	IGUIElement* Focus;
	IGUISkin* CurrentSkin;
	io::IFileSystem* FileSystem;
	IEventReceiver* UserReceiver;
	IOSOperator* Operator;
};

} // end namespace gui
} // end namespace irr

#endif

