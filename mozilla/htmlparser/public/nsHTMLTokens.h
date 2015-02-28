
/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
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
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */

/**
 * MODULE NOTES:
 * @update  gess 4/1/98
 *  
 * This file contains the declarations for all the HTML specific token types that 
 * our DTD's understand. In fact, the same set of token types are used for XML.
 * Currently we have tokens for text, comments, start and end tags, entities, 
 * attributes, style, script and skipped content. Whitespace and newlines also
 * have their own token types, but don't count on them to stay forever.
 * 
 * If you're looking for the html tags, they're in a file called nsHTMLTag.h/cpp.
 *
 * Most of the token types have a similar API. They have methods to get the type
 * of token (GetTokenType); those that represent HTML tags also have a method to
 * get type tag type (GetTypeID). In addition, most have a method that causes the
 * token to help in the parsing process called (Consume). We've also thrown in a 
 * few standard debugging methods as well. 
 */

#ifndef HTMLTOKENS_H
#define HTMLTOKENS_H

#include "nsToken.h"
#include "nsHTMLTags.h"
#include "nsParserError.h"
#include "nsString.h"
#include "nsScannerString.h"

class nsScanner;

  /*******************************************************************
   * This enum defines the set of token types that we currently support.
   *******************************************************************/

enum eHTMLTokenTypes {
  eToken_unknown=0,
  eToken_start=1,      eToken_end,     eToken_comment,         eToken_entity,
  eToken_whitespace,   eToken_newline, eToken_text,            eToken_attribute,
  eToken_script,       eToken_style,   eToken_skippedcontent,  eToken_instruction,
  eToken_cdatasection, eToken_error,   eToken_doctypeDecl,     eToken_markupDecl,
  eToken_last //make sure this stays the last token...
};

enum eHTMLCategory {
  eHTMLCategory_unknown=0,
  eHTMLCategory_inline,
  eHTMLCategory_block,
  eHTMLCategory_blockAndInline,
  eHTMLCategory_list,
  eHTMLCategory_table,
  eHTMLCategory_tablepart,
  eHTMLCategory_tablerow,
  eHTMLCategory_tabledata,
  eHTMLCategory_head,
  eHTMLCategory_html,
  eHTMLCategory_body,
  eHTMLCategory_form,
  eHTMLCategory_options,
  eHTMLCategory_frameset,
  eHTMLCategory_text
};


nsresult      ConsumeQuotedString(PRUnichar aChar,nsString& aString,nsScanner& aScanner);
nsresult      ConsumeAttributeText(PRUnichar aChar,nsString& aString,nsScanner& aScanner);
const PRUnichar* GetTagName(PRInt32 aTag);
//PRInt32     FindEntityIndex(nsString& aString,PRInt32 aCount=-1);



/**
 *  This declares the basic token type used in the HTML DTD's.
 *  @update  gess 3/25/98
 */
class CHTMLToken : public CToken {
public:
    virtual             ~CHTMLToken();

                        CHTMLToken(eHTMLTags aTag);

    virtual eContainerInfo GetContainerInfo(void) const {return eFormUnknown;}
    virtual void           SetContainerInfo(eContainerInfo aInfo) { }

protected:
};

/**
 *  This declares start tokens, which always take the form <xxxx>. 
 *	This class also knows how to consume related attributes.
 *  
 *  @update  gess 3/25/98
 */
class CStartToken: public CHTMLToken {
  CTOKEN_IMPL_SIZEOF

  public:
                          CStartToken(eHTMLTags aTag=eHTMLTag_unknown);
                          CStartToken(const nsAString& aString);
                          CStartToken(const nsAString& aName,eHTMLTags aTag);

    virtual nsresult      Consume(PRUnichar aChar,nsScanner& aScanner,PRInt32 aMode);
    virtual PRInt32       GetTypeID(void);
    virtual const char*   GetClassName(void);
    virtual PRInt32       GetTokenType(void);

    virtual PRBool        IsEmpty(void);
    virtual void          SetEmpty(PRBool aValue);

    virtual const nsAString& GetStringValue();
    virtual void          GetSource(nsString& anOutputString);
    virtual void          AppendSourceTo(nsAString& anOutputString);

      //the following info is used to set well-formedness state on start tags...
    virtual eContainerInfo GetContainerInfo(void) const {return mContainerInfo;}
    virtual void           SetContainerInfo(eContainerInfo aContainerInfo) {mContainerInfo=aContainerInfo;}
    virtual PRBool         IsWellFormed(void) const {return PRBool(eWellFormed==mContainerInfo);}


