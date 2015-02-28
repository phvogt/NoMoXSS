/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
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
 *   Michiel van Leeuwen (mvl@exedo.nl)
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

#include "nsXSSHostConnectPermissionManager.h"
#include "nsCRT.h"
#include "nsNetUtil.h"
#include "nsILineInputStream.h"
#include "nsAppDirectoryServiceDefs.h"
#include "prprf.h"

//#include "nsIInputstream.h"
//#include "nsIOutputStream.h"


////////////////////////////////////////////////////////////////////////////////
// nsXSSHostTableHashEntry Implementation

// constructor for the hashtable entries that initializes the internal hashtable
nsXSSHostTableHashEntry::nsXSSHostTableHashEntry()
{
	mTable.Init();
}
////////////////////////////////////////////////////////////////////////////////
// nsXSSHostConnectPermissionManager Implementation

// the name of the file with the permissions
static const char kPermissionsFileName[] = "xsshostconnectperm.1";

// writes the file after this miliseconds. multiple writes are combined
static const PRUint32 kLazyWriteTimeout = 2000; //msec

// implemented interfaces
NS_IMPL_ISUPPORTS2(nsXSSHostConnectPermissionManager, nsIXSSHostConnectPermissionManager, nsISupportsWeakReference)

// Constructor for a new object. Ensure that Init() is called before using
// the object.
nsXSSHostConnectPermissionManager::nsXSSHostConnectPermissionManager()
 : mHostCount(0),
   mChangedList(PR_FALSE)
{
}

// Destructor stops the timer and empties the hashtable
nsXSSHostConnectPermissionManager::~nsXSSHostConnectPermissionManager()
{
  if (mWriteTimer)
    mWriteTimer->Cancel();

  RemoveAllFromMemory();
}

// initialize the hashtable, open the file and read the contents
nsresult nsXSSHostConnectPermissionManager::Init()
{
  nsresult rv;

  if (!mHostTable.Init()) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  // Cache the permissions file
  rv = NS_GetSpecialDirectory(NS_APP_USER_PROFILE_50_DIR, getter_AddRefs(mPermissionsFile));
  if (NS_SUCCEEDED(rv)) {
    rv = mPermissionsFile->AppendNative(NS_LITERAL_CSTRING(kPermissionsFileName));
  }

  // Ignore an error. That is not a problem. No cookperm.txt usually.
  Read();

  return NS_OK;
}


/**
 * Creates a new instance and initializes it
 */
NS_METHOD
nsXSSHostConnectPermissionManager::Create(nsISupports* outer, const nsIID& aIID, void* *aInstancePtr)
{
    NS_ENSURE_NO_AGGREGATION(outer);
    nsXSSHostConnectPermissionManager* serv = new nsXSSHostConnectPermissionManager();
    if (serv == nsnull)
        return NS_ERROR_OUT_OF_MEMORY;
    NS_ADDREF(serv);
    nsresult rv = serv->QueryInterface(aIID, aInstancePtr);
	serv->Init();
    NS_RELEASE(serv);
    return rv;
}

/**
 * Add permission information for two given hosts and permission type. 
 *
 * @param fromHost    the host from which the connection starts
 * @param toHost      the target host of the connection
 * @param permission  allow (ALLOW_CONNECT) or deny (DENY_CONNECT) this 
 *                    host combination
 */
NS_IMETHODIMP
nsXSSHostConnectPermissionManager::Add(const nsACString &fromHost,
									   const nsACString &toHost,
									   PRUint32 permission)
{
	nsXSSHostTableHashEntry *mEntry;
	nsInt32HashSet* permissionset;

	mHostTable.Get(fromHost, &mEntry);
	if (!mEntry) {
		mEntry = new nsXSSHostTableHashEntry();
		mHostTable.Put(fromHost, mEntry);
	}
	mEntry->mTable.Get(toHost, &permissionset);
	if (!permissionset) {
		permissionset = new nsInt32HashSet();
		permissionset->Init(3);
		mEntry->mTable.Put(toHost, permissionset);
		++mHostCount;
	}
	permissionset->Put(permission);

	mChangedList = PR_TRUE;
	LazyWrite();
	return NS_OK;
}

