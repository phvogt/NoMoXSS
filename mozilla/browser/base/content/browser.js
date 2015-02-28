# -*- Mode: Java; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
# ***** BEGIN LICENSE BLOCK *****
# Version: NPL 1.1/GPL 2.0/LGPL 2.1
#
# The contents of this file are subject to the Netscape Public License
# Version 1.1 (the "License"); you may not use this file except in
# compliance with the License. You may obtain a copy of the License at
# http://www.mozilla.org/NPL/
#
# Software distributed under the License is distributed on an "AS IS" basis,
# WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
# for the specific language governing rights and limitations under the
# License.
#
# The Original Code is mozilla.org code.
#
# The Initial Developer of the Original Code is 
# Netscape Communications Corporation.
# Portions created by the Initial Developer are Copyright (C) 1998
# the Initial Developer. All Rights Reserved.
#
# Contributor(s):
#   Blake Ross <blake@cs.stanford.edu>
#   David Hyatt <hyatt@mozilla.org>
#   Peter Annema <disttsc@bart.nl>
#   Dean Tessman <dean_tessman@hotmail.com>
#   Kevin Puetz (puetzk@iastate.edu)
#   Ben Goodger <ben@netscape.com> 
#   Pierre Chanial <chanial@noos.fr>
#   Jason Eager <jce2@po.cwru.edu>
#   Joe Hewitt <hewitt@netscape.com>
#   Alec Flett <alecf@netscape.com>
#
# Alternatively, the contents of this file may be used under the terms of
# either the GNU General Public License Version 2 or later (the "GPL"), or 
# the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
# in which case the provisions of the GPL or the LGPL are applicable instead
# of those above. If you wish to allow use of your version of this file only
# under the terms of either the GPL or the LGPL, and not to allow others to
# use your version of this file under the terms of the NPL, indicate your
# decision by deleting the provisions above and replace them with the notice
# and other provisions required by the GPL or the LGPL. If you do not delete
# the provisions above, a recipient may use your version of this file under
# the terms of any one of the NPL, the GPL or the LGPL.
#
# ***** END LICENSE BLOCK *****

const NS_ERROR_MODULE_NETWORK = 2152398848;
const NS_NET_STATUS_READ_FROM = NS_ERROR_MODULE_NETWORK + 8;
const NS_NET_STATUS_WROTE_TO  = NS_ERROR_MODULE_NETWORK + 9;
const kXULNS = 
    "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul";

const nsIWebNavigation = Components.interfaces.nsIWebNavigation;

const MAX_HISTORY_MENU_ITEMS = 15;
const FIND_NORMAL = 0;
const FIND_TYPEAHEAD = 1;
const FIND_LINKS = 2;

var gRDF = null;
var gGlobalHistory = null;
var gURIFixup = null;
var gPageStyleButton = null;
var gLivemarksButton = null;
var gCharsetMenu = null;
var gLastBrowserCharset = null;
var gPrevCharset = null;
var gURLBar = null;
var gProxyButton = null;
var gProxyFavIcon = null;
var gProxyDeck = null;
var gNavigatorBundle = null;
var gIsLoadingBlank = false;
var gLastValidURLStr = "";
var gLastValidURL = null;
var gHaveUpdatedToolbarState = false;
var gClickSelectsAll = false;
var gIgnoreFocus = false;
var gIgnoreClick = false;
var gMustLoadSidebar = false;
var gProgressMeterPanel = null;
var gProgressCollapseTimer = null;
var gPrefService = null;
var appCore = null;
var gBrowser = null;
var gSidebarCommand = "";

// Global find variables
var gFindMode = FIND_NORMAL;
var gQuickFindTimeout = null;
var gQuickFindTimeoutLength = 0;
var gHighlightTimeout = null;
var gUseTypeAheadFind = false;
var gWrappedToTopStr = "";
var gWrappedToBottomStr = "";
var gNotFoundStr = "";
var gTypeAheadFindBuffer = "";
var gIsBack = true;
var gBackProtectBuffer = 3;
var gFlashFindBar = 0;
var gFlashFindBarCount = 6;
var gFlashFindBarTimeout = null;

// Global variable that holds the nsContextMenu instance.
var gContextMenu = null;

var gChromeState = null; // chrome state before we went into print preview

var gFormFillPrefListener = null;
var gFormHistory = null;
var gFormFillEnabled = true;

var gURLBarAutoFillPrefListener = null;

/**
* We can avoid adding multiple load event listeners and save some time by adding
* one listener that calls all real handlers.
*/

function loadEventHandlers(event)
{
  // Filter out events that are not about the document load we are interested in
  if (event.originalTarget == _content.document) {
    UpdateBookmarksLastVisitedDate(event);
    checkForDirectoryListing();
    charsetLoadListener(event);
    updatePageStyles();
    updatePageLivemarks();
  }

  // some event handlers want to be told what the original browser/listener is
  var targetBrowser = null;
  var targetListener = null;
  if (gBrowser.mTabbedMode) {
    var targetBrowserIndex = gBrowser.getBrowserIndexForDocument(event.originalTarget);
    if (targetBrowserIndex == -1)
      return;
    targetBrowser = gBrowser.getBrowserAtIndex(targetBrowserIndex);
    targetListener = gBrowser.mTabListeners[targetBrowserIndex];
  } else {
    targetBrowser = gBrowser.mCurrentBrowser;
    targetListener = null;      // no listener for non-tabbed-mode
  }

  updatePageFavIcon(targetBrowser, targetListener);
}

/**
 * Determine whether or not the content area is displaying a page with frames,
 * and if so, toggle the display of the 'save frame as' menu item.
 **/
function getContentAreaFrameCount()
{
  var saveFrameItem = document.getElementById("menu_saveFrame");
  if (!content || !_content.frames.length || !isDocumentFrame(document.commandDispatcher.focusedWindow))
    saveFrameItem.setAttribute("hidden", "true");
  else
    saveFrameItem.removeAttribute("hidden");
}

//////////////////////////////// BOOKMARKS ////////////////////////////////////

function UpdateBookmarksLastVisitedDate(event)
{
  var url = getWebNavigation().currentURI.spec;
  if (url) {
    // if the URL is bookmarked, update its "Last Visited" date
    BMSVC.updateLastVisitedDate(url, _content.document.characterSet);
  }
}

// DOMRange used during highlighting
var searchRange;
var startPt;
var endPt;

function toggleHighlight(aHighlight)
{
  var word = document.getElementById("find-field").value;
  if (aHighlight)
    highlightDoc('yellow', word);
  else
    highlightDoc(null, null); 
}

function highlightDoc(color, word, win)
{
  if (!win)
    win = window._content; 

  for (var i = 0; win.frames && i < win.frames.length; i++) {
    highlightDoc(color, word, win.frames[i]);
  }

  var doc = win.document;
  if (!document)
    return;

  // Remove highlighting
  if (!color) {
    var elem = null;
    while ((elem = doc.getElementById("__firefox-findbar-search-id"))) {
      var child = null;
      var docfrag = doc.createDocumentFragment();
      var next = elem.nextSibling;
      var parent = elem.parentNode;
      while((child = elem.firstChild)) {
        docfrag.appendChild(child);
      }
      parent.removeChild(elem);
      parent.insertBefore(docfrag, next);
    }
    return;
  }

  if (!("body" in doc))
    return;

  var body = doc.body;
  
  var count = body.childNodes.length;
  searchRange = doc.createRange();
  startPt = doc.createRange();
  endPt = doc.createRange();

  var baseNode = doc.createElement("span");
  baseNode.setAttribute("style", "background-color: " + color + ";");
  baseNode.setAttribute("id", "__firefox-findbar-search-id");

  searchRange.setStart(body, 0);
  searchRange.setEnd(body, count);

  startPt.setStart(body, 0);
  startPt.setEnd(body, 0);
  endPt.setStart(body, count);
  endPt.setEnd(body, count);
  highlightText(word, baseNode);
}

function highlightText(word, baseNode)
{
  var retRange = null;
  var finder = Components.classes["@mozilla.org/embedcomp/rangefind;1"].createInstance()
                         .QueryInterface(Components.interfaces.nsIFind);

  while((retRange = finder.Find(word, searchRange, startPt, endPt))) {
    // Highlight
    var nodeSurround = baseNode.cloneNode(true);
    var node = highlight(retRange, nodeSurround);
    startPt = node.ownerDocument.createRange();
    startPt.setStart(node, node.childNodes.length);
    startPt.setEnd(node, node.childNodes.length);
  }
}

function highlight(range, node)
{
  var startContainer = range.startContainer;
  var startOffset = range.startOffset;
  var endOffset = range.endOffset;
  var docfrag = range.extractContents();
  var before = startContainer.splitText(startOffset);
  var parent = before.parentNode;
  node.appendChild(docfrag);
  parent.insertBefore(node, before);
  return node;
}

function getSelectionController(ds)
{
  var display = ds.QueryInterface(Components.interfaces.nsIInterfaceRequestor).getInterface(Components.interfaces.nsISelectionDisplay);
  if (!display)
    return null;
  return display.QueryInterface(Components.interfaces.nsISelectionController);
}

function changeSelectionColor(aAttention)
{
  var ds = getBrowser().docShell;
  var dsEnum = ds.getDocShellEnumerator(Components.interfaces.nsIDocShellTreeItem.typeContent,
                                        Components.interfaces.nsIDocShell.ENUMERATE_FORWARDS);
  while (dsEnum.hasMoreElements()) {
    ds = dsEnum.getNext().QueryInterface(Components.interfaces.nsIDocShell);
    var controller = getSelectionController(ds);
    if (!controller)
      continue;
    const selCon = Components.interfaces.nsISelectionController;
    controller.setDisplaySelection(aAttention? selCon.SELECTION_ATTENTION : selCon.SELECTION_ON);
  }
}

function openFindBar()
{
  if (!gNotFoundStr || !gWrappedToTopStr || !gWrappedToBottomStr) {
    var bundle_browser = document.getElementById("bundle_browser");
    gNotFoundStr = bundle_browser.getString("NotFound");
    gWrappedToTopStr = bundle_browser.getString("WrappedToTop");
    gWrappedToBottomStr = bundle_browser.getString("WrappedToBottom");
  }
  
  var findToolbar = document.getElementById("FindToolbar");
  if (findToolbar.hidden) {
    findToolbar.hidden = false;
    return true;
  }
  return false;
}

function focusFindBar()
{
  var findField = document.getElementById("find-field");
  findField.focus();    
}

function selectFindBar()
{
  var findField = document.getElementById("find-field");
  findField.select();    
}

function closeFindBar()
{
  var findField = document.getElementById("find-field");
  var ww = Components.classes["@mozilla.org/embedcomp/window-watcher;1"]
                     .getService(Components.interfaces.nsIWindowWatcher);
  if (window == ww.activeWindow && document.commandDispatcher.focusedElement &&
      document.commandDispatcher.focusedElement.parentNode.parentNode == findField) {
    _content.focus();
  }

  var findToolbar = document.getElementById("FindToolbar");
  findToolbar.hidden = true;
  gTypeAheadFindBuffer = "";
  changeSelectionColor(false);
  if (gQuickFindTimeout) {
    clearTimeout(gQuickFindTimeout);
    gQuickFindTimeout = null;    
  }
  
  var statusIcon = document.getElementById("find-status-icon");
  findField.removeAttribute("status");
  statusIcon.removeAttribute("status");
}

function shouldFastFind(evt)
{
  if (evt.ctrlKey || evt.altKey || evt.metaKey)
    return false;
    
  var elt = document.commandDispatcher.focusedElement;
  if (elt) {
    var ln = elt.localName.toLowerCase();
    if (ln == "input" || ln == "textarea" || ln == "select" || ln == "button" || ln == "isindex")
      return false;
  }
  
  var win = document.commandDispatcher.focusedWindow;
  if (win && win.document.designMode == "on")
    return false;
  
  return true;
}

function onFindBarFocus()
{
  toggleLinkFocus(false);
}

function onFindBarBlur()
{
  toggleLinkFocus(true);
  changeSelectionColor(false);
}

function onBrowserKeyPress(evt)
{  
  // Check focused elt
  if (!shouldFastFind(evt))
    return;
  
  var findField = document.getElementById("find-field");
  if (gFindMode != FIND_NORMAL && gQuickFindTimeout) {    
    if (evt.keyCode == 8) { // Backspace
      if (findField.value) {
        findField.value = findField.value.substr(0, findField.value.length - 1);
        gIsBack = true;   
        gBackProtectBuffer = 3;
      }
      else if (gBackProtectBuffer > 0) {
        gBackProtectBuffer--;
      }
      
      if (gIsBack || gBackProtectBuffer > 0)
        evt.preventDefault();
        
      find(findField.value);
    }
    else if (evt.keyCode == 27) { // Escape
      closeFindBar();
    }
    else if (evt.charCode) {
      if (evt.charCode == 32) // Space
        evt.preventDefault();
        
      findField.value += String.fromCharCode(evt.charCode);
      find(findField.value);
    }
    return;
  }
  
  if (evt.charCode == 39 /* '*/ || evt.charCode == 47 /* / */ || (gUseTypeAheadFind && evt.charCode && evt.charCode != 32)) {   
    gFindMode = (evt.charCode == 39) ? FIND_LINKS : FIND_TYPEAHEAD;
    toggleLinkFocus(true);
    if (openFindBar()) {      
      setFindCloseTimeout();      
      if (gUseTypeAheadFind && evt.charCode != 39 && evt.charCode != 47) {
        gTypeAheadFindBuffer += String.fromCharCode(evt.charCode);        
        findField.value = gTypeAheadFindBuffer;
        find(findField.value);
      }
      else {
        findField.value = "";
      }
    }
    else {
      if (gFindMode == FIND_NORMAL) {
        selectFindBar();      
        focusFindBar();
      }
      else {
        findField.value = String.fromCharCode(evt.charCode);
        find(findField.value);
      }
    }        
  }
}

function toggleLinkFocus(aFocusLinks)
{
  var fastFind = getBrowser().fastFind;
  fastFind.focusLinks = aFocusLinks;
}

function onBrowserKeyUp(evt)
{
  if (evt.keyCode == 8)
    gIsBack = false;
}

function onFindBarKeyPress(evt)
{
  if (evt.keyCode == KeyEvent.DOM_VK_RETURN) {
    if (evt.shiftKey)
      findPrevious();
    else
      findNext();
  }
  else if (evt.keyCode == KeyEvent.DOM_VK_ESCAPE) {
    closeFindBar();
    evt.preventDefault();
  } 
  else if (evt.keyCode == KeyEvent.DOM_VK_PAGE_UP) {
    window.top._content.scrollByPages(-1);
    evt.preventDefault();
  }
  else if (evt.keyCode == KeyEvent.DOM_VK_PAGE_DOWN) {
    window.top._content.scrollByPages(1);
    evt.preventDefault();
  }
  else if (evt.keyCode == KeyEvent.DOM_VK_UP) {
    window.top._content.scrollByLines(-1);
    evt.preventDefault();
  }
  else if (evt.keyCode == KeyEvent.DOM_VK_DOWN) {
    window.top._content.scrollByLines(1);
    evt.preventDefault();
  }

} 

function enableFindButtons(aEnable)
{
  var findNext = document.getElementById("find-next");
  var findPrev = document.getElementById("find-previous");  
  var highlight = document.getElementById("highlight");
  findNext.disabled = findPrev.disabled = highlight.disabled = !aEnable;  
}

function find(val)
{
  if (!val)
    val = document.getElementById("find-field").value;
    
  enableFindButtons(val);
 
  var highlightBtn = document.getElementById("highlight");
  if (highlightBtn.checked)
    setHighlightTimeout();
        
  changeSelectionColor(true);
  var fastFind = getBrowser().fastFind;  
  var res = fastFind.find(val, gFindMode == FIND_LINKS);
  updateStatus(res, true);
  
  if (gFindMode != FIND_NORMAL)
    setFindCloseTimeout();
}

function flashFindBar()
{
  var findToolbar = document.getElementById("FindToolbar");
  if (gFlashFindBarCount-- == 0) {
    clearInterval(gFlashFindBarTimeout);
    findToolbar.removeAttribute("flash");
    gFlashFindBarCount = 6;
    return true;
  }
 
  findToolbar.setAttribute("flash", (gFlashFindBarCount % 2 == 0) ? "false" : "true");
}

function onFindCmd()
{
  gFindMode = FIND_NORMAL;
  openFindBar();
  if (gFlashFindBar) {
    gFlashFindBarTimeout = setInterval(flashFindBar, 500);
    gPrefService.setIntPref("accessibility.typeaheadfind.flashBar", --gFlashFindBar);
  }
  selectFindBar();
  focusFindBar();
}

function onFindAgainCmd()
{
  var findString = getBrowser().findString;
  if (!findString)
    return onFindCmd();

  var res = findNext();
  if (res == Components.interfaces.nsITypeAheadFind.FIND_NOTFOUND) {
    if (openFindBar()) {
      focusFindBar();
      selectFindBar();
      if (gFindMode != FIND_NORMAL)
        setFindCloseTimeout();
    }
  }
}

function onFindPreviousCmd()
{
  var findString = getBrowser().findString;
  if (!findString)
    return onFindCmd();
 
  var res = findPrevious();
  if (res == Components.interfaces.nsITypeAheadFind.FIND_NOTFOUND) {
    if (openFindBar()) {
      focusFindBar();
      selectFindBar();
      if (gFindMode != FIND_NORMAL)
        setFindCloseTimeout();
    }
  }
}

function setHighlightTimeout()
{
  if (gHighlightTimeout)
    clearTimeout(gHighlightTimeout);
  gHighlightTimeout = setTimeout(function() { toggleHighlight(false); toggleHighlight(true); }, 500);  
}

function isFindBarVisible()
{
  var findBar = document.getElementById("FindToolbar");
  return !findBar.hidden;
}

function findNext()
{
  changeSelectionColor(true);
  var fastFind = getBrowser().fastFind; 
  var res = fastFind.findNext();  
  updateStatus(res, true);
    
  if (gFindMode != FIND_NORMAL && isFindBarVisible())
    setFindCloseTimeout();
  
  return res;
}

function findPrevious()
{
  changeSelectionColor(true);
  var fastFind = getBrowser().fastFind;
  var res = fastFind.findPrevious();
  updateStatus(res, false);
  
  if (gFindMode != FIND_NORMAL && isFindBarVisible())
    setFindCloseTimeout();
  
  return res;
}

function updateStatus(res, findNext)
{
  var findBar = document.getElementById("FindToolbar");
  var field = document.getElementById("find-field");
  var statusIcon = document.getElementById("find-status-icon");
  var statusText = document.getElementById("find-status");
  switch(res) {
    case Components.interfaces.nsITypeAheadFind.FIND_WRAPPED:
      statusIcon.setAttribute("status", "wrapped");      
      statusText.value = findNext ? gWrappedToTopStr : gWrappedToBottomStr;
      break;
    case Components.interfaces.nsITypeAheadFind.FIND_NOTFOUND:
      statusIcon.setAttribute("status", "notfound");
      statusText.value = gNotFoundStr;
      field.setAttribute("status", "notfound");      
      break;
    case Components.interfaces.nsITypeAheadFind.FIND_FOUND:
    default:
      statusIcon.removeAttribute("status");      
      statusText.value = "";
      field.removeAttribute("status");
      break;
  }
}

function setFindCloseTimeout()
{
  if (gQuickFindTimeout)
    clearTimeout(gQuickFindTimeout);
  gQuickFindTimeout = setTimeout(function() { if (gFindMode != FIND_NORMAL) closeFindBar(); }, gQuickFindTimeoutLength);
}

function UpdateBackForwardButtons()
{
  var backBroadcaster = document.getElementById("Browser:Back");
  var forwardBroadcaster = document.getElementById("Browser:Forward");
  
  var webNavigation = gBrowser.webNavigation;

  // Avoid setting attributes on broadcasters if the value hasn't changed!
  // Remember, guys, setting attributes on elements is expensive!  They
  // get inherited into anonymous content, broadcast to other widgets, etc.!
  // Don't do it if the value hasn't changed! - dwh

  var backDisabled = backBroadcaster.hasAttribute("disabled");
  var forwardDisabled = forwardBroadcaster.hasAttribute("disabled");
  if (backDisabled == webNavigation.canGoBack) {
    if (backDisabled)
      backBroadcaster.removeAttribute("disabled");
    else
      backBroadcaster.setAttribute("disabled", true);
  }
  
  if (forwardDisabled == webNavigation.canGoForward) {
    if (forwardDisabled)
      forwardBroadcaster.removeAttribute("disabled");
    else
      forwardBroadcaster.setAttribute("disabled", true);
  }
}

#ifdef MOZ_ENABLE_XREMOTE
const gTabOpenObserver = {
  observe: function(subject, topic, data)
  {
    if (topic != "open-new-tab-request" || subject != window)
      return;

    delayedOpenTab(data);
  }
};
#endif

const gSessionHistoryObserver = {
  observe: function(subject, topic, data)
  {
    if (topic != "browser:purge-session-history")
      return;

    var backCommand = document.getElementById("Browser:Back");
    backCommand.setAttribute("disabled", "true");
    var fwdCommand = document.getElementById("Browser:Forward");
    fwdCommand.setAttribute("disabled", "true");
  }
};

