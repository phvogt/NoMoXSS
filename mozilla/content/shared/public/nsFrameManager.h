/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 * vim:cindent:ts=2:et:sw=2:
 *
 * The contents of this file are subject to the Netscape Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s):
 *   Pierre Phaneuf <pp@ludusdesign.com>
 *   Brian Ryner <bryner@brianryner.com>
 *
 * This Original Code has been modified by IBM Corporation. Modifications made
 * by IBM described herein are Copyright (c) International Business Machines
 * Corporation, 2000. Modifications to Mozilla code or documentation identified
 * per MPL Section 3.3
 *
 * Date             Modified by     Description of modification
 * 04/20/2000       IBM Corp.      OS/2 VisualAge build.
 */

#ifndef _nsFrameManager_h_
#define _nsFrameManager_h_

#include "nsIFrame.h"
#include "nsIStatefulFrame.h"
#include "nsChangeHint.h"
#include "nsFrameManagerBase.h"

// Option flags for GetFrameProperty() member function
#define NS_IFRAME_MGR_REMOVE_PROP   0x0001

/**
 * Frame manager interface. The frame manager serves two purposes:
 * <li>provides a service for mapping from content to frame and from
 * out-of-flow frame to placeholder frame.
 * <li>handles structural modifications to the frame model. If the frame model
 * lock can be acquired, then the changes are processed immediately; otherwise,
 * they're queued and processed later.
 *
 * Do not add virtual methods to this class, or bryner will punish you.
 */

class nsFrameManager : public nsFrameManagerBase
{
public:
  nsFrameManager() NS_HIDDEN;
  ~nsFrameManager() NS_HIDDEN;

  void* operator new(size_t aSize, nsIPresShell* aHost) {
    NS_ASSERTION(aSize == sizeof(nsFrameManager), "Unexpected subclass");
    NS_ASSERTION(aSize == sizeof(nsFrameManagerBase),
                 "Superclass/subclass mismatch");
    return aHost->FrameManager();
  }

  // Initialization
  NS_HIDDEN_(nsresult) Init(nsIPresShell* aPresShell, nsStyleSet* aStyleSet);

  /*
   * After Destroy is called, it is an error to call any FrameManager methods.
   * Destroy should be called when the frame tree managed by the frame
   * manager is no longer being displayed.
   */
  NS_HIDDEN_(void) Destroy();

  /*
   * Gets and sets the root frame (typically the viewport). The lifetime of the
   * root frame is controlled by the frame manager. When the frame manager is
   * destroyed, it destroys the entire frame hierarchy.
   */
  NS_HIDDEN_(nsIFrame*) GetRootFrame() { return mRootFrame; }
  NS_HIDDEN_(void)      SetRootFrame(nsIFrame* aRootFrame)
  {
    NS_ASSERTION(!mRootFrame, "already have a root frame");
    mRootFrame = aRootFrame;
  }

  /*
   * Get the canvas frame, searching from the root frame down.
   * The canvas frame may or may not exist, so this may return null.
   */
  NS_HIDDEN_(nsIFrame*) GetCanvasFrame();

  // Primary frame functions
  NS_HIDDEN_(nsIFrame*) GetPrimaryFrameFor(nsIContent* aContent);
  NS_HIDDEN_(nsresult)  SetPrimaryFrameFor(nsIContent* aContent,
                                           nsIFrame* aPrimaryFrame);
  NS_HIDDEN_(void)      ClearPrimaryFrameMap();

  // Placeholder frame functions
  NS_HIDDEN_(nsPlaceholderFrame*) GetPlaceholderFrameFor(nsIFrame* aFrame);
  NS_HIDDEN_(nsresult)
    RegisterPlaceholderFrame(nsPlaceholderFrame* aPlaceholderFrame);

  NS_HIDDEN_(void)
    UnregisterPlaceholderFrame(nsPlaceholderFrame* aPlaceholderFrame);

  NS_HIDDEN_(void)      ClearPlaceholderFrameMap();

