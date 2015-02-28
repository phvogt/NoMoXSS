/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
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
 * The Original Code is TransforMiiX XSLT processor.
 * 
 * The Initial Developer of the Original Code is Keith Visco.
 * Portions created by Keith Visco
 * (C) 1999-2000 Keith Visco. All Rights Reserved.
 * 
 * Contributor(s): 
 *   Keith Visco <kvisco@ziplink.net>
 *
 */

#ifndef TRANSFRMX_XML_EVENT_HANDLER_H
#define TRANSFRMX_XML_EVENT_HANDLER_H

#include "baseutils.h"
#include "txError.h"

class nsAString;
class txOutputFormat;

#ifdef TX_EXE
#include <iostream.h>
#else
#define kTXNameSpaceURI "http://www.mozilla.org/TransforMiix"
#define kTXWrapper "transformiix:result"

class nsIContent;
class nsIDOMDocument;
class nsIDOMHTMLScriptElement;
class nsITransformObserver;
#endif

/**
 * An interface for handling XML documents, loosely modeled
 * after Dave Megginson's SAX 1.0 API.
 */

class txAXMLEventHandler
{
public:
    virtual ~txAXMLEventHandler() {};

    /**
     * Signals to receive the start of an attribute.
     *
     * @param aName the name of the attribute
     * @param aNsID the namespace ID of the attribute
     * @param aValue the value of the attribute
     */
    virtual void attribute(const nsAString& aName,
                           const PRInt32 aNsID,
                           const nsAString& aValue) = 0;

    /**
     * Signals to receive characters.
     *
     * @param aData the characters to receive
     * @param aDOE disable output escaping for these characters
     */
    virtual void characters(const nsAString& aData, PRBool aDOE) = 0;

    /**
     * Signals to receive data that should be treated as a comment.
     *
     * @param data the comment data to receive
     */
    virtual void comment(const nsAString& aData) = 0;

    /**
     * Signals the end of a document. It is an error to call
     * this method more than once.
     */
    virtual void endDocument() = 0;

    /**
     * Signals to receive the end of an element.
     *
     * @param aName the name of the element
     * @param aNsID the namespace ID of the element
     */
    virtual void endElement(const nsAString& aName,
                            const PRInt32 aNsID) = 0;

    /**
     * Signals to receive a processing instruction.
     *
     * @param aTarget the target of the processing instruction
     * @param aData the data of the processing instruction
     */
    virtual void processingInstruction(const nsAString& aTarget, 
                                       const nsAString& aData) = 0;

    /**
     * Signals the start of a document.
     */
    virtual void startDocument() = 0;

    /**
     * Signals to receive the start of an element.
     *
     * @param aName the name of the element
     * @param aNsID the namespace ID of the element
     */
    virtual void startElement(const nsAString& aName,
                              const PRInt32 aNsID) = 0;
};

#define TX_DECL_TXAXMLEVENTHANDLER                                          \
    virtual void attribute(const nsAString& aName, const PRInt32 aNsID,     \
                           const nsAString& aValue);                        \
    virtual void characters(const nsAString& aData, PRBool aDOE);           \
    virtual void comment(const nsAString& aData);                           \
    virtual void endDocument();                                             \
    virtual void endElement(const nsAString& aName, const PRInt32 aNsID);   \
    virtual void processingInstruction(const nsAString& aTarget,            \
                                       const nsAString& aData);             \
    virtual void startDocument();                                           \
    virtual void startElement(const nsAString& aName, const PRInt32 aNsID);


#ifdef TX_EXE
typedef txAXMLEventHandler txAOutputXMLEventHandler;
#define TX_DECL_TXAOUTPUTXMLEVENTHANDLER
#else
class txAOutputXMLEventHandler : public txAXMLEventHandler
{
public:
    /**
     * Gets the Mozilla output document
     *
     * @param aDocument the Mozilla output document
     */
    virtual void getOutputDocument(nsIDOMDocument** aDocument) = 0;
};

#define TX_DECL_TXAOUTPUTXMLEVENTHANDLER                        \
    virtual void getOutputDocument(nsIDOMDocument** aDocument);
#endif

/**
 * Interface used to create the appropriate outputhandler
 */
class txAOutputHandlerFactory
{
public:
    virtual ~txAOutputHandlerFactory() {};

    /**
     * Creates an outputhandler for the specified format.
     * @param aFromat  format to get handler for
     * @param aHandler outparam. The created handler
     */
    virtual nsresult
    createHandlerWith(txOutputFormat* aFormat,
                      txAXMLEventHandler** aHandler) = 0;

    /**
     * Creates an outputhandler for the specified format, with the specified
     * name and namespace for the root element.
     * @param aFromat  format to get handler for
     * @param aName    name of the root element
     * @param aNsID    namespace-id of the root element
     * @param aHandler outparam. The created handler
     */
    virtual nsresult
    createHandlerWith(txOutputFormat* aFormat,
                      const nsAString& aName,
                      PRInt32 aNsID,
                      txAXMLEventHandler** aHandler) = 0;
};

#define TX_DECL_TXAOUTPUTHANDLERFACTORY                        \
    nsresult createHandlerWith(txOutputFormat* aFormat,        \
                               txAXMLEventHandler** aHandler); \
    nsresult createHandlerWith(txOutputFormat* aFormat,        \
                               const nsAString& aName,         \
                               PRInt32 aNsID,                  \
                               txAXMLEventHandler** aHandler);

#endif