const gPopupBlockerObserver = {
  _reportButton: null,
  _kIPM: Components.interfaces.nsIPermissionManager,
  
  onUpdatePageReport: function ()
  {
    if (!this._reportButton) 
      this._reportButton = document.getElementById("page-report-button");
    
    if (gBrowser.selectedBrowser.pageReport) {
      this._reportButton.setAttribute("blocked", "true");
      if (gPrefService.getBoolPref("privacy.popups.showBrowserMessage")) {
        var bundle_browser = document.getElementById("bundle_browser");
        var brandBundle = document.getElementById("bundle_brand");
        var brandShortName = brandBundle.getString("brandShortName");
        var message;
        var popupCount = gBrowser.selectedBrowser.pageReport.length;
        if (popupCount > 1) 
          message = bundle_browser.getFormattedString("popupWarningMultiple", [brandShortName, popupCount]);
        else
          message = bundle_browser.getFormattedString("popupWarning", [brandShortName]);
        gBrowser.showMessage(gBrowser.selectedBrowser, "chrome://browser/skin/Info.png", 
                             message, "", null, null, "blockedPopupOptions", "top", false);
      }
    }
    else
      this._reportButton.removeAttribute("blocked");
  },
  
  toggleAllowPopupsForSite: function (aEvent)
  {
    var currentURI = gBrowser.selectedBrowser.webNavigation.currentURI;
    var pm = Components.classes["@mozilla.org/permissionmanager;1"]
                       .getService(this._kIPM);
    var shouldBlock = aEvent.target.getAttribute("block") == "true";
    var perm = shouldBlock ? this._kIPM.DENY_ACTION : this._kIPM.ALLOW_ACTION;
    pm.add(currentURI, "popup", perm);
    
    gBrowser.hideMessage(gBrowser.selectedBrowser, "top");
  },
  
  fillPopupList: function (aEvent)
  {
    var bundle_browser = document.getElementById("bundle_browser");
    // XXXben - rather than using |currentURI| here, which breaks down on multi-framed sites
    //          we should really walk the pageReport and create a list of "allow for <host>"
    //          menuitems for the common subset of hosts present in the report, this will
    //          make us frame-safe.
    var uri = gBrowser.selectedBrowser.webNavigation.currentURI;
    var blockedPopupAllowSite = document.getElementById("blockedPopupAllowSite");
    try {
      blockedPopupAllowSite.removeAttribute("hidden");
      
      var pm = Components.classes["@mozilla.org/permissionmanager;1"]
                        .getService(this._kIPM);
      if (pm.testPermission(uri, "popup") == this._kIPM.ALLOW_ACTION) {
        // Offer an item to block popups for this site, if a whitelist entry exists
        // already for it.
        var blockString = bundle_browser.getFormattedString("popupBlock", [uri.host]);
        blockedPopupAllowSite.setAttribute("label", blockString);
        blockedPopupAllowSite.setAttribute("block", "true");
      }
      else {
        // Offer an item to allow popups for this site
        var allowString = bundle_browser.getFormattedString("popupAllow", [uri.host]);
        blockedPopupAllowSite.setAttribute("label", allowString);
        blockedPopupAllowSite.removeAttribute("block");
      }
    }
    catch (e) {
      blockedPopupAllowSite.setAttribute("hidden", "true");
    }
    
    var item = aEvent.target.lastChild;
    while (item && item.getAttribute("observes") != "blockedPopupsSeparator") {
      var next = item.previousSibling;
      item.parentNode.removeChild(item);
      item = next;
    }
    var pageReport = gBrowser.selectedBrowser.pageReport;
    if (pageReport && pageReport.length > 0) {
      var blockedPopupsSeparator = document.getElementById("blockedPopupsSeparator");
      blockedPopupsSeparator.removeAttribute("hidden");
      for (var i = 0; i < pageReport.length; ++i) {
        var menuitem = document.createElement("menuitem");
        var label = bundle_browser.getFormattedString("popupShowPopupPrefix", 
                                                      [pageReport[i].popupWindowURI.spec]);
        menuitem.setAttribute("label", label);
        menuitem.setAttribute("requestingWindowURI", pageReport[i].requestingWindowURI.spec);
        menuitem.setAttribute("popupWindowURI", pageReport[i].popupWindowURI.spec);
        menuitem.setAttribute("popupWindowFeatures", pageReport[i].popupWindowFeatures);
        menuitem.setAttribute("oncommand", "gPopupBlockerObserver.showBlockedPopup(event);");
        aEvent.target.appendChild(menuitem);
      }
    }
    else {
      var blockedPopupsSeparator = document.getElementById("blockedPopupsSeparator");
      blockedPopupsSeparator.setAttribute("hidden", "true");
    }
        
    var blockedPopupDontShowMessage = document.getElementById("blockedPopupDontShowMessage");
    var showMessage = gPrefService.getBoolPref("privacy.popups.showBrowserMessage");
    blockedPopupDontShowMessage.setAttribute("checked", !showMessage);
    if (aEvent.target.localName == "popup")
      blockedPopupDontShowMessage.setAttribute("label", bundle_browser.getString("popupWarningDontShowFromMessage"));
    else
      blockedPopupDontShowMessage.setAttribute("label", bundle_browser.getString("popupWarningDontShowFromStatusbar"));
  },
  
  _findChildShell: function (aDocShell, aSoughtURI)
  {
    var webNav = aDocShell.QueryInterface(Components.interfaces.nsIWebNavigation);
    if (webNav.currentURI.spec == aSoughtURI.spec)
      return aDocShell;
      
    var node = aDocShell.QueryInterface(Components.interfaces.nsIDocShellTreeNode);
    for (var i = 0; i < node.childCount; ++i) {
      var docShell = node.getChildAt(i);
      docShell = this._findChildShell(docShell, aSoughtURI);
      if (docShell)
        return docShell;
    }
    return null;
  },

  showBlockedPopup: function (aEvent)
  {
    var requestingWindowURI = Components.classes["@mozilla.org/network/standard-url;1"]
                                        .createInstance(Components.interfaces.nsIURI);
    requestingWindowURI.spec = aEvent.target.getAttribute("requestingWindowURI");
    var popupWindowURI = aEvent.target.getAttribute("popupWindowURI");
    var features = aEvent.target.getAttribute("popupWindowFeatures");
    
    var shell = this._findChildShell(gBrowser.selectedBrowser.docShell, 
                                     requestingWindowURI);
    if (shell) {
      var ifr = shell.QueryInterface(Components.interfaces.nsIInterfaceRequestor);
      var dwi = ifr.getInterface(Components.interfaces.nsIDOMWindowInternal);
      // XXXben - nsIDOMPopupBlockedEvent needs to store target, too!
      dwi.open(popupWindowURI, "", features);
    }
  },
  
  editPopupSettings: function ()
  {
    var host = "";
    try {
      var uri = gBrowser.selectedBrowser.webNavigation.currentURI;
      host = uri.host;
    }
    catch (e) { } 

    var wm = Components.classes["@mozilla.org/appshell/window-mediator;1"]
                        .getService(Components.interfaces.nsIWindowMediator);
    var existingWindow = wm.getMostRecentWindow("exceptions");
    if (existingWindow) {
      existingWindow.setHost(host);
      existingWindow.focus();
    }
    else {
      var params = { blockVisible: false, 
                     allowVisible: true, 
                     prefilledHost: host, 
                     permissionType: "popup" };
      window.openDialog("chrome://browser/content/cookieviewer/CookieExceptions.xul?permission=popup",
                        "_blank", "chrome,modal,resizable=yes", params);
    }
  },
  
  dontShowMessage: function ()
  {
    var showMessage = gPrefService.getBoolPref("privacy.popups.showBrowserMessage");
    var firstTime = gPrefService.getBoolPref("privacy.popups.firstTime");
    
    // If the info message is showing at the top of the window, and the user has never 
    // hidden the message before, show an info box telling the user where the info
    // will be displayed. 
    if (showMessage && firstTime)
      this._displayPageReportFirstTime();

    gPrefService.setBoolPref("privacy.popups.showBrowserMessage", !showMessage);

    gBrowser.hideMessage(gBrowser.selectedBrowser, "top");
  },

  _displayPageReportFirstTime: function ()
  {
    window.openDialog("chrome://browser/content/pageReportFirstTime.xul", "_blank",
                      "dependent");
  }
};

const gXPInstallObserver = {
  _findChildShell: function (aDocShell, aSoughtShell)
  {
    if (aDocShell == aSoughtShell)
      return aDocShell;
      
    var node = aDocShell.QueryInterface(Components.interfaces.nsIDocShellTreeNode);
    for (var i = 0; i < node.childCount; ++i) {
      var docShell = node.getChildAt(i);
      docShell = this._findChildShell(docShell, aSoughtShell);
      if (docShell == aSoughtShell)
        return docShell;
    }
    return null;
  },
  
  _getBrowser: function (aDocShell) 
  {
    var tabbrowser = getBrowser();
    for (var i = 0; i < tabbrowser.browsers.length; ++i) {
      var browser = tabbrowser.getBrowserAtIndex(i);
      var soughtShell = aDocShell;
      var shell = this._findChildShell(browser.docShell, soughtShell);
      if (shell)
        return browser;
    }
    return null;
  },

  observe: function (aSubject, aTopic, aData)
  {
    var brandBundle = document.getElementById("bundle_brand");
    var browserBundle = document.getElementById("bundle_browser");
    switch (aTopic) {
    case "xpinstall-install-blocked":
      var shell = aSubject.QueryInterface(Components.interfaces.nsIDocShell);
      var browser = this._getBrowser(shell);
      if (browser) {
        var host = browser.docShell.QueryInterface(Components.interfaces.nsIWebNavigation).currentURI.host;
        var brandShortName = brandBundle.getString("brandShortName");
        var iconURL, messageKey, buttonKey;
        if (aData == "install-chrome") {
          // XXXben - use regular software install warnings for now until we can
          // properly differentiate themes. It's likely in fact that themes won't
          // be blocked so this code path will only be reached for extensions.
          iconURL = "chrome://mozapps/skin/xpinstall/xpinstallItemGeneric.png";
          messageKey = "xpinstallWarning";
          buttonKey = "xpinstallWarningButton";
        }
        else {
          iconURL = "chrome://mozapps/skin/xpinstall/xpinstallItemGeneric.png";
          messageKey = "xpinstallWarning";
          buttonKey = "xpinstallWarningButton";
        }
        
        if (!gPrefService.getBoolPref("xpinstall.enabled")) {
          var messageString = browserBundle.getFormattedString("xpinstallDisabledWarning", 
                                                                [brandShortName, host]);
          var buttonString = browserBundle.getString("xpinstallDisabledWarningButton");
          getBrowser().showMessage(browser, iconURL, messageString, buttonString, 
                                   null, "xpinstall-install-edit-prefs",
                                   null, "top", false);
        }
        else {            
          var messageString = browserBundle.getFormattedString(messageKey, [brandShortName, host]);
          var buttonString = browserBundle.getString(buttonKey);
          var webNav = shell.QueryInterface(Components.interfaces.nsIWebNavigation);
          getBrowser().showMessage(browser, iconURL, messageString, buttonString, 
                                   shell, "xpinstall-install-edit-permissions",
                                   null, "top", false);
        }
      }
      break;
    case "xpinstall-install-edit-prefs":
      var wm = Components.classes["@mozilla.org/appshell/window-mediator;1"]
                         .getService(Components.interfaces.nsIWindowMediator);
      var optionsWindow = wm.getMostRecentWindow("Browser:Options");
      if (optionsWindow) {
        optionsWindow.focus();
        optionsWindow.switchPage("catFeaturesbutton");
      }
      else
        openDialog("chrome://browser/content/pref/pref.xul", "PrefWindow",
                   "chrome,titlebar,resizable,modal", "catFeaturesbutton");
      var tabbrowser = getBrowser();
      tabbrowser.hideMessage(tabbrowser.selectedBrowser, "top");
      break;
    case "xpinstall-install-edit-permissions":
      var browser = this._getBrowser(aSubject.QueryInterface(Components.interfaces.nsIDocShell));
      if (browser) {
        var webNav = aSubject.QueryInterface(Components.interfaces.nsIWebNavigation);
        var wm = Components.classes["@mozilla.org/appshell/window-mediator;1"]
                          .getService(Components.interfaces.nsIWindowMediator);
        var existingWindow = wm.getMostRecentWindow("exceptions");
        if (existingWindow) {
          existingWindow.setHost(webNav.currentURI.host);
          existingWindow.focus();
        }
        else {
          var params = { blockVisible:    false,
                         allowVisible:    true,
                         prefilledHost:   webNav.currentURI.host,
                         permissionType:  "install" };
          window.openDialog("chrome://browser/content/cookieviewer/CookieExceptions.xul?permission=install",
                            "_blank", "chrome,modal,resizable=yes", params);
        }
              
        var tabbrowser = getBrowser();
        tabbrowser.hideMessage(tabbrowser.selectedBrowser, "top");
      }
      break;
    }
  }
};

function Startup()
{
  gBrowser = document.getElementById("content");

  var uriToLoad = null;
  // Check for window.arguments[0]. If present, use that for uriToLoad.
  if ("arguments" in window && window.arguments.length >= 1 && window.arguments[0])
    uriToLoad = window.arguments[0];
    
  gIsLoadingBlank = uriToLoad == "about:blank";

  if (!gIsLoadingBlank)
    prepareForStartup();

#ifdef ENABLE_PAGE_CYCLER
  appCore.startPageCycler();
#else
  // only load url passed in when we're not page cycling
 
  if (uriToLoad && !gIsLoadingBlank) {
    if ("arguments" in window && window.arguments.length >= 3)
      loadURI(uriToLoad, window.arguments[2], null);
    else
      loadOneOrMoreURIs(uriToLoad);
  }

#ifdef MOZ_ENABLE_XREMOTE
  // hook up remote support
  var remoteService;
  remoteService = Components.classes["@mozilla.org/browser/xremoteservice;1"]
                            .getService(Components.interfaces.nsIXRemoteService);
  remoteService.addBrowserInstance(window);

  var observerService = Components.classes["@mozilla.org/observer-service;1"]
    .getService(Components.interfaces.nsIObserverService);
  observerService.addObserver(gTabOpenObserver, "open-new-tab-request", false);
#endif
#endif

  var sidebarSplitter;
  if (window.opener) {
    if (window.opener.gFindMode == FIND_NORMAL) {
      var openerFindBar = window.opener.document.getElementById("FindToolbar");
      if (openerFindBar && !openerFindBar.hidden)
        openFindBar();
    }
      
    var openerSidebarBox = window.opener.document.getElementById("sidebar-box");
    // The opener can be the hidden window too, if we're coming from the state
    // where no windows are open, and the hidden window has no sidebar box. 
    if (openerSidebarBox && !openerSidebarBox.hidden) {
      var sidebarBox = document.getElementById("sidebar-box");
      var sidebarTitle = document.getElementById("sidebar-title");
      sidebarTitle.setAttribute("value", window.opener.document.getElementById("sidebar-title").getAttribute("value"));
      sidebarBox.setAttribute("width", openerSidebarBox.boxObject.width);
      var sidebarCmd = openerSidebarBox.getAttribute("sidebarcommand");
      sidebarBox.setAttribute("sidebarcommand", sidebarCmd);
      sidebarBox.setAttribute("src", window.opener.document.getElementById("sidebar").getAttribute("src"));
      gMustLoadSidebar = true;
      sidebarBox.hidden = false;
      sidebarSplitter = document.getElementById("sidebar-splitter");
      sidebarSplitter.hidden = false;
      document.getElementById(sidebarCmd).setAttribute("checked", "true");
    }
  }
  else {
    var box = document.getElementById("sidebar-box");
    if (box.hasAttribute("sidebarcommand")) { 
      var cmd = box.getAttribute("sidebarcommand");
      if (cmd) {
        gMustLoadSidebar = true;
        box.hidden = false;
        sidebarSplitter = document.getElementById("sidebar-splitter");
        sidebarSplitter.hidden = false;
        document.getElementById(cmd).setAttribute("checked", "true");
      }
    }
  }

  // Certain kinds of automigration rely on this notification to complete their
  // tasks BEFORE the browser window is shown.   
  var obs = Components.classes["@mozilla.org/observer-service;1"].getService(Components.interfaces.nsIObserverService);
  obs.notifyObservers(null, "browser-window-before-show", "");

  // Set a sane starting width/height for all resolutions on new profiles.
  if (!document.documentElement.hasAttribute("width")) {
    var defaultWidth = 994, defaultHeight;
    if (screen.availHeight <= 600) {
      document.documentElement.setAttribute("sizemode", "maximized");
      defaultWidth = 610;
      defaultHeight = 450;
    }
    else {
      // Create a narrower window for large or wide-aspect displays, to suggest
      // side-by-side page view. 
      if ((screen.availWidth / 2) >= 800)
        defaultWidth = (screen.availWidth / 2) - 20;
      defaultHeight = screen.availHeight - 10;
    }
    document.documentElement.setAttribute("width", defaultWidth);
    document.documentElement.setAttribute("height", defaultHeight);
  }

  setTimeout(delayedStartup, 0);
}

function prepareForStartup()
{
  gURLBar = document.getElementById("urlbar");  
  gNavigatorBundle = document.getElementById("bundle_browser");
  gProgressMeterPanel = document.getElementById("statusbar-progresspanel");
  gBrowser.addEventListener("DOMUpdatePageReport", gPopupBlockerObserver.onUpdatePageReport, false);
  gBrowser.addEventListener("DOMLinkAdded", livemarkOnLinkAdded, false);
  gBrowser.addEventListener("PluginNotFound", gMissingPluginInstaller.newMissingPlugin, false);

  var webNavigation;
  try {
    // Create the browser instance component.
    appCore = Components.classes["@mozilla.org/appshell/component/browser/instance;1"]
                        .createInstance(Components.interfaces.nsIBrowserInstance);
    if (!appCore)
      throw "couldn't create a browser instance";

    webNavigation = getWebNavigation();
    if (!webNavigation)
      throw "no XBL binding for browser";
  } catch (e) {
    alert("Error launching browser window:" + e);
    window.close(); // Give up.
    return;
  }

  // initialize observers and listeners
  window.XULBrowserWindow = new nsBrowserStatusHandler();
  window.browserContentListener =
    new nsBrowserContentListener(window, gBrowser);

  // set default character set if provided
  if ("arguments" in window && window.arguments.length > 1 && window.arguments[1]) {
    if (window.arguments[1].indexOf("charset=") != -1) {
      var arrayArgComponents = window.arguments[1].split("=");
      if (arrayArgComponents) {
        //we should "inherit" the charset menu setting in a new window
        getMarkupDocumentViewer().defaultCharacterSet = arrayArgComponents[1];
      }
    }
  }

  // Initialize browser instance..
  appCore.setWebShellWindow(window);

  // Wire up session and global history before any possible
  // progress notifications for back/forward button updating
  webNavigation.sessionHistory = Components.classes["@mozilla.org/browser/shistory;1"]
                                           .createInstance(Components.interfaces.nsISHistory);

  // enable global history
  gBrowser.docShell.QueryInterface(Components.interfaces.nsIDocShellHistory).useGlobalHistory = true;

  const selectedBrowser = gBrowser.selectedBrowser;
  if (selectedBrowser.securityUI)
    selectedBrowser.securityUI.init(selectedBrowser.contentWindow);

  // hook up UI through progress listener
  gBrowser.addProgressListener(window.XULBrowserWindow, Components.interfaces.nsIWebProgress.NOTIFY_ALL);
}

function delayedStartup()
{
  var os = Components.classes["@mozilla.org/observer-service;1"].getService(Components.interfaces.nsIObserverService);
  os.addObserver(gSessionHistoryObserver, "browser:purge-session-history", false);
  
  // We have to do this because we manually hook up history for the first browser in prepareForStartup
  os.addObserver(gBrowser.browsers[0], "browser:purge-session-history", false);
  os.addObserver(gXPInstallObserver, "xpinstall-install-blocked", false);
  os.addObserver(gXPInstallObserver, "xpinstall-install-edit-prefs", false);
  os.addObserver(gXPInstallObserver, "xpinstall-install-edit-permissions", false);
  os.addObserver(gMissingPluginInstaller, "missing-plugin", false);

  gPrefService = Components.classes["@mozilla.org/preferences-service;1"]
                           .getService(Components.interfaces.nsIPrefBranch);
  BrowserOffline.init();
  
  if (document.documentElement.getAttribute("chromehidden").indexOf("toolbar") != -1) {
    gURLBar.setAttribute("readonly", "true"); 
    gURLBar.setAttribute("enablehistory", "false");
  }

  if (gIsLoadingBlank)
    prepareForStartup();

  if (gURLBar)
    gURLBar.addEventListener("dragdrop", URLBarOnDrop, true);

  // loads the services
  initServices();
  initBMService();
  gBrowser.addEventListener("load", function(evt) { setTimeout(loadEventHandlers, 0, evt); }, true);

  window.addEventListener("keypress", ctrlNumberTabSelection, false);

  if (gMustLoadSidebar) {
    var sidebar = document.getElementById("sidebar");
    var sidebarBox = document.getElementById("sidebar-box");
    sidebar.setAttribute("src", sidebarBox.getAttribute("src"));
  }
 
  // now load bookmarks
  BMSVC.readBookmarks();  
  var bt = document.getElementById("bookmarks-ptf");
  if (bt) {
    var btf = BMSVC.getBookmarksToolbarFolder().Value;
    bt.ref = btf;
    document.getElementById("bookmarks-chevron").ref = btf;
    bt.database.AddObserver(BookmarksToolbarRDFObserver);
    bt.controllers.appendController(BookmarksMenuController);
  }
  var bm = document.getElementById("bookmarks-menu");
  bm.controllers.appendController(BookmarksMenuController);
  window.addEventListener("resize", BookmarksToolbar.resizeFunc, false);

  // called when we go into full screen, even if it is 
  // initiated by a web page script
  window.addEventListener("fullscreen", onFullScreen, false);

  var element;
  if (gIsLoadingBlank && gURLBar && !gURLBar.hidden && !gURLBar.parentNode.parentNode.collapsed)
    element = gURLBar;
  else
    element = _content;

  // This is a redo of the fix for jag bug 91884
  var ww = Components.classes["@mozilla.org/embedcomp/window-watcher;1"]
                     .getService(Components.interfaces.nsIWindowWatcher);
  if (window == ww.activeWindow) {
    element.focus();
  } else {
    // set the element in command dispatcher so focus will restore properly
    // when the window does become active
    if (element instanceof Components.interfaces.nsIDOMWindow) {
      document.commandDispatcher.focusedWindow = element;
      document.commandDispatcher.focusedElement = null;
    } else if (element instanceof Components.interfaces.nsIDOMElement) {
      document.commandDispatcher.focusedWindow = element.ownerDocument.defaultView;
      document.commandDispatcher.focusedElement = element;
    }
  }

  SetPageProxyState("invalid", null);

  var toolbox = document.getElementById("navigator-toolbox");
  toolbox.customizeDone = BrowserToolboxCustomizeDone;

  // Enable/Disable Form Fill
  gFormFillPrefListener = new FormFillPrefListener();
  var pbi = gPrefService.QueryInterface(Components.interfaces.nsIPrefBranchInternal);
  pbi.addObserver(gFormFillPrefListener.domain, gFormFillPrefListener, false);
  gFormFillPrefListener.toggleFormFill();

  // Enable/Disable URL Bar Auto Fill
  gURLBarAutoFillPrefListener = new URLBarAutoFillPrefListener();
  pbi.addObserver(gURLBarAutoFillPrefListener.domain, gURLBarAutoFillPrefListener, false);

  pbi.addObserver(gHomeButton.prefDomain, gHomeButton, false);
  gHomeButton.updateTooltip();
  
  pbi.addObserver(gTypeAheadFind.prefDomain, gTypeAheadFind, false);
  gUseTypeAheadFind = gPrefService.getBoolPref("accessibility.typeaheadfind");
  
  // Initialize Plugin Overrides
  const kOverridePref = "browser.download.pluginOverrideTypes";
  if (gPrefService.prefHasUserValue(kOverridePref)) {
    var types = gPrefService.getCharPref(kOverridePref);
    types = types.split(",");
    
    const kPluginOverrideTypesNotHandled = "browser.download.pluginOverrideTypesNotHandled";
    
    var catman = Components.classes["@mozilla.org/categorymanager;1"].getService(Components.interfaces.nsICategoryManager);
    var typesNotHandled = "";
    for (var i = 0; i < types.length; ++i) {
      // Keep track of all overrides for plugins that aren't actually installed,
      // so we know not to show them in the plugin configuration dialog BUT 
      // don't delete the overrides such that when the user actually installs the 
      // plugin in this build their preferences are remembered.
      try {
        var catEntry = catman.getCategoryEntry("Gecko-Content-Viewers", types[i]);
      }
      catch (e) {
        catEntry = "";
      }
      if (catEntry == "")
        typesNotHandled += types[i] + ",";
    
      catman.deleteCategoryEntry("Gecko-Content-Viewers", types[i], false);
    }
    
    if (typesNotHandled) {
      typesNotHandled = typesNotHandled.substr(0, typesNotHandled.length - 1);
      gPrefService.setCharPref(kPluginOverrideTypesNotHandled, typesNotHandled);
    }
    else if (gPrefService.prefHasUserValue(kPluginOverrideTypesNotHandled))
      gPrefService.clearUserPref(kPluginOverrideTypesNotHandled);
  }

  gClickSelectsAll = gPrefService.getBoolPref("browser.urlbar.clickSelectsAll");

  clearObsoletePrefs();
 
  gQuickFindTimeoutLength = gPrefService.getIntPref("accessibility.typeaheadfind.timeout");
  gFlashFindBar = gPrefService.getIntPref("accessibility.typeaheadfind.flashBar");

#ifdef HAVE_SHELL_SERVICE
  // Perform default browser checking (after window opens).
  var shell = getShellService();
  if (shell) {
    var shouldCheck = shell.shouldCheckDefaultBrowser;
    if (shouldCheck && !shell.isDefaultBrowser(true)) {
      var brandBundle = document.getElementById("bundle_brand");
      var shellBundle = document.getElementById("bundle_shell");

      var brandShortName = brandBundle.getString("brandShortName");
      var promptTitle = shellBundle.getString("setDefaultBrowserTitle");
      var promptMessage = shellBundle.getFormattedString("setDefaultBrowserMessage", 
                                                         [brandShortName]);
      var checkboxLabel = shellBundle.getFormattedString("setDefaultBrowserDontAsk",
                                                         [brandShortName]);
      const IPS = Components.interfaces.nsIPromptService;
      var ps = Components.classes["@mozilla.org/embedcomp/prompt-service;1"]
                                                .getService(IPS);
      var checkEveryTime = { value: shouldCheck };
      var rv = ps.confirmEx(window, promptTitle, promptMessage, 
                            (IPS.BUTTON_TITLE_YES * IPS.BUTTON_POS_0) + 
                            (IPS.BUTTON_TITLE_NO * IPS.BUTTON_POS_1),
                            null, null, null, checkboxLabel, checkEveryTime);
      if (rv == 0)
        shell.setDefaultBrowser(true, false);
      shell.shouldCheckDefaultBrowser = checkEveryTime.value;
    }
  } else {
    // We couldn't get the shell service; go hide the mail toolbar button.
    var mailbutton = document.getElementById("mail-button");
    if (mailbutton)
      mailbutton.hidden = true;
  }
#endif

  var updatePanel = document.getElementById("updates");
  try {
    updatePanel.init();
  }
  catch (e) { }

  // BiDi UI
  if (isBidiEnabled()) {
    document.getElementById("documentDirection-separator").hidden = false;
    document.getElementById("documentDirection-swap").hidden = false;
    document.getElementById("textfieldDirection-separator").hidden = false;
    document.getElementById("textfieldDirection-swap").hidden = false;
  }
}

