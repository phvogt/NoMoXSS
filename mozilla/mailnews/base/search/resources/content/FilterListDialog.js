/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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
 * The Original Code is Mozilla Communicator client code, released
 * March 31, 1998.
 *
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation. Portions created by Netscape are
 * Copyright (C) 1998-1999 Netscape Communications Corporation. All
 * Rights Reserved.
 */

const MSG_FOLDER_FLAG_INBOX = 0x1000

var gRDF = Components.classes["@mozilla.org/rdf/rdf-service;1"].getService(Components.interfaces.nsIRDFService);

var gEditButton;
var gDeleteButton;
var gReorderUpButton;
var gReorderDownButton;
var gRunFiltersFolderPickerLabel;
var gRunFiltersFolderPicker;
var gRunFiltersButton;
var gFilterBundle;
var gPromptService;
var gFilterListMsgWindow = null;
var gFilterTree;
var gStatusBar;
var gStatusText;
var gCurrentServerURI = null;

var gStatusFeedback = {
	showStatusString: function(status)
  {
    gStatusText.setAttribute("value", status);
  },
	startMeteors: function()
  {
    // change run button to be a stop button
    gRunFiltersButton.setAttribute("label", gRunFiltersButton.getAttribute("stoplabel"));
    gRunFiltersButton.setAttribute("accesskey", gRunFiltersButton.getAttribute("stopaccesskey"));
    gStatusBar.setAttribute("mode", "undetermined");
  },
	stopMeteors: function() 
  {
    try {
      // change run button to be a stop button
      gRunFiltersButton.setAttribute("label", gRunFiltersButton.getAttribute("runlabel"));
      gRunFiltersButton.setAttribute("accesskey", gRunFiltersButton.getAttribute("runaccesskey"));
      gStatusBar.setAttribute("mode", "normal");
    }
    catch (ex) {
      // can get here if closing window when running filters
    }
  },
  showProgress: function(percentage)
  {
      //dump("XXX progress" + percentage + "\n");
  },
  closeWindow: function()
  {
  }
};

const nsMsgFilterMotion = Components.interfaces.nsMsgFilterMotion;

function onLoad()
{
    var gPromptService = Components.classes["@mozilla.org/embedcomp/prompt-service;1"].getService();
    gPromptService = gPromptService.QueryInterface(Components.interfaces.nsIPromptService);

    gFilterListMsgWindow = Components.classes["@mozilla.org/messenger/msgwindow;1"].createInstance(Components.interfaces.nsIMsgWindow);
    gFilterListMsgWindow.statusFeedback = gStatusFeedback;
    gFilterListMsgWindow.SetDOMWindow(window); 

    gFilterBundle = document.getElementById("bundle_filter");
    gFilterTree = document.getElementById("filterTree");

    gEditButton = document.getElementById("editButton");
    gDeleteButton = document.getElementById("deleteButton");
    gReorderUpButton = document.getElementById("reorderUpButton");
    gReorderDownButton = document.getElementById("reorderDownButton");
    gRunFiltersFolderPickerLabel = document.getElementById("folderPickerPrefix");
    gRunFiltersFolderPicker = document.getElementById("runFiltersFolder");
    gRunFiltersButton = document.getElementById("runFiltersButton");
    gStatusBar = document.getElementById("statusbar-icon");
    gStatusText = document.getElementById("statusText");

    updateButtons();

    // get the selected server if it can have filters.
    var firstItem = getSelectedServerForFilters();

    // if the selected server cannot have filters, get the default server
    // if the default server cannot have filters, check all accounts
    // and get a server that can have filters.
    if (!firstItem)
        firstItem = getServerThatCanHaveFilters();

    if (firstItem) {
        selectServer(firstItem);
    }

    gFilterTree.addEventListener("click",onFilterClick,true);
}

