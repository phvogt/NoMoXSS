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
 * The Original Code is the Netscape security libraries.
 * 
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are 
 * Copyright (C) 2000 Netscape Communications Corporation.  All
 * Rights Reserved.
 * 
 * Contributor(s):
 *  Ian McGreer <mcgreer@netscape.com>
 * 
 * Alternatively, the contents of this file may be used under the
 * terms of the GNU General Public License Version 2 or later (the
 * "GPL"), in which case the provisions of the GPL are applicable 
 * instead of those above.  If you wish to allow use of your 
 * version of this file only under the terms of the GPL and not to
 * allow others to use your version of this file under the MPL,
 * indicate your decision by deleting the provisions above and
 * replace them with the notice and other provisions required by
 * the GPL.  If you do not delete the provisions above, a recipient
 * may use your version of this file under either the MPL or the
 * GPL.
 *
 */

#include "nsNSSComponent.h" // for PIPNSS string bundle calls.
#include "nsCertTree.h"
#include "nsIX509Cert.h"
#include "nsIX509CertValidity.h"
#include "nsIX509CertDB.h"
#include "nsXPIDLString.h"
#include "nsReadableUtils.h"
#include "nsNSSCertificate.h"
#include "nsNSSCertHelper.h"
#include "nsINSSCertCache.h"

#include "prlog.h"
#ifdef PR_LOGGING
extern PRLogModuleInfo* gPIPNSSLog;
#endif

static NS_DEFINE_CID(kNSSComponentCID, NS_NSSCOMPONENT_CID);

// treeArrayElStr
//
// structure used to hold map of tree.  Each thread (an organization
// field from a cert) has an element in the array.  The numChildren field
// stores the number of certs corresponding to that thread.
struct treeArrayElStr {
  nsString   orgName;     /* heading for thread                   */
  PRBool     open;        /* toggle open state for thread         */
  PRInt32    certIndex;   /* index into cert array for 1st cert   */
  PRInt32    numChildren; /* number of chidren (certs) for thread */
};

CompareCacheHashEntry::CompareCacheHashEntry()
:key(nsnull)
{
  for (int i = 0; i < max_criterions; ++i) {
    mCritInit[i] = PR_FALSE;
  }
}

PR_STATIC_CALLBACK(const void *)
CompareCacheGetKey(PLDHashTable *table, PLDHashEntryHdr *hdr)
{
  CompareCacheHashEntry *entry = NS_STATIC_CAST(CompareCacheHashEntry*, hdr);
  return entry->key;
}

PR_STATIC_CALLBACK(PRBool)
CompareCacheMatchEntry(PLDHashTable *table, const PLDHashEntryHdr *hdr,
                         const void *key)
{
  const CompareCacheHashEntry *entry = NS_STATIC_CAST(const CompareCacheHashEntry*, hdr);
  return entry->key == key;
}

PR_STATIC_CALLBACK(PRBool)
CompareCacheInitEntry(PLDHashTable *table, PLDHashEntryHdr *hdr,
                     const void *key)
{
  new (hdr) CompareCacheHashEntry();
  CompareCacheHashEntry *entry = NS_STATIC_CAST(CompareCacheHashEntry*, hdr);
  entry->key = (void*)key;
  return PR_TRUE;
}

PR_STATIC_CALLBACK(void)
CompareCacheClearEntry(PLDHashTable *table, PLDHashEntryHdr *hdr)
{
  CompareCacheHashEntry *entry = NS_STATIC_CAST(CompareCacheHashEntry*, hdr);
  entry->~CompareCacheHashEntry();
}

static PLDHashTableOps gMapOps = {
  PL_DHashAllocTable,
  PL_DHashFreeTable,
  CompareCacheGetKey,
  PL_DHashVoidPtrKeyStub,
  CompareCacheMatchEntry,
  PL_DHashMoveEntryStub,
  CompareCacheClearEntry,
  PL_DHashFinalizeStub,
  CompareCacheInitEntry
};



NS_IMPL_ISUPPORTS2(nsCertTree, nsICertTree, nsITreeView)

nsCertTree::nsCertTree() : mTreeArray(NULL)
{
  mCompareCache.ops = nsnull;
  mNSSComponent = do_GetService(kNSSComponentCID);
}

void nsCertTree::ClearCompareHash()
{
  if (mCompareCache.ops) {
    PL_DHashTableFinish(&mCompareCache);
    mCompareCache.ops = nsnull;
  }
}

nsresult nsCertTree::InitCompareHash()
{
  ClearCompareHash();
  if (!PL_DHashTableInit(&mCompareCache, &gMapOps, nsnull,
                         sizeof(CompareCacheHashEntry), 128)) {
    mCompareCache.ops = nsnull;
    return NS_ERROR_OUT_OF_MEMORY;
  }
  return NS_OK;
}