function Shutdown()
{
  var os = Components.classes["@mozilla.org/observer-service;1"]
    .getService(Components.interfaces.nsIObserverService);
  os.removeObserver(gSessionHistoryObserver, "browser:purge-session-history");
  os.removeObserver(gBrowser.browsers[0], "browser:purge-session-history");
  os.removeObserver(gXPInstallObserver, "xpinstall-install-blocked");
  os.removeObserver(gXPInstallObserver, "xpinstall-install-edit-permissions");
  os.removeObserver(gXPInstallObserver, "xpinstall-install-edit-prefs");
  os.removeObserver(gMissingPluginInstaller, "missing-plugin");

#ifdef MOZ_ENABLE_XREMOTE
  // remove remote support
  var remoteService;
  remoteService = Components.classes["@mozilla.org/browser/xremoteservice;1"]
                            .getService(Components.interfaces.nsIXRemoteService);
  remoteService.removeBrowserInstance(window);

  os.removeObserver(gTabOpenObserver, "open-new-tab-request");
#endif
  try {
    gBrowser.removeProgressListener(window.XULBrowserWindow);
  } catch (ex) {
  }

  var bt = document.getElementById("bookmarks-ptf");
  if (bt) {
    try {
      bt.database.RemoveObserver(BookmarksToolbarRDFObserver);
      bt.controllers.removeController(BookmarksMenuController);
    } catch (ex) {
    }
  }

  try {
    var bm = document.getElementById("bookmarks-menu");
    bm.controllers.removeController(BookmarksMenuController);
  } catch (ex) {
  }

  try {
    var pbi = gPrefService.QueryInterface(Components.interfaces.nsIPrefBranchInternal);
    pbi.removeObserver(gFormFillPrefListener.domain, gFormFillPrefListener);
    pbi.removeObserver(gURLBarAutoFillPrefListener.domain, gURLBarAutoFillPrefListener);
    pbi.removeObserver(gHomeButton.prefDomain, gHomeButton);
    pbi.removeObserver(gTypeAheadFind.prefDomain, gTypeAheadFind);
  } catch (ex) {
  }

  BrowserOffline.uninit();

  var windowManager = Components.classes['@mozilla.org/appshell/window-mediator;1'].getService();
  var windowManagerInterface = windowManager.QueryInterface(Components.interfaces.nsIWindowMediator);
  var enumerator = windowManagerInterface.getEnumerator(null);
  enumerator.getNext();
  if (!enumerator.hasMoreElements()) {
    document.persist("sidebar-box", "sidebarcommand");
    document.persist("sidebar-box", "width");
    document.persist("sidebar-box", "src");
    document.persist("sidebar-title", "value");
  }

  window.XULBrowserWindow.destroy();
  window.XULBrowserWindow = null;

  window.browserContentListener.close();
  // Close the app core.
  if (appCore)
    appCore.close();
}

function FormFillPrefListener()
{
  gBrowser.attachFormFill();
}

FormFillPrefListener.prototype =
{
  domain: "browser.formfill.enable",
  observe: function (aSubject, aTopic, aPrefName)
  {
    if (aTopic != "nsPref:changed" || aPrefName != this.domain)
      return;
      
    this.toggleFormFill();
  },
  
  toggleFormFill: function ()
  {
    try {
      gFormFillEnabled = gPrefService.getBoolPref(this.domain);
    }
    catch (e) {
    }
    var formController = Components.classes["@mozilla.org/satchel/form-fill-controller;1"].getService(Components.interfaces.nsIAutoCompleteInput);
    formController.disableAutoComplete = !gFormFillEnabled;

    var searchBar = document.getElementsByTagName("searchbar");
    for (var i=0; i<searchBar.length;i++) {
      if (gFormFillEnabled)
        searchBar[i].removeAttribute("disableautocomplete");
      else
        searchBar[i].setAttribute("disableautocomplete", "true");
    }
  }
}
 
function URLBarAutoFillPrefListener()
{
  this.toggleAutoFillInURLBar();
}

URLBarAutoFillPrefListener.prototype =
{
  domain: "browser.urlbar.autoFill",
  observe: function (aSubject, aTopic, aPrefName)
  {
    if (aTopic != "nsPref:changed" || aPrefName != this.domain)
      return;
      
    this.toggleAutoFillInURLBar();
  },
  
  toggleAutoFillInURLBar: function ()
  {
    var prefValue = false;
    try {
      prefValue = gPrefService.getBoolPref(this.domain);
    }
    catch (e) {
    }

    if (prefValue)
      gURLBar.setAttribute("completedefaultindex", "true");
    else
      gURLBar.removeAttribute("completedefaultindex");
  }
}

function ctrlNumberTabSelection(event)
{  
  if (event.altKey && event.keyCode == KeyEvent.DOM_VK_RETURN) {    
    // XXXblake Proper fix is to just check whether focus is in the urlbar. However, focus with the autocomplete widget is all
    // hacky and broken and there's no way to do that right now. So this just patches it to ensure that alt+enter works when focus
    // is on a link.
    if (!document.commandDispatcher.focusedElement || document.commandDispatcher.focusedElement.localName.toLowerCase() != "a") {
      // Don't let winxp beep on ALT+ENTER, since the URL bar uses it.
      event.preventDefault();
      return;
    }
  }

#ifdef XP_MACOSX
  if (!event.metaKey)
#else
  if (!event.ctrlKey)
#endif
    return;

  var index = event.charCode - 49;
  if (index < 0 || index > 8)
    return;

  if (index >= gBrowser.tabContainer.childNodes.length)
    return;

  var oldTab = gBrowser.selectedTab;
  var newTab = gBrowser.tabContainer.childNodes[index];
  if (newTab != oldTab) {
    oldTab.selected = false;
    gBrowser.selectedTab = newTab;
  }

  event.preventDefault();
  event.preventBubble();
  event.preventCapture();
  event.stopPropagation();
}

function gotoHistoryIndex(aEvent)
{
  var index = aEvent.target.getAttribute("index");
  if (!index)
    return false;

  var where = whereToOpenLink(aEvent);

  if (where == "current") {
    // Normal click.  Go there in the current tab and update session history.

    try {
      getWebNavigation().gotoIndex(index);
    }
    catch(ex) {
      return false;
    }
    return true;
  }
  else {
    // Modified click.  Go there in a new tab/window.
    // This code doesn't copy history or work well with framed pages.
    
    var sessionHistory = getWebNavigation().sessionHistory;
    var entry = sessionHistory.getEntryAtIndex(index, false);
    var url = entry.URI.spec;
    openUILinkIn(url, where);
    return true;
  }
}

function BrowserForward(aEvent, aIgnoreAlt)
{
  var where = whereToOpenLink(aEvent, false, aIgnoreAlt);

  if (where == "current") {
    try {
      getWebNavigation().goForward();
    }
    catch(ex) {
    }
  }
  else {
    var sessionHistory = getWebNavigation().sessionHistory;
    var currentIndex = sessionHistory.index;
    var entry = sessionHistory.getEntryAtIndex(currentIndex + 1, false);
    var url = entry.URI.spec;
    openUILinkIn(url, where);
  }
}

function BrowserBack(aEvent, aIgnoreAlt)
{
  var where = whereToOpenLink(aEvent, false, aIgnoreAlt);

  if (where == "current") {
    try {
      getWebNavigation().goBack();
    }
    catch(ex) {
    }
  }
  else {
    var sessionHistory = getWebNavigation().sessionHistory;
    var currentIndex = sessionHistory.index;
    var entry = sessionHistory.getEntryAtIndex(currentIndex - 1, false);
    var url = entry.URI.spec;
    openUILinkIn(url, where);
  }
}

#ifndef XP_MACOSX
function BrowserHandleBackspace()
{
  BrowserBack();
}
#endif

function BrowserBackMenu(event)
{
  return FillHistoryMenu(event.target, "back");
}

function BrowserForwardMenu(event)
{
  return FillHistoryMenu(event.target, "forward");
}

function BrowserStop()
{
  try {
    const stopFlags = nsIWebNavigation.STOP_ALL;
    getWebNavigation().stop(stopFlags);
  }
  catch(ex) {
  }
}

function BrowserReload()
{
  const reloadFlags = nsIWebNavigation.LOAD_FLAGS_NONE;
  return BrowserReloadWithFlags(reloadFlags);
}

function BrowserReloadSkipCache()
{
  // Bypass proxy and cache.
  const reloadFlags = nsIWebNavigation.LOAD_FLAGS_BYPASS_PROXY | nsIWebNavigation.LOAD_FLAGS_BYPASS_CACHE;
  return BrowserReloadWithFlags(reloadFlags);
}

function BrowserHome()
{
  var homePage = gHomeButton.getHomePage();
  loadOneOrMoreURIs(homePage);
}

function BrowserHomeClick(aEvent)
{
  if (aEvent.button == 2) // right-click: do nothing
    return;

  var homePage = gHomeButton.getHomePage();
  var where = whereToOpenLink(aEvent);
  var urls;

  // openUILinkIn in utilityOverlay.js doesn't handle loading multiple pages
  switch (where) {
  case "save":
    urls = homePage.split("|");
    saveURL(urls[0], null, null, true);  // only save the first page
    break;
  case "current":
    loadOneOrMoreURIs(homePage);
    break;
  case "tabshifted":
  case "tab":
    urls = homePage.split("|");
    var firstTabAdded = gBrowser.addTab(urls[0]);
    for (var i = 1; i < urls.length; ++i)
      gBrowser.addTab(urls[i]);
    if ((where == "tab") ^ getBoolPref("browser.tabs.loadBookmarksInBackground", false)) {
      gBrowser.selectedTab = firstTabAdded;
      _content.focus();
    }
    break;
  case "window":
    OpenBrowserWindow();
    break;
  }
}

function loadOneOrMoreURIs(aURIString)
{
  var urls = aURIString.split("|");
  loadURI(urls[0]);
  for (var i = 1; i < urls.length; ++i)
    gBrowser.addTab(urls[i]);
}

function constructGoMenuItem(goMenu, beforeItem, url, title)
{
  var menuitem = document.createElementNS(kXULNS, "menuitem");
  menuitem.setAttribute("statustext", url);
  menuitem.setAttribute("label", title);
  goMenu.insertBefore(menuitem, beforeItem);
  return menuitem;
}

function onGoMenuHidden()
{
  setTimeout(destroyGoMenuItems, 0, document.getElementById('goPopup'));
}

function destroyGoMenuItems(goMenu) {
  var startSeparator = document.getElementById("startHistorySeparator");
  var endSeparator = document.getElementById("endHistorySeparator");
  endSeparator.hidden = true;

  // Destroy the items.
  var destroy = false;
  for (var i = 0; i < goMenu.childNodes.length; i++) {
    var item = goMenu.childNodes[i];
    if (item == endSeparator)
      break;

    if (destroy) {
      i--;
      goMenu.removeChild(item);
    }

    if (item == startSeparator)
      destroy = true;
  }
}

function updateGoMenu(goMenu)
{
  // In case the timer didn't fire.
  destroyGoMenuItems(goMenu);

  var history = document.getElementById("hiddenHistoryTree");
  
  if (history.hidden) {
    history.hidden = false;
    var globalHistory = Components.classes["@mozilla.org/browser/global-history;2"]
                                  .getService(Components.interfaces.nsIRDFDataSource);
    history.database.AddDataSource(globalHistory);
  }

  if (!history.ref)
    history.ref = "NC:HistoryRoot";
  
  var count = history.treeBoxObject.view.rowCount;
  if (count > 10)
    count = 10;

  if (count == 0)
    return;

  const NC_NS     = "http://home.netscape.com/NC-rdf#";

  if (!gRDF)
     gRDF = Components.classes["@mozilla.org/rdf/rdf-service;1"]
                      .getService(Components.interfaces.nsIRDFService);

  var builder = history.builder.QueryInterface(Components.interfaces.nsIXULTreeBuilder);
  
  var beforeItem = document.getElementById("endHistorySeparator");
  
  var nameResource = gRDF.GetResource(NC_NS + "Name");

  var endSep = beforeItem;
  var showSep = false;

  for (var i = count-1; i >= 0; i--) {
    var res = builder.getResourceAtIndex(i);
    var url = res.Value;
    var titleRes = history.database.GetTarget(res, nameResource, true);
    if (!titleRes)
      continue;

    showSep = true;
    var titleLiteral = titleRes.QueryInterface(Components.interfaces.nsIRDFLiteral);
    beforeItem = constructGoMenuItem(goMenu, beforeItem, url, titleLiteral.Value);
  }

  if (showSep)
    endSep.hidden = false;
}

function addBookmarkAs(aBrowser, aIsWebPanel)
{
  const browsers = aBrowser.browsers;
  if (browsers && browsers.length > 1)
    addBookmarkForTabBrowser(aBrowser);
  else
    addBookmarkForBrowser(aBrowser.webNavigation, aIsWebPanel);
}

function addBookmarkForTabBrowser(aTabBrowser, aSelect)
{
  var tabsInfo = [];
  var currentTabInfo = { name: "", url: "", charset: null };

  const activeBrowser = aTabBrowser.selectedBrowser;
  const browsers = aTabBrowser.browsers;
  for (var i = 0; i < browsers.length; ++i) {
    var webNav = browsers[i].webNavigation;
    var url = webNav.currentURI.spec;
    var name = "";
    var charSet;
    try {
      var doc = webNav.document;
      name = doc.title || url;
      charSet = doc.characterSet;
    } catch (e) {
      name = url;
    }
    tabsInfo[i] = { name: name, url: url, charset: charSet };
    if (browsers[i] == activeBrowser)
      currentTabInfo = tabsInfo[i];
  }
  openDialog("chrome://browser/content/bookmarks/addBookmark2.xul", "",
             "centerscreen,chrome,dialog,resizable,dependent",
             currentTabInfo.name, currentTabInfo.url, null,
             currentTabInfo.charset, "addGroup" + (aSelect ? ",group" : ""), tabsInfo);
}

function addBookmarkForBrowser(aDocShell, aIsWebPanel)
{
  // Bug 52536: We obtain the URL and title from the nsIWebNavigation
  // associated with a <browser/> rather than from a DOMWindow.
  // This is because when a full page plugin is loaded, there is
  // no DOMWindow (?) but information about the loaded document
  // may still be obtained from the webNavigation. 
  var url = aDocShell.currentURI.spec;
  var title, charSet = null;
  try {
    title = aDocShell.document.title || url;
    charSet = aDocShell.document.characterSet;
  }
  catch (e) {
    title = url;
  }
  BookmarksUtils.addBookmark(url, title, charSet, aIsWebPanel);
}

function openLocation()
{
  if (gURLBar && !gURLBar.parentNode.parentNode.collapsed) {
    gURLBar.focus();
    gURLBar.select();
  }
  else {
#ifdef XP_MACOSX
    if (window.location.href == "chrome://browser/content/hiddenWindow.xul") {
      // If no windows are active, open a new one. 
      window.openDialog("chrome://browser/content/", "_blank", "chrome,all,dialog=no", "about:blank");
    }
    else
#endif
      openDialog("chrome://browser/content/openLocation.xul", "_blank", "chrome,modal,titlebar", window);
  }
}

function BrowserOpenTab()
{
  gBrowser.selectedTab = gBrowser.addTab('about:blank');
  if (gURLBar)
    setTimeout(function() { gURLBar.focus(); }, 0);
}

/* Called from the openLocation dialog. This allows that dialog to instruct
   its opener to open a new window and then step completely out of the way.
   Anything less byzantine is causing horrible crashes, rather believably,
   though oddly only on Linux. */
function delayedOpenWindow(chrome,flags,url)
{
  // The other way to use setTimeout,
  // setTimeout(openDialog, 10, chrome, "_blank", flags, url),
  // doesn't work here.  The extra "magic" extra argument setTimeout adds to
  // the callback function would confuse prepareForStartup() by making
  // window.arguments[1] be an integer instead of null.
  setTimeout(function() { openDialog(chrome, "_blank", flags, url); }, 10);
}

/* Required because the tab needs time to set up its content viewers and get the load of
   the URI kicked off before becoming the active content area. */
function delayedOpenTab(url)
{
  setTimeout(function(aTabElt) { gBrowser.selectedTab = aTabElt; }, 0, gBrowser.addTab(url));
}

function BrowserOpenFileWindow()
{
  // Get filepicker component.
  try {
    const nsIFilePicker = Components.interfaces.nsIFilePicker;
    var fp = Components.classes["@mozilla.org/filepicker;1"].createInstance(nsIFilePicker);
    fp.init(window, gNavigatorBundle.getString("openFile"), nsIFilePicker.modeOpen);
    fp.appendFilters(nsIFilePicker.filterAll | nsIFilePicker.filterText | nsIFilePicker.filterImages |
                     nsIFilePicker.filterXML | nsIFilePicker.filterHTML);

    if (fp.show() == nsIFilePicker.returnOK)
      openTopWin(fp.fileURL.spec);
  } catch (ex) {
  }
}

function BrowserCloseTabOrWindow()
{
  if (gBrowser.localName == 'tabbrowser' && gBrowser.tabContainer.childNodes.length > 1) {
    // Just close up a tab.
    gBrowser.removeCurrentTab();
    return;
  }

  BrowserCloseWindow();
}

function BrowserCloseWindow() 
{
  // This code replicates stuff in Shutdown().  It is here because
  // window.screenX and window.screenY have real values.  We need
  // to fix this eventually but by replicating the code here, we
  // provide a means of saving position (it just requires that the
  // user close the window via File->Close (vs. close box).
  
  // Get the current window position/size.
  var x = window.screenX;
  var y = window.screenY;
  var h = window.outerHeight;
  var w = window.outerWidth;

  // Store these into the window attributes (for persistence).
  var win = document.getElementById( "main-window" );
  win.setAttribute( "x", x );
  win.setAttribute( "y", y );
  win.setAttribute( "height", h );
  win.setAttribute( "width", w );

  closeWindow(true);
}

function loadURI(uri, referrer, postData)
{
  try {
    if (postData === undefined)
      postData = null;
    getWebNavigation().loadURI(uri, nsIWebNavigation.LOAD_FLAGS_NONE, referrer, postData, null);
  } catch (e) {
  }
}

function BrowserLoadURL(aTriggeringEvent, aPostData)
{
  var url = gURLBar.value;
  if (url.match(/^view-source:/)) {
    BrowserViewSourceOfURL(url.replace(/^view-source:/, ""), null, null);
  } else {
    if (gBrowser.localName == "tabbrowser" &&
        aTriggeringEvent && 'altKey' in aTriggeringEvent &&
        aTriggeringEvent.altKey) {
      _content.focus();
      var t = gBrowser.addTab(url, aPostData); // open link in new tab
      gBrowser.selectedTab = t;
      gURLBar.value = url;
      event.preventDefault();
      event.preventBubble();
      event.preventCapture();
      event.stopPropagation();
    }
    else  
      loadURI(url, null, aPostData);
    _content.focus();
  }
}

function getShortcutOrURI(aURL, aPostDataRef)
{
  // rjc: added support for URL shortcuts (3/30/1999)
  try {
    var shortcutURL = BMSVC.resolveKeyword(aURL, aPostDataRef);
    if (!shortcutURL) {
      // rjc: add support for string substitution with shortcuts (4/4/2000)
      //      (see bug # 29871 for details)
      var aOffset = aURL.indexOf(" ");
      if (aOffset > 0) {
        var cmd = aURL.substr(0, aOffset);
        var text = aURL.substr(aOffset+1);
        shortcutURL = BMSVC.resolveKeyword(cmd, aPostDataRef);
        if (shortcutURL && text) {
          if (aPostDataRef && aPostDataRef.value) {
            // XXXben - currently we only support "application/x-www-form-urlencoded"
            //          enctypes.
            aPostDataRef.value = unescape(aPostDataRef.value);
            if (aPostDataRef.value.match(/%s/))
              aPostDataRef.value = getPostDataStream(aPostDataRef.value, text, 
                                                     "application/x-www-form-urlencoded");
            else {
              shortcutURL = null;
              aPostDataRef.value = null;
            }
          }
          else
            shortcutURL = shortcutURL.match(/%s/) ? shortcutURL.replace(/%s/, text) : null;
        }
      }
    }

    if (shortcutURL)
      aURL = shortcutURL;

  } catch (ex) {
  }
  return aURL;
}

#if 0
// XXXben - this is only useful if we ever support text/plain encoded forms in
// smart keywords. 
function normalizePostData(aStringData)
{
  var parts = aStringData.split("&");
  var result = "";
  for (var i = 0; i < parts.length; ++i) {
    var part = unescape(parts[i]);
    if (part)
      result += part + "\r\n";
  }
  return result;
}
#endif
function getPostDataStream(aStringData, aKeyword, aType)
{
  var dataStream = Components.classes["@mozilla.org/io/string-input-stream;1"]
                            .createInstance(Components.interfaces.nsIStringInputStream);
  aStringData = aStringData.replace(/%s/, aKeyword);
  dataStream.setData(aStringData, aStringData.length);

  var mimeStream = Components.classes["@mozilla.org/network/mime-input-stream;1"]
                              .createInstance(Components.interfaces.nsIMIMEInputStream);
  mimeStream.addHeader("Content-Type", aType);
  mimeStream.addContentLength = true;
  mimeStream.setData(dataStream);
  return mimeStream.QueryInterface(Components.interfaces.nsIInputStream);
}


function readFromClipboard()
{
  var url;

  try {
    // Get clipboard.
    var clipboard = Components.classes["@mozilla.org/widget/clipboard;1"]
                              .getService(Components.interfaces.nsIClipboard);

    // Create tranferable that will transfer the text.
    var trans = Components.classes["@mozilla.org/widget/transferable;1"]
                          .createInstance(Components.interfaces.nsITransferable);

    trans.addDataFlavor("text/unicode");

    // If available, use selection clipboard, otherwise global one
    if (clipboard.supportsSelectionClipboard())
      clipboard.getData(trans, clipboard.kSelectionClipboard);
    else
      clipboard.getData(trans, clipboard.kGlobalClipboard);

    var data = {};
    var dataLen = {};
    trans.getTransferData("text/unicode", data, dataLen);

    if (data) {
      data = data.value.QueryInterface(Components.interfaces.nsISupportsString);
      url = data.data.substring(0, dataLen.value / 2);
    }
  } catch (ex) {
  }

  return url;
}

function BrowserViewSourceOfDocument(aDocument)
{
  var docCharset;
  var pageCookie;
  var webNav;

  // Get the document charset
  docCharset = "charset=" + aDocument.characterSet;

  // Get the nsIWebNavigation associated with the document
  try {
      var win;
      var ifRequestor;

      // Get the DOMWindow for the requested document.  If the DOMWindow
      // cannot be found, then just use the _content window...
      //
      // XXX:  This is a bit of a hack...
      win = aDocument.defaultView;
      if (win == window) {
        win = _content;
      }
      ifRequestor = win.QueryInterface(Components.interfaces.nsIInterfaceRequestor);

      webNav = ifRequestor.getInterface(nsIWebNavigation);
  } catch(err) {
      // If nsIWebNavigation cannot be found, just get the one for the whole
      // window...
      webNav = getWebNavigation();
  }
  //
  // Get the 'PageDescriptor' for the current document. This allows the
  // view-source to access the cached copy of the content rather than
  // refetching it from the network...
  //
  try{
    var PageLoader = webNav.QueryInterface(Components.interfaces.nsIWebPageDescriptor);

    pageCookie = PageLoader.currentDescriptor;
  } catch(err) {
    // If no page descriptor is available, just use the view-source URL...
  }

  BrowserViewSourceOfURL(webNav.currentURI.spec, docCharset, pageCookie);
}

function BrowserViewSourceOfURL(url, charset, pageCookie)
{
  // try to open a view-source window while inheriting the charset (if any)
  openDialog("chrome://global/content/viewSource.xul",
             "_blank",
             "scrollbars,resizable,chrome,dialog=no",
             url, charset, pageCookie);
}

// doc=null for regular page info, doc=owner document for frame info.
function BrowserPageInfo(doc)
{
  window.openDialog("chrome://browser/content/pageInfo.xul",
                    "_blank",
                    "chrome,dialog=no",
                    doc);
}

#ifdef DEBUG
// Initialize the LeakDetector class.
function LeakDetector(verbose)
{
  this.verbose = verbose;
}

const NS_LEAKDETECTOR_CONTRACTID = "@mozilla.org/xpcom/leakdetector;1";

if (NS_LEAKDETECTOR_CONTRACTID in Components.classes) {
  try {
    LeakDetector.prototype = Components.classes[NS_LEAKDETECTOR_CONTRACTID]
                                       .createInstance(Components.interfaces.nsILeakDetector);
  } catch (err) {
    LeakDetector.prototype = Object.prototype;
  }
} else {
  LeakDetector.prototype = Object.prototype;
}

var leakDetector = new LeakDetector(false);

// Dumps current set of memory leaks.
function dumpMemoryLeaks()
{
  leakDetector.dumpLeaks();
}

// Traces all objects reachable from the chrome document.
function traceChrome()
{
  leakDetector.traceObject(document, leakDetector.verbose);
}

// Traces all objects reachable from the content document.
function traceDocument()
{
  // keep the chrome document out of the dump.
  leakDetector.markObject(document, true);
  leakDetector.traceObject(_content, leakDetector.verbose);
  leakDetector.markObject(document, false);
}

// Controls whether or not we do verbose tracing.
function traceVerbose(verbose)
{
  leakDetector.verbose = (verbose == "true");
}
#endif

function checkForDirectoryListing()
{
  if ( "HTTPIndex" in _content &&
       _content.HTTPIndex instanceof Components.interfaces.nsIHTTPIndex ) {
    _content.defaultCharacterset = getMarkupDocumentViewer().defaultCharacterSet;
  }
}

function URLBarFocusHandler(aEvent, aElt)
{
  if (gIgnoreFocus)
    gIgnoreFocus = false;
  else if (gClickSelectsAll)
    aElt.select();
}

function URLBarMouseDownHandler(aEvent, aElt)
{
  if (aElt.hasAttribute("focused")) {
    gIgnoreClick = true;
  } else {
    gIgnoreFocus = true;
    gIgnoreClick = false;
    aElt.setSelectionRange(0, 0);
  }
}

function URLBarClickHandler(aEvent, aElt)
{
  if (!gIgnoreClick && gClickSelectsAll && aElt.selectionStart == aElt.selectionEnd)
    aElt.select();
}