/*
function onCancel()
{
    var firstItem = getSelectedServerForFilters();
    if (!firstItem)
        firstItem = getServerThatCanHaveFilters();
    
    if (firstItem) {
        var resource = gRDF.GetResource(firstItem);
        var msgFolder = resource.QueryInterface(Components.interfaces.nsIMsgFolder);
        if (msgFolder)
        {
           msgFolder.ReleaseDelegate("filter");
           msgFolder.setFilterList(null);
           try
           {
              //now find Inbox
              var outNumFolders = new Object();
              var inboxFolder = msgFolder.getFoldersWithFlag(0x1000, 1, outNumFolders);
              inboxFolder.setFilterList(null);
           }
           catch(ex)
           {
             dump ("ex " +ex + "\n");
           }
        }
    }
    window.close();
}
*/

function onFilterServerClick(selection)
{
    var itemURI = selection.getAttribute('id');

    if (!itemURI || itemURI == gCurrentServerURI)
      return;

    // Save the current filters to disk before switching because
    // the dialog may be closed and we'll lose current filters.
    var filterList = currentFilterList();
    if (filterList) 
      filterList.saveToDefaultFile();

    selectServer(itemURI);
}

function CanRunFiltersAfterTheFact(aServer)
{
  // can't manually run news filters yet
  if (aServer.type == "nntp")
    return false;

  // filter after the fact is implement using search
  // so if you can't search, you can't filter after the fact
  return aServer.canSearchMessages;
}

// roots the tree at the specified server
function setServer(uri)
{
   if (uri == gCurrentServerURI)
     return;

   var resource = gRDF.GetResource(uri);
   var msgFolder = resource.QueryInterface(Components.interfaces.nsIMsgFolder);

   //Calling getFilterList will detect any errors in rules.dat, backup the file, and alert the user
   //we need to do this because gFilterTree.setAttribute will cause rdf to call getFilterList and there is 
   //no way to pass msgWindow in that case. 

   if (msgFolder)
     msgFolder.getFilterList(gFilterListMsgWindow);

   rebuildFilterTree(uri);
   
   // root the folder picker to this server
   gRunFiltersFolderPicker.setAttribute("ref", uri);
 
   // run filters after the fact not supported by news
   if (CanRunFiltersAfterTheFact(msgFolder.server)) {
     gRunFiltersFolderPicker.removeAttribute("hidden");
     gRunFiltersButton.removeAttribute("hidden");
     gRunFiltersFolderPickerLabel.removeAttribute("hidden");

     // for POP3 and IMAP, select the first folder, which is the INBOX
     gRunFiltersFolderPicker.selectedIndex = 0;
   }
   else {
     gRunFiltersFolderPicker.setAttribute("hidden", "true");
     gRunFiltersButton.setAttribute("hidden", "true");
     gRunFiltersFolderPickerLabel.setAttribute("hidden", "true");
   }

   // Get the first folder uri for this server. INBOX for
   // imap and pop accts and 1st news group for news.
   var firstFolderURI = getFirstFolderURI(msgFolder);
   SetFolderPicker(firstFolderURI, "runFiltersFolder");
   updateButtons();

   gCurrentServerURI = uri;
}

function toggleFilter(aFilterURI)
{
    var filterResource = gRDF.GetResource(aFilterURI);
    var filter = filterResource.GetDelegate("filter",
                                            Components.interfaces.nsIMsgFilter);
    if (filter.unparseable)
    {
      var str = gFilterBundle.getString("cannotEnableFilter");
      window.alert(str);
      return;
    }
    filter.enabled = !filter.enabled;
    refresh();
}

// sets up the menulist and the gFilterTree
function selectServer(uri)
{
    // update the server menu
    var serverMenu = document.getElementById("serverMenu");
    
    var resource = gRDF.GetResource(uri);
    var msgFolder = resource.QueryInterface(Components.interfaces.nsIMsgFolder);

    // XXX todo
    // See msgFolderPickerOverlay.js, SetFolderPicker()
    // why do we have to do this?  seems like a hack to work around a bug.
    // the bug is that the (deep) content isn't there
    // and so this won't work:
    //
    //   var menuitems = serverMenu.getElementsByAttribute("id", uri);
    //   serverMenu.selectedItem = menuitems[0];
    //
    // we might need help from a XUL template expert to help out here.
    // see bug #XXXXXX
    serverMenu.setAttribute("label", msgFolder.name);
    serverMenu.setAttribute("uri",uri);

    setServer(uri);
}