    /*
     * Get and set the ID attribute atom for this element.  
     * See http://www.w3.org/TR/1998/REC-xml-19980210#sec-attribute-types
     * for the definition of an ID attribute.
     *
     */
    virtual nsresult      GetIDAttributeAtom(nsIAtom** aResult);
    virtual nsresult      SetIDAttributeAtom(nsIAtom* aID);
  
            nsString          mTextValue;
            nsString          mTrailingContent;
  protected:    
            eContainerInfo    mContainerInfo;
            nsCOMPtr<nsIAtom> mIDAttributeAtom;
            PRPackedBool      mEmpty;  
#ifdef DEBUG
            PRPackedBool      mAttributed;
#endif
};


/**
 *  This declares end tokens, which always take the 
 *  form </xxxx>. This class also knows how to consume
 *  related attributes.
 *  
 *  @update  gess 3/25/98
 */
class CEndToken: public CHTMLToken {
  CTOKEN_IMPL_SIZEOF

  public:
                        CEndToken(eHTMLTags aTag);
                        CEndToken(const nsAString& aString);
                        CEndToken(const nsAString& aName,eHTMLTags aTag);
    virtual nsresult    Consume(PRUnichar aChar,nsScanner& aScanner,PRInt32 aMode);
    virtual PRInt32     GetTypeID(void);
    virtual const char* GetClassName(void);
    virtual PRInt32     GetTokenType(void);

    virtual const nsAString& GetStringValue();
    virtual void        GetSource(nsString& anOutputString);
    virtual void        AppendSourceTo(nsAString& anOutputString);

  protected:
    nsString          mTextValue;
};


/**
 *  This declares comment tokens. Comments are usually 
 *  thought of as tokens, but we treat them that way 
 *  here so that the parser can have a consistent view
 *  of all tokens.
 *  
 *  @update  gess 3/25/98
 */
class CCommentToken: public CHTMLToken {
  CTOKEN_IMPL_SIZEOF

  public:
                        CCommentToken();
                        CCommentToken(const nsAString& aString);
    virtual nsresult    Consume(PRUnichar aChar,nsScanner& aScanner,PRInt32 aMode);
    virtual const char* GetClassName(void);
    virtual PRInt32     GetTokenType(void);
    virtual const nsAString& GetStringValue(void);
    virtual void        AppendSourceTo(nsAString& anOutputString);

    nsresult ConsumeStrictComment(nsScanner& aScanner);
    nsresult ConsumeQuirksComment(nsScanner& aScanner);

  protected:
    nsScannerSubstring mComment; // does not include MDO & MDC
    nsScannerSubstring mCommentDecl; // includes MDO & MDC
};


/**
 *  This class declares entity tokens, which always take
 *  the form &xxxx;. This class also offers a few utility
 *  methods that allow you to easily reduce entities.
 *  
 *  @update  gess 3/25/98
 */
class CEntityToken : public CHTMLToken {
  CTOKEN_IMPL_SIZEOF

  public:
                        CEntityToken();
                        CEntityToken(const nsAString& aString);
    virtual const char* GetClassName(void);
    virtual PRInt32     GetTokenType(void);
            PRInt32     TranslateToUnicodeStr(nsString& aString);
    virtual nsresult    Consume(PRUnichar aChar,nsScanner& aScanner,PRInt32 aMode);
    static  nsresult    ConsumeEntity(PRUnichar aChar,nsString& aString,nsScanner& aScanner);
    static  PRInt32     TranslateToUnicodeStr(PRInt32 aValue,nsString& aString);

    virtual const nsAString& GetStringValue(void);
    virtual void        GetSource(nsString& anOutputString);
    virtual void        AppendSourceTo(nsAString& anOutputString);

  protected:
    nsString          mTextValue;
};


/**
 *  Whitespace tokens are used where whitespace can be 
 *  detected as distinct from text. This allows us to 
 *  easily skip leading/trailing whitespace when desired.
 *  
 *  @update  gess 3/25/98
 */
class CWhitespaceToken: public CHTMLToken {
  CTOKEN_IMPL_SIZEOF