// If "ESC" is pressed in the url bar, we replace the urlbar's value with the url of the page
// and highlight it, unless it is about:blank, where we reset it to "".
function handleURLBarRevert()
{
  var url = getWebNavigation().currentURI.spec;
  var throbberElement = document.getElementById("navigator-throbber");

  var isScrolling = gURLBar.popupOpen;
  
  // don't revert to last valid url unless page is NOT loading
  // and user is NOT key-scrolling through autocomplete list
  if ((!throbberElement || !throbberElement.hasAttribute("busy")) && !isScrolling) {
    if (url != "about:blank") { 
      gURLBar.value = url;
      gURLBar.select();
      SetPageProxyState("valid", null); // XXX Build a URI and pass it in here.
    } else { //if about:blank, urlbar becomes ""
      gURLBar.value = "";
    }
  }
  
  gBrowser.userTypedValue = null;

  // tell widget to revert to last typed text only if the user
  // was scrolling when they hit escape
  return !isScrolling; 
}

function handleURLBarCommand(aTriggeringEvent)
{
  var postData = { };
  canonizeUrl(aTriggeringEvent, postData);

  try { 
    addToUrlbarHistory();
  } catch (ex) {
    // Things may go wrong when adding url to session history,
    // but don't let that interfere with the loading of the url.
  }
  
  BrowserLoadURL(aTriggeringEvent, postData.value); 
}

function canonizeUrl(aTriggeringEvent, aPostDataRef)
{
  if (!gURLBar)
    return;
  
  var url = gURLBar.value;

  // Prevent suffix when already exists www , http , /
  if (!/^(www|http)|\/\s*$/i.test(url)) {
    var suffix = null;

    if (aTriggeringEvent && 'ctrlKey' in aTriggeringEvent &&
        aTriggeringEvent.ctrlKey && 'shiftKey' in aTriggeringEvent &&
        aTriggeringEvent.shiftKey)
      suffix = ".org/";

    else if (aTriggeringEvent && 'ctrlKey' in aTriggeringEvent &&
        aTriggeringEvent.ctrlKey)
      suffix = ".com/";

    else if (aTriggeringEvent && 'shiftKey' in aTriggeringEvent &&
        aTriggeringEvent.shiftKey)
      suffix = ".net/";

    if (suffix != null) {
      // trim leading/trailing spaces (bug 233205)
      url = url.replace( /^\s+/, "");
      url = url.replace( /\s+$/, "");
      // Tack www. and suffix on.
      url = "http://www." + url + suffix;
    }
  }

  gURLBar.value = getShortcutOrURI(url, aPostDataRef);
}

function UpdatePageProxyState()
{
  if (gURLBar && gURLBar.value != gLastValidURLStr)
    SetPageProxyState("invalid", null);
}

function SetPageProxyState(aState, aURI)
{
  if (!gURLBar)
    return;

  if (!gProxyButton)
    gProxyButton = document.getElementById("page-proxy-button");
  if (!gProxyFavIcon)
    gProxyFavIcon = document.getElementById("page-proxy-favicon");
  if (!gProxyDeck)
    gProxyDeck = document.getElementById("page-proxy-deck");

  gProxyButton.setAttribute("pageproxystate", aState);

  // the page proxy state is set to valid via OnLocationChange, which
  // gets called when we switch tabs.  We'll let updatePageFavIcon
  // take care of updating the mFavIconURL because it knows exactly
  // for which tab to update things, instead of confusing the issue
  // here.
  if (aState == "valid") {
    gLastValidURLStr = gURLBar.value;
    gURLBar.addEventListener("input", UpdatePageProxyState, false);

    if (gBrowser.mCurrentBrowser.mFavIconURL != null) {
      gProxyFavIcon.setAttribute("src", gBrowser.mCurrentBrowser.mFavIconURL);
      gProxyDeck.selectedIndex = 1;
    } else {
      gProxyDeck.selectedIndex = 0;
      gProxyFavIcon.removeAttribute("src");
    }
  } else if (aState == "invalid") {
    gURLBar.removeEventListener("input", UpdatePageProxyState, false);
    gProxyDeck.selectedIndex = 0;
    gProxyFavIcon.removeAttribute("src");
  }

}

function PageProxyDragGesture(aEvent)
{
  if (gProxyButton.getAttribute("pageproxystate") == "valid") {
    nsDragAndDrop.startDrag(aEvent, proxyIconDNDObserver);
    return true;
  }
  return false;
}

function URLBarOnDrop(evt)
{
  nsDragAndDrop.drop(evt, urlbarObserver);
}

var urlbarObserver = {
  onDrop: function (aEvent, aXferData, aDragSession)
    {
      var url = transferUtils.retrieveURLFromData(aXferData.data, aXferData.flavour.contentType);

      // The URL bar automatically handles inputs with newline characters, 
      // so we can get away with treating text/x-moz-url flavours as text/unicode.
      if (url)  {
        // XXXBlake Workaround caret crash when you try to set the textbox's value on dropping
        setTimeout(function(u) { gURLBar.value = u; handleURLBarCommand(); }, 0, url);
      }
    },
  getSupportedFlavours: function ()
    {
      var flavourSet = new FlavourSet();

      // Plain text drops are often misidentified as "text/x-moz-url", so favor plain text.
      flavourSet.appendFlavour("text/unicode");
      flavourSet.appendFlavour("text/x-moz-url");
      flavourSet.appendFlavour("application/x-moz-file", "nsIFile");     
      return flavourSet;
    }
}

function updateToolbarStates(toolbarMenuElt)
{
  if (!gHaveUpdatedToolbarState) {
    var mainWindow = document.getElementById("main-window");
    if (mainWindow.hasAttribute("chromehidden")) {
      gHaveUpdatedToolbarState = true;
      var i;
      for (i = 0; i < toolbarMenuElt.childNodes.length; ++i)
        document.getElementById(toolbarMenuElt.childNodes[i].getAttribute("observes")).removeAttribute("checked");
      var toolbars = document.getElementsByTagName("toolbar");
      
      // Start i at 1, since we skip the menubar.
      for (i = 1; i < toolbars.length; ++i) {
        if (toolbars[i].getAttribute("class").indexOf("chromeclass") != -1)
          toolbars[i].setAttribute("collapsed", "true");
      }
      var statusbars = document.getElementsByTagName("statusbar");
      for (i = 1; i < statusbars.length; ++i) {
        if (statusbars[i].getAttribute("class").indexOf("chromeclass") != -1)
          statusbars[i].setAttribute("collapsed", "true");
      }
      mainWindow.removeAttribute("chromehidden");
    }
  }
}

function BrowserImport()
{
#ifdef XP_MACOSX
  var features = "centerscreen,chrome,resizable=no";
#else
  var features = "modal,centerscreen,chrome,resizable=no";
#endif
  window.openDialog("chrome://browser/content/migration/migration.xul", "migration", features);
}

function BrowserFullScreen()
{
  window.fullScreen = !window.fullScreen;
}

function onFullScreen()
{
  FullScreen.toggle();
}

function getWebNavigation()
{
  try {
    return gBrowser.webNavigation;
  } catch (e) {
    return null;
  }
}

function BrowserReloadWithFlags(reloadFlags)
{
  /* First, we'll try to use the session history object to reload so 
   * that framesets are handled properly. If we're in a special 
   * window (such as view-source) that has no session history, fall 
   * back on using the web navigation's reload method.
   */

  var webNav = getWebNavigation();
  try {
    var sh = webNav.sessionHistory;
    if (sh)
      webNav = sh.QueryInterface(nsIWebNavigation);
  } catch (e) {
  }

  try {
    webNav.reload(reloadFlags);
  } catch (e) {
  }
}

function toggleAffectedChrome(aHide)
{
  // chrome to toggle includes:
  //   (*) menubar
  //   (*) navigation bar
  //   (*) bookmarks toolbar
  //   (*) sidebar
  //   (*) find bar

  var navToolbox = document.getElementById("navigator-toolbox");
  navToolbox.hidden = aHide;
  if (aHide)
  {
    gChromeState = {};
    var sidebar = document.getElementById("sidebar-box");
    gChromeState.sidebarOpen = !sidebar.hidden;
    gSidebarCommand = sidebar.getAttribute("sidebarcommand");
      
    var findBar = document.getElementById("FindToolbar");
    gChromeState.findOpen = !findBar.hidden;
    closeFindBar();
  }
  else {
    if (gChromeState.findOpen)
      openFindBar();
  }
  
  if (gChromeState.sidebarOpen)
    toggleSidebar(gSidebarCommand);
}

function onEnterPrintPreview()
{
  toggleAffectedChrome(true);
}

function onExitPrintPreview()
{
  // restore chrome to original state
  toggleAffectedChrome(false);
}

function getMarkupDocumentViewer()
{
  return gBrowser.markupDocumentViewer;
}

/**
 * Content area tooltip.
 * XXX - this must move into XBL binding/equiv! Do not want to pollute
 *       browser.js with functionality that can be encapsulated into
 *       browser widget. TEMPORARY!
 *
 * NOTE: Any changes to this routine need to be mirrored in ChromeListener::FindTitleText()
 *       (located in mozilla/embedding/browser/webBrowser/nsDocShellTreeOwner.cpp)
 *       which performs the same function, but for embedded clients that
 *       don't use a XUL/JS layer. It is important that the logic of
 *       these two routines be kept more or less in sync.
 *       (pinkerton)
 **/
function FillInHTMLTooltip(tipElement)
{
  var retVal = false;
  if (tipElement.namespaceURI == "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul")
    return retVal;

  const XLinkNS = "http://www.w3.org/1999/xlink";


  var titleText = null;
  var XLinkTitleText = null;
  
  while (!titleText && !XLinkTitleText && tipElement) {
    if (tipElement.nodeType == Node.ELEMENT_NODE) {
      titleText = tipElement.getAttribute("title");
      XLinkTitleText = tipElement.getAttributeNS(XLinkNS, "title");
    }
    tipElement = tipElement.parentNode;
  }

  var texts = [titleText, XLinkTitleText];
  var tipNode = document.getElementById("aHTMLTooltip");

  for (var i = 0; i < texts.length; ++i) {
    var t = texts[i];
    if (t && t.search(/\S/) >= 0) {
      tipNode.setAttribute("label", t);
      retVal = true;
    }
  }

  return retVal;
}

var proxyIconDNDObserver = {
  onDragStart: function (aEvent, aXferData, aDragAction)
    {
      var value = gURLBar.value;
      // XXX - do we want to allow the user to set a blank page to their homepage?
      //       if so then we want to modify this a little to set about:blank as
      //       the homepage in the event of an empty urlbar.
      if (!value) return;

      var urlString = value + "\n" + window._content.document.title;
      var htmlString = "<a href=\"" + value + "\">" + value + "</a>";

      aXferData.data = new TransferData();
      aXferData.data.addDataForFlavour("text/x-moz-url", urlString);
      aXferData.data.addDataForFlavour("text/unicode", value);
      aXferData.data.addDataForFlavour("text/html", htmlString);

      // we're copying the URL from the proxy icon, not moving
      // we specify all of them though, because d&d sucks and OS's
      // get confused if they don't get the one they want
      aDragAction.action =
        Components.interfaces.nsIDragService.DRAGDROP_ACTION_COPY |
        Components.interfaces.nsIDragService.DRAGDROP_ACTION_MOVE |
        Components.interfaces.nsIDragService.DRAGDROP_ACTION_LINK;
    }
}

var homeButtonObserver = {
  onDrop: function (aEvent, aXferData, aDragSession)
    {
      var url = transferUtils.retrieveURLFromData(aXferData.data, aXferData.flavour.contentType);
      setTimeout(openHomeDialog, 0, url);
    },

  onDragOver: function (aEvent, aFlavour, aDragSession)
    {
      var statusTextFld = document.getElementById("statusbar-display");
      statusTextFld.label = gNavigatorBundle.getString("droponhomebutton");
      aDragSession.dragAction = Components.interfaces.nsIDragService.DRAGDROP_ACTION_LINK;
    },

  onDragExit: function (aEvent, aDragSession)
    {
      var statusTextFld = document.getElementById("statusbar-display");
      statusTextFld.label = "";
    },

  getSupportedFlavours: function ()
    {
      var flavourSet = new FlavourSet();
      flavourSet.appendFlavour("application/x-moz-file", "nsIFile");
      flavourSet.appendFlavour("text/x-moz-url");
      flavourSet.appendFlavour("text/unicode");
      return flavourSet;
    }
}

function openHomeDialog(aURL)
{
  var promptService = Components.classes["@mozilla.org/embedcomp/prompt-service;1"].getService(Components.interfaces.nsIPromptService);
  var promptTitle = gNavigatorBundle.getString("droponhometitle");
  var promptMsg   = gNavigatorBundle.getString("droponhomemsg");
  var pressedVal  = promptService.confirmEx(window, promptTitle, promptMsg,
                          (promptService.BUTTON_TITLE_YES * promptService.BUTTON_POS_0) +
                          (promptService.BUTTON_TITLE_NO * promptService.BUTTON_POS_1),
                          null, null, null, null, {value:0});

  if (pressedVal == 0) {
    try {
      var str = Components.classes["@mozilla.org/supports-string;1"]
                          .createInstance(Components.interfaces.nsISupportsString);
      str.data = aURL;
      gPrefService.setComplexValue("browser.startup.homepage",
                                   Components.interfaces.nsISupportsString, str);
      var homeButton = document.getElementById("home-button");
      homeButton.setAttribute("tooltiptext", aURL);
    } catch (ex) {
      dump("Failed to set the home page.\n"+ex+"\n");
    }
  }
}

var bookmarksButtonObserver = {
  onDrop: function (aEvent, aXferData, aDragSession)
  {
    var split = aXferData.data.split("\n");
    var url = split[0];
    if (url != aXferData.data) {  //do nothing if it's not a valid URL
      var name = split[1];
      openDialog("chrome://browser/content/bookmarks/addBookmark2.xul", "",
                 "centerscreen,chrome,dialog,resizable,dependent", name, url);
    }
  },

  onDragOver: function (aEvent, aFlavour, aDragSession)
  {
    var statusTextFld = document.getElementById("statusbar-display");
    statusTextFld.label = gNavigatorBundle.getString("droponbookmarksbutton");
    aDragSession.dragAction = Components.interfaces.nsIDragService.DRAGDROP_ACTION_LINK;
  },

  onDragExit: function (aEvent, aDragSession)
  {
    var statusTextFld = document.getElementById("statusbar-display");
    statusTextFld.label = "";
  },

  getSupportedFlavours: function ()
  {
    var flavourSet = new FlavourSet();
    flavourSet.appendFlavour("application/x-moz-file", "nsIFile");
    flavourSet.appendFlavour("text/x-moz-url");
    flavourSet.appendFlavour("text/unicode");
    return flavourSet;
  }
}

var goButtonObserver = {
  onDragOver: function(aEvent, aFlavour, aDragSession)
    {
      var statusTextFld = document.getElementById("statusbar-display");
      statusTextFld.label = gNavigatorBundle.getString("dropongobutton");
      aEvent.target.setAttribute("dragover", "true");
      return true;
    },
  onDragExit: function (aEvent, aDragSession)
    {
      var statusTextFld = document.getElementById("statusbar-display");
      statusTextFld.label = "";
      aEvent.target.removeAttribute("dragover");
    },
  onDrop: function (aEvent, aXferData, aDragSession)
    {
      var xferData = aXferData.data.split("\n");
      var uri = xferData[0] ? xferData[0] : xferData[1];
      if (uri)
        loadURI(uri, null, null);
    },
  getSupportedFlavours: function ()
    {
      var flavourSet = new FlavourSet();
      flavourSet.appendFlavour("application/x-moz-file", "nsIFile");
      flavourSet.appendFlavour("text/x-moz-url");
      flavourSet.appendFlavour("text/unicode");
      return flavourSet;
    }
}

var DownloadsButtonDNDObserver = {
  /////////////////////////////////////////////////////////////////////////////
  // nsDragAndDrop
  onDragOver: function (aEvent, aFlavour, aDragSession)
  {
    var statusTextFld = document.getElementById("statusbar-display");
    statusTextFld.label = gNavigatorBundle.getString("dropondownloadsbutton");
    aDragSession.canDrop = (aFlavour.contentType == "text/x-moz-url" || 
                            aFlavour.contentType == "text/unicode");
  },

  onDragExit: function (aEvent, aDragSession)
  {
    var statusTextFld = document.getElementById("statusbar-display");
    statusTextFld.label = "";
  },

  onDrop: function (aEvent, aXferData, aDragSession)
  {
    var split = aXferData.data.split("\n");
    var url = split[0];
    if (url != aXferData.data) {  //do nothing, not a valid URL
      var name = split[1];
      saveURL(url, name, null, true, true);
    }
  },
  getSupportedFlavours: function ()
  {
    var flavourSet = new FlavourSet();
    flavourSet.appendFlavour("text/x-moz-url");
    flavourSet.appendFlavour("text/unicode");
    return flavourSet;
  }
}

function focusSearchBar()
{
  var searchBar = document.getElementsByTagName("searchbar");
  if (searchBar.length > 0) {
    searchBar[0].select();
    searchBar[0].focus();
  }
}

function OpenSearch(tabName, searchStr, newTabFlag)
{
  //This function needs to be split up someday.
  //XXXnoririty I don't want any prefs switching open by tabs to window
  //XXXpch: this routine needs to be cleaned up.

  var defaultSearchURL = null;
  var navigatorRegionBundle = document.getElementById("bundle_browser_region");
  var fallbackDefaultSearchURL = navigatorRegionBundle.getString("fallbackDefaultSearchURL");

  //Check to see if search string contains "://" or "ftp." or white space.
  //If it does treat as url and match for pattern
  
  var urlmatch= /(:\/\/|^ftp\.)[^ \S]+$/ 
  var forceAsURL = urlmatch.test(searchStr);

  try {
    defaultSearchURL = gPrefService.getComplexValue("browser.search.defaulturl",
                                            Components.interfaces.nsIPrefLocalizedString).data;
  } catch (ex) {
  }

  // Fallback to a default url (one that we can get sidebar search results for)
  if (!defaultSearchURL)
    defaultSearchURL = fallbackDefaultSearchURL;

  if (!searchStr) {
    BrowserSearchInternet();
  } else {

    //Check to see if location bar field is a url
    //If it is a url go to URL.  A Url is "://" or "." as commented above
    //Otherwise search on entry
    if (forceAsURL) {
       BrowserLoadURL(null, null)
    } else {
      if (searchStr) {
        var escapedSearchStr = encodeURIComponent(searchStr);
        defaultSearchURL += escapedSearchStr;
        var searchDS = Components.classes["@mozilla.org/rdf/datasource;1?name=internetsearch"]
                                 .getService(Components.interfaces.nsIInternetSearchService);

        searchDS.RememberLastSearchText(escapedSearchStr);
        try {
          var searchEngineURI = gPrefService.getCharPref("browser.search.defaultengine");
          if (searchEngineURI) {          
            var searchURL = getSearchUrl("actionButton");
            if (searchURL) {
              defaultSearchURL = searchURL + escapedSearchStr; 
            } else {
              searchURL = searchDS.GetInternetSearchURL(searchEngineURI, escapedSearchStr, 0, 0, {value:0});
              if (searchURL)
                defaultSearchURL = searchURL;
            }
          }
        } catch (ex) {
        }

        if (!newTabFlag) {
          loadURI(defaultSearchURL, null, null);
        }
        else {
          var newTab = getBrowser().addTab(defaultSearchURL);
          if (!pref.getBoolPref("browser.tabs.loadInBackground"))
            getBrowser().selectedTab = newTab;
        }
      }
    }
  }
}

function FillHistoryMenu(aParent, aMenu)
  {
    // Remove old entries if any
    deleteHistoryItems(aParent);

    var sessionHistory = getWebNavigation().sessionHistory;

    var count = sessionHistory.count;
    var index = sessionHistory.index;
    var end;
    var j;
    var entry;

    switch (aMenu)
      {
        case "back":
          end = (index > MAX_HISTORY_MENU_ITEMS) ? index - MAX_HISTORY_MENU_ITEMS : 0;
          if ((index - 1) < end) return false;
          for (j = index - 1; j >= end; j--)
            {
              entry = sessionHistory.getEntryAtIndex(j, false);
              if (entry)
                createMenuItem(aParent, j, entry.title);
            }
          break;
        case "forward":
          end  = ((count-index) > MAX_HISTORY_MENU_ITEMS) ? index + MAX_HISTORY_MENU_ITEMS : count;
          if ((index + 1) >= end) return false;
          for (j = index + 1; j < end; j++)
            {
              entry = sessionHistory.getEntryAtIndex(j, false);
              if (entry)
                createMenuItem(aParent, j, entry.title);
            }
          break;
        case "go":
          aParent.lastChild.hidden = (count == 0);
          end = count > MAX_HISTORY_MENU_ITEMS ? count - MAX_HISTORY_MENU_ITEMS : 0;
          for (j = count - 1; j >= end; j--)
            {
              entry = sessionHistory.getEntryAtIndex(j, false);
              if (entry)
                createRadioMenuItem(aParent, j, entry.title, j==index);
            }
          break;
      }
    return true;
  }

function addToUrlbarHistory()
{
  var urlToAdd = gURLBar.value;
  if (!urlToAdd)
     return;
  if (urlToAdd.search(/[\x00-\x1F]/) != -1) // don't store bad URLs
     return;

  if (!gGlobalHistory)
    gGlobalHistory = Components.classes["@mozilla.org/browser/global-history;2"]
                               .getService(Components.interfaces.nsIBrowserHistory);
  
  if (!gURIFixup)
    gURIFixup = Components.classes["@mozilla.org/docshell/urifixup;1"]
                          .getService(Components.interfaces.nsIURIFixup);
   try {
     if (urlToAdd.indexOf(" ") == -1) {
       var fixedUpURI = gURIFixup.createFixupURI(urlToAdd, 0);
       gGlobalHistory.markPageAsTyped(fixedUpURI);
     }
   }
   catch(ex) {
   }
}

function createMenuItem( aParent, aIndex, aLabel)
  {
    var menuitem = document.createElement( "menuitem" );
    menuitem.setAttribute( "label", aLabel );
    menuitem.setAttribute( "index", aIndex );
    aParent.appendChild( menuitem );
  }

function createRadioMenuItem( aParent, aIndex, aLabel, aChecked)
  {
    var menuitem = document.createElement( "menuitem" );
    menuitem.setAttribute( "type", "radio" );
    menuitem.setAttribute( "label", aLabel );
    menuitem.setAttribute( "index", aIndex );
    if (aChecked==true)
      menuitem.setAttribute( "checked", "true" );
    aParent.appendChild( menuitem );
  }

function deleteHistoryItems(aParent)
{
  var children = aParent.childNodes;
  for (var i = 0; i < children.length; i++)
    {
      var index = children[i].getAttribute("index");
      if (index)
        aParent.removeChild(children[i]);
    }
}

function toJavaScriptConsole()
{
  toOpenWindowByType("global:console", "chrome://global/content/console.xul");
}

function toOpenWindowByType(inType, uri, features)
{
  var windowManager = Components.classes['@mozilla.org/appshell/window-mediator;1'].getService();
  var windowManagerInterface = windowManager.QueryInterface(Components.interfaces.nsIWindowMediator);
  var topWindow = windowManagerInterface.getMostRecentWindow(inType);
  
  if (topWindow)
    topWindow.focus();
  else if (features)
    window.open(uri, "_blank", features);
  else
    window.open(uri, "_blank", "chrome,extrachrome,menubar,resizable,scrollbars,status,toolbar");
}


function OpenBrowserWindow()
{
  var charsetArg = new String();
  var handler = Components.classes['@mozilla.org/commandlinehandler/general-startup;1?type=browser'];
  handler = handler.getService();
  handler = handler.QueryInterface(Components.interfaces.nsICmdLineHandler);
  var startpage = handler.defaultArgs;
  var url = handler.chromeUrlForTask;
  var wintype = document.firstChild.getAttribute('windowtype');

  // if and only if the current window is a browser window and it has a document with a character
  // set, then extract the current charset menu setting from the current document and use it to
  // initialize the new browser window...
  if (window && (wintype == "navigator:browser") && window._content && window._content.document)
  {
    var DocCharset = window._content.document.characterSet;
    charsetArg = "charset="+DocCharset;

    //we should "inherit" the charset menu setting in a new window
    window.openDialog(url, "_blank", "chrome,all,dialog=no", startpage, charsetArg);
  }
  else // forget about the charset information.
  {
    window.openDialog(url, "_blank", "chrome,all,dialog=no", startpage);
  }
}

function openAboutDialog()
{
  window.openDialog("chrome://browser/content/aboutDialog.xul", "About", "modal,centerscreen,chrome,resizable=no");
}

function BrowserCustomizeToolbar()
{
  // Disable the toolbar context menu items
  var menubar = document.getElementById("main-menubar");
  for (var i = 0; i < menubar.childNodes.length; ++i)
    menubar.childNodes[i].setAttribute("disabled", true);
    
  var cmd = document.getElementById("cmd_CustomizeToolbars");
  cmd.setAttribute("disabled", "true");
  
  window.openDialog("chrome://global/content/customizeToolbar.xul", "CustomizeToolbar",
                    "chrome,all,dependent", document.getElementById("navigator-toolbox"));
}

