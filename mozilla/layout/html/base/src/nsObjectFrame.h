/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: NPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is 
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or 
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the NPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the NPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */
#ifndef nsObjectFrame_h___
#define nsObjectFrame_h___

#include "nsHTMLParts.h"
#include "nsHTMLContainerFrame.h"
#include "nsIPresContext.h"
#include "nsIPresShell.h"
#include "nsIPluginHost.h"
#include "nsplugin.h"
#include "nsIWidget.h"
#include "nsIObjectFrame.h"

#ifdef ACCESSIBILITY
class nsIAccessible;
#endif

class nsPluginInstanceOwner;

#define nsObjectFrameSuper nsHTMLContainerFrame

class nsObjectFrame : public nsObjectFrameSuper, public nsIObjectFrame {
public:
  // nsISupports 
  NS_IMETHOD QueryInterface(const nsIID& aIID, void** aInstancePtr);

  NS_IMETHOD SetInitialChildList(nsIPresContext* aPresContext,
                                 nsIAtom*        aListName,
                                 nsIFrame*       aChildList);
  NS_IMETHOD Init(nsIPresContext*  aPresContext,
                  nsIContent*      aContent,
                  nsIFrame*        aParent,
                  nsStyleContext*  aContext,
                  nsIFrame*        aPrevInFlow);
  NS_IMETHOD Reflow(nsIPresContext*          aPresContext,
                    nsHTMLReflowMetrics&     aDesiredSize,
                    const nsHTMLReflowState& aReflowState,
                    nsReflowStatus&          aStatus);
  NS_IMETHOD DidReflow(nsIPresContext*           aPresContext,
                       const nsHTMLReflowState*  aReflowState,
                       nsDidReflowStatus         aStatus);
  NS_IMETHOD Paint(nsIPresContext*      aPresContext,
                   nsIRenderingContext& aRenderingContext,
                   const nsRect&        aDirtyRect,
                   nsFramePaintLayer    aWhichLayer,
                   PRUint32             aFlags = 0);

  NS_IMETHOD  HandleEvent(nsIPresContext* aPresContext,
                          nsGUIEvent*     aEvent,
                          nsEventStatus*  aEventStatus);

  virtual nsIAtom* GetType() const;
  virtual PRBool SupportsVisibilityHidden() { return PR_FALSE; }

#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const;
#endif

  NS_IMETHOD Destroy(nsIPresContext* aPresContext);

  NS_IMETHOD GetPluginInstance(nsIPluginInstance*& aPluginInstance);

  /* fail on any requests to get a cursor from us because plugins set their own! see bug 118877 */
  NS_IMETHOD GetCursor(nsIPresContext* aPresContext, nsPoint& aPoint, PRInt32& aCursor) 
  { return NS_ERROR_NOT_IMPLEMENTED;  };

  //i18n helper
  nsresult MakeAbsoluteURL(nsIURI* *aFullURI, 
                          nsString aSrc,
                          nsIURI* aBaseURI);
  // accessibility support
#ifdef ACCESSIBILITY
  NS_IMETHOD GetAccessible(nsIAccessible** aAccessible);
#ifdef XP_WIN
  NS_IMETHOD GetPluginPort(HWND *aPort);
#endif
#endif

  //local methods
  nsresult CreateWidget(nsIPresContext* aPresContext,
                        nscoord aWidth,
                        nscoord aHeight,
                        PRBool aViewOnly);
  nsIURI* GetFullURL() { return mFullURL; }
  
  PRBool IsSupportedImage(nsIContent* aContent);
  PRBool IsSupportedDocument(nsIContent* aContent);

  // for a given aRoot, this walks the frame tree looking for the next outFrame
  static nsIObjectFrame* GetNextObjectFrame(nsIPresContext* aPresContext,
                                            nsIFrame* aRoot);

  void FixUpURLS(const nsString &name, nsAString &value);

  void PluginNotAvailable(const char *aMimeType);

  // Returns true if this object frame links to content that we have
  // no enabled plugin for, that means not even the default plugin.
  PRBool IsBroken() const
  {
    return mIsBrokenPlugin;
  }

  virtual PRBool IsContainingBlock() const
  {
    // Broken plugins are containing blocks.

    return IsBroken();
  }


protected:
  // nsISupports
  NS_IMETHOD_(nsrefcnt) AddRef(void);
  NS_IMETHOD_(nsrefcnt) Release(void);

  virtual ~nsObjectFrame();

  virtual PRIntn GetSkipSides() const;

  // NOTE:  This frame class does not inherit from |nsLeafFrame|, so
  // this is not a virtual method implementation.
  void GetDesiredSize(nsIPresContext* aPresContext,
                      const nsHTMLReflowState& aReflowState,
                      nsHTMLReflowMetrics& aDesiredSize);

  nsresult InstantiateWidget(nsIPresContext*          aPresContext,
                             nsHTMLReflowMetrics&     aMetrics,
                             const nsHTMLReflowState& aReflowState,
                             nsCID aWidgetCID);

  nsresult InstantiatePlugin(nsIPresContext*          aPresContext,
                             nsHTMLReflowMetrics&     aMetrics,
                             const nsHTMLReflowState& aReflowState,
                             nsIPluginHost* aPluginHost, 
                             const char* aMimetype,
                             nsIURI* aURL);

  nsresult ReinstantiatePlugin(nsIPresContext* aPresContext, 
                               nsHTMLReflowMetrics& aMetrics, 
                               const nsHTMLReflowState& aReflowState);

  nsresult HandleChild(nsIPresContext*          aPresContext,
                       nsHTMLReflowMetrics&     aMetrics,
                       const nsHTMLReflowState& aReflowState,
                       nsReflowStatus&          aStatus,
                       nsIFrame*                child);
 
  // check attributes and optionally CSS to see if we should display anything
  PRBool IsHidden(PRBool aCheckVisibilityStyle = PR_TRUE) const;

  void NotifyContentObjectWrapper();

  nsPoint GetWindowOriginInPixels(PRBool aWindowless);

  void CreateDefaultFrames(nsIPresContext *aPresContext,
                           nsHTMLReflowMetrics& aMetrics,
                           const nsHTMLReflowState& aReflowState);

  friend class nsPluginInstanceOwner;
private:
  nsPluginInstanceOwner *mInstanceOwner;
  nsCOMPtr<nsIURI>      mFullURL;
  nsIFrame              *mFirstChild;
  nsCOMPtr<nsIWidget>   mWidget;
  nsRect                mWindowlessRect;
  PRPackedBool          mIsBrokenPlugin;
};


#endif /* nsObjectFrame_h___ */