  public:
                        CWhitespaceToken();
                        CWhitespaceToken(const nsAString& aString);
    virtual nsresult    Consume(PRUnichar aChar,nsScanner& aScanner,PRInt32 aMode);
    virtual const char* GetClassName(void);
    virtual PRInt32     GetTokenType(void);
    virtual const nsAString& GetStringValue(void);

  protected:
    nsString          mTextValue;
};

/**
 *  Text tokens contain the normalized form of html text.
 *  These tokens are guaranteed not to contain entities,
 *  start or end tags, or newlines.
 *  
 *  @update  gess 3/25/98
 */
class CTextToken: public CHTMLToken {
  CTOKEN_IMPL_SIZEOF

  public:
                        CTextToken();
                        CTextToken(const nsAString& aString);
    virtual nsresult    Consume(PRUnichar aChar,nsScanner& aScanner,PRInt32 aMode);
            nsresult    ConsumeUntil(PRUnichar aChar,PRBool aIgnoreComments,nsScanner& aScanner,
                                     nsString& aEndTagName,PRInt32 aFlag,PRBool& aFlushTokens);
    virtual const char* GetClassName(void);
    virtual PRInt32     GetTokenType(void);
    virtual PRInt32     GetTextLength(void);
    virtual void        CopyTo(nsAString& aStr);
    virtual const nsAString& GetStringValue(void);
    virtual void        Bind(nsScanner* aScanner, nsScannerIterator& aStart, nsScannerIterator& aEnd);
    virtual void        Bind(const nsAString& aStr);

  protected:
    nsScannerSubstring          mTextValue;
};


/**
 *  CDATASection tokens contain raw unescaped text content delimited by
 *  a ![CDATA[ and ]]. 
 *  XXX Not really a HTML construct - maybe we need a separation
 *  
 *  @update  vidur 11/12/98
 */
class CCDATASectionToken : public CHTMLToken {
  CTOKEN_IMPL_SIZEOF

public:
                        CCDATASectionToken(eHTMLTags aTag = eHTMLTag_unknown);
                        CCDATASectionToken(const nsAString& aString);
    virtual nsresult    Consume(PRUnichar aChar,nsScanner& aScanner,PRInt32 aMode);
    virtual const char* GetClassName(void);
    virtual PRInt32     GetTokenType(void);  
    virtual const nsAString& GetStringValue(void);

  protected:
    nsString          mTextValue;
};


/**
 *  Declaration tokens contain raw unescaped text content (not really, but
 *  right now we use this only for view source).
 *  XXX Not really a HTML construct - maybe we need a separation
 *  
 */
class CMarkupDeclToken : public CHTMLToken {
  CTOKEN_IMPL_SIZEOF

public:
                        CMarkupDeclToken();
                        CMarkupDeclToken(const nsAString& aString);
    virtual nsresult    Consume(PRUnichar aChar,nsScanner& aScanner,PRInt32 aMode);
    virtual const char* GetClassName(void);
    virtual PRInt32     GetTokenType(void);  
    virtual const nsAString& GetStringValue(void);

protected:
    nsScannerSubstring  mTextValue;
};


/**
 *  Attribute tokens are used to contain attribute key/value
 *  pairs whereever they may occur. Typically, they should
 *  occur only in start tokens. However, we may expand that
 *  ability when XML tokens become commonplace.
 *  
 *  @update  gess 3/25/98
 */
class CAttributeToken: public CHTMLToken {
  CTOKEN_IMPL_SIZEOF

  public:
                          CAttributeToken();
                          CAttributeToken(const nsAString& aString);
                          CAttributeToken(const nsAString& aKey, const nsAString& aString);
                           ~CAttributeToken() {}
    virtual nsresult      Consume(PRUnichar aChar,nsScanner& aScanner,PRInt32 aMode);
    virtual const char*   GetClassName(void);
    virtual PRInt32       GetTokenType(void);
    virtual const nsAString&     GetKey(void); // XXX {return mTextKey;}
    virtual void          SetKey(const nsAString& aKey);
    virtual void          BindKey(nsScanner* aScanner, nsScannerIterator& aStart, nsScannerIterator& aEnd);
    virtual const nsAString&     GetValue(void) {return mTextValue;}
    virtual void          SanitizeKey();
    virtual const nsAString& GetStringValue(void);
    virtual void          GetSource(nsString& anOutputString);
    virtual void          AppendSourceTo(nsAString& anOutputString);
    