nsCertTree::~nsCertTree()
{
  ClearCompareHash();
  delete [] mTreeArray;
}

void
nsCertTree::FreeCertArray()
{
  if (mCertArray) {
    PRUint32 count;
    nsresult rv = mCertArray->Count(&count);
    if (NS_FAILED(rv))
    {
      NS_ASSERTION(0, "Count failed");
      return;
    }
    PRInt32 i;
    for (i = count - 1; i >= 0; i--)
    {
      mCertArray->RemoveElementAt(i);
    }
  }
}

CompareCacheHashEntry *
nsCertTree::getCacheEntry(void *cache, void *aCert)
{
  PLDHashTable &aCompareCache = *NS_REINTERPRET_CAST(PLDHashTable*, cache);
  return NS_STATIC_CAST(CompareCacheHashEntry*,
                        PL_DHashTableOperate(&aCompareCache, aCert,
                                             PL_DHASH_ADD));
}

void nsCertTree::RemoveCacheEntry(void *key)
{
  PL_DHashTableOperate(&mCompareCache, key, PL_DHASH_REMOVE);
}

// CountOrganizations
//
// Count the number of different organizations encountered in the cert
// list.
PRInt32
nsCertTree::CountOrganizations()
{
  PRUint32 i, certCount;
  nsresult rv = mCertArray->Count(&certCount);
  if (NS_FAILED(rv)) return -1;
  if (certCount == 0) return 0;
  nsCOMPtr<nsISupports> isupport = dont_AddRef(mCertArray->ElementAt(0));
  nsCOMPtr<nsIX509Cert> orgCert = do_QueryInterface(isupport);
  nsCOMPtr<nsIX509Cert> nextCert = nsnull;
  PRInt32 orgCount = 1;
  for (i=1; i<certCount; i++) {
    isupport = dont_AddRef(mCertArray->ElementAt(i));
    nextCert = do_QueryInterface(isupport);
    // XXX we assume issuer org is always criterion 1
    if (CmpBy(&mCompareCache, orgCert, nextCert, sort_IssuerOrg, sort_None, sort_None) != 0) {
      orgCert = nextCert;
      orgCount++;
    }
  }
  return orgCount;
}

// GetThreadDescAtIndex
//
// If the row at index is an organization thread, return the collection
// associated with that thread.  Otherwise, return null.
treeArrayEl *
nsCertTree::GetThreadDescAtIndex(PRInt32 index)
{
  int i, idx=0;
  if (index < 0) return nsnull;
  for (i=0; i<mNumOrgs; i++) {
    if (index == idx) {
      return &mTreeArray[i];
    }
    if (mTreeArray[i].open) {
      idx += mTreeArray[i].numChildren;
    }
    idx++;
    if (idx > index) break;
  }
  return nsnull;
}

//  GetCertAtIndex
//
//  If the row at index is a cert, return that cert.  Otherwise, return null.
nsIX509Cert *
nsCertTree::GetCertAtIndex(PRInt32 index)
{
  int i, idx = 0, cIndex = 0, nc;
  nsIX509Cert *rawPtr = nsnull;
  if (index < 0) return nsnull;
  // Loop over the threads
  for (i=0; i<mNumOrgs; i++) {
    if (index == idx) return nsnull; // index is for thread
    idx++; // get past the thread
    nc = (mTreeArray[i].open) ? mTreeArray[i].numChildren : 0;
    if (index < idx + nc) { // cert is within range of this thread
      PRInt32 certIndex = cIndex + index - idx;
      nsCOMPtr<nsISupports> isupport = 
                             dont_AddRef(mCertArray->ElementAt(certIndex));
      nsCOMPtr<nsIX509Cert> cert = do_QueryInterface(isupport);
      rawPtr = cert;
      NS_IF_ADDREF(rawPtr);
      break;
    }
    if (mTreeArray[i].open)
      idx += mTreeArray[i].numChildren;
    cIndex += mTreeArray[i].numChildren;
    if (idx > index) break;
  }
  return rawPtr;
}

nsCertTree::nsCertCompareFunc
nsCertTree::GetCompareFuncFromCertType(PRUint32 aType)
{
  switch (aType) {
    case nsIX509Cert::USER_CERT:
      return CmpUserCert;
    case nsIX509Cert::CA_CERT:
      return CmpCACert;
    case nsIX509Cert::EMAIL_CERT:
      return CmpEmailCert;
    case nsIX509Cert::SERVER_CERT:
    default:
      return CmpWebSiteCert;
  }
}

