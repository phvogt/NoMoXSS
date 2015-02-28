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
 * The Original Code is Mozilla Communicator client code.
 *
 * The Initial Developer of the Original Code is 
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Robert Churchill <rjc@netscape.com>
 *   David Hyatt <hyatt@netscape.com>
 *   Chris Waterson <waterson@netscape.com>
 *   Pierre Phaneuf <pp@ludusdesign.com>
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

#ifndef nsXULTemplateBuilder_h__
#define nsXULTemplateBuilder_h__

#include "nsStubDocumentObserver.h"
#include "nsIScriptSecurityManager.h"
#include "nsISecurityCheckedComponent.h"
#include "nsIRDFCompositeDataSource.h"
#include "nsIRDFContainer.h"
#include "nsIRDFContainerUtils.h"
#include "nsIRDFDataSource.h"
#include "nsIRDFObserver.h"
#include "nsIRDFService.h"
#include "nsIXULTemplateBuilder.h"

#include "nsConflictSet.h"
#include "nsFixedSizeAllocator.h"
#include "nsResourceSet.h"
#include "nsRuleNetwork.h"
#include "nsVoidArray.h"
#include "nsCOMArray.h"

#include "prlog.h"
#ifdef PR_LOGGING
extern PRLogModuleInfo* gXULTemplateLog;
#endif

class nsClusterKeySet;
class nsTemplateMatch;
class nsTemplateRule;
class nsIXULDocument;
class nsIRDFCompositeDataSource;

/**
 * An object that translates an RDF graph into a presentation using a
 * set of rules.
 */