function BrowserToolboxCustomizeDone(aToolboxChanged)
{
  // Update global UI elements that may have been added or removed
  if (aToolboxChanged) {
    gURLBar = document.getElementById("urlbar");
    gProxyButton = document.getElementById("page-proxy-button");
    gProxyFavIcon = document.getElementById("page-proxy-favicon");
    gProxyDeck = document.getElementById("page-proxy-deck");
    gHomeButton.updateTooltip();
    window.XULBrowserWindow.init();
  }

  // Update the urlbar
  var url = getWebNavigation().currentURI.spec;
  if (gURLBar) {
    gURLBar.value = url;
    var uri = Components.classes["@mozilla.org/network/standard-url;1"]
                        .createInstance(Components.interfaces.nsIURI);
    uri.spec = url;
    SetPageProxyState("valid", uri);
  }

  // Re-enable parts of the UI we disabled during the dialog
  var menubar = document.getElementById("main-menubar");
  for (var i = 0; i < menubar.childNodes.length; ++i)
    menubar.childNodes[i].setAttribute("disabled", false);
  var cmd = document.getElementById("cmd_CustomizeToolbars");
  cmd.removeAttribute("disabled");

  // fix up the personal toolbar folder
  var bt = document.getElementById("bookmarks-ptf");
  if (bt) {
    var btf = BMSVC.getBookmarksToolbarFolder().Value;
    var btchevron = document.getElementById("bookmarks-chevron");
    bt.ref = btf;
    btchevron.ref = btf;
    // no uniqueness is guaranteed, so we have to remove first
    try {
      bt.database.RemoveObserver(BookmarksToolbarRDFObserver);
      bt.controllers.removeController(BookmarksMenuController);
    } catch (ex) {
      // ignore
    }
    bt.database.AddObserver(BookmarksToolbarRDFObserver);
    bt.controllers.appendController(BookmarksMenuController);
    bt.builder.rebuild();
    btchevron.builder.rebuild();

    // fake a resize; this function takes care of flowing bookmarks
    // from the bar to the overflow item
    BookmarksToolbar.resizeFunc(null);
  }

  // XXX Shouldn't have to do this, but I do
  window.focus();
}

var FullScreen = 
{
  toggle: function()
  {
    // show/hide all menubars, toolbars, and statusbars (except the full screen toolbar)
    this.showXULChrome("toolbar", window.fullScreen);
    this.showXULChrome("statusbar", window.fullScreen);
  },
  
  showXULChrome: function(aTag, aShow)
  {
    var XULNS = "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul";
    var els = document.getElementsByTagNameNS(XULNS, aTag);
    
    var i;
    for (i = 0; i < els.length; ++i) {
      // XXX don't interfere with previously collapsed toolbars
      if (els[i].getAttribute("fullscreentoolbar") == "true") {
        if (!aShow) {
          var toolbarMode = els[i].getAttribute("mode");
          var iconSize = els[i].getAttribute("iconsize");
          var contextMenu = els[i].getAttribute("context");

          if (toolbarMode != "text") {
            els[i].setAttribute("saved-mode", toolbarMode);
            els[i].setAttribute("saved-iconsize", iconSize);
            els[i].setAttribute("mode", "icons");
            els[i].setAttribute("iconsize", "small");
          }

          // XXX See bug 202978: we disable the context menu
          // to prevent customization while in fullscreen, which
          // causes menu breakage.
          els[i].setAttribute("saved-context", contextMenu);
          els[i].removeAttribute("context");
        }
        else {
          if (els[i].hasAttribute("saved-mode")) {
            var savedMode = els[i].getAttribute("saved-mode");
            els[i].setAttribute("mode", savedMode);
            els[i].removeAttribute("saved-mode");
          }

          if (els[i].hasAttribute("saved-iconsize")) {
            var savedIconSize = els[i].getAttribute("saved-iconsize");
            els[i].setAttribute("iconsize", savedIconSize);
            els[i].removeAttribute("saved-iconsize");
          }

          // XXX see above.
          if (els[i].hasAttribute("saved-context")) {
            var savedContext = els[i].getAttribute("saved-context");
            els[i].setAttribute("context", savedContext);
            els[i].removeAttribute("saved-context");
          }
        }
      } else {
        // use moz-collapsed so it doesn't persist hidden/collapsed,
        // so that new windows don't have missing toolbars
        if (aShow)
          els[i].removeAttribute("moz-collapsed");
        else
          els[i].setAttribute("moz-collapsed", "true");
      }
    }
    
    var controls = document.getElementsByAttribute("fullscreencontrol", "true");
    for (i = 0; i < controls.length; ++i)
      controls[i].hidden = aShow;


    // XXXvladimir this was a fix for bug 174174, but I don't think it's necessary
    // any more?
    var toolbox = document.getElementById("navigator-toolbox");
    if (!aShow) {
      var toolboxMode = toolbox.getAttribute("mode");
      var toolboxIconSize = toolbox.getAttribute("iconsize");
      if (toolboxMode != "text") {
        toolbox.setAttribute("saved-mode", toolboxMode);
        toolbox.setAttribute("saved-iconsize", toolboxIconSize);
        toolbox.setAttribute("mode", "icons");
        toolbox.setAttribute("iconsize", "small");
      }
      else {
        if (toolbox.hasAttribute("saved-mode")) {
          var savedMode = toolbox.getAttribute("saved-mode");
          toolbox.setAttribute("mode", savedMode);
          toolbox.removeAttribute("saved-mode");
        }

        if (toolbox.hasAttribute("saved-iconsize")) {
          var savedIconSize = toolbox.getAttribute("saved-iconsize");
          toolbox.setAttribute("iconsize", savedIconSize);
          toolbox.removeAttribute("saved-iconsize");
        }
      }
    }
  }
};

function nsBrowserStatusHandler()
{
  this.init();
}

nsBrowserStatusHandler.prototype =
{
  // Stored Status, Link and Loading values
  status : "",
  defaultStatus : "",
  jsStatus : "",
  jsDefaultStatus : "",
  overLink : "",
  startTime : 0,
  statusText: "",
  lastURI: null,

  statusTimeoutInEffect : false,

  QueryInterface : function(aIID)
  {
    if (aIID.equals(Components.interfaces.nsIWebProgressListener) ||
        aIID.equals(Components.interfaces.nsISupportsWeakReference) ||
        aIID.equals(Components.interfaces.nsIXULBrowserWindow) ||
        aIID.equals(Components.interfaces.nsISupports))
      return this;
    throw Components.results.NS_NOINTERFACE;
  },

  init : function()
  {
    this.throbberElement = document.getElementById("navigator-throbber");
    this.statusMeter     = document.getElementById("statusbar-icon");
    this.stopCommand     = document.getElementById("Browser:Stop");
    this.statusTextField = document.getElementById("statusbar-display");
    this.securityButton  = document.getElementById("security-button");
    this.urlBar          = document.getElementById("urlbar");

    // Initialize the security button's state and tooltip text
    const nsIWebProgressListener = Components.interfaces.nsIWebProgressListener;
    this.onSecurityChange(null, null, nsIWebProgressListener.STATE_IS_INSECURE);
  },

  destroy : function()
  {
    // XXXjag to avoid leaks :-/, see bug 60729
    this.throbberElement = null;
    this.statusMeter     = null;
    this.stopCommand     = null;
    this.statusTextField = null;
    this.securityButton  = null;
    this.urlBar          = null;
    this.statusText      = null;
    this.lastURI         = null;
  },

  setJSStatus : function(status)
  {
    this.jsStatus = status;
    this.updateStatusField();
  },

  setJSDefaultStatus : function(status)
  {
    this.jsDefaultStatus = status;
    this.updateStatusField();
  },

  setDefaultStatus : function(status)
  {
    this.defaultStatus = status;
    this.updateStatusField();
  },

  setOverLink : function(link, b)
  {
    this.overLink = link;
    this.updateStatusField();
  },

  updateStatusField : function()
  {
    var text = this.overLink || this.status || this.jsStatus || this.jsDefaultStatus || this.defaultStatus;

    // check the current value so we don't trigger an attribute change
    // and cause needless (slow!) UI updates
    if (this.statusText != text) {
      this.statusTextField.label = text;
      this.statusText = text;
    }
  },

  onLinkIconAvailable : function(aBrowser, aHref) 
  {
    if (gProxyFavIcon &&
        gBrowser.mCurrentBrowser == aBrowser &&
        getBrowser().userTypedValue === null) {
      gProxyFavIcon.setAttribute("src", aHref);
    }

    aBrowser.mFavIconURL = aHref;
  },

  onProgressChange : function (aWebProgress, aRequest,
                               aCurSelfProgress, aMaxSelfProgress,
                               aCurTotalProgress, aMaxTotalProgress)
  {
    if (aMaxTotalProgress > 0) {
      // This is highly optimized.  Don't touch this code unless
      // you are intimately familiar with the cost of setting
      // attrs on XUL elements. -- hyatt
      var percentage = (aCurTotalProgress * 100) / aMaxTotalProgress;
      this.statusMeter.value = percentage;
    } 
  },

  onStateChange : function(aWebProgress, aRequest, aStateFlags, aStatus)
  {  
    const nsIWebProgressListener = Components.interfaces.nsIWebProgressListener;
    const nsIChannel = Components.interfaces.nsIChannel;
    if (aStateFlags & nsIWebProgressListener.STATE_START) {
        // This (thanks to the filter) is a network start or the first
        // stray request (the first request outside of the document load),
        // initialize the throbber and his friends.

        // Call start document load listeners (only if this is a network load)
        if (aStateFlags & nsIWebProgressListener.STATE_IS_NETWORK &&
            aRequest && aWebProgress.DOMWindow == content)
          this.startDocumentLoad(aRequest);
  
        if (this.throbberElement) {          
          // Turn the throbber on.
          this.throbberElement.setAttribute("busy", "true");
        }

        // Turn the status meter on.
        this.statusMeter.value = 0;  // be sure to clear the progress bar
        if (gProgressCollapseTimer) {
          window.clearTimeout(gProgressCollapseTimer);
          gProgressCollapseTimer = null;
        }
        else
          this.statusMeter.parentNode.collapsed = false;

        // XXX: This needs to be based on window activity...
        this.stopCommand.removeAttribute("disabled");
    }
    else if (aStateFlags & nsIWebProgressListener.STATE_STOP) {
      if (aStateFlags & nsIWebProgressListener.STATE_IS_NETWORK) {
        if (aRequest) {
          if (aWebProgress.DOMWindow == content)
            this.endDocumentLoad(aRequest, aStatus);
        }
      }

      // This (thanks to the filter) is a network stop or the last
      // request stop outside of loading the document, stop throbbers
      // and progress bars and such
      if (aRequest) {
        var msg = "";
        // Get the channel if the request is a channel
        var channel;
        try {
          channel = aRequest.QueryInterface(nsIChannel);
        }
        catch(e) { };
          if (channel) {
            var location = channel.URI.spec;
            if (location != "about:blank") {
              const kErrorBindingAborted = 0x804B0002;
              const kErrorNetTimeout = 0x804B000E;
              switch (aStatus) {
                case kErrorBindingAborted:
                  msg = gNavigatorBundle.getString("nv_stopped");
                  break;
                case kErrorNetTimeout:
                  msg = gNavigatorBundle.getString("nv_timeout");
                  break;
              }
            }
          }
          // If msg is false then we did not have an error (channel may have
          // been null, in the case of a stray image load).
          if (!msg) {
            msg = gNavigatorBundle.getString("nv_done");
          }
          this.status = "";
          this.setDefaultStatus(msg);
        }

        // Turn the progress meter and throbber off.
        gProgressCollapseTimer = window.setTimeout(
          function() { 
            gProgressMeterPanel.collapsed = true; 
            gProgressCollapseTimer = null;
          }, 100);

        if (this.throbberElement)
          this.throbberElement.removeAttribute("busy");

        this.stopCommand.setAttribute("disabled", "true");
    }
  },

  onLocationChange : function(aWebProgress, aRequest, aLocation)
  {
    // This code here does not compare uris exactly when determining
    // whether or not the message should be hidden since the message
    // may be prematurely hidden when an install is invoked by a click
    // on a link that looks like this:
    //
    // <a href="#" onclick="return install();">Install Foo</a>
    //
    // - which fires a onLocationChange message to uri + '#'...
    var selectedBrowser = getBrowser().selectedBrowser;
    if (selectedBrowser.lastURI) {
      var oldSpec = selectedBrowser.lastURI.spec;
      var oldIndexOfHash = oldSpec.indexOf("#");
      if (oldIndexOfHash != -1)
        oldSpec = oldSpec.substr(0, oldIndexOfHash);
      var newSpec = aLocation.spec;
      var newIndexOfHash = newSpec.indexOf("#");
      if (newIndexOfHash != -1)
        newSpec = newSpec.substr(0, newSpec.indexOf("#"));
      if (newSpec != oldSpec) {
        var tabbrowser = getBrowser();
        tabbrowser.hideMessage(tabbrowser.selectedBrowser, "both");
      }
    }
    selectedBrowser.lastURI = aLocation;

    this.setOverLink("", null);

    var location = aLocation.spec;

    if (location == "about:blank")
      location = "";
    
    // We should probably not do this if the value has changed since the user
    // searched
    // Update urlbar only if a new page was loaded on the primary content area
    // Do not update urlbar if there was a subframe navigation

    var browser = getBrowser().selectedBrowser;
    var findField = document.getElementById("find-field");
    if (aWebProgress.DOMWindow == content) {      
      // The document loaded correctly, clear the value if we should
      if (browser.userTypedClear)
        browser.userTypedValue = null;

      if (findField)
        setTimeout(function() { findField.value = browser.findString; }, 0, findField, browser); 

      // reset any favicon set for the browser
      browser.mFavIconURL = null;

      //XXXBlake don't we have to reinit this.urlBar, etc.
      //         when the toolbar changes?
      var userTypedValue = browser.userTypedValue;
      if (gURLBar && !userTypedValue) {
        // If the url has "wyciwyg://" as the protocol, strip it off.
        // Nobody wants to see it on the urlbar for dynamically generated
        // pages. 
        if (!gURIFixup)
          gURIFixup = Components.classes["@mozilla.org/docshell/urifixup;1"]
                                .getService(Components.interfaces.nsIURIFixup);
        if (location && gURIFixup)
          try {
            var locationURI = gURIFixup.createExposableURI(aLocation);
            location = locationURI.spec;
          } catch (exception) {}
        
        setTimeout(function(loc, aloc) { gURLBar.value = loc; SetPageProxyState("valid", aloc);}, 0, location, aLocation);
        
        // Setting the urlBar value in some cases causes userTypedValue to
        // become set because of oninput, so reset it to its old value.
        browser.userTypedValue = userTypedValue;
      } else {
        gURLBar.value = userTypedValue;
        SetPageProxyState("invalid", null);
      }
    }
    UpdateBackForwardButtons();

    if (findField && gFindMode != FIND_NORMAL) {
      // Close the Find toolbar if we're in old-style TAF mode
      closeFindBar();
      gBackProtectBuffer = 0;
    }
    
    // clear missing plugins
    gMissingPluginInstaller.clearMissingPlugins(getBrowser().selectedTab);
       
    setTimeout(function () { updatePageLivemarks(); }, 0);
    setTimeout(function () { updatePageStyles(); }, 0);
  },

  onStatusChange : function(aWebProgress, aRequest, aStatus, aMessage)
  {
    this.status = aMessage;
    this.updateStatusField();
  },

  onSecurityChange : function(aWebProgress, aRequest, aState)
  {
    const wpl = Components.interfaces.nsIWebProgressListener;
    this.securityButton.removeAttribute("label");

    switch (aState) {
      case wpl.STATE_IS_SECURE | wpl.STATE_SECURE_HIGH:
        this.securityButton.setAttribute("level", "high");
        this.urlBar.setAttribute("level", "high");
        try {
          this.securityButton.setAttribute("label",
            gBrowser.contentWindow.location.host);
        } catch(exception) {}
        break;
      case wpl.STATE_IS_SECURE | wpl.STATE_SECURE_LOW:
        this.securityButton.setAttribute("level", "low");
        this.urlBar.setAttribute("level", "low");
        try {
          this.securityButton.setAttribute("label", 
            gBrowser.contentWindow.location.host);
        } catch(exception) {}        
        break;
      case wpl.STATE_IS_BROKEN:
        this.securityButton.setAttribute("level", "broken");
        this.urlBar.setAttribute("level", "broken");
        break;
      case wpl.STATE_IS_INSECURE:
      default:
        this.securityButton.removeAttribute("level");
        this.urlBar.removeAttribute("level");
        break;
    }

    var securityUI = gBrowser.securityUI;
    if (securityUI) {
      this.securityButton.setAttribute("tooltiptext", securityUI.tooltipText);
      this.urlBar.setAttribute("infotext", securityUI.tooltipText);
    }
    else {
      this.securityButton.setAttribute("tooltiptext", securityUI.tooltipText);
      this.urlBar.removeAttribute("infotext");
    }
  },

  startDocumentLoad : function(aRequest)
  {
    // It's okay to clear what the user typed when we start
    // loading a document. If the user types, this flag gets
    // set to false, if the document load ends without an
    // onLocationChange, this flag also gets set to false
    // (so we keep it while switching tabs after failed load
    getBrowser().userTypedClear = true;

    // clear out livemark data
    gBrowser.mCurrentBrowser.livemarkLinks = null;
    
    const nsIChannel = Components.interfaces.nsIChannel;
    var urlStr = aRequest.QueryInterface(nsIChannel).URI.spec;
    var observerService = Components.classes["@mozilla.org/observer-service;1"]
                                    .getService(Components.interfaces.nsIObserverService);
    try {
      observerService.notifyObservers(_content, "StartDocumentLoad", urlStr);
    } catch (e) {
    }
  },

  endDocumentLoad : function(aRequest, aStatus)
  {
    // The document is done loading, it's okay to clear
    // the value again.
    getBrowser().userTypedClear = false;

    const nsIChannel = Components.interfaces.nsIChannel;
    var urlStr = aRequest.QueryInterface(nsIChannel).originalURI.spec;

    var observerService = Components.classes["@mozilla.org/observer-service;1"]
                                    .getService(Components.interfaces.nsIObserverService);

    var notification = Components.isSuccessCode(aStatus) ? "EndDocumentLoad" : "FailDocumentLoad";
    try {
      observerService.notifyObservers(_content, notification, urlStr);
    } catch (e) {
    }
    setTimeout(function() { if (document.getElementById("highlight").checked) toggleHighlight(true); }, 0);
  }
}

function onViewToolbarsPopupShowing(aEvent)
{
  var popup = aEvent.target;
  var i;

  // Empty the menu
  for (i = popup.childNodes.length-1; i >= 0; --i) {
    var deadItem = popup.childNodes[i];
    if (deadItem.hasAttribute("toolbarindex"))
      popup.removeChild(deadItem);
  }
  
  var firstMenuItem = popup.firstChild;
  
  var toolbox = document.getElementById("navigator-toolbox");
  for (i = 0; i < toolbox.childNodes.length; ++i) {
    var toolbar = toolbox.childNodes[i];
    var toolbarName = toolbar.getAttribute("toolbarname");
    var type = toolbar.getAttribute("type");
    if (toolbarName && type != "menubar") {
      var menuItem = document.createElement("menuitem");
      menuItem.setAttribute("toolbarindex", i);
      menuItem.setAttribute("type", "checkbox");
      menuItem.setAttribute("label", toolbarName);
      menuItem.setAttribute("accesskey", toolbar.getAttribute("accesskey"));
      menuItem.setAttribute("checked", toolbar.getAttribute("collapsed") != "true");
      popup.insertBefore(menuItem, firstMenuItem);        
      
      menuItem.addEventListener("command", onViewToolbarCommand, false);
    }
    toolbar = toolbar.nextSibling;
  }
}

function onViewToolbarCommand(aEvent)
{
  var toolbox = document.getElementById("navigator-toolbox");
  var index = aEvent.originalTarget.getAttribute("toolbarindex");
  var toolbar = toolbox.childNodes[index];

  toolbar.collapsed = aEvent.originalTarget.getAttribute("checked") != "true";
  document.persist(toolbar.id, "collapsed");
}

function displaySecurityInfo()
{
  window.openDialog("chrome://browser/content/pageInfo.xul", "_blank",
                    "dialog=no", null, "securityTab");
}

function nsBrowserContentListener(toplevelWindow, contentWindow)
{
    // this one is not as easy as you would hope.
    // need to convert toplevelWindow to an XPConnected object, instead
    // of a DOM-based object, to be able to QI() it to nsIXULWindow
    
    this.init(toplevelWindow, contentWindow);
}

/* implements nsIURIContentListener */

nsBrowserContentListener.prototype =
{
    init: function(toplevelWindow, contentWindow)
    {
        const nsIWebBrowserChrome = Components.interfaces.nsIWebBrowserChrome;
        this.toplevelWindow = toplevelWindow;
        this.contentWindow = contentWindow;

        // hook up the whole parent chain thing
        var windowDocShell = this.convertWindowToDocShell(toplevelWindow);
        if (windowDocShell)
            windowDocshell.parentURIContentListener = this;
    
        var registerWindow = false;
        try {          
          var treeItem = contentWindow.docShell.QueryInterface(Components.interfaces.nsIDocShellTreeItem);
          var treeOwner = treeItem.treeOwner;
          var interfaceRequestor = treeOwner.QueryInterface(Components.interfaces.nsIInterfaceRequestor);
          var webBrowserChrome = interfaceRequestor.getInterface(nsIWebBrowserChrome);
          if (webBrowserChrome)
          {
            var chromeFlags = webBrowserChrome.chromeFlags;
            var res = chromeFlags & nsIWebBrowserChrome.CHROME_ALL;
            var res2 = chromeFlags & nsIWebBrowserChrome.CHROME_DEFAULT;
            if ( res == nsIWebBrowserChrome.CHROME_ALL || res2 == nsIWebBrowserChrome.CHROME_DEFAULT)
            {             
              registerWindow = true;
            }
         }
       } catch (ex) {} 

        // register ourselves
       if (registerWindow)
       {
        var uriLoader = Components.classes["@mozilla.org/uriloader;1"].getService(Components.interfaces.nsIURILoader);
        uriLoader.registerContentListener(this);
       }
    },
    close: function()
    {
        this.contentWindow = null;
        var uriLoader = Components.classes["@mozilla.org/uriloader;1"].getService(Components.interfaces.nsIURILoader);

        uriLoader.unRegisterContentListener(this);
    },
    QueryInterface: function(iid)
    {
        if (iid.equals(Components.interfaces.nsIURIContentListener) ||
            iid.equals(Components.interfaces.nsISupportsWeakReference) ||
            iid.equals(Components.interfaces.nsISupports))
          return this;
        throw Components.results.NS_NOINTERFACE;
    },
    onStartURIOpen: function(uri)
    {
        // ignore and don't abort
        return false;
    },

    doContent: function(contentType, isContentPreferred, request, contentHandler)
    {
        // forward the doContent to our content area webshell
        var docShell = this.contentWindow.docShell;
        var contentListener;
        try {
            contentListener =
                docShell.QueryInterface(Components.interfaces.nsIInterfaceRequestor)
                .getInterface(Components.interfaces.nsIURIContentListener);
        } catch (ex) {
            dump(ex);
        }
        
        if (!contentListener) return false;
        
        return contentListener.doContent(contentType, isContentPreferred, request, contentHandler);
        
    },

    isPreferred: function(contentType, desiredContentType)
    {
        // seems like we should be getting this from helper apps or something
        switch(contentType) {
            case "text/html":
            case "text/xul":
            case "text/rdf":
            case "text/xml":
            case "text/css":
            case "image/gif":
            case "image/jpeg":
            case "image/png":
            case "text/plain":
            case "application/http-index-format":
                return true;
        }
        return false;
    },
    canHandleContent: function(contentType, isContentPreferred, desiredContentType)
    {
        var docShell = this.contentWindow.docShell;
        var contentListener;
        try {
            contentListener =
                docShell.QueryInterface(Components.interfaces.nsIInterfaceRequestor).getInterface(Components.interfaces.nsIURIContentListener);
        } catch (ex) {
            dump(ex);
        }
        if (!contentListener) return false;
        
        return contentListener.canHandleContent(contentType, isContentPreferred, desiredContentType);
    },
    convertWindowToDocShell: function(win) {
        // don't know how to do this
        return null;
    },
    loadCookie: null,
    parentContentListener: null
}

