/* -*- Mode: IDL; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 * 
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 * 
 * The Original Code is the Mozilla browser.
 * 
 * The Initial Developer of the Original Code is Netscape
 * Communications, Inc.  Portions created by Netscape are
 * Copyright (C) 1999, Mozilla.  All Rights Reserved.
 * 
 * Contributor(s):
 *   Conrad Carlen <ccarlen@netscape.com>
 */

#include "CWebBrowserCMAttachment.h"


//*****************************************************************************
//  CWebBrowserChrome
//
//  This override is needed because LCMAttachment intercepts mouse clicks, tracks
//  the click, and handles it if the click is on a context menu. We don't want so
//  complete a solution. In our case, mozilla handles the click, determines that
//  it's a context menu click and then calls nsIContextMenuListener::OnShowContextMenu.
//  This override allows DoContextMenuClick to be called after the click.
//
//*****************************************************************************

CWebBrowserCMAttachment::CWebBrowserCMAttachment()
{
}

CWebBrowserCMAttachment::CWebBrowserCMAttachment(LCommander*    inTarget,
	                                             PaneIDT		inTargetPaneID)
	: LCMAttachment(inTarget, inTargetPaneID)
{
}

CWebBrowserCMAttachment::CWebBrowserCMAttachment(LStream*	inStream)
	: LCMAttachment(inStream)
{
}

CWebBrowserCMAttachment::~CWebBrowserCMAttachment()
{
}


void CWebBrowserCMAttachment::ExecuteSelf(MessageT	inMessage,
	                                      void*		ioParam)
{
	SetExecuteHost(true);
	
		// Ensure the CMM is installed and initialized. If not, just
		// return quietly.
	if (not mCMMInitialized) {
		return;
	}

		// Dispatch to the various messages we handle.
		
	if (inMessage == msg_AdjustCursor) {
	
			// Only display the cursor if the control key is depressed
		if (((static_cast<EventRecord*>(ioParam))->modifiers & controlKey) != 0) {
		
				//+++ This needs to be updated to use kThemeContextualMenuArrowCursor,
				//+++ but UCursor needs to be updated to be Theme-savvy first.
			UCursor::SetTheCursor(mCMMCursorID);
			SetExecuteHost(false);
		}
		
	} else if (inMessage == msg_Event) {
	
	} else if (inMessage == msg_Click) {

			// It's a click, but is it a CMM click?
		if (::IsShowContextualMenuClick(
				&(static_cast<SMouseDownEvent*>(ioParam))->macEvent)) {

			// It is; let's handle it. However, first we must switch the
			// target to our object, and update the port.
			
			LCommander*	target = FindCommandTarget();
			if ((target != nil) && LCommander::SwitchTarget(target)) {
				LPane*	ownerAsPane = dynamic_cast<LPane*>(mOwnerHost);				
				if (ownerAsPane != nil) {
					ownerAsPane->UpdatePort();
				}
			}
		}
	}
}


void CWebBrowserCMAttachment::SetCommandList(SInt16 mcmdResID)
{
    Handle mcmdRes = ::GetResource(ResType_MenuCommands, mcmdResID);
    ThrowIfResFail_(mcmdRes);
    ::DetachResource(mcmdRes);  // LHandleStream takes ownership and uses DisposeHandle()
    LHandleStream inStream(mcmdRes);
    
    mCommandList.RemoveItemsAt(mCommandList.GetCount(), LArray::index_First);
    
	UInt16 numCommands;
	inStream >> numCommands;
	
	for (SInt16 i = 1; i <= numCommands; i++) {
		CommandT theCommand;
		inStream >> theCommand;
		mCommandList.AddItem(theCommand);
	}
}


void CWebBrowserCMAttachment::DoContextMenuClick(const EventRecord&	inMacEvent)
{
    DoCMMClick(inMacEvent);
}

