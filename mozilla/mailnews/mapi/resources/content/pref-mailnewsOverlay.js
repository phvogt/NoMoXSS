/* 
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
 * The Original Code is mozilla.org code.
 * 
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are 
 * Copyright (C) 2001 Netscape Communications Corporation.  All
 * Rights Reserved.
 * 
 * Contributor(s): 
 * Srilatha Moturi <srilatha@netscape.com>
 * Rajiv Dayal <rdayal@netscape.com>
 */

function mailnewsOverlayStartup() {
    mailnewsOverlayInit();
    if (!("mapiPref" in parent)) {
        parent.mapiPref = new Object;
        parent.mapiPref.isDefaultMailClient = 
               document.getElementById("mailnewsEnableMapi").checked;
        
        var mailnewsEnableNews = document.getElementById("mailnewsEnableNews"); 
        if (mailnewsEnableNews)
          parent.mapiPref.isDefaultNewsClient = mailnewsEnableNews.checked;
    }
    else { 
        // when we switch between different panes
        // set the checkbox based on the saved state
        var mailnewsEnableMapi = document.getElementById("mailnewsEnableMapi");
        var mailnewsEnableNews = document.getElementById("mailnewsEnableNews"); 
        
        if (parent.mapiPref.isDefaultMailClient)
            mailnewsEnableMapi.setAttribute("checked", "true");
        else
            mailnewsEnableMapi.setAttribute("checked", "false");
        
        if (mailnewsEnableNews)
        {
          if (parent.mapiPref.isDefaultNewsClient)
             mailnewsEnableNews.setAttribute("checked", "true");
          else
             mailnewsEnableNews.setAttribute("checked", "false");
        }
    }
}

function registerCallback(){
    if ("mapiCallback" in parent && parent.mapiCallback)
        return;
    if ("hPrefWindow" in parent && parent.hPrefWindow) {
        parent.hPrefWindow.registerOKCallbackFunc( onOK );
        parent.mapiCallback = true;
    }
}

function mailnewsOverlayInit() {
    try {
        var mapiRegistry = Components.classes[ "@mozilla.org/mapiregistry;1" ].
                   getService( Components.interfaces.nsIMapiRegistry );
    }
    catch(ex){
        mapiRegistry = null;
    }

    const prefbase = "system.windows.lock_ui.";
    var mailnewsEnableMapi = document.getElementById("mailnewsEnableMapi");
    var mailnewsEnableNews = document.getElementById("mailnewsEnableNews");
    
    if (mapiRegistry) {
    // initialise preference component.
    // While the data is coming from the system registry, we use a set
    // of parallel preferences to indicate if the ui should be locked.
        try { 
            var prefService = Components.classes["@mozilla.org/preferences-service;1"]
                          .getService()
                          .QueryInterface(Components.interfaces.nsIPrefService);
            var prefBranch = prefService.getBranch(prefbase);
            if (prefBranch && prefBranch.prefIsLocked("defaultMailClient")) {
                mapiRegistry.isDefaultMailClient = prefBranch.getBoolPref("defaultMailClient") ;
                mailnewsEnableMapi.setAttribute("disabled", "true");
           }
        }
        catch(ex) {}

        mailnewsEnableMapi.setAttribute("checked", mapiRegistry.isDefaultMailClient ? "true" : "false");
        if (mailnewsEnableNews)
          mailnewsEnableNews.setAttribute("checked", mapiRegistry.isDefaultNewsClient ? "true" : "false");
    }
    else
    {
        mailnewsEnableMapi.setAttribute("disabled", "true");
        if (mailnewsEnableNews)
          mailnewsEnableNews.setAttribute("disabled", "true");
    }
}

function onEnableMapi() {
    // save the state of the checkbox
    if ("mapiPref" in parent)
        parent.mapiPref.isDefaultMailClient = 
               document.getElementById("mailnewsEnableMapi").checked;
}

function onEnableNews() {
    // save the state of the checkbox
    if ("mapiPref" in parent)
        parent.mapiPref.isDefaultNewsClient = document.getElementById("mailnewsEnableNews").checked;
}

function onOK()
{
    try {
        var mapiRegistry = Components.classes[ "@mozilla.org/mapiregistry;1" ].getService( Components.interfaces.nsIMapiRegistry );
    }
    catch(ex){
        mapiRegistry = null;
    } 

    if (mapiRegistry && ("mapiPref" in parent)) {
      if (mapiRegistry.isDefaultMailClient != parent.mapiPref.isDefaultMailClient)
        mapiRegistry.isDefaultMailClient = parent.mapiPref.isDefaultMailClient ;
      
      if ("isDefaultNewsClient" in parent.mapiPref)
      {
        if (mapiRegistry.isDefaultNewsClient != parent.mapiPref.isDefaultNewsClient)
          mapiRegistry.isDefaultNewsClient = parent.mapiPref.isDefaultNewsClient;
      }
    }
}

// Install the onload handler
addEventListener("load", mailnewsOverlayStartup, false);