// |forceOpen| is a bool that indicates that the sidebar should be forced open.  In other words
// the toggle won't be allowed to close the sidebar.
function toggleSidebar(aCommandID, forceOpen) {

  var sidebarBox = document.getElementById("sidebar-box");
  if (!aCommandID)
    aCommandID = sidebarBox.getAttribute("sidebarcommand");

  var elt = document.getElementById(aCommandID);
  var sidebar = document.getElementById("sidebar");
  var sidebarTitle = document.getElementById("sidebar-title");
  var sidebarSplitter = document.getElementById("sidebar-splitter");

  if (!forceOpen && elt.getAttribute("checked") == "true") {
    elt.removeAttribute("checked");
    sidebarBox.setAttribute("sidebarcommand", "");
    sidebarTitle.setAttribute("value", "");
    sidebarBox.hidden = true;
    sidebarSplitter.hidden = true;
    _content.focus();
    return;
  }
  
  var elts = document.getElementsByAttribute("group", "sidebar");
  for (var i = 0; i < elts.length; ++i)
    elts[i].removeAttribute("checked");

  elt.setAttribute("checked", "true");;

  if (sidebarBox.hidden) {
    sidebarBox.hidden = false;
    sidebarSplitter.hidden = false;
  }
  
  var url = elt.getAttribute("sidebarurl");
  var title = elt.getAttribute("sidebartitle");
  if (!title)
    title = elt.getAttribute("label");
  sidebar.setAttribute("src", url);
  sidebarBox.setAttribute("src", url);
  sidebarBox.setAttribute("sidebarcommand", elt.id);
  sidebarTitle.setAttribute("value", title);
  if (aCommandID != "viewWebPanelsSidebar") { // no searchbox there
    // if the sidebar we want is already constructed, focus the searchbox
    if ((aCommandID == "viewBookmarksSidebar" && sidebar.contentDocument.getElementById("bookmarksPanel"))
    || (aCommandID == "viewHistorySidebar" && sidebar.contentDocument.getElementById("history-panel")))
      sidebar.contentDocument.getElementById("search-box").focus();
    // otherwiese, attach an onload handler
    else
      sidebar.addEventListener("load", asyncFocusSearchBox, true);
  }
}

function asyncFocusSearchBox(event)
{
  var sidebar = document.getElementById("sidebar");
  var searchBox = sidebar.contentDocument.getElementById("search-box");
  searchBox.focus();
  sidebar.removeEventListener("load", asyncFocusSearchBox, true);
 }

function openPreferences()
{
  openDialog("chrome://browser/content/pref/pref.xul","PrefWindow", 
             "chrome,titlebar,resizable,modal");
}

var gTypeAheadFind = {
  prefDomain: "accessibility.typeaheadfind",
  observe: function(aSubject, aTopic, aPrefName)
  {
    if (aTopic != "nsPref:changed" || aPrefName != this.prefDomain)
      return;
      
    gUseTypeAheadFind = gPrefService.getBoolPref("accessibility.typeaheadfind");
  }
};

var gHomeButton = {
  prefDomain: "browser.startup.homepage",
  observe: function (aSubject, aTopic, aPrefName)
  {
    if (aTopic != "nsPref:changed" || aPrefName != this.prefDomain)
      return;
      
    this.updateTooltip();
  },
  
  updateTooltip: function ()
  {
    var homeButton = document.getElementById("home-button");
    if (homeButton) {
      var homePage = this.getHomePage();
      homePage = homePage.replace(/\|/g,', ');
      homeButton.setAttribute("tooltiptext", homePage);
    }
  },
    
  getHomePage: function ()
  {
    var url;
    try {
      url = gPrefService.getComplexValue(this.prefDomain,
                                Components.interfaces.nsIPrefLocalizedString).data;
    } catch (e) {
    }

    // use this if we can't find the pref
    if (!url) {
      var navigatorRegionBundle = document.getElementById("bundle_browser_region");
      url = navigatorRegionBundle.getString("homePageDefault");
    }

    return url;
  }
};

function nsContextMenu( xulMenu ) {
    this.target         = null;
    this.menu           = null;
    this.onTextInput    = false;
    this.onKeywordField = false;
    this.onImage        = false;
    this.onLink         = false;
    this.onMailtoLink   = false;
    this.onSaveableLink = false;
    this.onMetaDataItem = false;
    this.onMathML       = false;
    this.link           = false;
    this.inFrame        = false;
    this.hasBGImage     = false;
    this.isTextSelected = false;
    this.inDirList      = false;
    this.shouldDisplay  = true;

    // Initialize new menu.
    this.initMenu( xulMenu );
}

// Prototype for nsContextMenu "class."
nsContextMenu.prototype = {
    // onDestroy is a no-op at this point.
    onDestroy : function () {
    },
    // Initialize context menu.
    initMenu : function ( popup ) {
        // Save menu.
        this.menu = popup;

        // Get contextual info.
        this.setTarget( document.popupNode );
        
        this.isTextSelected = this.isTextSelection();

        // Initialize (disable/remove) menu items.
        this.initItems();
    },
    initItems : function () {
        this.initOpenItems();
        this.initNavigationItems();
        this.initViewItems();
        this.initMiscItems();
        this.initSaveItems();
        this.initClipboardItems();
        this.initMetadataItems();
    },
    initOpenItems : function () {
        this.showItem( "context-openlink", this.onSaveableLink || ( this.inDirList && this.onLink ) );
        this.showItem( "context-openlinkintab", this.onSaveableLink || ( this.inDirList && this.onLink ) );

        this.showItem( "context-sep-open", this.onSaveableLink || ( this.inDirList && this.onLink ) );
    },
    initNavigationItems : function () {
        // Back determined by canGoBack broadcaster.
        this.setItemAttrFromNode( "context-back", "disabled", "canGoBack" );

        // Forward determined by canGoForward broadcaster.
        this.setItemAttrFromNode( "context-forward", "disabled", "canGoForward" );
        
        this.showItem( "context-back", !( this.isTextSelected || this.onLink || this.onImage || this.onTextInput ) );
        this.showItem( "context-forward", !( this.isTextSelected || this.onLink || this.onImage || this.onTextInput ) );

        this.showItem( "context-reload", !( this.isTextSelected || this.onLink || this.onImage || this.onTextInput ) );
        
        this.showItem( "context-stop", !( this.isTextSelected || this.onLink || this.onImage || this.onTextInput ) );
        this.showItem( "context-sep-stop", !( this.isTextSelected || this.onLink || this.onTextInput || this.onImage ) );

        // XXX: Stop is determined in navigator.js; the canStop broadcaster is broken
        //this.setItemAttrFromNode( "context-stop", "disabled", "canStop" );
    },
    initSaveItems : function () {
        this.showItem( "context-savepage", !( this.inDirList || this.isTextSelected || this.onTextInput || this.onLink || this.onImage ));
        this.showItem( "context-sendpage", !( this.inDirList || this.isTextSelected || this.onTextInput || this.onLink || this.onImage ));

        // Save link depends on whether we're in a link.
        this.showItem( "context-savelink", this.onSaveableLink );

        // Save image depends on whether there is one.
        this.showItem( "context-saveimage", this.onImage );
        
        this.showItem( "context-sendimage", this.onImage );
        
        // Send link depends on whether we're in a link.
        this.showItem( "context-sendlink", this.onSaveableLink );   
    },
    initViewItems : function () {
        // View source is always OK, unless in directory listing.
        this.showItem( "context-viewpartialsource-selection", this.isTextSelected );
        this.showItem( "context-viewpartialsource-mathml", this.onMathML && !this.isTextSelected );
        this.showItem( "context-viewsource", !( this.inDirList || this.onImage || this.isTextSelected || this.onLink || this.onTextInput ) );
        this.showItem( "context-viewinfo", !( this.inDirList || this.onImage || this.isTextSelected || this.onLink || this.onTextInput ) );

        this.showItem( "context-sep-properties", !( this.inDirList || this.isTextSelected || this.onTextInput ) );
        // Set As Wallpaper depends on whether an image was clicked on, and only works if we have a shell service.
        var haveSetWallpaper = false;
#ifdef HAVE_SHELL_SERVICE
        // Only enable set as wallpaper if we can get the shell service.
        var shell = getShellService();
        if (shell)
          haveSetWallpaper = true;
#endif
        this.showItem( "context-setWallpaper", haveSetWallpaper && this.onImage );

        if( haveSetWallpaper && this.onImage )
            // Disable the Set As Wallpaper menu item if we're still trying to load the image
          this.setItemAttr( "context-setWallpaper", "disabled", (("complete" in this.target) && !this.target.complete) ? "true" : null );

        // View Image depends on whether an image was clicked on.
        this.showItem( "context-viewimage", this.onImage );

        // View background image depends on whether there is one.
        this.showItem( "context-viewbgimage", !( this.inDirList || this.onImage || this.isTextSelected || this.onLink || this.onTextInput ) );
        this.showItem( "context-sep-viewbgimage", !( this.inDirList || this.onImage || this.isTextSelected || this.onLink || this.onTextInput ) );
        this.setItemAttr( "context-viewbgimage", "disabled", this.hasBGImage ? null : "true");
    },
    initMiscItems : function () {
        // Use "Bookmark This Link" if on a link.
        this.showItem( "context-bookmarkpage", !( this.isTextSelected || this.onTextInput || this.onLink || this.onImage ) );
        this.showItem( "context-bookmarklink", this.onLink && !this.onMailtoLink );
        this.showItem( "context-searchselect", this.isTextSelected );
        this.showItem( "context-keywordfield", this.onTextInput && this.onKeywordField );
        this.showItem( "frame", this.inFrame );
        this.showItem( "frame-sep", this.inFrame );
        this.showItem( "context-blockimage", this.onImage);

        // BiDi UI
        var showBIDI = isBidiEnabled();
        this.showItem( "context-sep-bidi", showBIDI);
        this.showItem( "context-bidi-text-direction-toggle", this.onTextInput && showBIDI);
        this.showItem( "context-bidi-page-direction-toggle", !this.onTextInput && showBIDI);

        if (this.onImage) {
          var blockImage = document.getElementById("context-blockimage");

          var uri = Components.classes['@mozilla.org/network/standard-url;1'].createInstance(Components.interfaces.nsIURI);
          uri.spec = this.imageURL;

          var shortenedUriHost = uri.host.replace(/^www\./i,"");
          if (shortenedUriHost.length > 15)
            shortenedUriHost = shortenedUriHost.substr(0,15) + "...";
          blockImage.label = gNavigatorBundle.getFormattedString ("blockImages", [shortenedUriHost]);

          if (this.isImageBlocked()) {
            blockImage.setAttribute("checked", "true");
          }
          else
            blockImage.removeAttribute("checked");
        }
    },
    initClipboardItems : function () {

        // Copy depends on whether there is selected text.
        // Enabling this context menu item is now done through the global
        // command updating system
        // this.setItemAttr( "context-copy", "disabled", !this.isTextSelected() );

        goUpdateGlobalEditMenuItems();

        this.showItem( "context-undo", this.onTextInput );
        this.showItem( "context-sep-undo", this.onTextInput );
        this.showItem( "context-cut", this.onTextInput );
        this.showItem( "context-copy", this.isTextSelected || this.onTextInput );
        this.showItem( "context-paste", this.onTextInput );
        this.showItem( "context-delete", this.onTextInput );
        this.showItem( "context-sep-paste", this.onTextInput );
        this.showItem( "context-selectall", !( this.onLink || this.onImage ) );
        this.showItem( "context-sep-selectall", this.isTextSelected );

        // XXX dr
        // ------
        // nsDocumentViewer.cpp has code to determine whether we're
        // on a link or an image. we really ought to be using that...

        // Copy email link depends on whether we're on an email link.
        this.showItem( "context-copyemail", this.onMailtoLink );

        // Copy link location depends on whether we're on a link.
        this.showItem( "context-copylink", this.onLink );
        this.showItem( "context-sep-copylink", this.onLink && this.onImage);

#ifndef XP_UNIX
#ifndef XP_OS2
        // Copy image contents depends on whether we're on an image.
        this.showItem( "context-copyimage-contents", this.onImage );
#endif
#endif
        // Copy image location depends on whether we're on an image.
        this.showItem( "context-copyimage", this.onImage );
        this.showItem( "context-sep-copyimage", this.onImage );
    },
    initMetadataItems : function () {
        // Show if user clicked on something which has metadata.
        this.showItem( "context-metadata", this.onMetaDataItem );
    },
    // Set various context menu attributes based on the state of the world.
    setTarget : function ( node ) {
        const xulNS = "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul";
        if ( node.namespaceURI == xulNS ) {
          this.shouldDisplay = false;
          return;
        }
        // Initialize contextual info.
        this.onImage    = false;
        this.onMetaDataItem = false;
        this.onTextInput = false;
        this.onKeywordField = false;
        this.imageURL   = "";
        this.onLink     = false;
        this.onMathML   = false;
        this.inFrame    = false;
        this.hasBGImage = false;
        this.bgImageURL = "";

        // Remember the node that was clicked.
        this.target = node;

        // See if the user clicked on an image.
        if ( this.target.nodeType == Node.ELEMENT_NODE ) {
             if ( this.target.localName.toUpperCase() == "IMG" ) {
                this.onImage = true;
                this.imageURL = this.target.src;
                // Look for image map.
                var mapName = this.target.getAttribute( "usemap" );
                if ( mapName ) {
                    // Find map.
                    var map = this.target.ownerDocument.getElementById( mapName.substr(1) );
                    if ( map ) {
                        // Search child <area>s for a match.
                        var areas = map.childNodes;
                        //XXX Client side image maps are too hard for now!
                        areas.length = 0;
                        for ( var i = 0; i < areas.length && !this.onLink; i++ ) {
                            var area = areas[i];
                            if ( area.nodeType == Node.ELEMENT_NODE
                                 &&
                                 area.localName.toUpperCase() == "AREA" ) {
                                // Get type (rect/circle/polygon/default).
                                var type = area.getAttribute( "type" );
                                var coords = this.parseCoords( area );
                                switch ( type.toUpperCase() ) {
                                    case "RECT":
                                    case "RECTANGLE":
                                        break;
                                    case "CIRC":
                                    case "CIRCLE":
                                        break;
                                    case "POLY":
                                    case "POLYGON":
                                        break;
                                    case "DEFAULT":
                                        // Default matches entire image.
                                        this.onLink = true;
                                        this.link = area;
                                        this.onSaveableLink = this.isLinkSaveable( this.link );
                                        break;
                                }
                            }
                        }
                    }
                }
             } else if ( this.target.localName.toUpperCase() == "OBJECT"
                         &&
                         // See if object tag is for an image.
                         this.objectIsImage( this.target ) ) {
                // This is an image.
                this.onImage = true;
                // URL must be constructed.
                this.imageURL = this.objectImageURL( this.target );
             } else if ( this.target.localName.toUpperCase() == "INPUT") {
               type = this.target.getAttribute("type");
               if(type && type.toUpperCase() == "IMAGE") {
                 this.onImage = true;
                 // Convert src attribute to absolute URL.
                 this.imageURL = makeURLAbsolute( this.target.baseURI,
                                                  this.target.src );
               } else /* if (this.target.getAttribute( "type" ).toUpperCase() == "TEXT") */ {
                 this.onTextInput = this.isTargetATextBox(this.target);
                 this.onKeywordField = this.isTargetAKeywordField(this.target);
               }
            } else if ( this.target.localName.toUpperCase() == "TEXTAREA" ) {
                 this.onTextInput = true;
            } else if ( this.target.localName.toUpperCase() == "HTML" ) {
               // pages with multiple <body>s are lame. we'll teach them a lesson.
               var bodyElt = this.target.ownerDocument.getElementsByTagName("body")[0];
               if ( bodyElt ) {
                 var computedURL = this.getComputedURL( bodyElt, "background-image" );
                 if ( computedURL ) {
                   this.hasBGImage = true;
                   this.bgImageURL = makeURLAbsolute( bodyElt.baseURI,
                                                      computedURL );
                 }
               }
            } else if ( "HTTPIndex" in _content &&
                        _content.HTTPIndex instanceof Components.interfaces.nsIHTTPIndex ) {
                this.inDirList = true;
                // Bubble outward till we get to an element with URL attribute
                // (which should be the href).
                var root = this.target;
                while ( root && !this.link ) {
                    if ( root.tagName == "tree" ) {
                        // Hit root of tree; must have clicked in empty space;
                        // thus, no link.
                        break;
                    }
                    if ( root.getAttribute( "URL" ) ) {
                        // Build pseudo link object so link-related functions work.
                        this.onLink = true;
                        this.link = { href : root.getAttribute("URL"),
                                      getAttribute: function (attr) {
                                          if (attr == "title") {
                                              return root.firstChild.firstChild.getAttribute("label");
                                          } else {
                                              return "";
                                          }
                                      }
                                    };
                        // If element is a directory, then you can't save it.
                        if ( root.getAttribute( "container" ) == "true" ) {
                            this.onSaveableLink = false;
                        } else {
                            this.onSaveableLink = true;
                        }
                    } else {
                        root = root.parentNode;
                    }
                }
            }
        }

        // We have meta data on images.
        this.onMetaDataItem = this.onImage;
        
        // See if the user clicked on MathML
        const NS_MathML = "http://www.w3.org/1998/Math/MathML";
        if ((this.target.nodeType == Node.TEXT_NODE &&
             this.target.parentNode.namespaceURI == NS_MathML)
             || (this.target.namespaceURI == NS_MathML))
          this.onMathML = true;

        // See if the user clicked in a frame.
        if ( this.target.ownerDocument != window._content.document ) {
            this.inFrame = true;
        }
        
        // Bubble out, looking for items of interest
        var elem = this.target;
        while ( elem ) {
            if ( elem.nodeType == Node.ELEMENT_NODE ) {
                var localname = elem.localName.toUpperCase();
                
                // Link?
                if ( !this.onLink && 
                    ( (localname === "A" && elem.href) ||
                      localname === "AREA" ||
                      localname === "LINK" ||
                      elem.getAttributeNS( "http://www.w3.org/1999/xlink", "type") == "simple" ) ) {
                    // Clicked on a link.
                    this.onLink = true;
                    this.onMetaDataItem = true;
                    // Remember corresponding element.
                    this.link = elem;
                    this.onMailtoLink = this.isLinkType( "mailto:", this.link );
                    // Remember if it is saveable.
                    this.onSaveableLink = this.isLinkSaveable( this.link );
                }
                
                // Text input?
                if ( !this.onTextInput ) {
                    // Clicked on a link.
                    this.onTextInput = this.isTargetATextBox(elem);
                }
                
                // Metadata item?
                if ( !this.onMetaDataItem ) {
                    // We currently display metadata on anything which fits
                    // the below test.
                    if ( ( localname === "BLOCKQUOTE" && 'cite' in elem && elem.cite)  ||
                         ( localname === "Q" && 'cite' in elem && elem.cite)           ||
                         ( localname === "TABLE" && 'summary' in elem && elem.summary) ||
                         ( ( localname === "INS" || localname === "DEL" ) &&
                           ( ( 'cite' in elem && elem.cite ) ||
                             ( 'dateTime' in elem && elem.dateTime ) ) )               ||
                         ( 'title' in elem && elem.title )                             ||
                         ( 'lang' in elem && elem.lang ) ) {
                        this.onMetaDataItem = true;
                    }
                }

                // Background image?  Don't bother if we've already found a 
                // background image further down the hierarchy.  Otherwise,
                // we look for the computed background-image style.
                if ( !this.hasBGImage ) {
                    var bgImgUrl = this.getComputedURL( elem, "background-image" );
                    if ( bgImgUrl ) {
                        this.hasBGImage = true;
                        this.bgImageURL = makeURLAbsolute( elem.baseURI,
                                                           bgImgUrl );
                    }
                }
            }
            elem = elem.parentNode;    
        }
    },
    // Returns the computed style attribute for the given element.
    getComputedStyle: function( elem, prop ) {
         return elem.ownerDocument.defaultView.getComputedStyle( elem, '' ).getPropertyValue( prop );
    },
    // Returns a "url"-type computed style attribute value, with the url() stripped.
    getComputedURL: function( elem, prop ) {
         var url = elem.ownerDocument.defaultView.getComputedStyle( elem, '' ).getPropertyCSSValue( prop );
         return ( url.primitiveType == CSSPrimitiveValue.CSS_URI ) ? url.getStringValue() : null;
    },
    // Returns true iff clicked on link is saveable.
    isLinkSaveable : function ( link ) {
        // We don't do the Right Thing for news/snews yet, so turn them off
        // until we do.
        return !(this.isLinkType( "mailto:" , link )     ||
                 this.isLinkType( "javascript:" , link ) ||
                 this.isLinkType( "news:", link )        || 
                 this.isLinkType( "snews:", link ) ); 
    },
    // Returns true iff clicked on link is of type given.
    isLinkType : function ( linktype, link ) {        
        try {
            // Test for missing protocol property.
            if ( !link.protocol ) {
                // We must resort to testing the URL string :-(.
                var protocol;
                if ( link.href ) {
                    protocol = link.href.substr( 0, linktype.length );
                } else {
                    protocol = link.getAttributeNS("http://www.w3.org/1999/xlink","href");
                    if ( protocol ) {
                        protocol = protocol.substr( 0, linktype.length );
                    }
                }
                return protocol.toLowerCase() === linktype;        
            } else {
                // Presume all but javascript: urls are saveable.
                return link.protocol.toLowerCase() === linktype;
            }
        } catch (e) {
            // something was wrong with the link,
            // so we won't be able to save it anyway
            return false;
        }
    },
    // Open linked-to URL in a new window.
    openLink : function () {
        // Determine linked-to URL.
        openNewWindowWith(this.linkURL(), this.link, true);
    },
    // Open linked-to URL in a new tab.
    openLinkInTab : function () {
        // Determine linked-to URL.
        openNewTabWith(this.linkURL(), this.link, null, true);
    },
    // Open frame in a new tab.
    openFrameInTab : function () {
        // Determine linked-to URL.
        openNewTabWith(this.target.ownerDocument.location.href, null, null, true);
    },
    // Reload clicked-in frame.
    reloadFrame : function () {
        this.target.ownerDocument.location.reload();
    },
    // Open clicked-in frame in its own window.
    openFrame : function () {
        openNewWindowWith(this.target.ownerDocument.location.href, null, true);
    },
    // Open clicked-in frame in the same window
    showOnlyThisFrame : function () {
        window.loadURI(this.target.ownerDocument.location.href, null, null);
    },
    // View Partial Source
    viewPartialSource : function ( context ) {
        var focusedWindow = document.commandDispatcher.focusedWindow;
        if (focusedWindow == window)
          focusedWindow = _content;
        var docCharset = null;
        if (focusedWindow)
          docCharset = "charset=" + focusedWindow.document.characterSet;

        // "View Selection Source" and others such as "View MathML Source"
        // are mutually exclusive, with the precedence given to the selection
        // when there is one
        var reference = null;
        if (context == "selection")
          reference = focusedWindow.__proto__.getSelection.call(focusedWindow);
        else if (context == "mathml")
          reference = this.target;
        else
          throw "not reached";

        var docUrl = null; // unused (and play nice for fragments generated via XSLT too)
        window.openDialog("chrome://global/content/viewPartialSource.xul",
                          "_blank", "scrollbars,resizable,chrome,dialog=no",
                          docUrl, docCharset, reference, context);
    },
    // Open new "view source" window with the frame's URL.
    viewFrameSource : function () {
        BrowserViewSourceOfDocument(this.target.ownerDocument);
    },
    viewInfo : function () {
      BrowserPageInfo();
    },
    viewFrameInfo : function () {
      BrowserPageInfo(this.target.ownerDocument);
    },
    // Change current window to the URL of the image.
    viewImage : function (e) {
        openUILink( this.imageURL, e );
    },
    // Change current window to the URL of the background image.
    viewBGImage : function (e) {
        openUILink( this.bgImageURL, e );
    },
    setWallpaper: function() {
      // Confirm since it's annoying if you hit this accidentally.
      openDialog("chrome://browser/content/setWallpaper.xul", "",
                 "centerscreen,chrome,dialog,modal,dependent",
                 this.target);
    },    
    // Save URL of clicked-on frame.
    saveFrame : function () {
        saveDocument( this.target.ownerDocument );
    },
    // Save URL of clicked-on link.
    saveLink : function () {
        saveURL( this.linkURL(), this.linkText(), null, true, false );
    },
    sendLink : function () {
        MailIntegration.sendMessage( this.linkURL(), "" ); // we don't know the title of the link so pass in an empty string
    },
    // Save URL of clicked-on image.
    saveImage : function () {
        saveURL( this.imageURL, null, "SaveImageTitle", false );
    },
    sendImage : function () {
        MailIntegration.sendMessage(this.imageURL, "");
    },
    toggleImageBlocking : function (aBlock) {
      var nsIPermissionManager = Components.interfaces.nsIPermissionManager;
      var permissionmanager =
        Components.classes["@mozilla.org/permissionmanager;1"]
          .getService(nsIPermissionManager);
      var uri = Components.classes["@mozilla.org/network/standard-url;1"]
                          .createInstance(Components.interfaces.nsIURI);
      uri.spec = this.imageURL;
      permissionmanager.add(uri, "image",
                            aBlock ? nsIPermissionManager.DENY_ACTION : nsIPermissionManager.ALLOW_ACTION);
    },
    isImageBlocked : function() {
      var nsIPermissionManager = Components.interfaces.nsIPermissionManager;
      var permissionmanager =
        Components.classes["@mozilla.org/permissionmanager;1"]
          .getService(Components.interfaces.nsIPermissionManager);
      var uri = Components.classes["@mozilla.org/network/standard-url;1"]
                          .createInstance(Components.interfaces.nsIURI);
      uri.spec = this.imageURL;
      return permissionmanager.testPermission(uri, "image") == nsIPermissionManager.DENY_ACTION;
    },
    // Generate email address and put it on clipboard.
    copyEmail : function () {
        // Copy the comma-separated list of email addresses only.
        // There are other ways of embedding email addresses in a mailto:
        // link, but such complex parsing is beyond us.
        var url = this.linkURL();
        var qmark = url.indexOf( "?" );
        var addresses;
        
        if ( qmark > 7 ) {                   // 7 == length of "mailto:"
            addresses = url.substring( 7, qmark );
        } else {
            addresses = url.substr( 7 );
        }

        // Let's try to unescape it using a character set
        // in case the address is not ASCII.
        try {
          var characterSet = Components.lookupMethod(this.target.ownerDocument, "characterSet")
                                       .call(this.target.ownerDocument);
          const textToSubURI = Components.classes["@mozilla.org/intl/texttosuburi;1"]
                                         .getService(Components.interfaces.nsITextToSubURI);
          addresses = textToSubURI.unEscapeNonAsciiURI(characterSet, addresses);
        }
        catch(ex) {
          // Do nothing.
        }

        var clipboard = this.getService( "@mozilla.org/widget/clipboardhelper;1",
                                         Components.interfaces.nsIClipboardHelper );
        clipboard.copyString(addresses);
    },    
    addBookmark : function() {
      var docshell = document.getElementById( "content" ).webNavigation;
      BookmarksUtils.addBookmark( docshell.currentURI.spec,
                                  docshell.document.title,
                                  docshell.document.charset);
    },
    addBookmarkForFrame : function() {
      var doc = this.target.ownerDocument;
      var uri = doc.location.href;
      var title = doc.title;
      if ( !title )
        title = uri;
      BookmarksUtils.addBookmark(uri, title, doc.charset);
    },
    // Open Metadata window for node
    showMetadata : function () {
        window.openDialog(  "chrome://browser/content/metaData.xul",
                            "_blank",
                            "scrollbars,resizable,chrome,dialog=no",
                            this.target);
    },

    ///////////////
    // Utilities //
    ///////////////

    // Create instance of component given contractId and iid (as string).
    createInstance : function ( contractId, iidName ) {
        var iid = Components.interfaces[ iidName ];
        return Components.classes[ contractId ].createInstance( iid );
    },
    // Get service given contractId and iid (as string).
    getService : function ( contractId, iidName ) {
        var iid = Components.interfaces[ iidName ];
        return Components.classes[ contractId ].getService( iid );
    },
    // Show/hide one item (specified via name or the item element itself).
    showItem : function ( itemOrId, show ) {
        var item = itemOrId.constructor == String ? document.getElementById(itemOrId) : itemOrId;
        if (item) 
          item.hidden = !show;
    },
    // Set given attribute of specified context-menu item.  If the
    // value is null, then it removes the attribute (which works
    // nicely for the disabled attribute).
    setItemAttr : function ( id, attr, val ) {
        var elem = document.getElementById( id );
        if ( elem ) {
            if ( val == null ) {
                // null indicates attr should be removed.
                elem.removeAttribute( attr );
            } else {
                // Set attr=val.
                elem.setAttribute( attr, val );
            }
        }
    },
    // Set context menu attribute according to like attribute of another node
    // (such as a broadcaster).
    setItemAttrFromNode : function ( item_id, attr, other_id ) {
        var elem = document.getElementById( other_id );
        if ( elem && elem.getAttribute( attr ) == "true" ) {
            this.setItemAttr( item_id, attr, "true" );
        } else {
            this.setItemAttr( item_id, attr, null );
        }
    },
    // Temporary workaround for DOM api not yet implemented by XUL nodes.
    cloneNode : function ( item ) {
        // Create another element like the one we're cloning.
        var node = document.createElement( item.tagName );

        // Copy attributes from argument item to the new one.
        var attrs = item.attributes;
        for ( var i = 0; i < attrs.length; i++ ) {
            var attr = attrs.item( i );
            node.setAttribute( attr.nodeName, attr.nodeValue );
        }

        // Voila!
        return node;
    },
    // Generate fully-qualified URL for clicked-on link.
    linkURL : function () {
        if (this.link.href) {
          return this.link.href;
        }
        var href = this.link.getAttributeNS("http://www.w3.org/1999/xlink","href");
        if (!href || !href.match(/\S/)) {
          throw "Empty href"; // Without this we try to save as the current doc, for example, HTML case also throws if empty
        }
        href = makeURLAbsolute(this.link.baseURI,href);
        return href;
    },
    // Get text of link.
    linkText : function () {
        var text = gatherTextUnder( this.link );
        if (!text || !text.match(/\S/)) {
          text = this.link.getAttribute("title");
          if (!text || !text.match(/\S/)) {
            text = this.link.getAttribute("alt");
            if (!text || !text.match(/\S/)) {
              if (this.link.href) {                
                text = this.link.href;
              } else {
                text = getAttributeNS("http://www.w3.org/1999/xlink", "href");
                if (text && text.match(/\S/)) {
                  text = makeURLAbsolute(this.link.baseURI, text);
                }
              }
            }
          }
        }

        return text;
    },

    //Get selected object and convert it to a string to get
    //selected text.   Only use the first 15 chars.
    isTextSelection : function() {
        var result = false;
        var selection = this.searchSelected(16);

        var searchSelectText;
        if (selection) {
            searchSelectText = selection.toString();
            if (searchSelectText.length > 15)
                searchSelectText = searchSelectText.substr(0,15) + "...";
            result = true;

          // format "Search for <selection>" string to show in menu
          searchSelectText = gNavigatorBundle.getFormattedString("searchText", [searchSelectText]);
          this.setItemAttr("context-searchselect", "label", searchSelectText);
        } 
        return result;
    },
    
    searchSelected : function( charlen ) {
        var focusedWindow = document.commandDispatcher.focusedWindow;
        var searchStr = focusedWindow.__proto__.getSelection.call(focusedWindow);
        searchStr = searchStr.toString();
        // searching for more than 150 chars makes no sense
        if (!charlen)
            charlen = 150;
        if (charlen < searchStr.length) {
            // only use the first charlen important chars. see bug 221361
            var pattern = new RegExp("^(?:\\s*.){0," + charlen + "}");
            pattern.test(searchStr);
            searchStr = RegExp.lastMatch;
        }
        searchStr = searchStr.replace(/\s*(.*?)\s*$/, "$1");
        searchStr = searchStr.replace(/\s+/g, " ");
        return searchStr;
    },
    
    // Determine if target <object> is an image.
    objectIsImage : function ( objElem ) {
        var result = false;
        // Get type and data attributes.
        var type = objElem.getAttribute( "type" );
        var data = objElem.getAttribute( "data" );
        // Presume any mime type of the form "image/..." is an image.
        // There must be a data= attribute with an URL, also.
        if ( type.substring( 0, 6 ) == "image/" && data) {
            result = true;
        }
        return result;
    },
    // Extract image URL from <object> tag.
    objectImageURL : function ( objElem ) {
        // Extract url from data= attribute.
        var data = objElem.getAttribute( "data" );
        // Make it absolute.
        return makeURLAbsolute( objElem.baseURI, data );
    },
    // Parse coords= attribute and return array.
    parseCoords : function ( area ) {
        return [];
    },
    toString : function () {
        return "contextMenu.target     = " + this.target + "\n" +
               "contextMenu.onImage    = " + this.onImage + "\n" +
               "contextMenu.onLink     = " + this.onLink + "\n" +
               "contextMenu.link       = " + this.link + "\n" +
               "contextMenu.inFrame    = " + this.inFrame + "\n" +
               "contextMenu.hasBGImage = " + this.hasBGImage + "\n";
    },
    isTargetATextBox : function ( node )
    {
      if (node.nodeType != Node.ELEMENT_NODE)
        return false;

      if (node.localName.toUpperCase() == "INPUT") {
        var attrib = "";
        var type = node.getAttribute("type");

        if (type)
          attrib = type.toUpperCase();

        return( (attrib != "IMAGE") &&
                (attrib != "CHECKBOX") &&
                (attrib != "RADIO") &&
                (attrib != "SUBMIT") &&
                (attrib != "RESET") &&
                (attrib != "FILE") &&
                (attrib != "HIDDEN") &&
                (attrib != "RESET") &&
                (attrib != "BUTTON") );
      } else  {
        return(node.localName.toUpperCase() == "TEXTAREA");
      }
    },
    isTargetAKeywordField : function ( node )
    {
      var form = node.form;
      if (!form)
        return false;
      var method = form.method.toUpperCase();
      
      // These are the following types of forms we can create keywords for:
      //
      // method   encoding type       can create keyword
      // GET      *                                 YES
      //          *                                 YES
      // POST                                       YES
      // POST     application/x-www-form-urlencoded YES
      // POST     text/plain                        NO (a little tricky to do)
      // POST     multipart/form-data               NO
      // POST     everything else                   YES
      return (method == "GET" || method == "") || 
             (form.enctype != "text/plain") && (form.enctype != "multipart/form-data");
    },
    
    // Determines whether or not the separator with the specified ID should be 
    // shown or not by determining if there are any non-hidden items between it
    // and the previous separator. 
    shouldShowSeparator : function ( aSeparatorID )
    {
      var separator = document.getElementById(aSeparatorID);
      if (separator) {
        var sibling = separator.previousSibling;
        while (sibling && sibling.localName != "menuseparator") {
          if (sibling.getAttribute("hidden") != "true")
            return true;
          sibling = sibling.previousSibling;
        }
      }
      return false;  
    }
}