PRBool
nsCertTree::GetCertsByTypeFromCertList(CERTCertList *aCertList,
                                       PRUint32 aType,
                                       nsCertCompareFunc  aCertCmpFn,
                                       void *aCertCmpFnArg,
                                       nsISupportsArray **_certs)
{
  PR_LOG(gPIPNSSLog, PR_LOG_DEBUG, ("GetCertsByTypeFromCertList"));
  if (!aCertList)
    return PR_FALSE;
  nsCOMPtr<nsISupportsArray> certarray;
  nsresult rv = NS_NewISupportsArray(getter_AddRefs(certarray));
  if (NS_FAILED(rv)) return PR_FALSE;
  CERTCertListNode *node;
  int i, count = 0;
  for (node = CERT_LIST_HEAD(aCertList);
       !CERT_LIST_END(node, aCertList);
       node = CERT_LIST_NEXT(node)) {
    if (getCertType(node->cert) == aType) {
      nsCOMPtr<nsIX509Cert> pipCert = new nsNSSCertificate(node->cert);
      if (pipCert) {
        for (i=0; i<count; i++) {
          nsCOMPtr<nsISupports> isupport = 
            dont_AddRef(certarray->ElementAt(i));
          nsCOMPtr<nsIX509Cert> cert = do_QueryInterface(isupport);
          if ((*aCertCmpFn)(aCertCmpFnArg, pipCert, cert) < 0) {
            certarray->InsertElementAt(pipCert, i);
            break;
          }
        }
        if (i == count) certarray->AppendElement(pipCert);
          count++;
      }
    }
  }
  *_certs = certarray;
  NS_ADDREF(*_certs);
  return PR_TRUE;
}

PRBool 
nsCertTree::GetCertsByType(PRUint32           aType,
                           nsCertCompareFunc  aCertCmpFn,
                           void              *aCertCmpFnArg,
                           nsISupportsArray **_certs)
{
  nsNSSShutDownPreventionLock locker;
  CERTCertList *certList = NULL;
  nsCOMPtr<nsIInterfaceRequestor> cxt = new PipUIContext();
  certList = PK11_ListCerts(PK11CertListUnique, cxt);
  PRBool rv = GetCertsByTypeFromCertList(certList, aType, aCertCmpFn, aCertCmpFnArg, _certs);
  if (certList)
    CERT_DestroyCertList(certList);
  return rv;
}

PRBool 
nsCertTree::GetCertsByTypeFromCache(nsINSSCertCache   *aCache,
                                    PRUint32           aType,
                                    nsCertCompareFunc  aCertCmpFn,
                                    void              *aCertCmpFnArg,
                                    nsISupportsArray **_certs)
{
  NS_ENSURE_ARG_POINTER(aCache);
  CERTCertList *certList = NS_REINTERPRET_CAST(CERTCertList*, aCache->GetCachedCerts());
  if (!certList)
    return NS_ERROR_FAILURE;
  return GetCertsByTypeFromCertList(certList, aType, aCertCmpFn, aCertCmpFnArg, _certs);
}

// LoadCerts
//
// Load all of the certificates in the DB for this type.  Sort them
// by token, organization, then common name.
NS_IMETHODIMP 
nsCertTree::LoadCertsFromCache(nsINSSCertCache *aCache, PRUint32 aType)
{
  if (mTreeArray) {
    FreeCertArray();
    delete [] mTreeArray;
    mTreeArray = nsnull;
    mNumRows = 0;
  }
  nsresult rv = InitCompareHash();
  if (NS_FAILED(rv)) return rv;

  rv = GetCertsByTypeFromCache(aCache, aType, 
                               GetCompareFuncFromCertType(aType), &mCompareCache,
                               getter_AddRefs(mCertArray));
  if (NS_FAILED(rv)) return rv;
  return UpdateUIContents();
}

NS_IMETHODIMP 
nsCertTree::LoadCerts(PRUint32 aType)
{
  if (mTreeArray) {
    FreeCertArray();
    delete [] mTreeArray;
    mTreeArray = nsnull;
    mNumRows = 0;
  }
  nsresult rv = InitCompareHash();
  if (NS_FAILED(rv)) return rv;

  rv = GetCertsByType(aType, 
                      GetCompareFuncFromCertType(aType), &mCompareCache,
                      getter_AddRefs(mCertArray));
  if (NS_FAILED(rv)) return rv;
  return UpdateUIContents();
}

