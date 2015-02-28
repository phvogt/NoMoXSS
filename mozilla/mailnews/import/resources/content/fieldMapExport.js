/* -*- Mode: Java; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1999 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s):
 */

var importService = 0;
var fieldMap = null;

function OnLoadFieldMapExport()
{
  top.importService = Components.classes["@mozilla.org/import/import-service;1"].getService();
  top.importService = top.importService.QueryInterface(Components.interfaces.nsIImportService);

  // We need a field map object...
  // assume we have one passed in? or just make one?
  if (window.arguments && window.arguments[0])
    top.fieldMap = window.arguments[0].fieldMap;
  if (!top.fieldMap) {
    top.fieldMap = top.importService.CreateNewFieldMap();
    top.fieldMap.DefaultFieldMap( top.fieldMap.numMozFields);
  }

  doSetOKCancel( FieldExportOKButton, 0);

  ListFields();
}

function SetDivText(id, text)
{
  var div = document.getElementById(id);

  if ( div )
  {
    if ( div.childNodes.length == 0 )
    {
      var textNode = document.createTextNode(text);
      div.appendChild(textNode);
    }
    else if ( div.childNodes.length == 1 )
      div.childNodes[0].nodeValue = text;
  }
}


function FieldExportOKButton()
{
  // hmmm... can we re-build the map from the tree values?
  return true;
}


function FieldSelectionChanged()
{
  var tree = document.getElementById('fieldList');
  if ( tree && tree.selectedItems && (tree.selectedItems.length == 1) )
  {
  }
}

function ExportSelectionChanged()
{
  var tree = document.getElementById('exportList');
  if ( tree && tree.selectedItems && (tree.selectedItems.length == 1) )
  {
  }
}


function ListFields() {
  if (top.fieldMap == null)
    return;

  // we should fill in the field list with the data from the field map?
  var body = document.getElementById("fieldBody");
  var count = top.fieldMap.numMozFields;
  for (i = 0; i < count; i++) {
    AddFieldToList( body, top.fieldMap.GetFieldDescription( i), i);
  }

  body = document.getElementById("exportBody");
  count = top.fieldMap.mapSize;
  var index;
  for (i = 0; i < count; i++) {
    index = top.fieldMap.GetFieldMap( i);
    AddFieldToList( body, top.fieldMap.GetFieldDescription( index), index);
  }
}

function AddFieldToList(body, name, index)
{

  var item = document.createElement('treeitem');
  var row = document.createElement('treerow');
  var cell = document.createElement('treecell');
  cell.setAttribute('label', name);
  item.setAttribute('field-index', index);

  row.appendChild(cell);
  item.appendChild(row);
  body.appendChild(item);
}

function DeleteExportItems()
{
  var tree = document.getElementById('exportList');
  if ( tree && tree.selectedItems && (tree.selectedItems.length > 0) ) {
    var body = document.getElementById("exportBody");
    if (body) {
      while (tree.selectedItems.length) {
        body.removeChild( tree.selectedItems[0]);
      }
    }
  }
}

function OnAddExport()
{
  var tree = document.getElementById('fieldList');
  if ( tree && tree.selectedItems && (tree.selectedItems.length > 0) ) {
    var body = document.getElementById("exportBody");
    if (body) {
      var name;
      var fIndex;
      for (var index = 0; index < tree.selectedItems.length; index++) {
        name = tree.selectedItems[index].firstChild.firstChild.getAttribute( 'label');
        fIndex = tree.selectedItems[index].getAttribute( 'field-index');
        AddFieldToList( body, name, fIndex);
      }
    }
  }
}

function OnAddAllExport()
{
  var body = document.getElementById("exportBody");
  var count = top.fieldMap.numMozFields;
  for (var i = 0; i < count; i++) {
    AddFieldToList( body, top.fieldMap.GetFieldDescription( i), i);
  }
}