/**
 * Test whether the two hosts are allowed to connect
 * @param fromHost    the host from which the connection starts
 * @param toHost      the target host of the connection
 * @param return      ALLOW_CONNECT if the connection is allowed, 
 *                    ALLOW_DENY if it is forbidden or ALLOW_UNKOWN
 *                    if there is no rule for this connection.
 */
NS_IMETHODIMP
nsXSSHostConnectPermissionManager::TestPermission(const nsACString &fromHost,
												  const nsACString &toHost,
												  PRUint32   *aPermission)
{
	NS_ASSERTION(aPermission, "no permission pointer");

	// set the default
	*aPermission = nsIXSSHostConnectPermissionManager::UNKNOWN_CONNECT;

	nsXSSHostTableHashEntry *mEntry;
	nsInt32HashSet* permissionset;

	mHostTable.Get(fromHost, &mEntry);
	if (mEntry) {
		mEntry->mTable.Get(toHost, &permissionset);
		if (permissionset) {
			if (permissionset->Contains(nsIXSSHostConnectPermissionManager::ALLOW_CONNECT)) 
				*aPermission = nsIXSSHostConnectPermissionManager::ALLOW_CONNECT;
			if (permissionset->Contains(nsIXSSHostConnectPermissionManager::DENY_CONNECT)) 
				*aPermission = nsIXSSHostConnectPermissionManager::DENY_CONNECT;
		}
	}

	return NS_OK;
}

//*****************************************************************************
//*** nsXSSHostConnectPermissionManager private methods
//*****************************************************************************

/**
 * Callback to count the number of entries
 * @param aKey        the key of the entry
 * @param aData       the value of the entry
 * @param userArg     not used here
 */
PR_STATIC_CALLBACK(PLDHashOperator)
nsXSSEnumCount(const nsACString& aKey, nsXSSHostTableHashEntry* aData, void* userArg) {
	return PL_DHASH_NEXT;
}

/**
 * Callback to count the number of entries
 * @param aKey        the key of the entry
 * @param aData       the value of the entry
 * @param userArg     not used here
 */
PR_STATIC_CALLBACK(PLDHashOperator)
nsXSSEnumCountHashsets(const nsACString& aKey, nsInt32HashSet* aData, void* userArg) {
	return PL_DHASH_NEXT;
}

/**
 * Callback to clear the entries
 *
 * @param aKey        the key of the entry
 * @param aData       the value of the entry
 * @param userArg     not used here
 */
PR_STATIC_CALLBACK(PLDHashOperator)
nsXSSEnumClear(const nsACString& aKey, nsXSSHostTableHashEntry* aData, void* userArg) {

	if (aData) {
		aData->mTable.Clear();
	}
	return PL_DHASH_NEXT;
}

/**
 * A helper function that adds the pointer to the entry to the list.
 * This is not threadsafe, and only safe if the consumer does not 
 * modify the list. It is only used in Write() where we are sure
 * that nothing else changes the list while we are saving.
 *
 * @param aKey        the key of the entry
 * @param aData       the value of the entry
 * @param userArg     not used here
 */
PR_STATIC_CALLBACK(PLDHashOperator)
nsXSSEnumAddEntryToList(const nsACString& aKey, nsXSSHostTableHashEntry* aData, void *arg)
{
  // add the key to the list
  nsACString*** elementPtr = NS_STATIC_CAST(nsACString***, arg);
  **elementPtr = (nsACString*)&aKey;
  ++(*elementPtr);
  return PL_DHASH_NEXT;
}

/**
 * A helper function that adds the pointer to the entry to the list.
 * This is not threadsafe, and only safe if the consumer does not 
 * modify the list. It is only used in Write() where we are sure
 * that nothing else changes the list while we are saving.
 *
 * @param aKey        the key of the entry
 * @param aData       the value of the entry
 * @param userArg     not used here
 */
PR_STATIC_CALLBACK(PLDHashOperator)
nsXSSEnumAddEntryToListHashsets(const nsACString& aKey, nsInt32HashSet* aData, void* arg) {

  // add the key to the list
  nsACString*** elementPtr = NS_STATIC_CAST(nsACString***, arg);
  **elementPtr = (nsACString*)&aKey;
  ++(*elementPtr);
  return PL_DHASH_NEXT;
}