nsresult
nsCertTree::UpdateUIContents()
{
  PRUint32 count;
  nsresult rv = mCertArray->Count(&count);
  if (NS_FAILED(rv)) return rv;
  mNumOrgs = CountOrganizations();
  mTreeArray = new treeArrayEl[mNumOrgs];
  if (!mTreeArray)
    return NS_ERROR_OUT_OF_MEMORY;

  PRUint32 j = 0;
  nsCOMPtr<nsISupports> isupport = dont_AddRef(mCertArray->ElementAt(j));
  nsCOMPtr<nsIX509Cert> orgCert = do_QueryInterface(isupport);
  for (PRInt32 i=0; i<mNumOrgs; i++) {
    orgCert->GetIssuerOrganization(mTreeArray[i].orgName);
    mTreeArray[i].open = PR_TRUE;
    mTreeArray[i].certIndex = j;
    mTreeArray[i].numChildren = 1;
    if (++j >= count) break;
    isupport = dont_AddRef(mCertArray->ElementAt(j));
    nsCOMPtr<nsIX509Cert> nextCert = do_QueryInterface(isupport);
    while (0 == CmpBy(&mCompareCache, orgCert, nextCert, sort_IssuerOrg, sort_None, sort_None)) {
      mTreeArray[i].numChildren++;
      if (++j >= count) break;
      isupport = dont_AddRef(mCertArray->ElementAt(j));
      nextCert = do_QueryInterface(isupport);
    }
    orgCert = nextCert;
  }
  if (mTree) {
    mTree->BeginUpdateBatch();
    mTree->RowCountChanged(0, -mNumRows);
  }
  mNumRows = count + mNumOrgs;
  if (mTree)
    mTree->EndUpdateBatch();
  return NS_OK;
}

NS_IMETHODIMP 
nsCertTree::RemoveCert(PRUint32 index)
{
  if (!mCertArray || !mTreeArray || index < 0) {
    return NS_ERROR_FAILURE;
  }

  int i;
  PRUint32 idx = 0, cIndex = 0, nc;
  // Loop over the threads
  for (i=0; i<mNumOrgs; i++) {
    if (index == idx)
      return NS_OK; // index is for thread
    idx++; // get past the thread
    nc = (mTreeArray[i].open) ? mTreeArray[i].numChildren : 0;
    if (index < idx + nc) { // cert is within range of this thread
      PRInt32 certIndex = cIndex + index - idx;
      nsCOMPtr<nsISupports> isupport = dont_AddRef(mCertArray->ElementAt(certIndex));
      RemoveCacheEntry(isupport);
      mCertArray->RemoveElementAt(certIndex);
      delete [] mTreeArray;
      mTreeArray = nsnull;
      return UpdateUIContents();
    }
    if (mTreeArray[i].open)
      idx += mTreeArray[i].numChildren;
    cIndex += mTreeArray[i].numChildren;
    if (idx > index)
      break;
  }
  return NS_ERROR_FAILURE;
}

//////////////////////////////////////////////////////////////////////////////
//
//  Begin nsITreeView methods
//
/////////////////////////////////////////////////////////////////////////////

/* nsIX509Cert getCert(in unsigned long index); */
NS_IMETHODIMP
nsCertTree::GetCert(PRUint32 aIndex, nsIX509Cert **_cert)
{
  NS_ENSURE_ARG(_cert);
  *_cert = GetCertAtIndex(aIndex);
  //nsCOMPtr<nsIX509Cert> cert = GetCertAtIndex(aIndex);
  //if (cert) {
    //*_cert = cert;
    //NS_ADDREF(*_cert);
  //}
  return NS_OK;
}

/* readonly attribute long rowCount; */
NS_IMETHODIMP 
nsCertTree::GetRowCount(PRInt32 *aRowCount)
{
  if (!mTreeArray)
    return NS_ERROR_NOT_INITIALIZED;
  PRUint32 count = 0;
  for (PRInt32 i=0; i<mNumOrgs; i++) {
    if (mTreeArray[i].open) {
      count += mTreeArray[i].numChildren;
    }
    count++;
  }
  *aRowCount = count;
  return NS_OK;
}

/* attribute nsITreeSelection selection; */
NS_IMETHODIMP 
nsCertTree::GetSelection(nsITreeSelection * *aSelection)
{
  *aSelection = mSelection;
  NS_IF_ADDREF(*aSelection);
  return NS_OK;
}

NS_IMETHODIMP 
nsCertTree::SetSelection(nsITreeSelection * aSelection)
{
  mSelection = aSelection;
  return NS_OK;
}

/* void getRowProperties (in long index, in nsISupportsArray properties); */
NS_IMETHODIMP 
nsCertTree::GetRowProperties(PRInt32 index, nsISupportsArray *properties)
{
  return NS_OK;
}

/* void getCellProperties (in long row, in wstring colID, 
 *                           in nsISupportsArray properties); 
 */
