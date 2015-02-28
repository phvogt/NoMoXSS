/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
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
 * The Original Code is Mozilla Spellchecker Component.
 *
 * The Initial Developer of the Original Code is
 * David Einstein.
 * Portions created by the Initial Developer are Copyright (C) 2001
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s): David Einstein <Deinst@world.std.com>
 *                 Kevin Hendricks <kevin.hendricks@sympatico.ca>
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
 *  This spellchecker is based on the MySpell spellchecker made for Open Office
 *  by Kevin Hendricks.  Although the algorithms and code, have changed 
 *  slightly, the architecture is still the same. The Mozilla implementation
 *  is designed to be compatible with the Open Office dictionaries.
 *  Please do not make changes to the affix or dictionary file formats 
 *  without attempting to coordinate with Kevin.  For more information 
 *  on the original MySpell see 
 *  http://whiteboard.openoffice.org/source/browse/whiteboard/lingucomponent/source/spellcheck/myspell/
 *
 *  A special thanks and credit goes to Geoff Kuenning
 * the creator of ispell.  MySpell's affix algorithms were
 * based on those of ispell which should be noted is
 * copyright Geoff Kuenning et.al. and now available
 * under a BSD style license. For more information on ispell
 * and affix compression in general, please see:
 * http://www.cs.ucla.edu/ficus-members/geoff/ispell.html
 * (the home page for ispell)
 *
 * ***** END LICENSE BLOCK ***** */
#ifndef _AFFIXMGR_HXX_
#define _AFFIXMGR_HXX_
#include "nsString.h"
#include "mozIPersonalDictionary.h"
#include "mozCStr2CStrHashtable.h"
#include "mozAffixMod.h"
#include "nsNetUtil.h"
#include "nsIUnicodeEncoder.h"
#include "nsIUnicodeDecoder.h"

/* Modifications for mozilla Copyright 2001 David Einstein Deinst@world.std.com  */



// shamelesly stolen from nsDirectoryService.cpp
// Probably should move to nsDirectoryService.h?
#if defined(XP_MAC)
#define COMPONENT_DIRECTORY "Components"
#else
#define COMPONENT_DIRECTORY "components" 
#endif 

#define MAXAFFIXES      256

#define XPRODUCT        1

struct mozReplaceTable {
  nsString pattern;
  nsString replacement;
};

class myspPrefix;
class myspSuffix;

class myspAffixMgr
{
public:
 
  myspAffixMgr();
  ~myspAffixMgr();
  nsresult GetPersonalDictionary(mozIPersonalDictionary * *aPersonalDictionary);
  nsresult SetPersonalDictionary(mozIPersonalDictionary * aPersonalDictionary);
  mozReplaceTable *getReplaceTable();
  PRUint32 getReplaceTableLength();
  PRBool check(const nsAFlatString &word);
  void get_try_string(nsAString &aTryString);
  nsresult Load(const nsString &aDictionary);

protected:

  PRBool  prefixCheck(const nsAFlatCString &word);
  PRBool  suffixCheck(const nsAFlatCString &word,PRBool cross=PR_FALSE,char crossID=' ');

  nsresult LoadDictionary(nsIInputStream *strm);
  nsresult  parse_file(nsIInputStream *strm);

  nsresult DecodeString(const nsAFlatCString &aSource, nsAString &aDest);

  mozAffixState prefixes;
  mozAffixState suffixes;

  nsCString             trystring;
  nsCString             mEncoding;
  nsString              mLanguage;
  mozCStr2CStrHashtable mHashTable;
  mozReplaceTable      *mReplaceTable;
  PRUint32              mReplaceTableLength;
  nsCOMPtr<mozIPersonalDictionary> mPersonalDictionary;
  nsCOMPtr<nsIUnicodeEncoder>      mEncoder; 
  nsCOMPtr<nsIUnicodeDecoder>      mDecoder; 
};

#endif