var gWebPanelURI;
function openWebPanel(aTitle, aURI)
{
    // Ensure that the web panels sidebar is open.
    toggleSidebar('viewWebPanelsSidebar', true);
    
    // Set the title of the panel.
    document.getElementById("sidebar-title").value = aTitle;
    
    // Tell the Web Panels sidebar to load the bookmark.
    var sidebar = document.getElementById("sidebar");
    if (sidebar.contentDocument && sidebar.contentDocument.getElementById('web-panels-browser')) {
        sidebar.contentWindow.loadWebPanel(aURI);
        if (gWebPanelURI) {
            gWebPanelURI = "";
            sidebar.removeEventListener("load", asyncOpenWebPanel, true);
        }
    }
    else {
        // The panel is still being constructed.  Attach an onload handler.
        if (!gWebPanelURI)
            sidebar.addEventListener("load", asyncOpenWebPanel, true);
        gWebPanelURI = aURI;
    }
}

function asyncOpenWebPanel(event)
{
    var sidebar = document.getElementById("sidebar");
    if (gWebPanelURI && sidebar.contentDocument && sidebar.contentDocument.getElementById('web-panels-browser'))
        sidebar.contentWindow.loadWebPanel(gWebPanelURI);
    gWebPanelURI = "";
    sidebar.removeEventListener("load", asyncOpenWebPanel, true);
}

/*
 * - [ Dependencies ] ---------------------------------------------------------
 *  utilityOverlay.js:
 *    - gatherTextUnder
 */

 // Called whenever the user clicks in the content area,
 // except when left-clicking on links (special case)
 // should always return true for click to go through
 function contentAreaClick(event, fieldNormalClicks) 
 {
   var target = event.target;
   var linkNode;

   var local_name = target.localName;

   if (local_name) {
     local_name = local_name.toLowerCase();
   }

   switch (local_name) {
     case "a":
     case "area":
     case "link":
       if (target.hasAttribute("href")) 
         linkNode = target;
       break;
     default:
       linkNode = findParentNode(event.originalTarget, "a");
       // <a> cannot be nested.  So if we find an anchor without an
       // href, there is no useful <a> around the target
       if (linkNode && !linkNode.hasAttribute("href"))
         linkNode = null;
       break;
   }
   if (linkNode) {
     if (event.button == 0 && !event.ctrlKey && !event.shiftKey &&
         !event.altKey && !event.metaKey) {
       // A Web panel's links should target the main content area.  Do this
       // if no modifier keys are down and if there's no target or the target equals
       // _main (the IE convention) or _content (the Mozilla convention).
       // The only reason we field _main and _content here is for the markLinkVisited
       // hack.
       target = linkNode.getAttribute("target");
       if (fieldNormalClicks && 
           (!target || target == "_content" || target  == "_main")) 
         // IE uses _main, SeaMonkey uses _content, we support both
       {
         if (!linkNode.href) return true;
         if (linkNode.getAttribute("onclick")) return true;
         var postData = { };
         var url = getShortcutOrURI(linkNode.href, postData);
         if (!url)
           return true;
         markLinkVisited(linkNode.href, linkNode);
         loadURI(url, null, postData.value);
         event.preventDefault();
         return false;
       }
       else if (linkNode.getAttribute("rel") == "sidebar") {
         // This is the Opera convention for a special link that - when clicked - allows
         // you to add a sidebar panel.  We support the Opera convention here.  The link's
         // title attribute contains the title that should be used for the sidebar panel.
         openDialog("chrome://browser/content/bookmarks/addBookmark2.xul", "",
                    "centerscreen,chrome,dialog,resizable,dependent",
                    linkNode.getAttribute("title"), 
                    linkNode.href, null, null, null, null, true);
         event.preventDefault();
         return false;
       }
       else if (target == "_search") {
         // Used in WinIE as a way of transiently loading pages in a sidebar.  We
         // mimic that WinIE functionality here and also load the page transiently.
         openWebPanel(gNavigatorBundle.getString("webPanels"), linkNode.href);
         event.preventDefault();
         return false;
       }
     }
     else
       handleLinkClick(event, linkNode.href, linkNode);
     return true;
   } else {
     // Try simple XLink
     var href;
     linkNode = target;
     while (linkNode) {
       if (linkNode.nodeType == Node.ELEMENT_NODE) {
         href = linkNode.getAttributeNS("http://www.w3.org/1999/xlink", "href");
         break;
       }
       linkNode = linkNode.parentNode;
     }
     if (href) {
       href = makeURLAbsolute(target.baseURI,href);
       handleLinkClick(event, href, null);
       return true;
     }
   }
   if (event.button == 1 &&
       !findParentNode(event.originalTarget, "scrollbar") &&
       gPrefService.getBoolPref("middlemouse.contentLoadURL")) {
     middleMousePaste(event);
   }
   return true;
 }