NS_IMETHODIMP 
nsCertTree::GetCellProperties(PRInt32 row, const PRUnichar *colID, 
                                  nsISupportsArray *properties)
{
  return NS_OK;
}

/* void getColumnProperties (in wstring colID, 
 *                           in nsIDOMElement colElt, 
 *                           in nsISupportsArray properties); 
 */
NS_IMETHODIMP 
nsCertTree::GetColumnProperties(const PRUnichar *colID, 
                                    nsIDOMElement *colElt, 
                                    nsISupportsArray *properties)
{
  return NS_OK;
}

/* boolean isContainer (in long index); */
NS_IMETHODIMP 
nsCertTree::IsContainer(PRInt32 index, PRBool *_retval)
{
  if (!mTreeArray)
    return NS_ERROR_NOT_INITIALIZED;
  treeArrayEl *el = GetThreadDescAtIndex(index);
  if (el) {
    *_retval = PR_TRUE;
  } else {
    *_retval = PR_FALSE;
  }
  return NS_OK;
}

/* boolean isContainerOpen (in long index); */
NS_IMETHODIMP 
nsCertTree::IsContainerOpen(PRInt32 index, PRBool *_retval)
{
  if (!mTreeArray)
    return NS_ERROR_NOT_INITIALIZED;
  treeArrayEl *el = GetThreadDescAtIndex(index);
  if (el && el->open) {
    *_retval = PR_TRUE;
  } else {
    *_retval = PR_FALSE;
  }
  return NS_OK;
}

/* boolean isContainerEmpty (in long index); */
NS_IMETHODIMP 
nsCertTree::IsContainerEmpty(PRInt32 index, PRBool *_retval)
{
  *_retval = !mTreeArray;
  return NS_OK;
}

/* boolean isSeparator (in long index); */
NS_IMETHODIMP 
nsCertTree::IsSeparator(PRInt32 index, PRBool *_retval)
{
  *_retval = PR_FALSE;
  return NS_OK;
}

/* long getParentIndex (in long rowIndex); */
NS_IMETHODIMP 
nsCertTree::GetParentIndex(PRInt32 rowIndex, PRInt32 *_retval)
{
  if (!mTreeArray)
    return NS_ERROR_NOT_INITIALIZED;
  int i, idx = 0;
  for (i = 0; i < mNumOrgs && idx < rowIndex; i++, idx++) {
    if (mTreeArray[i].open) {
      if (rowIndex <= idx + mTreeArray[i].numChildren) {
        *_retval = idx;
        return NS_OK;
      }
      idx += mTreeArray[i].numChildren;
    }
  }
  *_retval = -1;
  return NS_OK;
}

/* boolean hasNextSibling (in long rowIndex, in long afterIndex); */
NS_IMETHODIMP 
nsCertTree::HasNextSibling(PRInt32 rowIndex, PRInt32 afterIndex, 
                               PRBool *_retval)
{
  if (!mTreeArray)
    return NS_ERROR_NOT_INITIALIZED;

  int i, idx = 0;
  for (i = 0; i < mNumOrgs && idx <= rowIndex; i++, idx++) {
    if (mTreeArray[i].open) {
      idx += mTreeArray[i].numChildren;
      if (afterIndex <= idx) {
        *_retval = afterIndex < idx;
        return NS_OK;
      }
    }
  }
  *_retval = PR_FALSE;
  return NS_OK;
}

/* long getLevel (in long index); */
NS_IMETHODIMP 
nsCertTree::GetLevel(PRInt32 index, PRInt32 *_retval)
{
  if (!mTreeArray)
    return NS_ERROR_NOT_INITIALIZED;
  treeArrayEl *el = GetThreadDescAtIndex(index);
  if (el) {
    *_retval = 0;
  } else {
    *_retval = 1;
  }
  return NS_OK;
}

/* Astring getImageSrc (in long row, in wstring colID); */
NS_IMETHODIMP 
nsCertTree::GetImageSrc(PRInt32 row, const PRUnichar *colID, 
                            nsAString& _retval)
{
  _retval.Truncate();
  return NS_OK;
}

/* long getProgressMode (in long row, in wstring colID); */
NS_IMETHODIMP
nsCertTree::GetProgressMode(PRInt32 row, const PRUnichar *colID, PRInt32* _retval)
{
  return NS_OK;
}

/* Astring getCellValue (in long row, in wstring colID); */
NS_IMETHODIMP 
nsCertTree::GetCellValue(PRInt32 row, const PRUnichar *colID, 
                             nsAString& _retval)
{
  _retval.Truncate();
  return NS_OK;
}