function getFilter(index)
{
  var filter = gFilterTree.builderView.getResourceAtIndex(index);
  filter = filter.GetDelegate("filter", Components.interfaces.nsIMsgFilter);
  return filter;
}

function currentFilter()
{
    var currentIndex = gFilterTree.currentIndex;
    if (currentIndex == -1)
      return null;
    
    var filter;

    try {
      filter = getFilter(currentIndex);
    } catch (ex) {
      filter = null;
    }
    return filter;
}

function currentFilterList()
{
    // note, serverUri might be a newsgroup
    var serverUri = document.getElementById("serverMenu").getAttribute("uri");
    var filterList = gRDF.GetResource(serverUri).GetDelegate("filter", Components.interfaces.nsIMsgFilterList);
    return filterList;
}

function onFilterSelect(event)
{
    updateButtons();
}

function onEditFilter() 
{
  var selectedFilter = currentFilter();
  var args = {filter: selectedFilter};

  window.openDialog("chrome://messenger/content/FilterEditor.xul", "FilterEditor", "chrome,modal,titlebar,resizable,centerscreen", args);

  if ("refresh" in args && args.refresh)
    refresh();
}

function onNewFilter(emailAddress)
{
  var curFilterList = currentFilterList();
  var args = {filterList: curFilterList};
  
  window.openDialog("chrome://messenger/content/FilterEditor.xul", "FilterEditor", "chrome,modal,titlebar,resizable,centerscreen", args);

  if ("refresh" in args && args.refresh)
    refresh();
}

function onDeleteFilter()
{
    var filter = currentFilter();
    if (!filter) return;
    var filterList = currentFilterList();
    if (!filterList) return;

    var confirmStr = gFilterBundle.getString("deleteFilterConfirmation");

    if (!window.confirm(confirmStr)) 
      return;

    filterList.removeFilter(filter);
    refresh();
}

function onUp(event)
{
    moveCurrentFilter(nsMsgFilterMotion.up);
}

function onDown(event)
{
    moveCurrentFilter(nsMsgFilterMotion.down);
}

function viewLog()
{
  var uri = gFilterTree.getAttribute("ref");
  var server = gRDF.GetResource(uri).QueryInterface(Components.interfaces.nsIMsgFolder).server;

  var filterList = currentFilterList();
  var args = {filterList: filterList};

  window.openDialog("chrome://messenger/content/viewLog.xul", "FilterLog", "chrome,modal,titlebar,resizable,centerscreen", args);
}

function onFilterClose()
{
  // make sure to save the filter to disk
  var filterList = currentFilterList();
  if (filterList) 
    filterList.saveToDefaultFile();

  var nsIPromptService = Components.interfaces.nsIPromptService;
  if (gRunFiltersButton.getAttribute("label") == gRunFiltersButton.getAttribute("stoplabel")) {
    var promptService = Components.classes["@mozilla.org/embedcomp/prompt-service;1"].getService(Components.interfaces.nsIPromptService);

    var promptTitle = gFilterBundle.getString("promptTitle");
    var promptMsg = gFilterBundle.getString("promptMsg");;
    var stopButtonLabel = gFilterBundle.getString("stopButtonLabel");
    var continueButtonLabel = gFilterBundle.getString("continueButtonLabel");
    
    var result = promptService.confirmEx(window, promptTitle, promptMsg,
          (promptService.BUTTON_TITLE_IS_STRING*promptService.BUTTON_POS_0) +
          (promptService.BUTTON_TITLE_IS_STRING*promptService.BUTTON_POS_1),
          continueButtonLabel, stopButtonLabel, null, null, {value:0});

    if (result)
      gFilterListMsgWindow.StopUrls();
    else
      return false;
  }

  window.close();
  return true;
}