/**
 * Removes all entries from the memory
 */
nsresult
nsXSSHostConnectPermissionManager::RemoveAllFromMemory()
{
  PRUint32 count = mHostTable.EnumerateRead(nsXSSEnumClear, nsnull);
  mHostTable.Clear();
  mHostCount = 0;
  mChangedList = PR_TRUE;
  return NS_OK;
}

// used as a seperator in a line in the file
static const char kTab = '\t';
// used as a line terminator
static const char kNew = '\n';

/**
 * Reads the content of the file and stores it in the hash.
 * 
 * format is:
 * fromHost \t toHost \t permission \n
 *
 * empty lines, lines that start with # or lines with too few
 * arguments are ignored.
 */
nsresult
nsXSSHostConnectPermissionManager::Read()
{
	nsresult rv;

	// get a stream for the permissionfile
	nsCOMPtr<nsIInputStream> fileInputStream;
	rv = NS_NewLocalFileInputStream(getter_AddRefs(fileInputStream), mPermissionsFile);
	NS_ENSURE_SUCCESS(rv, rv);
	nsCOMPtr<nsILineInputStream> lineInputStream = do_QueryInterface(fileInputStream, &rv);
	NS_ENSURE_SUCCESS(rv, rv);

	nsCAutoString buffer;
	PRBool isMore = PR_TRUE;

	// read to the end of the file
	while (isMore && NS_SUCCEEDED(lineInputStream->ReadLine(buffer, &isMore))) {
		// filter the empty and comment lines
		if (buffer.IsEmpty() || buffer.First() == '#') {
			continue;
		}

		nsCStringArray lineArray;

		// Split the line at tabs
		lineArray.ParseString(buffer.get(), "\t");

		// we need 3 parts, ignore the rest
		if (lineArray.Count() == 3) {

			// last part is the permission
			PRInt32 error;
			PRUint32 permission = lineArray[2]->ToInteger(&error);
			if (error)
				continue;

			// add the tupel to the permissionlist
			rv = Add(*lineArray[0], *lineArray[1], permission);
			NS_ENSURE_SUCCESS(rv, rv);

		}
	}

	mChangedList = PR_FALSE;

	return NS_OK;
}

/**
 * Starts a timer that delays the write.
 */
void
nsXSSHostConnectPermissionManager::LazyWrite()
{
  if (mWriteTimer) {
    mWriteTimer->SetDelay(kLazyWriteTimeout);
  } else {
    mWriteTimer = do_CreateInstance("@mozilla.org/timer;1");
    if (mWriteTimer) {
      mWriteTimer->InitWithFuncCallback(DoLazyWrite, this, kLazyWriteTimeout,
                                        nsITimer::TYPE_ONE_SHOT);
    }
  }
}

/**
 * Callback that is called from the timer and writes the hash to the file
 * 
 * @param aTimer    the Timer that called the function
 * @param aClosure  the permissionmanager that called it.
 */
void
nsXSSHostConnectPermissionManager::DoLazyWrite(nsITimer *aTimer,
                                 void     *aClosure)
{
  nsXSSHostConnectPermissionManager *service = NS_REINTERPRET_CAST(nsXSSHostConnectPermissionManager*, aClosure);
  service->Write();
  service->mWriteTimer = 0;
}

/**
 * Writes the permission to the file
 */