/* Astring getCellText (in long row, in wstring colID); */
NS_IMETHODIMP 
nsCertTree::GetCellText(PRInt32 row, const PRUnichar *colID, 
                        nsAString& _retval)
{
  if (!mTreeArray)
    return NS_ERROR_NOT_INITIALIZED;

  nsresult rv;
  _retval.Truncate();
  NS_ConvertUCS2toUTF8 aUtf8ColID(colID);
  const char *col = aUtf8ColID.get();
  treeArrayEl *el = GetThreadDescAtIndex(row);
  if (el != nsnull) {
    if (strcmp(col, "certcol") == 0)
      _retval.Assign(el->orgName);
    else
      _retval.SetCapacity(0);
    return NS_OK;
  }
  nsCOMPtr<nsIX509Cert> cert = dont_AddRef(GetCertAtIndex(row));
  if (cert == nsnull) return NS_ERROR_FAILURE;
  if (strcmp(col, "certcol") == 0) {
    rv = cert->GetCommonName(_retval);
    if (NS_FAILED(rv) || _retval.IsEmpty()) {
      // kaie: I didn't invent the idea to cut off anything before 
      //       the first colon. :-)
      nsAutoString nick;
      rv = cert->GetNickname(nick);
      
      nsAString::const_iterator start, end, end2;
      nick.BeginReading(start);
      nick.EndReading(end);
      end2 = end;

      if (FindInReadable(NS_LITERAL_STRING(":"), start, end)) {
        // found. end points to the first char after the colon,
        // that's what we want.
        _retval = Substring(end, end2);
      }
      else {
        _retval = nick;
      }
    }
  } else if (strcmp(col, "tokencol") == 0) {
    rv = cert->GetTokenName(_retval);
  } else if (strcmp(col, "emailcol") == 0) {
    rv = cert->GetEmailAddress(_retval);
  } else if (strcmp(col, "purposecol") == 0 && mNSSComponent) {
    PRUint32 verified;

    nsAutoString theUsages;
    rv = cert->GetUsagesString(PR_TRUE, &verified, theUsages); // ignore OCSP
    if (NS_FAILED(rv)) {
      verified = nsIX509Cert::NOT_VERIFIED_UNKNOWN;
    }

    switch (verified) {
      case nsIX509Cert::VERIFIED_OK:
        _retval = theUsages;
        break;

      case nsIX509Cert::CERT_REVOKED:
        rv = mNSSComponent->GetPIPNSSBundleString("VerifyRevoked", _retval);
        break;
      case nsIX509Cert::CERT_EXPIRED:
        rv = mNSSComponent->GetPIPNSSBundleString("VerifyExpired", _retval);
        break;
      case nsIX509Cert::CERT_NOT_TRUSTED:
        rv = mNSSComponent->GetPIPNSSBundleString("VerifyNotTrusted", _retval);
        break;
      case nsIX509Cert::ISSUER_NOT_TRUSTED:
        rv = mNSSComponent->GetPIPNSSBundleString("VerifyIssuerNotTrusted", _retval);
        break;
      case nsIX509Cert::ISSUER_UNKNOWN:
        rv = mNSSComponent->GetPIPNSSBundleString("VerifyIssuerUnknown", _retval);
        break;
      case nsIX509Cert::INVALID_CA:
        rv = mNSSComponent->GetPIPNSSBundleString("VerifyInvalidCA", _retval);
        break;
      case nsIX509Cert::NOT_VERIFIED_UNKNOWN:
      case nsIX509Cert::USAGE_NOT_ALLOWED:
      default:
        rv = mNSSComponent->GetPIPNSSBundleString("VerifyUnknown", _retval);
        break;
    }
  } else if (strcmp(col, "issuedcol") == 0) {
    nsCOMPtr<nsIX509CertValidity> validity;

    rv = cert->GetValidity(getter_AddRefs(validity));
    if (NS_SUCCEEDED(rv)) {
      validity->GetNotBeforeLocalDay(_retval);
    }
  } else if (strcmp(col, "expiredcol") == 0) {
    nsCOMPtr<nsIX509CertValidity> validity;

    rv = cert->GetValidity(getter_AddRefs(validity));
    if (NS_SUCCEEDED(rv)) {
      validity->GetNotAfterLocalDay(_retval);
    }
  } else if (strcmp(col, "serialnumcol") == 0) {
    rv = cert->GetSerialNumber(_retval);
  } else {
    return NS_ERROR_FAILURE;
  }
  return rv;
}

/* void setTree (in nsITreeBoxObject tree); */
NS_IMETHODIMP 
nsCertTree::SetTree(nsITreeBoxObject *tree)
{
  mTree = tree;
  return NS_OK;
}