    PRPackedBool       mHasEqualWithoutValue;
  protected:
#ifdef DEBUG
    PRPackedBool       mLastAttribute;
#endif
    nsAutoString       mTextValue;
    nsScannerSubstring mTextKey;
}; 


/**
 *  Newline tokens contain, you guessed it, newlines. 
 *  They consume newline (CR/LF) either alone or in pairs.
 *  
 *  @update  gess 3/25/98
 */
class CNewlineToken: public CHTMLToken { 
  CTOKEN_IMPL_SIZEOF

  public:
                        CNewlineToken();
    virtual nsresult    Consume(PRUnichar aChar,nsScanner& aScanner,PRInt32 aMode);
    virtual const char* GetClassName(void);
    virtual PRInt32     GetTokenType(void);
    virtual const nsAString&   GetStringValue(void);

    static void AllocNewline();
    static void FreeNewline();
};


/**
 *  Script tokens contain sequences of javascript (or, gulp,
 *  any other script you care to send). We don't tokenize
 *  it here, nor validate it. We just wrap it up, and pass
 *  it along to the html parser, who sends it (later on) 
 *  to the scripting engine.
 *  
 *  @update  gess 3/25/98
 */
class CScriptToken: public CHTMLToken {
  CTOKEN_IMPL_SIZEOF

  public:
                        CScriptToken();
                        CScriptToken(const nsAString& aString);
    virtual const char* GetClassName(void);
    virtual PRInt32     GetTokenType(void);
    virtual const nsAString&   GetStringValue(void);

  protected:
    nsString          mTextValue;
};


/**
 *  Style tokens contain sequences of css style. We don't 
 *  tokenize it here, nor validate it. We just wrap it up, 
 *  and pass it along to the html parser, who sends it 
 *  (later on) to the style engine.
 *  
 *  @update  gess 3/25/98
 */
class CStyleToken: public CHTMLToken {
  CTOKEN_IMPL_SIZEOF

  public:
                         CStyleToken();
                         CStyleToken(const nsAString& aString);
    virtual const char*  GetClassName(void);
    virtual PRInt32      GetTokenType(void);
    virtual const nsAString&   GetStringValue(void);

  protected:
    nsString          mTextValue;
};


/**
 *  Whitespace tokens are used where whitespace can be 
 *  detected as distinct from text. This allows us to 
 *  easily skip leading/trailing whitespace when desired.
 *  
 *  @update  gess 3/25/98
 */
class CInstructionToken: public CHTMLToken {
  CTOKEN_IMPL_SIZEOF

  public:
                        CInstructionToken();
                        CInstructionToken(const nsAString& aString);
    virtual nsresult    Consume(PRUnichar aChar,nsScanner& aScanner,PRInt32 aMode);
    virtual const char* GetClassName(void);
    virtual PRInt32     GetTokenType(void);
    virtual const nsAString&   GetStringValue(void);

  protected:
    nsString          mTextValue;
};

class CErrorToken : public CHTMLToken {
  CTOKEN_IMPL_SIZEOF

public:
  CErrorToken(nsParserError* aError=0);
  ~CErrorToken();
  virtual const char* GetClassName(void);
  virtual PRInt32     GetTokenType(void);
  
  void SetError(nsParserError* aError);  // CErrorToken takes ownership of aError

  // The nsParserError object returned by GetError is still owned by CErrorToken.
  // DO NOT use the delete operator on it.  Should we change this so that a copy
  // of nsParserError is returned which needs to be destroyed by the consumer?
  const nsParserError* GetError(void);   

    virtual const nsAString&   GetStringValue(void);
protected:
  nsString          mTextValue;
  nsParserError* mError;
};

/** 
 * This token is generated by the HTML and Expat tokenizers
 * when they see the doctype declaration ("<!DOCTYPE ... >")
 *
 */

class CDoctypeDeclToken: public CHTMLToken {
  CTOKEN_IMPL_SIZEOF

public:
                        CDoctypeDeclToken(eHTMLTags aTag=eHTMLTag_unknown);
                        CDoctypeDeclToken(const nsAString& aString,eHTMLTags aTag=eHTMLTag_unknown);
    virtual nsresult    Consume(PRUnichar aChar,nsScanner& aScanner,PRInt32 aMode);
    virtual const char* GetClassName(void);
    virtual PRInt32     GetTokenType(void);
    virtual const nsAString& GetStringValue(void);
    virtual void SetStringValue(const nsAString& aStr);

  protected:
    nsString          mTextValue;
};

#endif