class nsXULTemplateBuilder : public nsIXULTemplateBuilder,
                             public nsISecurityCheckedComponent,
                             public nsStubDocumentObserver,
                             public nsIRDFObserver
{
public:
    nsXULTemplateBuilder();
    virtual ~nsXULTemplateBuilder();

    nsresult Init();

    // nsISupports interface
    NS_DECL_ISUPPORTS

    // nsIXULTemplateBuilder interface
    NS_DECL_NSIXULTEMPLATEBUILDER
   
    // nsISecurityCheckedComponent
    NS_DECL_NSISECURITYCHECKEDCOMPONENT

    // nsIDocumentObserver
    virtual void AttributeChanged(nsIDocument *aDocument, nsIContent* aContent,
                                  PRInt32 aNameSpaceID, nsIAtom* aAttribute,
                                  PRInt32 aModType);
    virtual void DocumentWillBeDestroyed(nsIDocument *aDocument);

    // nsIRDFObserver interface
    NS_DECL_NSIRDFOBSERVER

    nsresult
    ComputeContainmentProperties();

    static PRBool
    IsTemplateElement(nsIContent* aContent);

    /**
     * Initialize the rule network.
     */
    virtual nsresult
    InitializeRuleNetwork();

    /**
     * Initialize the rule network for handling rules that use the
     * ``simple'' syntax.
     */
    virtual nsresult
    InitializeRuleNetworkForSimpleRules(InnerNode** aChildNode) = 0;

    virtual nsresult
    RebuildAll() = 0; // must be implemented by subclasses

    /**
     * Find the <template> tag that applies for this builder
     */
    nsresult
    GetTemplateRoot(nsIContent** aResult);

    /**
     * Compile the template's rules
     */
    nsresult
    CompileRules();

    /**
     * Compile a rule that's specified using the extended template
     * syntax.
     */
    nsresult
    CompileExtendedRule(nsIContent* aRuleElement,
                        PRInt32 aPriority,
                        InnerNode* aParentNode);

    /**
     * Compile the <conditions> of a rule that uses the extended
     * template syntax.
     */
    nsresult
    CompileConditions(nsTemplateRule* aRule,
                      nsIContent* aConditions,
                      InnerNode* aParentNode,
                      InnerNode** aLastNode);

    /**
     * Compile a single condition from an extended template syntax
     * rule. Subclasses may override to provide additional,
     * subclass-specific condition processing.
     */
    virtual nsresult
    CompileCondition(nsIAtom* aTag,
                     nsTemplateRule* aRule,
                     nsIContent* aConditions,
                     InnerNode* aParentNode,
                     TestNode** aResult);

    /**
     * Compile a <triple> condition
     */
    nsresult
    CompileTripleCondition(nsTemplateRule* aRule,
                           nsIContent* aCondition,
                           InnerNode* aParentNode,
                           TestNode** aResult);

    /**
     * Compile a <member> condition
     */
    nsresult
    CompileMemberCondition(nsTemplateRule* aRule,
                           nsIContent* aCondition,
                           InnerNode* aParentNode,
                           TestNode** aResult);


    /**
     * Compile the <bindings> for an extended template syntax rule.
     */
    nsresult
    CompileBindings(nsTemplateRule* aRule, nsIContent* aBindings);

    /**
     * Compile a single binding for an extended template syntax rule.
     */
    nsresult
    CompileBinding(nsTemplateRule* aRule, nsIContent* aBinding);

    /**
     * Compile a rule that's specified using the simple template
     * syntax.
     */
    nsresult
    CompileSimpleRule(nsIContent* aRuleElement, PRInt32 aPriorty, InnerNode* aParentNode);

    /**
     * Can be overridden by subclasses to handle special attribute conditions
     * for the simple syntax.
     * @return PR_TRUE if the condition was handled
     */
    virtual PRBool
    CompileSimpleAttributeCondition(PRInt32 aNameSpaceID,
                                    nsIAtom* aAttribute,
                                    const nsAString& aValue,
                                    InnerNode* aParentNode,
                                    TestNode** aResult);
    /**
     * Add automatic bindings for simple rules
     */
    nsresult
    AddSimpleRuleBindings(nsTemplateRule* aRule, nsIContent* aElement);

    static void
    AddBindingsFor(nsXULTemplateBuilder* aSelf,
                   const nsAString& aVariable,
                   void* aClosure);

    // XXX sigh, the string template foo doesn't mix with
    // operator->*() on egcs-1.1.2, so we'll need to explicitly pass
    // "this" and use good ol' fashioned static callbacks.
    void
    ParseAttribute(const nsAString& aAttributeValue,
                   void (*aVariableCallback)(nsXULTemplateBuilder* aThis, const nsAString&, void*),
                   void (*aTextCallback)(nsXULTemplateBuilder* aThis, const nsAString&, void*),
                   void* aClosure);

    nsresult
    LoadDataSources(nsIDocument* aDoc);

    nsresult
    InitHTMLTemplateRoot();

    nsresult
    SubstituteText(nsTemplateMatch& aMatch,
                   const nsAString& aAttributeValue,
                   nsAString& aResult);

    static void
    SubstituteTextAppendText(nsXULTemplateBuilder* aThis, const nsAString& aText, void* aClosure);

    static void
    SubstituteTextReplaceVariable(nsXULTemplateBuilder* aThis, const nsAString& aVariable, void* aClosure);    

    PRBool
    IsAttrImpactedByVars(nsTemplateMatch& aMatch,
                         const nsAString& aAttributeValue,
                         const VariableSet& aModifiedVars);

    static void
    IsVarInSet(nsXULTemplateBuilder* aThis, const nsAString& aVariable, void* aClosure);

    nsresult
    SynchronizeAll(nsIRDFResource* aSource,
                   nsIRDFResource* aProperty,
                   nsIRDFNode* aOldTarget,
                   nsIRDFNode* aNewTarget);

    nsresult
    Propagate(nsIRDFResource* aSource,
              nsIRDFResource* aProperty,
              nsIRDFNode* aTarget,
              nsClusterKeySet& aNewKeys);

    nsresult
    FireNewlyMatchedRules(const nsClusterKeySet& aNewKeys);

    nsresult
    Retract(nsIRDFResource* aSource,
            nsIRDFResource* aProperty,
            nsIRDFNode* aTarget);

    nsresult
    CheckContainer(nsIRDFResource* aTargetResource, PRBool* aIsContainer, PRBool* aIsEmpty);

    nsresult 
    IsSystemPrincipal(nsIPrincipal *principal, PRBool *result);

#ifdef PR_LOGGING
    nsresult
    Log(const char* aOperation,
        nsIRDFResource* aSource,
        nsIRDFResource* aProperty,
        nsIRDFNode* aTarget);

#define LOG(_op, _src, _prop, _targ) \
    Log(_op, _src, _prop, _targ)

#else
#define LOG(_op, _src, _prop, _targ)
#endif

protected:
    // We are an observer of the composite datasource. The cycle is
    // broken when the document is destroyed.
    nsCOMPtr<nsIRDFDataSource> mDB;
    nsCOMPtr<nsIRDFCompositeDataSource> mCompDB;

    // Circular reference, broken when the document is destroyed.
    nsCOMPtr<nsIContent> mRoot;

    nsCOMPtr<nsIRDFDataSource> mCache;

    nsCOMArray<nsIXULBuilderListener> mListeners;

    PRInt32     mUpdateBatchNest;

    // For the rule network
    nsResourceSet mContainmentProperties;
    PRBool        mRulesCompiled;

public:
    nsRuleNetwork    mRules;
    PRInt32          mContainerVar;
    nsString         mContainerSymbol;
    PRInt32          mMemberVar;
    nsString         mMemberSymbol;
    nsConflictSet    mConflictSet;
    ReteNodeSet      mRDFTests;

protected:
    // pseudo-constants
    static nsrefcnt gRefCnt;
    static nsIRDFService*            gRDFService;
    static nsIRDFContainerUtils*     gRDFContainerUtils;
    static nsIScriptSecurityManager* gScriptSecurityManager;
    static nsIPrincipal*             gSystemPrincipal;

    enum {
        eDontTestEmpty = (1 << 0)
    };

    PRInt32 mFlags;

    /**
     * Stack-based helper class to maintain a list of ``activated''
     * resources; i.e., resources for which we are currently building
     * content.
     */
    class ActivationEntry {
    public:
        nsIRDFResource   *mResource;
        ActivationEntry  *mPrevious;
        ActivationEntry **mLink;

        ActivationEntry(nsIRDFResource *aResource, ActivationEntry **aLink)
            : mResource(aResource),
              mPrevious(*aLink),
              mLink(aLink) { *mLink = this; }

        ~ActivationEntry() { *mLink = mPrevious; }
    };

    /**
     * The top of the stack of resources that we're currently building
     * content for.
     */
    ActivationEntry *mTop;

    /**
     * Determine if a resource is currently on the activation stack.
     */
    PRBool
    IsActivated(nsIRDFResource *aResource);

    /**
     * Must be implemented by subclasses. Handle replacing aOldMatch
     * with aNewMatch. Either aOldMatch or aNewMatch may be null.
     */
    virtual nsresult
    ReplaceMatch(nsIRDFResource* aMember, const nsTemplateMatch* aOldMatch, nsTemplateMatch* aNewMatch) = 0;

    /**
     * Must be implemented by subclasses. Handle change in bound
     * variable values for aMatch. aModifiedVars contains the set
     * of variables that have changed.
     * @param aMatch the match for which variable bindings has changed.
     * @param aModifiedVars the set of variables for which the bindings
     * have changed.
     */
    virtual nsresult
    SynchronizeMatch(nsTemplateMatch* aMatch, const VariableSet& aModifiedVars) = 0;
};

#endif // nsXULTemplateBuilder_h__