  // Mapping undisplayed content
  NS_HIDDEN_(nsStyleContext*) GetUndisplayedContent(nsIContent* aContent);
  NS_HIDDEN_(void) SetUndisplayedContent(nsIContent* aContent,
                                         nsStyleContext* aStyleContext);
  NS_HIDDEN_(void) ChangeUndisplayedContent(nsIContent* aContent,
                                            nsStyleContext* aStyleContext);
  NS_HIDDEN_(void) ClearUndisplayedContentIn(nsIContent* aContent,
                                             nsIContent* aParentContent);
  NS_HIDDEN_(void) ClearAllUndisplayedContentIn(nsIContent* aParentContent);
  NS_HIDDEN_(void) ClearUndisplayedContentMap();

  // Functions for manipulating the frame model
  NS_HIDDEN_(nsresult) AppendFrames(nsIFrame*       aParentFrame,
                                    nsIAtom*        aListName,
                                    nsIFrame*       aFrameList)
  {
    return aParentFrame->AppendFrames(GetPresContext(), *GetPresShell(),
                                      aListName, aFrameList);
  }

  NS_HIDDEN_(nsresult) InsertFrames(nsIFrame*       aParentFrame,
                                    nsIAtom*        aListName,
                                    nsIFrame*       aPrevFrame,
                                    nsIFrame*       aFrameList);

  NS_HIDDEN_(nsresult) RemoveFrame(nsIFrame*       aParentFrame,
                                   nsIAtom*        aListName,
                                   nsIFrame*       aOldFrame);

  NS_HIDDEN_(nsresult) ReplaceFrame(nsIFrame*       aParentFrame,
                                    nsIAtom*        aListName,
                                    nsIFrame*       aOldFrame,
                                    nsIFrame*       aNewFrame)
  {
    return aParentFrame->ReplaceFrame(GetPresContext(), *GetPresShell(),
                                      aListName, aOldFrame, aNewFrame);
  }

  // Notification that we were unable to render a replaced element
  NS_HIDDEN_(nsresult) CantRenderReplacedElement(nsIFrame* aFrame);

  /*
   * Notification that a frame is about to be destroyed. This allows any
   * outstanding references to the frame to be cleaned up.
   */
  NS_HIDDEN_(void)     NotifyDestroyingFrame(nsIFrame* aFrame);

  /*
   * Reparent the style contexts of this frame subtree to live under the new
   * given parent style context.  The StyleContextParent of aFrame should be
   * changed _before_ this method is called, so that style tree verification
   * can take place correctly.
   */
  NS_HIDDEN_(nsresult) ReParentStyleContext(nsIFrame* aFrame, 
                                            nsStyleContext* aNewParentContext);

  /*
   * Re-resolve the style contexts for a frame tree.  Returns the top-level
   * change hint resulting from the style re-resolution.
   */
  NS_HIDDEN_(nsChangeHint)
    ComputeStyleChangeFor(nsIFrame* aFrame,
                          nsStyleChangeList* aChangeList,
                          nsChangeHint aMinChange);

  // Determine whether an attribute affects style
  NS_HIDDEN_(nsReStyleHint) HasAttributeDependentStyle(nsIContent *aContent,
                                                       nsIAtom *aAttribute,
                                                       PRInt32 aModType);

  /*
   * Capture/restore frame state for the frame subtree rooted at aFrame.
   * aState is the document state storage object onto which each frame
   * stores its state.
   */

  NS_HIDDEN_(void) CaptureFrameState(nsIFrame*              aFrame,
                                     nsILayoutHistoryState* aState);

  NS_HIDDEN_(void) RestoreFrameState(nsIFrame*              aFrame,
                                     nsILayoutHistoryState* aState);

  /*
   * Add/restore state for one frame
   * (special, global type, like scroll position)
   */
  NS_HIDDEN_(void) CaptureFrameStateFor(nsIFrame*              aFrame,
                                        nsILayoutHistoryState* aState,
                                        nsIStatefulFrame::SpecialStateID aID =
                                                      nsIStatefulFrame::eNoID);

  NS_HIDDEN_(void) RestoreFrameStateFor(nsIFrame*              aFrame,
                                        nsILayoutHistoryState* aState,
                                        nsIStatefulFrame::SpecialStateID aID =
                                                      nsIStatefulFrame::eNoID);

