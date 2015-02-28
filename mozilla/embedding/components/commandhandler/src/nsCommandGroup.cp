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

#include "nsString.h"
#include "nsReadableUtils.h"
#include "nsVoidArray.h"


#include "nsCommandGroup.h"


/* Implementation file */
NS_IMPL_ISUPPORTS1(nsControllerCommandGroup, nsIControllerCommandGroup)

nsControllerCommandGroup::nsControllerCommandGroup()
{
}

nsControllerCommandGroup::~nsControllerCommandGroup()
{
  ClearGroupsHash();
}

void
nsControllerCommandGroup::ClearGroupsHash()
{
    mGroupsHash.Reset(ClearEnumerator, (void *)this);
}

#if 0
#pragma mark -
#endif

/* void addCommandToGroup (in DOMString aCommand, in DOMString aGroup); */
NS_IMETHODIMP
nsControllerCommandGroup::AddCommandToGroup(const nsAReadableString & aCommand, const nsAReadableString & aGroup)
{
  nsStringKey   groupKey(aGroup);  
  nsVoidArray*  commandList;  
  if ((commandList = (nsVoidArray *)mGroupsHash.Get(&groupKey)) == nsnull)
  {
    // make this list
    commandList = new nsVoidArray;    
    mGroupsHash.Put(&groupKey, (void *)commandList);
  }
  // add the command to the list. Note that we're not checking for duplicates here
  PRUnichar*  commandString = ToNewUnicode(aCommand);     // we store allocated PRUnichar* in the array
  if (!commandString) return NS_ERROR_OUT_OF_MEMORY;
  
  PRBool      appended = commandList->AppendElement((void *)commandString);
  NS_ASSERTION(appended, "Append failed");

  return NS_OK;
}

/* void removeCommandFromGroup (in DOMString aCommand, in DOMString aGroup); */
NS_IMETHODIMP
nsControllerCommandGroup::RemoveCommandFromGroup(const nsAReadableString & aCommand, const nsAReadableString & aGroup)
{
  nsStringKey   groupKey(aGroup);
  nsVoidArray*  commandList = (nsVoidArray *)mGroupsHash.Get(&groupKey);
  if (!commandList) return NS_OK;     // no group

  PRInt32   numEntries = commandList->Count();
  for (PRInt32 i = 0; i < numEntries; i ++)
  {
    PRUnichar*  commandString = (PRUnichar*)commandList->ElementAt(i);
    if (aCommand.Equals(commandString))
    {
      commandList->RemoveElementAt(i);
      nsMemory::Free(commandString);
      break;
    }
  }

  return NS_OK;
}

/* boolean isCommandInGroup (in DOMString aCommand, in DOMString aGroup); */
NS_IMETHODIMP
nsControllerCommandGroup::IsCommandInGroup(const nsAReadableString & aCommand, const nsAReadableString & aGroup, PRBool *_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = PR_FALSE;
  
  nsStringKey   groupKey(aGroup);
  nsVoidArray*  commandList = (nsVoidArray *)mGroupsHash.Get(&groupKey);
  if (!commandList) return NS_OK;     // no group
  
  PRInt32   numEntries = commandList->Count();
  for (PRInt32 i = 0; i < numEntries; i ++)
  {
    PRUnichar*  commandString = (PRUnichar*)commandList->ElementAt(i);
    if (aCommand.Equals(commandString))
    {
      *_retval = PR_TRUE;
      break;
    }
  }
  return NS_OK;
}

#if 0
#pragma mark -
#endif
 
PRBool nsControllerCommandGroup::ClearEnumerator(nsHashKey *aKey, void *aData, void* closure)
{
  nsVoidArray*    commandList = (nsVoidArray *)aData;
  if (commandList)
  {  
    PRInt32   numEntries = commandList->Count();
    for (PRInt32 i = 0; i < numEntries; i ++)
    {
      PRUnichar*  commandString = (PRUnichar*)commandList->ElementAt(i);
      nsMemory::Free(commandString);
    }
    
    delete commandList;
  }

  return PR_TRUE;
}
