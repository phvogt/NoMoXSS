/* -*- Mode: Java; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
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
 * The Original Code is Mozilla Communicator client code.
 *
 * The Initial Developer of the Original Code is 
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 * Alec Flett <alecf@netscape.com>
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

var smtpService = Components.classes["@mozilla.org/messengercompose/smtp;1"].getService(Components.interfaces.nsISmtpService);
var gPrefBranch = Components.classes["@mozilla.org/preferences-service;1"].getService(Components.interfaces.nsIPrefBranch);

function onLoad()
{
    parent.onPanelLoaded('am-smtp.xul');
    var defaultSmtpServer = null;
    try {
        defaultSmtpServer = smtpService.defaultServer;
    }
    catch (ex) {
        defaultSmtpServer = null;
    }
    initSmtpSettings(defaultSmtpServer);

    // Get the default smtp server preference to check if we need to lock the
    // advance button on the panel. 
    if (defaultSmtpServer) {
      var defaultSmtpServerKey = gPrefBranch.getCharPref("mail.smtp.defaultserver");
      var prefString = "mail.smtpserver."+ defaultSmtpServerKey + ".advanced.disable"; 

      var advButton = document.getElementById("smtp.advancedbutton");
      if (gPrefBranch.prefIsLocked(prefString)) {
        advButton.setAttribute("disabled", "true");
      }
    }
}

function onSave()
{
    var defaultSmtpServer = null;
    try {
        defaultSmtpServer = smtpService.defaultServer;
    }
    catch (ex) {
        defaultSmtpServer = null;
    }

    //if we have a null defaultSmtpServer and if the hostname field has valid 
    //values then we create a server and make it as the default.
    if ((defaultSmtpServer == null) && (!hostnameIsIllegal(gSmtpHostname.value))) {
      defaultSmtpServer = smtpService.createSmtpServer();
    }
   
    saveSmtpSettings(defaultSmtpServer);
}

function onExitAdvancedDialog(deleteSmtpServers,replaceWithDefault)
{
  for (var index in deleteSmtpServers) {
    var server = smtpService.getServerByKey(deleteSmtpServers[index]);
    smtpService.deleteSmtpServer(server);
    if (replaceWithDefault)
      window.parent.replaceWithDefaultSmtpServer(deleteSmtpServers[index]);
  }
}

function onAdvanced(event)
{
    if (smtpService.defaultServer && hostnameIsIllegal(gSmtpHostname.value)) {
      var alertTitle = window.parent.gBrandBundle.getString("brandShortName");
      var alertMsg = window.parent.gPrefsBundle.getString("enterValidHostname");
      var promptService = Components.classes["@mozilla.org/embedcomp/prompt-service;1"].getService(Components.interfaces.nsIPromptService);
      if (promptService)
        promptService.alert(window, alertTitle, alertMsg);
      else
        window.alert(alertMsg);

      return false;
    }

    // fix for bug #60647
    // when the user presses "Advanced..." we save any changes
    // they made so that the changes will show up in the advanced dialog
    // and when they return from the advanced dialog, the changes they
    // made won't be blown away.
    //
    // the only remaing problem is if the user "cancels" out of the
    // account manager dialog, those changes will get saved.  
    // that is covered in bug #63825
    onSave();

    var args = {result: false};
    window.openDialog('chrome://messenger/content/SmtpServerList.xul', 'smtp', 'modal,titlebar,chrome', args);

    if (args.result) {
        // this is the wrong way to do this.
        dump("reloading panel...\n");
        onLoad();
    }
}