/* void toggleOpenState (in long index); */
NS_IMETHODIMP 
nsCertTree::ToggleOpenState(PRInt32 index)
{
  if (!mTreeArray)
    return NS_ERROR_NOT_INITIALIZED;
  treeArrayEl *el = GetThreadDescAtIndex(index);
  if (el) el->open = !el->open;
  PRInt32 fac = (el->open) ? 1 : -1;
  if (mTree) mTree->RowCountChanged(index, fac * el->numChildren);
  mSelection->Select(index);
  return NS_OK;
}

/* void cycleHeader (in wstring colID, in nsIDOMElement elt); */
NS_IMETHODIMP 
nsCertTree::CycleHeader(const PRUnichar *colID, nsIDOMElement *elt)
{
  return NS_OK;
}

/* void selectionChanged (); */
NS_IMETHODIMP 
nsCertTree::SelectionChanged()
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* void cycleCell (in long row, in wstring colID); */
NS_IMETHODIMP 
nsCertTree::CycleCell(PRInt32 row, const PRUnichar *colID)
{
  return NS_OK;
}

/* boolean isEditable (in long row, in wstring colID); */
NS_IMETHODIMP 
nsCertTree::IsEditable(PRInt32 row, const PRUnichar *colID, PRBool *_retval)
{
  *_retval = PR_FALSE;
  return NS_OK;
}

/* void setCellText (in long row, in wstring colID, in wstring value); */
NS_IMETHODIMP 
nsCertTree::SetCellText(PRInt32 row, const PRUnichar *colID, 
                            const PRUnichar *value)
{
  return NS_OK;
}

/* void performAction (in wstring action); */
NS_IMETHODIMP 
nsCertTree::PerformAction(const PRUnichar *action)
{
  return NS_OK;
}

/* void performActionOnRow (in wstring action, in long row); */
NS_IMETHODIMP 
nsCertTree::PerformActionOnRow(const PRUnichar *action, PRInt32 row)
{
  return NS_OK;
}

/* void performActionOnCell (in wstring action, in long row, 
 *                           in wstring colID); 
 */
NS_IMETHODIMP 
nsCertTree::PerformActionOnCell(const PRUnichar *action, PRInt32 row, 
                                    const PRUnichar *colID)
{
  return NS_OK;
}

#ifdef DEBUG_CERT_TREE
void
nsCertTree::dumpMap()
{
  for (int i=0; i<mNumOrgs; i++) {
    nsAutoString org(mTreeArray[i].orgName);
    PR_LOG(gPIPNSSLog, PR_LOG_DEBUG, ("ORG[%s]", NS_LossyConvertUCS2toASCII(org).get()));
    PR_LOG(gPIPNSSLog, PR_LOG_DEBUG, ("OPEN[%d]", mTreeArray[i].open));
    PR_LOG(gPIPNSSLog, PR_LOG_DEBUG, ("INDEX[%d]", mTreeArray[i].certIndex));
    PR_LOG(gPIPNSSLog, PR_LOG_DEBUG, ("NCHILD[%d]", mTreeArray[i].numChildren));
  }
  for (int i=0; i<mNumRows; i++) {
    treeArrayEl *el = GetThreadDescAtIndex(i);
    if (el != nsnull) {
      nsAutoString td(el->orgName);
      PR_LOG(gPIPNSSLog, PR_LOG_DEBUG, ("thread desc[%d]: %s", i, NS_LossyConvertUCS2toASCII(td).get()));
    }
    nsCOMPtr<nsIX509Cert> ct = dont_AddRef(GetCertAtIndex(i));
    if (ct != nsnull) {
      PRUnichar *goo;
      ct->GetCommonName(&goo);
      nsAutoString doo(goo);
      PR_LOG(gPIPNSSLog, PR_LOG_DEBUG, ("cert [%d]: %s", i, NS_LossyConvertUCS2toASCII(doo).get()));
    }
  }
}
#endif

//
// CanDropOn
//
// Can't drop on the thread pane.
//
NS_IMETHODIMP nsCertTree::CanDropOn(PRInt32 index, PRBool *_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = PR_FALSE;
  
  return NS_OK;
}

//
// CanDropBeforeAfter
//
// Can't drop on the thread pane.
//
NS_IMETHODIMP nsCertTree::CanDropBeforeAfter(PRInt32 index, PRBool before, PRBool *_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = PR_FALSE;
  
  return NS_OK;
}


//
// Drop
//
// Can't drop on the thread pane.
//
NS_IMETHODIMP nsCertTree::Drop(PRInt32 row, PRInt32 orient)
{
  return NS_OK;
}