  /**
   * Gets a property value for a given frame.
   *
   * @param   aFrame          the frame with the property
   * @param   aPropertyName   property name as an atom
   * @param   aOptions        optional flags
   *                            NS_IFRAME_MGR_REMOVE_PROP removes the property
   * @param   aResult         NS_OK if the property is set,
   *                            NS_IFRAME_MGR_PROP_NOT_THERE is it is not set
   * @param   aPropertyValue  the property value or 0 if the
                                property is not set
   * @return  The property value or 0 if the property is not set
   */

  NS_HIDDEN_(void*) GetFrameProperty(const nsIFrame* aFrame,
                                     nsIAtom*        aPropertyName,
                                     PRUint32        aOptions,
                                     nsresult*       aResult = nsnull);

  /**
   * Sets the property value for a given frame.
   *
   * A frame may only have one property value at a time for a given property
   * name. The existing property value (if there is one) is overwritten, and
   * the old value destroyed
   *
   * @param   aFrame            the frame to set the property on
   * @param   aPropertyName     property name as an atom
   * @param   aPropertyValue    the property value
   * @param   aPropertyDtorFunc when setting a property you can specify the
   *                            dtor function (can be NULL) that will be used
   *                            to destroy the property value. There can be
   *                            only one dtor function for a given property
   *                            name
   * @return  NS_OK if successful,
   *          NS_IFRAME_MGR_PROP_OVERWRITTEN if there is an existing property
   *            value that was overwritten,
   *          NS_ERROR_INVALID_ARG if the dtor function does not match the
   *            existing dtor function
   */

  NS_HIDDEN_(nsresult) SetFrameProperty(const nsIFrame*         aFrame,
                                        nsIAtom*                aPropertyName,
                                        void*                   aPropertyValue,
                                        NSFramePropertyDtorFunc aPropDtorFunc);

  /**
   * Removes a property and destroys its property value by calling the dtor
   * function associated with the property name.
   *
   * When a frame is destroyed any remaining properties are automatically
   * removed.
   *
   * @param   aFrame          the frame to set the property on
   * @param   aPropertyName   property name as an atom
   * @return  NS_OK if the property is successfully removed,
   *          NS_IFRAME_MGR_PROP_NOT_THERE if the property is not set
   */

  NS_HIDDEN_(nsresult) RemoveFrameProperty(const nsIFrame* aFrame,
                                           nsIAtom*        aPropertyName);

#ifdef NS_DEBUG
  /**
   * DEBUG ONLY method to verify integrity of style tree versus frame tree
   */
  NS_HIDDEN_(void) DebugVerifyStyleTree(nsIFrame* aFrame);
#endif

private:

  friend struct CantRenderReplacedElementEvent;

  NS_HIDDEN_(nsIPresShell*) GetPresShell() const { return mPresShell; }
  NS_HIDDEN_(nsIPresContext*) GetPresContext() const {
    return mPresShell->GetPresContext();
  }

  NS_HIDDEN_(nsChangeHint)
    ReResolveStyleContext(nsIPresContext    *aPresContext,
                          nsIFrame          *aFrame,
                          nsIContent        *aParentContent,
                          nsStyleChangeList *aChangeList, 
                          nsChangeHint       aMinChange);

  NS_HIDDEN_(nsresult) RevokePostedEvents();
  NS_HIDDEN_(CantRenderReplacedElementEvent**)
    FindPostedEventFor(nsIFrame* aFrame);

  NS_HIDDEN_(void) DequeuePostedEventFor(nsIFrame* aFrame);
  NS_HIDDEN_(void) DestroyPropertyList(nsIPresContext* aPresContext);
  NS_HIDDEN_(PropertyList*) GetPropertyListFor(nsIAtom* aPropertyName) const;
  NS_HIDDEN_(void) RemoveAllPropertiesFor(nsIPresContext *aPresContext,
                                          nsIFrame       *aFrame);

  static NS_HIDDEN_(void)
    HandlePLEvent(CantRenderReplacedElementEvent* aEvent);

  static NS_HIDDEN_(void)
    DestroyPLEvent(CantRenderReplacedElementEvent* aEvent);
};

#endif