function handleLinkClick(event, href, linkNode)
{
  switch (event.button) {                                   
    case 0:
#ifdef XP_MACOSX
      if (event.metaKey) { // Cmd
#else
      if (event.ctrlKey) {
#endif
        openNewTabWith(href, linkNode, event, true);
        event.preventBubble();
        return true;
      } 
                                                       // if left button clicked
      if (event.shiftKey) {
        openNewWindowWith(href, linkNode, true);
        event.preventBubble();
        return true;
      }
      
      if (event.altKey) {
        saveURL(href, linkNode ? gatherTextUnder(linkNode) : "", null, true, true);
        return true;
      }
 
      return false;
    case 1:                                                         // if middle button clicked
      var tab;
      try {
        tab = gPrefService.getBoolPref("browser.tabs.opentabfor.middleclick")
      }
      catch(ex) {
        tab = true;
      }
      if (tab)
        openNewTabWith(href, linkNode, event, true);
      else
        openNewWindowWith(href, linkNode, true);
      event.preventBubble();
      return true;
  }
  return false;
}

function middleMousePaste(event)
{
  var url = readFromClipboard();
  if (!url)
    return;
  var postData = { };
  url = getShortcutOrURI(url, postData);
  if (!url)
    return;

  openUILink(url,
             event,
             true /* ignore the fact this is a middle click */);

  event.preventBubble();
}

function makeURLAbsolute( base, url ) 
{
  // Construct nsIURL.
  var ioService = Components.classes["@mozilla.org/network/io-service;1"]
                .getService(Components.interfaces.nsIIOService);
  var baseURI  = ioService.newURI(base, null, null);

  return ioService.newURI(baseURI.resolve(url), null, null).spec;
}

function findParentNode(node, parentNode)
{
  if (node && node.nodeType == Node.TEXT_NODE) {
    node = node.parentNode;
  }
  while (node) {
    var nodeName = node.localName;
    if (!nodeName)
      return null;
    nodeName = nodeName.toLowerCase();
    if (nodeName == "body" || nodeName == "html" ||
        nodeName == "#document") {
      return null;
    }
    if (nodeName == parentNode)
      return node;
    node = node.parentNode;
  }
  return null;
}

function saveFrameDocument()
{
  var focusedWindow = document.commandDispatcher.focusedWindow;
  if (isContentFrame(focusedWindow))
    saveDocument(focusedWindow.document);
}

/*
 * Note that most of this routine has been moved into C++ in order to
 * be available for all <browser> tags as well as gecko embedding. See
 * mozilla/content/base/src/nsContentAreaDragDrop.cpp.
 *
 * Do not add any new fuctionality here other than what is needed for
 * a standalone product.
 */

var contentAreaDNDObserver = {
  onDrop: function (aEvent, aXferData, aDragSession)
    {
      var url = transferUtils.retrieveURLFromData(aXferData.data, aXferData.flavour.contentType);

      // valid urls don't contain spaces ' '; if we have a space it
      // isn't a valid url, or if it's a javascript: or data: url,
      // bail out
      if (!url || !url.length || url.indexOf(" ", 0) != -1 ||
          /^\s*(javascript|data):/.test(url))
        return;

      switch (document.firstChild.getAttribute('windowtype')) {
        case "navigator:browser":
          var postData = { };
          var uri = getShortcutOrURI(url, postData);
          loadURI(uri, null, postData.value);
          break;
        case "navigator:view-source":
          viewSource(url);
          break;
      }
      
      // keep the event from being handled by the dragDrop listeners
      // built-in to gecko if they happen to be above us.    
      aEvent.preventDefault();
    },

  getSupportedFlavours: function ()
    {
      var flavourSet = new FlavourSet();
      flavourSet.appendFlavour("text/x-moz-url");
      flavourSet.appendFlavour("text/unicode");
      flavourSet.appendFlavour("application/x-moz-file", "nsIFile");
      return flavourSet;
    }
  
};

// For extensions
function getBrowser()
{
  if (!gBrowser)
    gBrowser = document.getElementById("content");
  return gBrowser;
}

function MultiplexHandler(event)
{ try {
    var node = event.target;
    var name = node.getAttribute('name');

    if (name == 'detectorGroup') {
        SetForcedDetector(true);
        SelectDetector(event, false);
    } else if (name == 'charsetGroup') {
        var charset = node.getAttribute('id');
        charset = charset.substring('charset.'.length, charset.length)
        SetForcedCharset(charset);
        SetDefaultCharacterSet(charset);
    } else if (name == 'charsetCustomize') {
        //do nothing - please remove this else statement, once the charset prefs moves to the pref window
    } else {
        SetForcedCharset(node.getAttribute('id'));
        SetDefaultCharacterSet(node.getAttribute('id'));
    }
    } catch(ex) { alert(ex); }
}

function SetDefaultCharacterSet(charset)
{
    BrowserSetDefaultCharacterSet(charset);
}

function SelectDetector(event, doReload)
{
    var uri =  event.target.getAttribute("id");
    var prefvalue = uri.substring('chardet.'.length, uri.length);
    if ("off" == prefvalue) { // "off" is special value to turn off the detectors
        prefvalue = "";
    }

    try {
        var pref = Components.classes["@mozilla.org/preferences-service;1"]
                             .getService(Components.interfaces.nsIPrefBranch);
        var str =  Components.classes["@mozilla.org/supports-string;1"]
                             .createInstance(Components.interfaces.nsISupportsString);

        str.data = prefvalue;
        pref.setComplexValue("intl.charset.detector",
                             Components.interfaces.nsISupportsString, str);
        if (doReload) window._content.location.reload();
    }
    catch (ex) {
        dump("Failed to set the intl.charset.detector preference.\n");
    }
}

function SetForcedDetector(doReload)
{
    BrowserSetForcedDetector(doReload);
}

function SetForcedCharset(charset)
{
    BrowserSetForcedCharacterSet(charset);
}

function BrowserSetDefaultCharacterSet(aCharset)
{
  // no longer needed; set when setting Force; see bug 79608
}

function BrowserSetForcedCharacterSet(aCharset)
{
  var docCharset = getBrowser().docShell.QueryInterface(
                            Components.interfaces.nsIDocCharset);
  docCharset.charset = aCharset;
  BrowserReloadWithFlags(nsIWebNavigation.LOAD_FLAGS_CHARSET_CHANGE);
}

function BrowserSetForcedDetector(doReload)
{
  getBrowser().documentCharsetInfo.forcedDetector = true;
  if (doReload)
    BrowserReloadWithFlags(nsIWebNavigation.LOAD_FLAGS_CHARSET_CHANGE);
}

function UpdateCurrentCharset()
{
    var menuitem = null;

    // exctract the charset from DOM
    var wnd = document.commandDispatcher.focusedWindow;
    if ((window == wnd) || (wnd == null)) wnd = window._content;
    menuitem = document.getElementById('charset.' + wnd.document.characterSet);

    if (menuitem) {
        // uncheck previously checked item to workaround Mac checkmark problem
        // bug 98625
        if (gPrevCharset) {
            var pref_item = document.getElementById('charset.' + gPrevCharset);
            if (pref_item)
              pref_item.setAttribute('checked', 'false');
        }
        menuitem.setAttribute('checked', 'true');
    }
}

function UpdateCharsetDetector()
{
    var prefvalue;

    try {
        var pref = Components.classes["@mozilla.org/preferences-service;1"]
                             .getService(Components.interfaces.nsIPrefBranch);
        prefvalue = pref.getComplexValue("intl.charset.detector",
                                         Components.interfaces.nsIPrefLocalizedString).data;
    }
    catch (ex) {
        prefvalue = "";
    }

    if (prefvalue == "") prefvalue = "off";
    dump("intl.charset.detector = "+ prefvalue + "\n");

    prefvalue = 'chardet.' + prefvalue;
    var menuitem = document.getElementById(prefvalue);

    if (menuitem) {
        menuitem.setAttribute('checked', 'true');
    }
}

function UpdateMenus(event)
{
    // use setTimeout workaround to delay checkmark the menu
    // when onmenucomplete is ready then use it instead of oncreate
    // see bug 78290 for the detail
    UpdateCurrentCharset();
    setTimeout(UpdateCurrentCharset, 0);
    UpdateCharsetDetector();
    setTimeout(UpdateCharsetDetector, 0);
}

function CreateMenu(node)
{
  var observerService = Components.classes["@mozilla.org/observer-service;1"].getService(Components.interfaces.nsIObserverService);
  observerService.notifyObservers(null, "charsetmenu-selected", node);
}

function charsetLoadListener (event)
{
    var charset = window._content.document.characterSet;

    if (charset.length > 0 && (charset != gLastBrowserCharset)) {
        if (!gCharsetMenu)
          gCharsetMenu = Components.classes['@mozilla.org/rdf/datasource;1?name=charset-menu'].getService().QueryInterface(Components.interfaces.nsICurrentCharsetListener);
        gCharsetMenu.SetCurrentCharset(charset);
        gPrevCharset = gLastBrowserCharset;
        gLastBrowserCharset = charset;
    }
}

#ifdef XP_MACOSX
const nsIWindowDataSource = Components.interfaces.nsIWindowDataSource;

function checkFocusedWindow()
{
  var windowManagerDS = Components.classes['@mozilla.org/rdf/datasource;1?name=window-mediator'].getService(nsIWindowDataSource);

  var sep = document.getElementById("sep-window-list");
  // Using double parens to avoid warning
  while ((sep = sep.nextSibling)) {
    var url = sep.getAttribute('id');
    var win = windowManagerDS.getWindowForResource(url);
    if (win == window) {
      sep.setAttribute("checked", "true");
      break;
    }
  }
}

function toOpenWindow( aWindow )
{
  aWindow.document.commandDispatcher.focusedWindow.focus();
}

function ShowWindowFromResource( node )
{
    var windowManagerDS = Components.classes['@mozilla.org/rdf/datasource;1?name=window-mediator'].getService(nsIWindowDataSource);
    
    var desiredWindow = null;
    var url = node.getAttribute('id');
	desiredWindow = windowManagerDS.getWindowForResource( url );
	if ( desiredWindow )
	{
            toOpenWindow(desiredWindow);
	}
}
#endif

/* Begin Page Style Functions */
function getStyleSheetArray(frame)
{
  var styleSheets = frame.document.styleSheets;
  var styleSheetsArray = new Array(styleSheets.length);
  for (var i = 0; i < styleSheets.length; i++) {
    styleSheetsArray[i] = styleSheets[i];
  }
  return styleSheetsArray;
}

function getAllStyleSheets(frameset)
{
  var styleSheetsArray = getStyleSheetArray(frameset);
  for (var i = 0; i < frameset.frames.length; i++) {
    var frameSheets = getAllStyleSheets(frameset.frames[i]);
    styleSheetsArray = styleSheetsArray.concat(frameSheets);
  }
  return styleSheetsArray;
}

function stylesheetFillPopup(menuPopup)
{
  var noStyle = menuPopup.firstChild;
  var persistentOnly = noStyle.nextSibling;
  var sep = persistentOnly.nextSibling;
  while (sep.nextSibling)
    menuPopup.removeChild(sep.nextSibling);

  var styleSheets = getAllStyleSheets(window._content);
  var currentStyleSheets = [];
  var styleDisabled = getMarkupDocumentViewer().authorStyleDisabled;
  var haveAltSheets = false;
  var altStyleSelected = false;
  
  for (var i = 0; i < styleSheets.length; ++i) {
    var currentStyleSheet = styleSheets[i];
    
    // Skip any stylesheets that don't match the screen media type.
    var media = currentStyleSheet.media.mediaText.toLowerCase();
    if (media && (media.indexOf("screen") == -1) && (media.indexOf("all") == -1))
        continue;
        
    if (currentStyleSheet.title) {
      if (!currentStyleSheet.disabled)
        altStyleSelected = true;

      haveAltSheets = true;
      
      var lastWithSameTitle = null;
      if (currentStyleSheet.title in currentStyleSheets)
        lastWithSameTitle = currentStyleSheets[currentStyleSheet.title];

      if (!lastWithSameTitle) {
        var menuItem = document.createElement("menuitem");
        menuItem.setAttribute("type", "radio");
        menuItem.setAttribute("label", currentStyleSheet.title);
        menuItem.setAttribute("data", currentStyleSheet.title);
        menuItem.setAttribute("checked", !currentStyleSheet.disabled && !styleDisabled);
        menuPopup.appendChild(menuItem);
        currentStyleSheets[currentStyleSheet.title] = menuItem;
      } else {
        if (currentStyleSheet.disabled)
          lastWithSameTitle.removeAttribute("checked");
      }
    }
  }
  
  noStyle.setAttribute("checked", styleDisabled);
  persistentOnly.setAttribute("checked", !altStyleSelected && !styleDisabled);
  persistentOnly.hidden = (window._content.document.preferredStylesheetSet) ? true : false;
  sep.hidden = (noStyle.hidden && persistentOnly.hidden) || !haveAltSheets;
  return true;
}

function stylesheetInFrame(frame, title) {
  var docStyleSheets = frame.document.styleSheets;

  for (var i = 0; i < docStyleSheets.length; ++i) {
    if (docStyleSheets[i].title == title)
      return true;
  }
  return false;
}

function stylesheetSwitchFrame(frame, title) {
  var docStyleSheets = frame.document.styleSheets;

  for (var i = 0; i < docStyleSheets.length; ++i) {
    var docStyleSheet = docStyleSheets[i];

    if (title == "_nostyle")
      docStyleSheet.disabled = true;
    else if (docStyleSheet.title)
      docStyleSheet.disabled = (docStyleSheet.title != title);
    else if (docStyleSheet.disabled)
      docStyleSheet.disabled = false;
  }
}

function stylesheetSwitchAll(frameset, title) {
  if (!title || title == "_nostyle" || stylesheetInFrame(frameset, title)) {
    stylesheetSwitchFrame(frameset, title);
  }
  for (var i = 0; i < frameset.frames.length; i++) {
    stylesheetSwitchAll(frameset.frames[i], title);
  }
}

function setStyleDisabled(disabled) {
  getMarkupDocumentViewer().authorStyleDisabled = disabled;
}

function updatePageStyles(evt)
{
  // XXX - Accessing window._content.document can generate an
  // onLocationChange for the current tab, this can cause the url
  // bar to be cleared. Prevent that happening by setting the clear
  // state to false for the duration of this function.
  var browser = getBrowser().selectedBrowser;
  var userTypedClear = browser.userTypedClear;
  browser.userTypedClear = false;

  if (!gPageStyleButton)
    gPageStyleButton = document.getElementById("page-theme-button");

  var hasAltSS = false;
  var stylesheets = window._content.document.styleSheets;
  var preferredSheet = window._content.document.preferredStylesheetSet;
  for (var i = 0; i < stylesheets.length; ++i) {
    var currentStyleSheet = stylesheets[i];
    
    if (currentStyleSheet.title && currentStyleSheet.title != preferredSheet) {
        // Skip any stylesheets that don't match the screen media type.
        var media = currentStyleSheet.media.mediaText.toLowerCase();
        if (media && (media.indexOf("screen") == -1) && (media.indexOf("all") == -1))
            continue;
        hasAltSS = true;
        break;
    }
  }

  if (hasAltSS) {
    gPageStyleButton.setAttribute("themes", "true");
    // FIXME: Do a first-time explanation of page styles here perhaps?
    // Avoid for now since Firebird's default home page has an alt sheet.
  }
  else
    gPageStyleButton.removeAttribute("themes");

  // Restore clear state
  browser.userTypedClear = userTypedClear;
}
/* End of the Page Style functions */

function clearObsoletePrefs()
{  
  // removed 10/03/2003
  try {
    PREF.clearUserPref("timebomb.first_launch_time");
  } catch (e) {}

  // removed 10/16/2003
  // migrate firebird cookie prefs, if they exist.
  try {
    PREF.clearUserPref("network.cookie.enable");
    // No error: it means this pref was not the default one, cookie were disabled.
    PREF.SetIntPref("network.cookie.cookieBehavior", 2);
  } catch (e) {
    try {
      PREF.clearUserPref("network.cookie.enableForOriginatingWebsiteOnly");
      // No error: it means that enableForOriginatingWebsiteOnly was true.
      PREF.SetIntPref("network.cookie.cookieBehavior", 1);
    } catch (e) {
    // here, either we already have migrated the cookie pref
    // or the new and old behavior are the same (0). In any case: nothing to do.
    }
  }

  // removed 03/09/2003
  // last of the forked cookie prefs
  try {
    PREF.clearUserPref("network.cookie.enableForCurrentSessionOnly");
    // No error, therefore we were limiting cookies to session
    PREF.setIntPref("network.cookie.lifetimePolicy", 2);
  } catch (e) {
    // nothing to do in this case
  }

  try {
    PREF.clearUserPref("network.cookie.warnAboutCookies");
    // No error: the pref is set to ask for cookies, set the correct pref
    // This will replace the setting if enableForCurrentSessionOnly was
    // also true, because dialogs explictly allow accepting for session
    PREF.setIntPref("network.cookie.lifetimePolicy", 1);
  } catch (e) {
    // nothing to do in this case
  }

  // removed 10/22/2003
  try {
    PREF.clearUserPref("browser.search.defaultengine");
  } catch (e) {}

  // removed 11/01/2003
  try {
    PREF.clearUserPref("print.use_global_printsettings");
  } catch (e) {}
  try {
    PREF.clearUserPref("print.save_print_settings");
  } catch (e) {}

}

var BrowserOffline = {
  /////////////////////////////////////////////////////////////////////////////
  // BrowserOffline Public Methods
  init: function ()
  {
    var os = Components.classes["@mozilla.org/observer-service;1"].getService(Components.interfaces.nsIObserverService);
    os.addObserver(this, "network:offline-status-changed", false);

    if (!this._uiElement)
      this._uiElement = document.getElementById("goOfflineMenuitem");

    // set the initial state
    var ioService = Components.classes["@mozilla.org/network/io-service;1"].getService(Components.interfaces.nsIIOService);
    var isOffline = false;
    try {
      isOffline = gPrefService.getBoolPref("browser.offline");
    }
    catch (e) { }
    ioService.offline = isOffline;

    this._updateOfflineUI(isOffline);
  },
  
  uninit: function ()
  {
    try {
      var os = Components.classes["@mozilla.org/observer-service;1"].getService(Components.interfaces.nsIObserverService);
      os.removeObserver(this, "network:offline-status-changed");
    } catch (ex) {
    }
  },

  toggleOfflineStatus: function ()
  {
    if (!this._canGoOffline()) {
      this._updateOfflineUI(false);
      return;
    }

    var ioService = Components.classes["@mozilla.org/network/io-service;1"].getService(Components.interfaces.nsIIOService);
    ioService.offline = !ioService.offline;
    
    gPrefService.setBoolPref("browser.offline", ioService.offline);
  },
  
  /////////////////////////////////////////////////////////////////////////////
  // nsIObserver
  observe: function (aSubject, aTopic, aState) 
  {
    if (aTopic != "network:offline-status-changed")
      return;
    
    this._updateOfflineUI(aState == "offline");
  },

  /////////////////////////////////////////////////////////////////////////////
  // BrowserOffline Implementation Methods
  _canGoOffline: function ()
  {
    var os = Components.classes["@mozilla.org/observer-service;1"].getService(Components.interfaces.nsIObserverService);
    if (os) {
      try {
        var cancelGoOffline = Components.classes["@mozilla.org/supports-PRBool;1"].createInstance(Components.interfaces.nsISupportsPRBool);
        os.notifyObservers(cancelGoOffline, "offline-requested", null);
        
        // Something aborted the quit process. 
        if (cancelGoOffline.data)
          return false;
      }
      catch (ex) {
      }
    }
    return true;
  },

  _uiElement: null,
  _updateOfflineUI: function (aOffline)
  {
    var offlineLocked = gPrefService.prefIsLocked("network.online");
    if (offlineLocked) 
      this._uiElement.setAttribute("disabled", "true");
      
    this._uiElement.setAttribute("checked", aOffline);
  }
};

function WindowIsClosing()
{
  var browser = getBrowser();
  var cn = browser.tabContainer.childNodes;
  var numtabs = cn.length;
  var reallyClose = browser.warnAboutClosingTabs(true);

  for (var i = 0; reallyClose && i < numtabs; ++i) {
    var ds = browser.getBrowserForTab(cn[i]).docShell;

    if (ds.contentViewer && !ds.contentViewer.permitUnload())
      reallyClose = false;
  }

  if (reallyClose)
    return closeWindow(false);

  return reallyClose;
}

var MailIntegration = {
  sendLinkForContent: function ()
  {
    this.sendMessage(Components.lookupMethod(window._content, 'location').call(window._content).href,
                     Components.lookupMethod(window._content.document, 'title').call(window._content.document));  
  },

  sendMessage: function (aBody, aSubject) 
  {
    // generate a mailto url based on the url and the url's title
    var mailtoUrl = aBody ? "mailto:?body=" + encodeURIComponent(aBody) + "&subject=" + encodeURIComponent(aSubject) : "mailto:";

    var ioService = Components.classes["@mozilla.org/network/io-service;1"].getService(Components.interfaces.nsIIOService);
    var uri = ioService.newURI(mailtoUrl, null, null);

    // now pass this url to the operating system
    this._launchExternalUrl(uri); 
  },

  // a generic method which can be used to pass arbitrary urls to the operating system.
  // aURL --> a nsIURI which represents the url to launch
  _launchExternalUrl: function(aURL)
  {
    var extProtocolSvc = Components.classes["@mozilla.org/uriloader/external-protocol-service;1"].getService(Components.interfaces.nsIExternalProtocolService);
    if (extProtocolSvc)
      extProtocolSvc.loadUrl(aURL);
#ifdef HAVE_SHELL_SERVICE
  },
  
  readMail: function ()
  {
    var shell = getShellService();
    if (shell)
      shell.openPreferredApplication(Components.interfaces.nsIShellService.APPLICATION_MAIL);
  },
  
  readNews: function ()
  {
    var shell = getShellService();
    if (shell)
      shell.openPreferredApplication(Components.interfaces.nsIShellService.APPLICATION_NEWS);
  },
  
  updateUnreadCount: function ()
  {
#ifdef XP_WIN
    var shell = Components.classes["@mozilla.org/browser/shell-service;1"]
                          .getService(Components.interfaces.nsIWindowsShellService);
    var unreadCount = shell.unreadMailCount;
    var message = gNavigatorBundle.getFormattedString("mailUnreadTooltip", [unreadCount]);
    var element = document.getElementById("mail-button");
    if (element)
      element.setAttribute("tooltiptext", message);
                                                              
    message = gNavigatorBundle.getFormattedString("mailUnreadMenuitem", [unreadCount]);
    element = document.getElementById("Browser:ReadMail");
    element.setAttribute("label", message);
#endif
  }
#else
  }
#endif
};

function BrowserOpenExtensions(aOpenMode)
{
  const EMTYPE = "Extension:Manager";
  
  var wm = Components.classes["@mozilla.org/appshell/window-mediator;1"]
                     .getService(Components.interfaces.nsIWindowMediator);
  var needToOpen = true;
  var windowType = EMTYPE + "-" + aOpenMode;
  var windows = wm.getEnumerator(windowType);
  while (windows.hasMoreElements()) {
    var theEM = windows.getNext().QueryInterface(Components.interfaces.nsIDOMWindowInternal);
    if (theEM.document.documentElement.getAttribute("windowtype") == windowType) {
      theEM.focus();
      needToOpen = false;
      break;
    }
  }

  if (needToOpen) {
    const EMURL = "chrome://mozapps/content/extensions/extensions.xul?type=" + aOpenMode;
    const EMFEATURES = "chrome,dialog=no,resizable";
    window.openDialog(EMURL, "", EMFEATURES);
  }
}

function AddKeywordForSearchField()
{
  var node = document.popupNode;
  var ioService = Components.classes["@mozilla.org/network/io-service;1"]
                            .getService(Components.interfaces.nsIIOService);
  var uri = Components.classes["@mozilla.org/network/standard-url;1"]
                      .getService(Components.interfaces.nsIURI);
  uri.spec = node.ownerDocument.URL;
  
  var keywordURL = ioService.newURI(node.form.action, node.ownerDocument.characterSet, uri);
  var spec = keywordURL.spec;
  var postData = "";
  
  if (node.form.method.toUpperCase() == "POST" && 
      (node.form.enctype == "application/x-www-form-urlencoded" || node.form.enctype == "")) {
    for (var i = 0; i < node.form.elements.length; ++i) {
      var e = node.form.elements[i];
      if (e.type.toLowerCase() == "text" || e.type.toLowerCase() == "hidden" || 
          e.localName.toLowerCase() == "textarea") 
        postData += escape(e.name + "=" + (e == node ? "%s" : e.value)) + "&";
      else if (e.localName.toLowerCase() == "select")
        postData += escape(e.name + "=" + e.options[e.selectedIndex].value) + "&";
    }
  }
  else {
    spec += "?" + escape(node.name) + "=%s";
    for (var i = 0; i < node.form.elements.length; ++i) {
      var e = node.form.elements[i];
      if (e == node) // avoid duplication of the target field value, which was populated above.
        continue;
        
      if (e.type.toLowerCase() == "text" || e.type.toLowerCase() == "hidden" || 
          e.localName.toLowerCase() == "textarea")
        spec += "&" + escape(e.name) + "=" + escape(e.value);
      else if (e.localName.toLowerCase() == "select")
        spec += "&" + escape(e.name) + "=" + escape(e.options[e.selectedIndex].value);
    }
  }
  openDialog("chrome://browser/content/bookmarks/addBookmark2.xul", "",
             "centerscreen,chrome,dialog,resizable,dependent",
             "", spec, null, node.ownerDocument.characterSet, null, null, 
             false, "", true, postData);
}

/////////////// livemark handling

// XXX this event listener can/should probably be combined with the onLinkAdded
// listener in tabbrowser.xml, which only listens for favicons and then passes
// them to onLinkIconAvailable in the ProgressListener.  We could extend the
// progress listener to have a generic onLinkAvailable and have tabbrowser pass
// along all events.  It should give us the browser for the tab, as well as
// the actual event.
function livemarkOnLinkAdded(event)
{
  if (!gLivemarksButton)
    gLivemarksButton = document.getElementById("livemark-button");

  // from tabbrowser.xml
  // mechanism for reading properties of the underlying XPCOM object
  // (ignoring potential getters/setters added by malicious content)
  var safeGetProperty = function(obj, propname) {
    return Components.lookupMethod(obj, propname).call(obj);
  }

  var erel = event.target.rel;
  var etype = event.target.type;
  var etitle = event.target.title;

  // this is a blogger post service URL; so skip it
  if (erel && erel == "service.post")
    return;

  if (etype == "application/rss+xml" ||
      etype == "application/atom+xml" ||
      etype == "application/x.atom+xml" ||
      etitle.indexOf("RSS") != -1 ||
      etitle.indexOf("Atom") != -1 ||
      etitle.indexOf("rss") != -1)
  {
    const targetDoc = safeGetProperty(event.target, "ownerDocument");

    // find which tab this is for, and set the attribute on the browser
    // should there be a getTabForDocument method on tabbedbrowser?
    var browserForLink = null;
    if (gBrowser.mTabbedMode) {
      var browserIndex = gBrowser.getBrowserIndexForDocument(targetDoc);
      if (browserIndex == -1)
        return;
      browserForLink = gBrowser.getBrowserAtIndex(browserIndex);
    } else if (gBrowser.mCurrentBrowser.contentDocument == targetDoc) {
      browserForLink = gBrowser.mCurrentBrowser;
    }

    if (!browserForLink) {
      // ??? this really shouldn't happen..
      return;
    }

    var livemarkLinks = [];
    if (browserForLink.livemarkLinks != null) {
      livemarkLinks = browserForLink.livemarkLinks;
    }

    livemarkLinks.push({ href: event.target.href,
                         type: event.target.type,
                        title: event.target.title});

    browserForLink.livemarkLinks = livemarkLinks;
    if (browserForLink == gBrowser || browserForLink == gBrowser.mCurrentBrowser)
      gLivemarksButton.setAttribute("livemarks", "true");
  }
}

// this is called both from onload and also whenever the user
// switches tabs; we update whether we show or hide the livemark
// button based on whether the window has livemarks set.
function updatePageLivemarks()
{
  if (!gLivemarksButton)
    gLivemarksButton = document.getElementById("livemark-button");

  var livemarkLinks = gBrowser.mCurrentBrowser.livemarkLinks;
  if (!livemarkLinks || livemarkLinks.length == 0) {
    gLivemarksButton.removeAttribute("livemarks");
    gLivemarksButton.setAttribute("tooltiptext", gNavigatorBundle.getString("livemarkNoLivemarksTooltip"));
  } else {
    gLivemarksButton.setAttribute("livemarks", "true");
    gLivemarksButton.setAttribute("tooltiptext", gNavigatorBundle.getString("livemarkHasLivemarksTooltip"));
  }
}

function livemarkFillPopup(menuPopup)
{
  var livemarkLinks = gBrowser.mCurrentBrowser.livemarkLinks;
  if (livemarkLinks == null) {
    // XXX hack -- menu opening depends on setting of an "open"
    // attribute, and the menu refuses to open if that attribute is
    // set (because it thinks it's already open).  onpopupshowing gets
    // called after the attribute is unset, and it doesn't get unset
    // if we return false.  so we unset it here; otherwise, the menu
    // refuses to work past this point.
    menuPopup.parentNode.removeAttribute("open");
    return false;
  }

  while (menuPopup.firstChild) {
    menuPopup.removeChild(menuPopup.firstChild);
  }

  for (var i = 0; i < livemarkLinks.length; i++) {
    var markinfo = livemarkLinks[i];

    var menuItem = document.createElement("menuitem");
    var baseTitle = markinfo.title || markinfo.href;
    var labelStr = gNavigatorBundle.getFormattedString("livemarkSubscribeTo", [baseTitle]);
    menuItem.setAttribute("label", labelStr);
    menuItem.setAttribute("data", markinfo.href);
    menuItem.setAttribute("tooltiptext", markinfo.href);
    menuPopup.appendChild(menuItem);
  }

  return true;
}

function livemarkAddMark(wincontent, data) {
  var title = wincontent.document.title;
  BookmarksUtils.addLivemark(wincontent.document.baseURI, data, title);
}

function updatePageFavIcon(aBrowser, aListener) {
  var uri = aBrowser.currentURI;

  if (!gBrowser.shouldLoadFavIcon(uri))
    return;

  // if we made it here with this null, then no <link> was found for
  // the page load.  We try to fetch a generic favicon.ico.
  if (aBrowser.mFavIconURL == null) {
    aBrowser.mFavIconURL = gBrowser.buildFavIconString(uri);
    // give it to the listener as well
    // XXX - there is no listener for non-tabbed-mode: this is why
    // the urlbar has no favicon when you switch from tabbed mode to
    // non-tabbed-mode.
    if (aListener)
      aListener.mIcon = aBrowser.mFavIconURL;
  }

  if (aBrowser == gBrowser.mCurrentBrowser) {
      if (gProxyFavIcon.src != aBrowser.mFavIconURL) {
          gProxyFavIcon.setAttribute("src", aBrowser.mFavIconURL);
          gProxyDeck.selectedIndex = 0;
      } else {
          gProxyDeck.selectedIndex = 1;
      }
  }

  if (aBrowser.mFavIconURL != null) {
    BookmarksUtils.loadFavIcon(uri.spec, aBrowser.mFavIconURL);
  }
}

function getUILink(item)
{
  var regionBundle = document.getElementById("bundle_browser_region");

  if (item == "tellAFriend")
    return regionBundle.getString("tellAFriendURL");
  
  if (item == "promote")
    return regionBundle.getString("promoteURL");
  
  return "";
}

function isBidiEnabled(){
  var rv = false;

  var systemLocale;
  try {
    var localService = Components.classes["@mozilla.org/intl/nslocaleservice;1"]
                                 .getService(Components.interfaces.nsILocaleService);
    systemLocale = localService.getSystemLocale().getCategory("NSILOCALE_CTYPE").substr(0,3);
  } catch (e){}

  rv = ( (systemLocale == "he-") || (systemLocale == "ar-") || (systemLocale == "syr") ||
         (systemLocale == "fa-") || (systemLocale == "ur-") );

  if (!rv) {
    // check the overriding pref
    var mPrefs = Components.classes["@mozilla.org/preferences-service;1"].getService(Components.interfaces.nsIPrefBranch);
    try {
      rv = mPrefs.getBoolPref("bidi.browser.ui");
    }
    catch (e){}
  }

  return rv;
}

function GetFrameDocumentsFromWindow(aWindow){
 if (aWindow.getComputedStyle(aWindow.document.body, "").direction == "ltr")
    aWindow.document.dir = "rtl";
  else
    aWindow.document.dir = "ltr";

  for (var run = 0; run < aWindow.frames.length; run++)
    GetFrameDocumentsFromWindow(aWindow.frames[run]);
}

function SwitchDocumentDirection(){
  GetFrameDocumentsFromWindow(window.content);
}

function SwitchFocusedTextEntryDirection() {
  if (isBidiEnabled())
  {
    var focusedElement = document.commandDispatcher.focusedElement;
    if (focusedElement)
      if (window.getComputedStyle(focusedElement, "").direction == "ltr")
        focusedElement.style.direction = "rtl";
      else
        focusedElement.style.direction = "ltr";
  }
}

function missingPluginInstaller(){
  this.missingPlugins = new Object();
}

missingPluginInstaller.prototype.installSinglePlugin = function(aEvent){
  var tabbrowser = getBrowser();
  var missingPluginsArray = new Object;
  missingPluginsArray[aEvent.target.type] = {mimetype: aEvent.target.type, pluginsPage: aEvent.target.getAttribute("pluginspage")};

  if (missingPluginsArray) {
    window.openDialog("chrome://mozapps/content/plugins/pluginInstallerWizard.xul",
      "_blank", "chrome,resizable=yes", {plugins: missingPluginsArray, tab: tabbrowser.mCurrentTab});
    }            

}
missingPluginInstaller.prototype.newMissingPlugin = function(aEvent){
  aEvent.target.addEventListener("mousedown", gMissingPluginInstaller.installSinglePlugin, false);

  var tabbrowser = getBrowser();
  const browsers = tabbrowser.mPanelContainer.childNodes;

  var window = aEvent.target.ownerDocument.__parent__;
  // walk up till the toplevel window
  while (window.parent != window)
    window = window.parent;

  var i = 0;
  for (; i < browsers.length; i++) {
    if (tabbrowser.getBrowserAtIndex(i).contentWindow == window)
      break;
  }

  var tab = tabbrowser.mTabContainer.childNodes[i];

  if (!gMissingPluginInstaller.missingPlugins)
    gMissingPluginInstaller.missingPlugins = new Object();

  if (!gMissingPluginInstaller.missingPlugins[tab])
    gMissingPluginInstaller.missingPlugins[tab] = new Object();

  gMissingPluginInstaller.missingPlugins[tab][aEvent.target.type] = {mimetype: aEvent.target.type, pluginsPage: aEvent.target.getAttribute("pluginspage")};

  var browser = tabbrowser.getBrowserAtIndex(i);

  var iconURL = "chrome://mozapps/skin/xpinstall/xpinstallItemGeneric.png";
  
  var bundle_browser = document.getElementById("bundle_browser");
  var messageString = bundle_browser.getString("missingpluginsMessage.title");
  var buttonString = bundle_browser.getString("missingpluginsMessage.button.label");

  tabbrowser.showMessage(browser, iconURL, messageString, buttonString, 
                                    "", "missing-plugin",
                                    null, "top", true);

  aEvent.preventDefault();
}

missingPluginInstaller.prototype.clearMissingPlugins = function(aTab){
  this.missingPlugins[aTab] = null;
}


missingPluginInstaller.prototype.observe = function(aSubject, aTopic, aData){
  switch (aTopic) {
    case "missing-plugin":
      // get the urls of missing plugins
      var tabbrowser = getBrowser();
      var missingPluginsArray = gMissingPluginInstaller.missingPlugins[tabbrowser.mCurrentTab];

      if (missingPluginsArray) {
        window.openDialog("chrome://mozapps/content/plugins/pluginInstallerWizard.xul",
           "_blank", "chrome,resizable=yes", {plugins: missingPluginsArray, tab: tabbrowser.mCurrentTab});
      }            

      tabbrowser.hideMessage(tabbrowser.selectedBrowser, "top");
      break;      
  }
}
var gMissingPluginInstaller = new missingPluginInstaller();