function runSelectedFilters()
{
  // if run button has "stop" label, do stop.
  if (gRunFiltersButton.getAttribute("label") == gRunFiltersButton.getAttribute("stoplabel")) {
    gFilterListMsgWindow.StopUrls();
    return;
  }
  
  var folderURI = gRunFiltersFolderPicker.getAttribute("uri");
  var resource = gRDF.GetResource(folderURI);
  var msgFolder = resource.QueryInterface(Components.interfaces.nsIMsgFolder);
  var filterService = Components.classes["@mozilla.org/messenger/services/filters;1"].getService(Components.interfaces.nsIMsgFilterService);
  var filterList = filterService.getTempFilterList(msgFolder);
  var folders = Components.classes["@mozilla.org/supports-array;1"].createInstance(Components.interfaces.nsISupportsArray);
  folders.AppendElement(msgFolder);

  // make sure the tmp filter list uses the real filter list log stream
  filterList.logStream = currentFilterList().logStream;
  filterList.loggingEnabled = currentFilterList().loggingEnabled;
  var sel = gFilterTree.view.selection;
  for (var i = 0; i < sel.getRangeCount(); i++) {
    var start = {}, end = {};
    sel.getRangeAt(i, start, end);
    for (var j = start.value; j <= end.value; j++) {
      var curFilter = getFilter(j);
      if (curFilter)
      {
        filterList.insertFilterAt(start.value - j, curFilter);
      }
    }
  }

  filterService.applyFiltersToFolders(filterList, folders, gFilterListMsgWindow);
}

function moveCurrentFilter(motion)
{
    var filterList = currentFilterList();
    var filter = currentFilter();
    if (!filterList || !filter) 
      return;

    filterList.moveFilter(filter, motion);
    refresh();
}

function rebuildFilterTree(uri)
{
  gFilterTree.view.selection.clearSelection();
  gFilterTree.removeAttribute("ref");
  gFilterTree.setAttribute("ref", uri);
}

function refresh() 
{
    if (!gFilterTree) 
      return;

    var selectedRes;

    // store the selected resource before we rebuild the tree
    try {
      selectedRes = gFilterTree.currentIndex >= 0 ? gFilterTree.builderView.getResourceAtIndex(gFilterTree.currentIndex) : null;
    }
    catch (ex) {
      dump("ex = " + ex + "\n");
      selectedRes = null;
    }

    rebuildFilterTree(gCurrentServerURI);

    // restore selection to the previous selected resource
    if (selectedRes) {
        var previousSel = gFilterTree.builderView.getIndexOfResource(selectedRes);
        if (previousSel >= 0) {
            // sometimes the selected element is gone.
            gFilterTree.treeBoxObject.selection.select(previousSel);
            gFilterTree.treeBoxObject.ensureRowIsVisible(previousSel);
        }
    }
}

function updateButtons()
{
    var numFiltersSelected = gFilterTree.view.selection.count;
    var oneFilterSelected = (numFiltersSelected == 1);

    var filter = currentFilter();
    // "edit" only enabled when one filter selected or if we couldn't parse the filter
    gEditButton.disabled = !oneFilterSelected || filter.unparseable;
    
    // "delete" only enabled when one filter selected
    gDeleteButton.disabled = !oneFilterSelected;

    // we can run multiple filters on a folder
    // so only disable this UI if no filters are selected
    gRunFiltersFolderPickerLabel.disabled = !numFiltersSelected;
    gRunFiltersFolderPicker.disabled = !numFiltersSelected;
    gRunFiltersButton.disabled = !numFiltersSelected;

    // "up" enabled only if one filter selected, and it's not the first
    gReorderUpButton.disabled = !(oneFilterSelected && gFilterTree.currentIndex > 0);
    // "down" enabled only if one filter selected, and it's not the last
    gReorderDownButton.disabled = !(oneFilterSelected && gFilterTree.currentIndex < gFilterTree.view.rowCount-1);
}

/**
  * get the selected server if it can have filters
  */
function getSelectedServerForFilters()
{
    var firstItem = null;
    var args = window.arguments;
    var selectedFolder = args[0].folder;
  
    if (args && args[0] && selectedFolder)
    {
        var msgFolder = selectedFolder.QueryInterface(Components.interfaces.nsIMsgFolder);
        try
        {
            var rootFolder = msgFolder.rootFolder;
            if (rootFolder.isServer)
            {
                var server = rootFolder.server;

                if (server.canHaveFilters)
                {
                    // for news, select the news folder
                    // for everything else, select the folder's server
                    firstItem = (server.type == "nntp") ? msgFolder.URI : rootFolder.URI;
                }
            }
        }
        catch (ex)
        {
        }
    }

    return firstItem;
}