//
// IsSorted
//
// ...
//
NS_IMETHODIMP nsCertTree::IsSorted(PRBool *_retval)
{
  *_retval = PR_FALSE;
  return NS_OK;
}

void 
nsCertTree::CmpInitCriterion(nsIX509Cert *cert, CompareCacheHashEntry *entry,
                             sortCriterion crit, PRInt32 level)
{
  entry->mCritInit[level] = PR_TRUE;
  nsXPIDLString &str = entry->mCrit[level];
  
  switch (crit) {
    case sort_IssuerOrg:
      cert->GetIssuerOrganization(str);
      break;
    case sort_Org:
      cert->GetOrganization(str);
      break;
    case sort_Token:
      cert->GetTokenName(str);
      break;
    case sort_CommonName:
      cert->GetCommonName(str);
      break;
    case sort_IssuedDateDescending:
      {
        nsresult rv;
        nsCOMPtr<nsIX509CertValidity> validity;
        PRTime notBefore;

        rv = cert->GetValidity(getter_AddRefs(validity));
        if (NS_SUCCEEDED(rv)) {
          rv = validity->GetNotBefore(&notBefore);
        }

        if (NS_SUCCEEDED(rv)) {
          PRExplodedTime explodedTime;
          PR_ExplodeTime(notBefore, PR_GMTParameters, &explodedTime);
          char datebuf[20]; // 4 + 2 + 2 + 2 + 2 + 2 + 1 = 15
          if (0 != PR_FormatTime(datebuf, sizeof(datebuf), "%Y%m%d%H%M%S", &explodedTime)) {
            str = NS_ConvertASCIItoUCS2(nsDependentCString(datebuf));
          }
        }
      }
      break;
    case sort_Email:
      cert->GetEmailAddress(str);
      break;
    case sort_None:
    default:
      break;
  }
}

PRInt32
nsCertTree::CmpByCrit(nsIX509Cert *a, CompareCacheHashEntry *ace, 
                      nsIX509Cert *b, CompareCacheHashEntry *bce, 
                      sortCriterion crit, PRInt32 level)
{
  if (!ace->mCritInit[level]) {
    CmpInitCriterion(a, ace, crit, level);
  }

  if (!bce->mCritInit[level]) {
    CmpInitCriterion(b, bce, crit, level);
  }

  nsXPIDLString &str_a = ace->mCrit[level];
  nsXPIDLString &str_b = bce->mCrit[level];

  PRInt32 result;
  if (str_a && str_b)
    result = Compare(str_a, str_b);
  else
    result = !str_a ? (!str_b ? 0 : -1) : 1;

  if (sort_IssuedDateDescending == crit)
    result *= -1; // reverse compare order

  return result;
}

PRInt32
nsCertTree::CmpBy(void *cache, nsIX509Cert *a, nsIX509Cert *b, 
                  sortCriterion c0, sortCriterion c1, sortCriterion c2)
{
  if (a == b)
    return 0;

  CompareCacheHashEntry *ace = getCacheEntry(cache, a);
  CompareCacheHashEntry *bce = getCacheEntry(cache, b);

  PRInt32 cmp;
  cmp = CmpByCrit(a, ace, b, bce, c0, 0);
  if (cmp != 0)
    return cmp;

  if (c1 != sort_None) {
    cmp = CmpByCrit(a, ace, b, bce, c1, 1);
    if (cmp != 0)
      return cmp;
    
    if (c2 != sort_None) {
      return CmpByCrit(a, ace, b, bce, c2, 2);
    }
  }

  return cmp;
}

PRInt32
nsCertTree::CmpCACert(void *cache, nsIX509Cert *a, nsIX509Cert *b)
{
  // XXX we assume issuer org is always criterion 1
  return CmpBy(cache, a, b, sort_IssuerOrg, sort_Org, sort_Token);
}

PRInt32
nsCertTree::CmpWebSiteCert(void *cache, nsIX509Cert *a, nsIX509Cert *b)
{
  // XXX we assume issuer org is always criterion 1
  return CmpBy(cache, a, b, sort_IssuerOrg, sort_CommonName, sort_None);
}

PRInt32
nsCertTree::CmpUserCert(void *cache, nsIX509Cert *a, nsIX509Cert *b)
{
  // XXX we assume issuer org is always criterion 1
  return CmpBy(cache, a, b, sort_IssuerOrg, sort_Token, sort_IssuedDateDescending);
}

PRInt32
nsCertTree::CmpEmailCert(void *cache, nsIX509Cert *a, nsIX509Cert *b)
{
  // XXX we assume issuer org is always criterion 1
  return CmpBy(cache, a, b, sort_IssuerOrg, sort_Email, sort_CommonName);
}

