/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is Mozilla addressbook.
 *
 * The Initial Developer of the Original Code is
 * Seth Spitzer <sspitzer@netscape.com>.
 * Portions created by the Initial Developer are Copyright (C) 2001
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
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

function GetAbViewListener()
{
  // the ab panel doesn't care if the total changes, or if the selection changes
  return null;
}

var gAddressBookPanelAbListener = {
  onItemAdded: function(parentDir, item) {
    // will not be called
  },
  onItemRemoved: function(parentDir, item) {
    // will only be called when an addressbook is deleted
    try {
      var directory = item.QueryInterface(Components.interfaces.nsIAbDirectory);
      // check if the item being removed is the directory
      // that we are showing in the addressbook sidebar
      // if so, select the person addressbook (it can't be removed)
      if (directory == GetAbView().directory) {
          var abPopup = document.getElementById('addressbookList');
          abPopup.setAttribute("selectedAB", kPersonalAddressbookURI);
          LoadPreviouslySelectedAB();
      } 
    }
    catch (ex) {
    }
  },
  onItemPropertyChanged: function(item, property, oldValue, newValue) {
    try {
      var directory = item.QueryInterface(Components.interfaces.nsIAbDirectory);
      // check if the item being changed is the directory
      // that we are showing in the addressbook sidebar
      if (directory == GetAbView().directory) {
          LoadPreviouslySelectedAB();
      }
    }
    catch (ex) {
    }
  }
};


// XXX todo
// can we combine some common code?  see OnLoadNewMailList()
// set popup with address book names
function LoadPreviouslySelectedAB()
{
  var abPopup = document.getElementById('addressbookList');
  if ( abPopup )
  {
    var menupopup = document.getElementById('addressbookList-menupopup');
    var selectedAB = abPopup.getAttribute("selectedAB");
    if (!selectedAB) 
      selectedAB = kPersonalAddressbookURI;
      
    if ( selectedAB && menupopup && menupopup.childNodes )
    {
      for ( var index = menupopup.childNodes.length - 1; index >= 0; index-- )
      {
        if ( menupopup.childNodes[index].getAttribute('value') == selectedAB )
        {
          abPopup.label = menupopup.childNodes[index].getAttribute('label');
          abPopup.value = menupopup.childNodes[index].getAttribute('value');
          break;
        }
      }
    }
  }
  ChangeDirectoryByURI(abPopup.selectedItem.id);
}

function AbPanelLoad() 
{
  InitCommonJS(); 

  UpgradeAddressBookResultsPaneUI("mailnews.ui.addressbook_panel_results.version");

  LoadPreviouslySelectedAB();

  // add a listener, so we can switch directories if
  // the current directory is deleted, and change the name if the
  // selected directory's name is modified
  var addrbookSession = Components.classes["@mozilla.org/addressbook/services/session;1"].getService().QueryInterface(Components.interfaces.nsIAddrBookSession);
  // this listener only cares when a directory is removed or modified
  addrbookSession.addAddressBookListener(gAddressBookPanelAbListener,
                  Components.interfaces.nsIAbListener.directoryRemoved | Components.interfaces.nsIAbListener.changed);

  gSearchInput = document.getElementById("searchInput");
}


function AbPanelOnChange(event)
{
  var abPopup = document.getElementById('addressbookList');
  abPopup.setAttribute("selectedAB", abPopup.value);
}

function AbPanelUnload()
{
  var addrbookSession = Components.classes["@mozilla.org/addressbook/services/session;1"].getService().QueryInterface(Components.interfaces.nsIAddrBookSession);
  addrbookSession.removeAddressBookListener(gAddressBookPanelAbListener);

  CloseAbView();
}

function AbPanelNewCard() 
{
  goNewCardDialog(abList.selectedItem.getAttribute('id'));
}

function AbPanelNewList() 
{
  goNewListDialog(abList.selectedItem.getAttribute('id'));
}

function ResultsPaneSelectionChanged() 
{
  // do nothing for ab panel
}

function OnClickedCard() 
{
  // do nothing for ab panel
}

function AbResultsPaneDoubleClick(card) 
{
  // double click for ab panel means "send mail to this person / list"
  AbNewMessage();
}

function UpdateCardView() 
{
  // do nothing for ab panel
}