nsresult
nsXSSHostConnectPermissionManager::Write()
{
  nsresult rv;

  // nothing changed, so nothing to do
  if (!mChangedList) {
    return NS_OK;
  }

  // only write, if we have a file
  if (!mPermissionsFile) {
    return NS_ERROR_FAILURE;
  }

  // get outputstream
  nsCOMPtr<nsIOutputStream> fileOutputStream;
  rv = NS_NewSafeLocalFileOutputStream(getter_AddRefs(fileOutputStream),
                                       mPermissionsFile,
                                       -1,
                                       0600);
  NS_ENSURE_SUCCESS(rv, rv);

  // get a buffered output stream 4096 bytes big, to optimize writes
  nsCOMPtr<nsIOutputStream> bufferedOutputStream;
  rv = NS_NewBufferedOutputStream(getter_AddRefs(bufferedOutputStream), fileOutputStream, 4096);
  NS_ENSURE_SUCCESS(rv, rv);

  // the header in the file
  static const char kHeader[] = 
    "# XSS host connect permission file\n"
    "# This is a generated file! Do not edit.\n\n";

  bufferedOutputStream->Write(kHeader, sizeof(kHeader) - 1, &rv);

  // write file!

  /* format is:
   * fromHost \t toHost \t permission
   */

  // get all fromHostnames
  PRUint32 countFromHost = mHostTable.EnumerateRead(nsXSSEnumCount, nsnull);
  nsACString* *fromHostNameList = new nsACString*[countFromHost];
  if (!fromHostNameList) return NS_ERROR_OUT_OF_MEMORY;

  nsACString** fromHostNameListCopy = fromHostNameList;
  mHostTable.EnumerateRead(nsXSSEnumAddEntryToList, &fromHostNameListCopy);

  // first loop through all fromHostnames
  PRUint32 i;
  for (i = 0; i < countFromHost; ++i) {
	  nsACString *fromHostName = NS_STATIC_CAST(nsACString*, fromHostNameList[i]);

	  nsXSSHostTableHashEntry *mEntry;
	  mHostTable.Get(*fromHostName, &mEntry);

	  // there is an entry, so process it
	  if (mEntry) {

		  // get all toHostnames
		  PRUint32 countToHost = mEntry->mTable.EnumerateRead(nsXSSEnumCountHashsets, nsnull);
		  nsACString* *toHostNameList = new nsACString*[countToHost];
		  if (!toHostNameList) return NS_ERROR_OUT_OF_MEMORY;

		  nsACString** toHostNameListCopy = toHostNameList;
		  mEntry->mTable.EnumerateRead(nsXSSEnumAddEntryToListHashsets, &toHostNameListCopy);

		  // now loop through all toHostnames
		  PRUint32 j;
		  for (j = 0; j < countToHost; ++j) {
			  nsACString *toHostName = NS_STATIC_CAST(nsACString*, toHostNameList[j]);

			  mHostTable.Get(*fromHostName, &mEntry);
			  nsInt32HashSet* permissionset;

			  // get the permissions
			  mEntry->mTable.Get(*toHostName, &permissionset);
			  if (permissionset) {

				  // all informations to write are present, so write them
				  bufferedOutputStream->Write(PromiseFlatCString(*fromHostName).get(), fromHostName->Length(), &rv);
				  bufferedOutputStream->Write(&kTab, 1, &rv);

				  bufferedOutputStream->Write(PromiseFlatCString(*toHostName).get(), toHostName->Length(), &rv);
				  bufferedOutputStream->Write(&kTab, 1, &rv);

				  char permissionString[5];
				  if (permissionset->Contains(nsIXSSHostConnectPermissionManager::ALLOW_CONNECT)) {
					  PRUint32 len = PR_snprintf(permissionString, sizeof(permissionString) - 1, "%u", nsIXSSHostConnectPermissionManager::ALLOW_CONNECT);
					  bufferedOutputStream->Write(permissionString, len, &rv);
				  }
				  if (permissionset->Contains(nsIXSSHostConnectPermissionManager::DENY_CONNECT)) {
					  PRUint32 len = PR_snprintf(permissionString, sizeof(permissionString) - 1, "%u", nsIXSSHostConnectPermissionManager::DENY_CONNECT);
					  bufferedOutputStream->Write(permissionString, len, &rv);
				  }

				  bufferedOutputStream->Write(&kNew, 1, &rv);
			  }
		  }
		  delete[] toHostNameList;
	  }
  }
  delete[] fromHostNameList;

  // All went ok. Maybe except for problems in Write(), but the stream detects
  // that for us
  nsCOMPtr<nsISafeOutputStream> safeStream = do_QueryInterface(bufferedOutputStream);
  NS_ASSERTION(safeStream, "expected a safe output stream!");
  if (safeStream) {
    rv = safeStream->Finish();
    if (NS_FAILED(rv)) {
      NS_WARNING("failed to save permissions file! possible dataloss");
      return rv;
    }
  }

  mChangedList = PR_FALSE;
  return NS_OK;
}