/** if the selected server cannot have filters, get the default server
  * if the default server cannot have filters, check all accounts
  * and get a server that can have filters.
  */
function getServerThatCanHaveFilters()
{
    var firstItem = null;

    var accountManager
        = Components.classes["@mozilla.org/messenger/account-manager;1"].
            getService(Components.interfaces.nsIMsgAccountManager);

    var defaultAccount = accountManager.defaultAccount;
    var defaultIncomingServer = defaultAccount.incomingServer;

    // check to see if default server can have filters
    if (defaultIncomingServer.canHaveFilters) {
        firstItem = defaultIncomingServer.serverURI;
    }
    // if it cannot, check all accounts to find a server
    // that can have filters
    else
    {
        var allServers = accountManager.allServers;
        var numServers = allServers.Count();
        var index = 0;
        for (index = 0; index < numServers; index++)
        {
            var currentServer
            = allServers.GetElementAt(index).QueryInterface(Components.interfaces.nsIMsgIncomingServer);

            if (currentServer.canHaveFilters)
            {
                firstItem = currentServer.serverURI;
                break;
            }
        }
    }

    return firstItem;
}

function onFilterClick(event)
{
    // we only care about button 0 (left click) events
    if (event.button != 0)
      return;
    
    var row = {}, colID = {}, childElt = {};
    var filterTree = document.getElementById("filterTree");
    filterTree.treeBoxObject.getCellAt(event.clientX, event.clientY, row, colID, childElt);
    if (row.value == -1 || row.value > filterTree.view.rowCount-1 || event.originalTarget.localName != "treechildren") {
      if (event.originalTarget.localName == "treecol") { 
        // clicking on the name column in the filter list should not sort
        event.preventBubble();
      }
      return;
    }
        
    if (colID.value == "activeColumn") {
      var res = filterTree.builderView.getResourceAtIndex(row.value);
      toggleFilter(res.Value);
    }
}

function onFilterDoubleClick(event)
{
    // we only care about button 0 (left click) events
    if (event.button != 0)
      return;

    var row = {}, colID = {}, childElt = {};
    var filterTree = document.getElementById("filterTree");
    filterTree.treeBoxObject.getCellAt(event.clientX, event.clientY, row, colID, childElt);
    if (row.value == -1 || row.value > filterTree.view.rowCount-1 || event.originalTarget.localName != "treechildren") {
      // double clicking on a non valid row should not open the edit filter dialog
      return;
    }

    // if the cell is in a "cycler" column (the enabled column)
    // don't open the edit filter dialog with the selected filter
    var col = document.getElementById(colID.value);
    if (col.getAttribute("cycler") != "true")
      onEditFilter();
}

function onFilterTreeKeyPress(event)
{
  // for now, only do something on space key
  if (event.keyCode != 0)
    return;

  var rangeCount = gFilterTree.view.selection.getRangeCount();
  for (var i = 0; i < rangeCount; ++i) {
    var start = {}, end = {};
    gFilterTree.view.selection.getRangeAt(i, start, end);
    for (var k = start.value; k <= end.value; ++k) {
      var res = gFilterTree.builderView.getResourceAtIndex(k);
      toggleFilter(res.Value);
    }
  }
}

function doHelpButton()
{
  openHelp("mail-filters");
}

/**
  * For a given server folder, get the uri for the first folder. For imap
  * and pop it's INBOX and it's the very first group for news accounts.
  */
function getFirstFolderURI(msgFolder)
{
  // Sanity check.
  if (! msgFolder.isServer)
    return msgFolder.URI;

  try {
    // Find Inbox for imap and pop
    if (msgFolder.server.type != "nntp")
    {
      var outNumFolders = new Object();
      var inboxFolder = msgFolder.getFoldersWithFlag(MSG_FOLDER_FLAG_INBOX, 1, outNumFolders);
      if (inboxFolder)
        return inboxFolder.URI;
      else
        // If inbox does not exist then use the server uri as default.
        return msgFolder.URI;
    }
    else
      // XXX TODO: For news, we should find the 1st group/folder off the news groups. For now use server uri.
      return msgFolder.URI;
  }
  catch (ex) {
    dump(ex + "\n");
  }
  return msgFolder.URI;
}
