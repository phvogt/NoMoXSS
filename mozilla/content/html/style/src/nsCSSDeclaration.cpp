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
 *   Daniel Glazman <glazman@netscape.com>
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
#include "nscore.h"
#include "nsCSSDeclaration.h"
#include "nsString.h"
#include "nsIAtom.h"
#include "nsUnicharUtils.h"
#include "nsCRT.h"
#include "nsCSSProps.h"
#include "nsUnitConversion.h"
#include "nsVoidArray.h"
#include "nsFont.h"
#include "nsReadableUtils.h"

#include "nsStyleConsts.h"

#include "nsCOMPtr.h"

#define B_BORDER_TOP_STYLE    0x001
#define B_BORDER_LEFT_STYLE   0x002
#define B_BORDER_RIGHT_STYLE  0x004
#define B_BORDER_BOTTOM_STYLE 0x008
#define B_BORDER_TOP_COLOR    0x010
#define B_BORDER_LEFT_COLOR   0x020
#define B_BORDER_RIGHT_COLOR  0x040
#define B_BORDER_BOTTOM_COLOR 0x080
#define B_BORDER_TOP_WIDTH    0x100
#define B_BORDER_LEFT_WIDTH   0x200
#define B_BORDER_RIGHT_WIDTH  0x400
#define B_BORDER_BOTTOM_WIDTH 0x800

#define B_BORDER_STYLE        0x00f
#define B_BORDER_COLOR        0x0f0
#define B_BORDER_WIDTH        0xf00

#define B_BORDER_TOP          0x111
#define B_BORDER_LEFT         0x222
#define B_BORDER_RIGHT        0x444
#define B_BORDER_BOTTOM       0x888

#define B_BORDER              0xfff

MOZ_DECL_CTOR_COUNTER(nsCSSDeclaration)

nsCSSDeclaration::nsCSSDeclaration() 
  : mOrder(eCSSProperty_COUNT_no_shorthands, 8),
    mData(nsnull),
    mImportantData(nsnull)
{
  MOZ_COUNT_CTOR(nsCSSDeclaration);
}

nsCSSDeclaration::nsCSSDeclaration(const nsCSSDeclaration& aCopy)
  : mOrder(eCSSProperty_COUNT_no_shorthands, aCopy.mOrder.Count()),
    mData(aCopy.mData ? aCopy.mData->Clone() : nsnull),
    mImportantData(aCopy.mImportantData ? aCopy.mImportantData->Clone()
                                         : nsnull)
{
  MOZ_COUNT_CTOR(nsCSSDeclaration);
  mOrder = aCopy.mOrder;
}

nsCSSDeclaration::~nsCSSDeclaration(void)
{
  if (mData) {
    mData->Destroy();
  }
  if (mImportantData) {
    mImportantData->Destroy();
  }

  MOZ_COUNT_DTOR(nsCSSDeclaration);
}

nsresult
nsCSSDeclaration::ValueAppended(nsCSSProperty aProperty)
{
  // order IS important for CSS, so remove and add to the end
  if (nsCSSProps::IsShorthand(aProperty)) {
    CSSPROPS_FOR_SHORTHAND_SUBPROPERTIES(p, aProperty) {
      mOrder.RemoveValue(*p);
      mOrder.AppendValue(*p);
    }
  } else {
    mOrder.RemoveValue(aProperty);
    mOrder.AppendValue(aProperty);
  }
  return NS_OK;
}

nsresult
nsCSSDeclaration::RemoveProperty(nsCSSProperty aProperty)
{
  nsCSSExpandedDataBlock data;
  data.Expand(&mData, &mImportantData);
  NS_ASSERTION(!mData && !mImportantData, "Expand didn't null things out");

  if (nsCSSProps::IsShorthand(aProperty)) {
    CSSPROPS_FOR_SHORTHAND_SUBPROPERTIES(p, aProperty) {
      data.ClearProperty(*p);
      mOrder.RemoveValue(*p);
    }
  } else {
    data.ClearProperty(aProperty);
    mOrder.RemoveValue(aProperty);
  }

  data.Compress(&mData, &mImportantData);
  return NS_OK;
}

nsresult
nsCSSDeclaration::AppendComment(const nsAString& aComment)
{
  return /* NS_ERROR_NOT_IMPLEMENTED, or not any longer that is */ NS_OK;
}

nsresult
nsCSSDeclaration::GetValueOrImportantValue(nsCSSProperty aProperty, nsCSSValue& aValue) const
{
  aValue.Reset();

  NS_ASSERTION(aProperty >= 0, "out of range");
  if (aProperty >= eCSSProperty_COUNT_no_shorthands ||
      nsCSSProps::kTypeTable[aProperty] != eCSSType_Value) {
    NS_ERROR("can't query for shorthand properties");
    return NS_ERROR_ILLEGAL_VALUE;
  }

  nsCSSCompressedDataBlock *data = GetValueIsImportant(aProperty)
                                     ? mImportantData : mData;
  const void *storage = data->StorageFor(aProperty);
  if (!storage)
    return NS_OK;
  aValue = *NS_STATIC_CAST(const nsCSSValue*, storage);
  return NS_OK;
}

nsresult
nsCSSDeclaration::GetValue(const nsAString& aProperty,
                           nsAString& aValue) const
{
  nsCSSProperty propID = nsCSSProps::LookupProperty(aProperty);
  return GetValue(propID, aValue);
}

PRBool nsCSSDeclaration::AppendValueToString(nsCSSProperty aProperty, nsAString& aResult) const
{
  nsCSSCompressedDataBlock *data = GetValueIsImportant(aProperty)
                                      ? mImportantData : mData;
  const void *storage = data->StorageFor(aProperty);
  if (storage) {
    switch (nsCSSProps::kTypeTable[aProperty]) {
      case eCSSType_Value: {
        const nsCSSValue *val = NS_STATIC_CAST(const nsCSSValue*, storage);
        AppendCSSValueToString(aProperty, *val, aResult);
      } break;
      case eCSSType_Rect: {
        const nsCSSRect *rect = NS_STATIC_CAST(const nsCSSRect*, storage);
        if (rect->mTop.GetUnit() == eCSSUnit_Inherit ||
            rect->mTop.GetUnit() == eCSSUnit_Initial) {
          NS_ASSERTION(rect->mRight.GetUnit() == rect->mTop.GetUnit(),
                       "Top inherit or initial, right isn't.  Fix the parser!");
          NS_ASSERTION(rect->mBottom.GetUnit() == rect->mTop.GetUnit(),
                       "Top inherit or initial, bottom isn't.  Fix the parser!");
          NS_ASSERTION(rect->mLeft.GetUnit() == rect->mTop.GetUnit(),
                       "Top inherit or initial, left isn't.  Fix the parser!");
          AppendCSSValueToString(aProperty, rect->mTop, aResult);
        } else {
          aResult.Append(NS_LITERAL_STRING("rect("));
          AppendCSSValueToString(aProperty, rect->mTop, aResult);
          NS_NAMED_LITERAL_STRING(comma, ", ");
          aResult.Append(comma);
          AppendCSSValueToString(aProperty, rect->mRight, aResult);
          aResult.Append(comma);
          AppendCSSValueToString(aProperty, rect->mBottom, aResult);
          aResult.Append(comma);
          AppendCSSValueToString(aProperty, rect->mLeft, aResult);
          aResult.Append(PRUnichar(')'));
        }
      } break;
      case eCSSType_ValueList: {
        const nsCSSValueList* val =
            *NS_STATIC_CAST(nsCSSValueList*const*, storage);
        do {
          AppendCSSValueToString(aProperty, val->mValue, aResult);
          val = val->mNext;
          if (val) {
            aResult.Append(PRUnichar(' '));
          }
        } while (val);
      } break;
      case eCSSType_CounterData: {
        const nsCSSCounterData* counter =
            *NS_STATIC_CAST(nsCSSCounterData*const*, storage);
        do {
          if (AppendCSSValueToString(aProperty, counter->mCounter, aResult)) {
            if (counter->mValue.GetUnit() != eCSSUnit_Null) {
              aResult.Append(PRUnichar(' '));
              AppendCSSValueToString(aProperty, counter->mValue, aResult);
            }
          }
          counter = counter->mNext;
          if (counter) {
            aResult.Append(PRUnichar(' '));
          }
        } while (counter);
      } break;
      case eCSSType_Quotes: {
        const nsCSSQuotes* quotes = 
            *NS_STATIC_CAST(nsCSSQuotes*const*, storage);
        do {
          AppendCSSValueToString(aProperty, quotes->mOpen, aResult);
          aResult.Append(PRUnichar(' '));
          AppendCSSValueToString(aProperty, quotes->mClose, aResult);
          quotes = quotes->mNext;
          if (quotes) {
            aResult.Append(PRUnichar(' '));
          }
        } while (quotes);
      } break;
      case eCSSType_Shadow: {
        const nsCSSShadow* shadow =
            *NS_STATIC_CAST(nsCSSShadow*const*, storage);
        if (shadow->mXOffset.IsLengthUnit()) {
          while (shadow) {
            if (AppendCSSValueToString(eCSSProperty_color, shadow->mColor,
                                       aResult))
              aResult.Append(PRUnichar(' '));
            if (AppendCSSValueToString(aProperty, shadow->mXOffset, aResult)) {
              aResult.Append(PRUnichar(' '));
              AppendCSSValueToString(aProperty, shadow->mYOffset, aResult);
              aResult.Append(PRUnichar(' '));
            }
            if (AppendCSSValueToString(aProperty, shadow->mRadius, aResult) &&
                shadow->mNext)
              aResult.Append(NS_LITERAL_STRING(", "));
            shadow = shadow->mNext;
          }
        }
        else {  // none or inherit
          AppendCSSValueToString(aProperty, shadow->mXOffset, aResult);
        }
      } break;
    }
  }
  return storage != nsnull;
}

PRBool nsCSSDeclaration::AppendCSSValueToString(nsCSSProperty aProperty, const nsCSSValue& aValue, nsAString& aResult) const
{
  nsCSSUnit unit = aValue.GetUnit();

  if (eCSSUnit_Null == unit) {
    return PR_FALSE;
  }

  if ((eCSSUnit_String <= unit) && (unit <= eCSSUnit_Counters)) {
    switch (unit) {
      case eCSSUnit_Attr:     aResult.Append(NS_LITERAL_STRING("attr("));
        break;
      case eCSSUnit_Counter:  aResult.Append(NS_LITERAL_STRING("counter("));
        break;
      case eCSSUnit_Counters: aResult.Append(NS_LITERAL_STRING("counters("));
        break;
      default:  break;
    }
    nsAutoString  buffer;
    aValue.GetStringValue(buffer);
    aResult.Append(buffer);
  }
  else if (eCSSUnit_Integer == unit) {
    switch (aProperty) {
      case eCSSProperty_color:
      case eCSSProperty_background_color:
      case eCSSProperty_border_top_color:
      case eCSSProperty_border_bottom_color:
      case eCSSProperty_border_left_color:
      case eCSSProperty_border_right_color:
      case eCSSProperty__moz_outline_color: {
        // we can lookup the property in the ColorTable and then
        // get a string mapping the name
        nsCAutoString str;
        if (nsCSSProps::GetColorName(aValue.GetIntValue(), str)){
          AppendASCIItoUTF16(str, aResult);
        } else {
          nsAutoString tmpStr;
          tmpStr.AppendInt(aValue.GetIntValue(), 10);
          aResult.Append(tmpStr);
        }
      }
      break;

      default:
        {
          nsAutoString tmpStr;
          tmpStr.AppendInt(aValue.GetIntValue(), 10);
          aResult.Append(tmpStr);
        }
      break;
    }
  }
  else if (eCSSUnit_Enumerated == unit) {
    if (eCSSProperty_text_decoration == aProperty) {
      PRInt32 intValue = aValue.GetIntValue();
      if (NS_STYLE_TEXT_DECORATION_NONE != intValue) {
        PRInt32 mask;
        for (mask = NS_STYLE_TEXT_DECORATION_UNDERLINE;
             mask <= NS_STYLE_TEXT_DECORATION_BLINK; 
             mask <<= 1) {
          if ((mask & intValue) == mask) {
            AppendASCIItoUTF16(nsCSSProps::LookupPropertyValue(aProperty, mask), aResult);
            intValue &= ~mask;
            if (0 != intValue) { // more left
              aResult.Append(PRUnichar(' '));
            }
          }
        }
      }
      else {
        AppendASCIItoUTF16(nsCSSProps::LookupPropertyValue(aProperty, NS_STYLE_TEXT_DECORATION_NONE), aResult);
      }
    }
    else if (eCSSProperty_azimuth == aProperty) {
      PRInt32 intValue = aValue.GetIntValue();
      AppendASCIItoUTF16(nsCSSProps::LookupPropertyValue(aProperty, (intValue & ~NS_STYLE_AZIMUTH_BEHIND)), aResult);
      if ((NS_STYLE_AZIMUTH_BEHIND & intValue) != 0) {
        aResult.Append(PRUnichar(' '));
        AppendASCIItoUTF16(nsCSSProps::LookupPropertyValue(aProperty, NS_STYLE_AZIMUTH_BEHIND), aResult);
      }
    }
    else if (eCSSProperty_play_during_flags == aProperty) {
      PRInt32 intValue = aValue.GetIntValue();
      if ((NS_STYLE_PLAY_DURING_MIX & intValue) != 0) {
        AppendASCIItoUTF16(nsCSSProps::LookupPropertyValue(aProperty, NS_STYLE_PLAY_DURING_MIX), aResult);
      }
      if ((NS_STYLE_PLAY_DURING_REPEAT & intValue) != 0) {
        if (NS_STYLE_PLAY_DURING_REPEAT != intValue) {
          aResult.Append(PRUnichar(' '));
        }
        AppendASCIItoUTF16(nsCSSProps::LookupPropertyValue(aProperty, NS_STYLE_PLAY_DURING_REPEAT), aResult);
      }
    }
    else if (eCSSProperty_marks == aProperty) {
      PRInt32 intValue = aValue.GetIntValue();
      if ((NS_STYLE_PAGE_MARKS_CROP & intValue) != 0) {
        AppendASCIItoUTF16(nsCSSProps::LookupPropertyValue(aProperty, NS_STYLE_PAGE_MARKS_CROP), aResult);
      }
      if ((NS_STYLE_PAGE_MARKS_REGISTER & intValue) != 0) {
        if ((NS_STYLE_PAGE_MARKS_CROP & intValue) != 0) {
          aResult.Append(PRUnichar(' '));
        }
        AppendASCIItoUTF16(nsCSSProps::LookupPropertyValue(aProperty, NS_STYLE_PAGE_MARKS_REGISTER), aResult);
      }
    }
    else {
      const nsAFlatCString& name = nsCSSProps::LookupPropertyValue(aProperty, aValue.GetIntValue());
      AppendASCIItoUTF16(name, aResult);
    }
  }
  else if (eCSSUnit_Color == unit) {
    nsAutoString tmpStr;
    nscolor color = aValue.GetColorValue();

    aResult.Append(NS_LITERAL_STRING("rgb("));

    NS_NAMED_LITERAL_STRING(comma, ", ");

    tmpStr.AppendInt(NS_GET_R(color), 10);
    aResult.Append(tmpStr + comma);

    tmpStr.Truncate();
    tmpStr.AppendInt(NS_GET_G(color), 10);
    aResult.Append(tmpStr + comma);

    tmpStr.Truncate();
    tmpStr.AppendInt(NS_GET_B(color), 10);
    aResult.Append(tmpStr);

    aResult.Append(PRUnichar(')'));
  }
  else if (eCSSUnit_URL == unit || eCSSUnit_Image == unit) {
    aResult.Append(NS_LITERAL_STRING("url(") +
                   nsDependentString(aValue.GetOriginalURLValue()) +
                   NS_LITERAL_STRING(")"));
  }
  else if (eCSSUnit_Percent == unit) {
    nsAutoString tmpStr;
    tmpStr.AppendFloat(aValue.GetPercentValue() * 100.0f);
    aResult.Append(tmpStr);
  }
  else if (eCSSUnit_Percent < unit) {  // length unit
    nsAutoString tmpStr;
    tmpStr.AppendFloat(aValue.GetFloatValue());
    aResult.Append(tmpStr);
  }

  switch (unit) {
    case eCSSUnit_Null:         break;
    case eCSSUnit_Auto:         aResult.Append(NS_LITERAL_STRING("auto"));     break;
    case eCSSUnit_Inherit:      aResult.Append(NS_LITERAL_STRING("inherit"));  break;
    case eCSSUnit_Initial:      aResult.Append(NS_LITERAL_STRING("initial"));  break;
    case eCSSUnit_None:         aResult.Append(NS_LITERAL_STRING("none"));     break;
    case eCSSUnit_Normal:       aResult.Append(NS_LITERAL_STRING("normal"));   break;

    case eCSSUnit_String:       break;
    case eCSSUnit_URL:          break;
    case eCSSUnit_Image:        break;
    case eCSSUnit_Attr:
    case eCSSUnit_Counter:
    case eCSSUnit_Counters:     aResult.Append(PRUnichar(')'));    break;
    case eCSSUnit_Integer:      break;
    case eCSSUnit_Enumerated:   break;
    case eCSSUnit_Color:        break;
    case eCSSUnit_Percent:      aResult.Append(PRUnichar('%'));    break;
    case eCSSUnit_Number:       break;

    case eCSSUnit_Inch:         aResult.Append(NS_LITERAL_STRING("in"));   break;
    case eCSSUnit_Foot:         aResult.Append(NS_LITERAL_STRING("ft"));   break;
    case eCSSUnit_Mile:         aResult.Append(NS_LITERAL_STRING("mi"));   break;
    case eCSSUnit_Millimeter:   aResult.Append(NS_LITERAL_STRING("mm"));   break;
    case eCSSUnit_Centimeter:   aResult.Append(NS_LITERAL_STRING("cm"));   break;
    case eCSSUnit_Meter:        aResult.Append(NS_LITERAL_STRING("m"));    break;
    case eCSSUnit_Kilometer:    aResult.Append(NS_LITERAL_STRING("km"));   break;
    case eCSSUnit_Point:        aResult.Append(NS_LITERAL_STRING("pt"));   break;
    case eCSSUnit_Pica:         aResult.Append(NS_LITERAL_STRING("pc"));   break;
    case eCSSUnit_Didot:        aResult.Append(NS_LITERAL_STRING("dt"));   break;
    case eCSSUnit_Cicero:       aResult.Append(NS_LITERAL_STRING("cc"));   break;

    case eCSSUnit_EM:           aResult.Append(NS_LITERAL_STRING("em"));   break;
    case eCSSUnit_EN:           aResult.Append(NS_LITERAL_STRING("en"));   break;
    case eCSSUnit_XHeight:      aResult.Append(NS_LITERAL_STRING("ex"));   break;
    case eCSSUnit_CapHeight:    aResult.Append(NS_LITERAL_STRING("cap"));  break;
    case eCSSUnit_Char:         aResult.Append(NS_LITERAL_STRING("ch"));   break;

    case eCSSUnit_Pixel:        aResult.Append(NS_LITERAL_STRING("px"));   break;

    case eCSSUnit_Proportional: aResult.Append(NS_LITERAL_STRING("*"));   break;

    case eCSSUnit_Degree:       aResult.Append(NS_LITERAL_STRING("deg"));  break;
    case eCSSUnit_Grad:         aResult.Append(NS_LITERAL_STRING("grad")); break;
    case eCSSUnit_Radian:       aResult.Append(NS_LITERAL_STRING("rad"));  break;

    case eCSSUnit_Hertz:        aResult.Append(NS_LITERAL_STRING("Hz"));   break;
    case eCSSUnit_Kilohertz:    aResult.Append(NS_LITERAL_STRING("kHz"));  break;

    case eCSSUnit_Seconds:      aResult.Append(PRUnichar('s'));    break;
    case eCSSUnit_Milliseconds: aResult.Append(NS_LITERAL_STRING("ms"));   break;
  }

  return PR_TRUE;
}

nsresult
nsCSSDeclaration::GetValue(nsCSSProperty aProperty,
                           nsAString& aValue) const
{
  aValue.Truncate(0);

  // simple properties are easy.
  if (!nsCSSProps::IsShorthand(aProperty)) {
    AppendValueToString(aProperty, aValue);
    return NS_OK;
  }

  // shorthands
  // XXX What about checking the consistency of '!important'?
  switch (aProperty) {
    case eCSSProperty_margin: 
    case eCSSProperty_padding: 
    case eCSSProperty_border_color: 
    case eCSSProperty_border_style: 
    case eCSSProperty__moz_border_radius: 
    case eCSSProperty__moz_outline_radius: 
    case eCSSProperty_border_width: {
      const nsCSSProperty* subprops =
        nsCSSProps::SubpropertyEntryFor(aProperty);
      NS_ASSERTION(nsCSSProps::kTypeTable[subprops[0]] == eCSSType_Value &&
                   nsCSSProps::kTypeTable[subprops[1]] == eCSSType_Value &&
                   nsCSSProps::kTypeTable[subprops[2]] == eCSSType_Value &&
                   nsCSSProps::kTypeTable[subprops[3]] == eCSSType_Value,
                   "type mismatch");
      if (!AppendValueToString(subprops[0], aValue) ||
          !(aValue.Append(PRUnichar(' ')),
            AppendValueToString(subprops[1], aValue)) ||
          !(aValue.Append(PRUnichar(' ')),
            AppendValueToString(subprops[2], aValue)) ||
          !(aValue.Append(PRUnichar(' ')),
            AppendValueToString(subprops[3], aValue))) {
        aValue.Truncate();
      }
      break;
    }
    case eCSSProperty_border:
      // XXX More consistency checking needed before falling through.
      aProperty = eCSSProperty_border_top;
    case eCSSProperty_border_top:
    case eCSSProperty_border_right:
    case eCSSProperty_border_bottom:
    case eCSSProperty_border_left:
    case eCSSProperty__moz_outline: {
      const nsCSSProperty* subprops =
        nsCSSProps::SubpropertyEntryFor(aProperty);
      NS_ASSERTION(nsCSSProps::kTypeTable[subprops[0]] == eCSSType_Value &&
                   nsCSSProps::kTypeTable[subprops[1]] == eCSSType_Value &&
                   nsCSSProps::kTypeTable[subprops[2]] == eCSSType_Value,
                   "type mismatch");
      if (!AppendValueToString(subprops[0], aValue) ||
          !(aValue.Append(PRUnichar(' ')),
            AppendValueToString(subprops[1], aValue)) ||
          !(aValue.Append(PRUnichar(' ')),
            AppendValueToString(subprops[2], aValue))) {
        aValue.Truncate();
      }
      break;
    }
    case eCSSProperty_margin_left:
    case eCSSProperty_margin_right:
    case eCSSProperty_margin_start:
    case eCSSProperty_margin_end:
    case eCSSProperty_padding_left:
    case eCSSProperty_padding_right:
    case eCSSProperty_padding_start:
    case eCSSProperty_padding_end: {
      const nsCSSProperty* subprops =
        nsCSSProps::SubpropertyEntryFor(aProperty);
      NS_ASSERTION(subprops[3] == eCSSProperty_UNKNOWN,
                   "not box property with physical vs. logical cascading");
      AppendValueToString(subprops[0], aValue);
      break;
    }
    case eCSSProperty_background: {
      if (AppendValueToString(eCSSProperty_background_color, aValue))
        aValue.Append(PRUnichar(' '));
      if (AppendValueToString(eCSSProperty_background_image, aValue))
        aValue.Append(PRUnichar(' '));
      if (AppendValueToString(eCSSProperty_background_repeat, aValue))
        aValue.Append(PRUnichar(' '));
      if (AppendValueToString(eCSSProperty_background_attachment, aValue))
        aValue.Append(PRUnichar(' '));
      if (AppendValueToString(eCSSProperty_background_x_position, aValue)) {
        aValue.Append(PRUnichar(' '));
#ifdef DEBUG
        PRBool check =
#endif
          AppendValueToString(eCSSProperty_background_y_position, aValue);
        NS_ASSERTION(check, "we parsed half of background-position");
      }
      break;
    }
    case eCSSProperty_border_spacing:
      if (AppendValueToString(eCSSProperty_border_x_spacing, aValue)) {
        aValue.Append(PRUnichar(' '));
#ifdef DEBUG
        PRBool check =
#endif
          AppendValueToString(eCSSProperty_border_y_spacing, aValue);
        NS_ASSERTION(check, "we parsed half of border-spacing");
      }
      break;
    case eCSSProperty_cue: {
      if (AppendValueToString(eCSSProperty_cue_after, aValue)) {
        aValue.Append(PRUnichar(' '));
        if (!AppendValueToString(eCSSProperty_cue_before, aValue))
          aValue.Truncate();
      }
      break;
    }
    case eCSSProperty_font: {
      if (AppendValueToString(eCSSProperty_font_style, aValue))
        aValue.Append(PRUnichar(' '));
      if (AppendValueToString(eCSSProperty_font_variant, aValue))
        aValue.Append(PRUnichar(' '));
      if (AppendValueToString(eCSSProperty_font_weight, aValue))
        aValue.Append(PRUnichar(' '));
      if (AppendValueToString(eCSSProperty_font_size, aValue)) {
          nsAutoString tmp;
          if (AppendValueToString(eCSSProperty_line_height, tmp)) {
            aValue.Append(PRUnichar('/'));
            aValue.Append(tmp);
          }
          aValue.Append(PRUnichar(' '));
          if (!AppendValueToString(eCSSProperty_font_family, aValue))
            aValue.Truncate();
      } else {
        aValue.Truncate();
      }
      break;
    }
    case eCSSProperty_list_style:
      if (AppendValueToString(eCSSProperty_list_style_type, aValue))
        aValue.Append(PRUnichar(' '));
      if (AppendValueToString(eCSSProperty_list_style_position, aValue))
        aValue.Append(PRUnichar(' '));
      AppendValueToString(eCSSProperty_list_style_image, aValue);
      break;
    case eCSSProperty_pause: {
      if (AppendValueToString(eCSSProperty_pause_after, aValue)) {
        aValue.Append(PRUnichar(' '));
        if (!AppendValueToString(eCSSProperty_pause_before, aValue))
          aValue.Truncate();
      }
      break;
    }
    case eCSSProperty_size: {
      if (AppendValueToString(eCSSProperty_size_width, aValue)) {
        aValue.Append(PRUnichar(' '));
        if (!AppendValueToString(eCSSProperty_size_height, aValue))
          aValue.Truncate();
      }
      break;
    }
    case eCSSProperty_background_position: {
      if (AppendValueToString(eCSSProperty_background_x_position, aValue)) {
        aValue.Append(PRUnichar(' '));
#ifdef DEBUG
        PRBool check =
#endif
        AppendValueToString(eCSSProperty_background_y_position, aValue);
        NS_ASSERTION(check, "we parsed half of background-position");
      }
      break;
    }
    case eCSSProperty_play_during: {
      if (AppendValueToString(eCSSProperty_play_during_uri, aValue)) {
        nsAutoString tmp;
        if (AppendValueToString(eCSSProperty_play_during_flags, tmp)) {
          aValue.Append(PRUnichar(' '));
          aValue.Append(tmp);
        }
      }
      break;
    }
    default:
      NS_NOTREACHED("no other shorthands");
      break;
  }
  return NS_OK;
}

PRBool
nsCSSDeclaration::GetValueIsImportant(const nsAString& aProperty) const
{
  nsCSSProperty propID = nsCSSProps::LookupProperty(aProperty);
  return GetValueIsImportant(propID);
}

PRBool
nsCSSDeclaration::GetValueIsImportant(nsCSSProperty aProperty) const
{
  if (!mImportantData)
    return PR_FALSE;

  // Inefficient, but we can assume '!important' is rare.
  return mImportantData->StorageFor(aProperty) != nsnull;
}

PRBool
nsCSSDeclaration::AllPropertiesSameImportance(PRInt32 aFirst, PRInt32 aSecond,
                                              PRInt32 aThird, PRInt32 aFourth,
                                              PRInt32 aFifth, PRInt32 aSixth,
                                              PRBool & aImportance) const
{
  aImportance = GetValueIsImportant(OrderValueAt(aFirst-1));
  if ((aSecond && aImportance != GetValueIsImportant(OrderValueAt(aSecond-1))) ||
      (aThird && aImportance != GetValueIsImportant(OrderValueAt(aThird-1))) ||
      (aFourth && aImportance != GetValueIsImportant(OrderValueAt(aFourth-1))) ||
      (aFifth && aImportance != GetValueIsImportant(OrderValueAt(aFifth-1))) ||
      (aSixth && aImportance != GetValueIsImportant(OrderValueAt(aSixth-1)))) {
    return PR_FALSE;
  }
  return PR_TRUE;
}

PRBool
nsCSSDeclaration::AllPropertiesSameValue(PRInt32 aFirst, PRInt32 aSecond,
                                         PRInt32 aThird, PRInt32 aFourth) const
{
  nsCSSValue firstValue, otherValue;
  // TryBorderShorthand does the bounds-checking for us; valid values there
  // are > 0; 0 is a flag for "not set".  We here are passed the actual
  // index, which comes from finding the value in the mOrder property array.
  // Of course, re-getting the mOrder value here is pretty silly.
  GetValueOrImportantValue(OrderValueAt(aFirst-1), firstValue);
  GetValueOrImportantValue(OrderValueAt(aSecond-1), otherValue);
  if (firstValue != otherValue) {
    return PR_FALSE;
  }
  GetValueOrImportantValue(OrderValueAt(aThird-1), otherValue);
  if (firstValue != otherValue) {
    return PR_FALSE;
  }
  GetValueOrImportantValue(OrderValueAt(aFourth-1), otherValue);
  if (firstValue != otherValue) {
    return PR_FALSE;
  }
  return PR_TRUE;
}

void
nsCSSDeclaration::AppendImportanceToString(PRBool aIsImportant, nsAString& aString) const
{
  if (aIsImportant) {
   aString.Append(NS_LITERAL_STRING(" ! important"));
  }
}

void
nsCSSDeclaration::AppendPropertyAndValueToString(nsCSSProperty aProperty,
                                                 nsCSSProperty aPropertyName,
                                                 nsAString& aResult) const
{
  NS_ASSERTION(0 <= aProperty && aProperty < eCSSProperty_COUNT_no_shorthands,
               "property enum out of range");
  AppendASCIItoUTF16(nsCSSProps::GetStringValue(aPropertyName), aResult);
  aResult.Append(NS_LITERAL_STRING(": "));
  AppendValueToString(aProperty, aResult);
  PRBool  isImportant = GetValueIsImportant(aProperty);
  AppendImportanceToString(isImportant, aResult);
  aResult.Append(NS_LITERAL_STRING("; "));
}

PRBool
nsCSSDeclaration::TryBorderShorthand(nsAString & aString, PRUint32 aPropertiesSet,
                                     PRInt32 aBorderTopWidth,
                                     PRInt32 aBorderTopStyle,
                                     PRInt32 aBorderTopColor,
                                     PRInt32 aBorderBottomWidth,
                                     PRInt32 aBorderBottomStyle,
                                     PRInt32 aBorderBottomColor,
                                     PRInt32 aBorderLeftWidth,
                                     PRInt32 aBorderLeftStyle,
                                     PRInt32 aBorderLeftColor,
                                     PRInt32 aBorderRightWidth,
                                     PRInt32 aBorderRightStyle,
                                     PRInt32 aBorderRightColor) const
{
  PRBool border = PR_FALSE, isImportant = PR_FALSE;
  // 0 means not in the mOrder array; otherwise it's index+1
  if (B_BORDER == aPropertiesSet
      && AllPropertiesSameValue(aBorderTopWidth, aBorderBottomWidth,
                                aBorderLeftWidth, aBorderRightWidth)
      && AllPropertiesSameValue(aBorderTopStyle, aBorderBottomStyle,
                                aBorderLeftStyle, aBorderRightStyle)
      && AllPropertiesSameValue(aBorderTopColor, aBorderBottomColor,
                                aBorderLeftColor, aBorderRightColor)) {
    border = PR_TRUE;
  }
  if (border) {
    border = PR_FALSE;
    PRBool  isWidthImportant, isStyleImportant, isColorImportant;
    if (AllPropertiesSameImportance(aBorderTopWidth, aBorderBottomWidth,
                                    aBorderLeftWidth, aBorderRightWidth,
                                    0, 0,
                                    isWidthImportant) &&
        AllPropertiesSameImportance(aBorderTopStyle, aBorderBottomStyle,
                                    aBorderLeftStyle, aBorderRightStyle,
                                    0, 0,
                                    isStyleImportant) &&
        AllPropertiesSameImportance(aBorderTopColor, aBorderBottomColor,
                                    aBorderLeftColor, aBorderRightColor,
                                    0, 0,
                                    isColorImportant)) {
      if (isWidthImportant == isStyleImportant && isWidthImportant == isColorImportant) {
        border = PR_TRUE;
        isImportant = isWidthImportant;
      }
    }
  }
  if (border) {
    AppendASCIItoUTF16(nsCSSProps::GetStringValue(eCSSProperty_border), aString);
    aString.Append(NS_LITERAL_STRING(": "));

    AppendValueToString(eCSSProperty_border_top_width, aString);
    aString.Append(PRUnichar(' '));

    AppendValueToString(eCSSProperty_border_top_style, aString);
    aString.Append(PRUnichar(' '));

    nsAutoString valueString;
    AppendValueToString(eCSSProperty_border_top_color, valueString);
    if (!valueString.Equals(NS_LITERAL_STRING("-moz-use-text-color"))) {
      /* don't output this value, it's proprietary Mozilla and  */
      /* not intended to be exposed ; we can remove it from the */
      /* values of the shorthand since this value represents the */
      /* initial value of border-*-color */
      aString.Append(valueString);
    }
    AppendImportanceToString(isImportant, aString);
    aString.Append(NS_LITERAL_STRING("; "));
  }
  return border;
}

PRBool
nsCSSDeclaration::TryBorderSideShorthand(nsAString & aString,
                                         nsCSSProperty aShorthand,
                                         PRInt32 aBorderWidth,
                                         PRInt32 aBorderStyle,
                                         PRInt32 aBorderColor) const
{
  PRBool isImportant;
  if (AllPropertiesSameImportance(aBorderWidth, aBorderStyle, aBorderColor,
                                  0, 0, 0,
                                  isImportant)) {
    AppendASCIItoUTF16(nsCSSProps::GetStringValue(aShorthand), aString);
    aString.Append(NS_LITERAL_STRING(": "));

    AppendValueToString(OrderValueAt(aBorderWidth-1), aString);

    aString.Append(PRUnichar(' '));
    AppendValueToString(OrderValueAt(aBorderStyle-1), aString);

    nsAutoString valueString;
    AppendValueToString(OrderValueAt(aBorderColor-1), valueString);
    if (!valueString.Equals(NS_LITERAL_STRING("-moz-use-text-color"))) {
      aString.Append(NS_LITERAL_STRING(" "));
      aString.Append(valueString);
    }
    AppendImportanceToString(isImportant, aString);
    aString.Append(NS_LITERAL_STRING("; "));
    return PR_TRUE;
  }
  return PR_FALSE;
}

PRBool
nsCSSDeclaration::TryFourSidesShorthand(nsAString & aString,
                                        nsCSSProperty aShorthand,
                                        PRInt32 & aTop,
                                        PRInt32 & aBottom,
                                        PRInt32 & aLeft,
                                        PRInt32 & aRight,
                                        PRBool aClearIndexes) const
{
  // 0 means not in the mOrder array; otherwise it's index+1
  PRBool isImportant;
  if (aTop && aBottom && aLeft && aRight &&
      AllPropertiesSameImportance(aTop, aBottom, aLeft, aRight,
                                  0, 0,
                                  isImportant)) {
    // all 4 properties are set, we can output a shorthand
    AppendASCIItoUTF16(nsCSSProps::GetStringValue(aShorthand), aString);
    aString.Append(NS_LITERAL_STRING(": "));
    nsCSSValue topValue, bottomValue, leftValue, rightValue;
    nsCSSProperty topProp    = OrderValueAt(aTop-1);
    nsCSSProperty bottomProp = OrderValueAt(aBottom-1);
    nsCSSProperty leftProp   = OrderValueAt(aLeft-1);
    nsCSSProperty rightProp  = OrderValueAt(aRight-1);
    GetValueOrImportantValue(topProp,    topValue);
    GetValueOrImportantValue(bottomProp, bottomValue);
    GetValueOrImportantValue(leftProp,   leftValue);
    GetValueOrImportantValue(rightProp,  rightValue);
    AppendCSSValueToString(topProp, topValue, aString);
    if (topValue != rightValue || topValue != leftValue || topValue != bottomValue) {
      aString.Append(PRUnichar(' '));
      AppendCSSValueToString(rightProp, rightValue, aString);
      if (topValue != bottomValue || rightValue != leftValue) {
        aString.Append(PRUnichar(' '));
        AppendCSSValueToString(bottomProp, bottomValue, aString);
        if (rightValue != leftValue) {
          aString.Append(PRUnichar(' '));
          AppendCSSValueToString(leftProp, leftValue, aString);
        }
      }
    }
    if (aClearIndexes) {
      aTop = 0; aBottom = 0; aLeft = 0; aRight = 0;
    }
    AppendImportanceToString(isImportant, aString);
    aString.Append(NS_LITERAL_STRING("; "));
    return PR_TRUE;
  }
  return PR_FALSE;
}

void
nsCSSDeclaration::TryBackgroundShorthand(nsAString & aString,
                                         PRInt32 & aBgColor,
                                         PRInt32 & aBgImage,
                                         PRInt32 & aBgRepeat,
                                         PRInt32 & aBgAttachment,
                                         PRInt32 & aBgPositionX,
                                         PRInt32 & aBgPositionY) const
{
  // 0 means not in the mOrder array; otherwise it's index+1
  // check if we have at least two properties set; otherwise, no need to
  // use a shorthand
  PRBool isImportant;
  if (aBgColor && aBgImage && aBgRepeat && aBgAttachment && aBgPositionX && aBgPositionY &&
      AllPropertiesSameImportance(aBgColor, aBgImage, aBgRepeat, aBgAttachment,
                                  aBgPositionX, aBgPositionY, isImportant)) {
    AppendASCIItoUTF16(nsCSSProps::GetStringValue(eCSSProperty_background), aString);
    aString.Append(NS_LITERAL_STRING(": "));

    AppendValueToString(eCSSProperty_background_color, aString);
    aBgColor = 0;

    aString.Append(PRUnichar(' '));
    AppendValueToString(eCSSProperty_background_image, aString);
    aBgImage = 0;

    aString.Append(PRUnichar(' '));
    AppendValueToString(eCSSProperty_background_repeat, aString);
    aBgRepeat = 0;

    aString.Append(PRUnichar(' '));
    AppendValueToString(eCSSProperty_background_attachment, aString);
    aBgAttachment = 0;

    aString.Append(PRUnichar(' '));
    UseBackgroundPosition(aString, aBgPositionX, aBgPositionY);
    AppendImportanceToString(isImportant, aString);
    aString.Append(NS_LITERAL_STRING("; "));
  }
}

void
nsCSSDeclaration::UseBackgroundPosition(nsAString & aString,
                                        PRInt32 & aBgPositionX,
                                        PRInt32 & aBgPositionY) const
{
  nsAutoString backgroundXValue, backgroundYValue;
  AppendValueToString(eCSSProperty_background_x_position, backgroundXValue);
  AppendValueToString(eCSSProperty_background_y_position, backgroundYValue);
  aString.Append(backgroundXValue);
  if (!backgroundXValue.Equals(backgroundYValue, nsCaseInsensitiveStringComparator())) {
    // the two values are different
    aString.Append(PRUnichar(' '));
    aString.Append(backgroundYValue);
  }
  aBgPositionX = 0;
  aBgPositionY = 0;
}

#define NS_CASE_OUTPUT_PROPERTY_VALUE(_prop, _index) \
case _prop: \
          if (_index) { \
            AppendPropertyAndValueToString(property, aString); \
            _index = 0; \
          } \
          break;

#define NS_CASE_OUTPUT_PROPERTY_VALUE_AS(_prop, _propas, _index) \
case _prop: \
          if (_index) { \
            AppendPropertyAndValueToString(property, _propas, aString); \
            _index = 0; \
          } \
          break;

#define NS_CASE_CONDITIONAL_OUTPUT_PROPERTY_VALUE(_condition, _prop, _index) \
case _prop: \
          if ((_condition) && _index) { \
            AppendPropertyAndValueToString(property, aString); \
            _index = 0; \
          } \
          break;

void nsCSSDeclaration::PropertyIsSet(PRInt32 & aPropertyIndex, PRInt32 aIndex, PRUint32 & aSet, PRUint32 aValue) const
{
  aPropertyIndex = aIndex + 1;
  aSet |= aValue;
}

nsresult
nsCSSDeclaration::ToString(nsAString& aString) const
{
  PRInt32 count = mOrder.Count();
  PRInt32 index;
  // 0 means not in the mOrder array; otherwise it's index+1
  PRInt32 borderTopWidth = 0, borderTopStyle = 0, borderTopColor = 0;
  PRInt32 borderBottomWidth = 0, borderBottomStyle = 0, borderBottomColor = 0;
  PRInt32 borderLeftWidth = 0, borderLeftStyle = 0, borderLeftColor = 0;
  PRInt32 borderRightWidth = 0, borderRightStyle = 0, borderRightColor = 0;
  PRInt32 marginTop = 0,  marginBottom = 0,  marginLeft = 0,  marginRight = 0;
  PRInt32 paddingTop = 0, paddingBottom = 0, paddingLeft = 0, paddingRight = 0;
  PRInt32 bgColor = 0, bgImage = 0, bgRepeat = 0, bgAttachment = 0;
  PRInt32 bgPositionX = 0, bgPositionY = 0;
  PRUint32 borderPropertiesSet = 0, finalBorderPropertiesToSet = 0;
  for (index = 0; index < count; index++) {
    nsCSSProperty property = OrderValueAt(index);
    switch (property) {
      case eCSSProperty_border_top_width:
        PropertyIsSet(borderTopWidth, index, borderPropertiesSet, B_BORDER_TOP_WIDTH);
        break;
      case eCSSProperty_border_bottom_width:
        PropertyIsSet(borderBottomWidth, index, borderPropertiesSet, B_BORDER_BOTTOM_WIDTH);
        break;
      case eCSSProperty_border_left_width:
        PropertyIsSet(borderLeftWidth, index, borderPropertiesSet, B_BORDER_LEFT_WIDTH);
        break;
      case eCSSProperty_border_right_width:
        PropertyIsSet(borderRightWidth, index, borderPropertiesSet, B_BORDER_RIGHT_WIDTH);
        break;

      case eCSSProperty_border_top_style:
        PropertyIsSet(borderTopStyle, index, borderPropertiesSet, B_BORDER_TOP_STYLE);
        break;
      case eCSSProperty_border_bottom_style:
        PropertyIsSet(borderBottomStyle, index, borderPropertiesSet, B_BORDER_BOTTOM_STYLE);
        break;
      case eCSSProperty_border_left_style:
        PropertyIsSet(borderLeftStyle, index, borderPropertiesSet, B_BORDER_LEFT_STYLE);
        break;
      case eCSSProperty_border_right_style:
        PropertyIsSet(borderRightStyle, index, borderPropertiesSet, B_BORDER_RIGHT_STYLE);
        break;

      case eCSSProperty_border_top_color:
        PropertyIsSet(borderTopColor, index, borderPropertiesSet, B_BORDER_TOP_COLOR);
        break;
      case eCSSProperty_border_bottom_color:
        PropertyIsSet(borderBottomColor, index, borderPropertiesSet, B_BORDER_BOTTOM_COLOR);
        break;
      case eCSSProperty_border_left_color:
        PropertyIsSet(borderLeftColor, index, borderPropertiesSet, B_BORDER_LEFT_COLOR);
        break;
      case eCSSProperty_border_right_color:
        PropertyIsSet(borderRightColor, index, borderPropertiesSet, B_BORDER_RIGHT_COLOR);
        break;

      case eCSSProperty_margin_top:            marginTop         = index+1; break;
      case eCSSProperty_margin_bottom:         marginBottom      = index+1; break;
      case eCSSProperty_margin_left_value:     marginLeft        = index+1; break;
      case eCSSProperty_margin_right_value:    marginRight       = index+1; break;

      case eCSSProperty_padding_top:           paddingTop        = index+1; break;
      case eCSSProperty_padding_bottom:        paddingBottom     = index+1; break;
      case eCSSProperty_padding_left_value:    paddingLeft       = index+1; break;
      case eCSSProperty_padding_right_value:   paddingRight      = index+1; break;

      case eCSSProperty_background_color:      bgColor           = index+1; break;
      case eCSSProperty_background_image:      bgImage           = index+1; break;
      case eCSSProperty_background_repeat:     bgRepeat          = index+1; break;
      case eCSSProperty_background_attachment: bgAttachment      = index+1; break;
      case eCSSProperty_background_x_position: bgPositionX       = index+1; break;
      case eCSSProperty_background_y_position: bgPositionY       = index+1; break;
      default:                                 ;
    }
  }

  if (!TryBorderShorthand(aString, borderPropertiesSet,
                          borderTopWidth, borderTopStyle, borderTopColor,
                          borderBottomWidth, borderBottomStyle, borderBottomColor,
                          borderLeftWidth, borderLeftStyle, borderLeftColor,
                          borderRightWidth, borderRightStyle, borderRightColor)) {
    PRUint32 borderPropertiesToSet = 0;
    if ((borderPropertiesSet & B_BORDER_STYLE) != B_BORDER_STYLE ||
        !TryFourSidesShorthand(aString, eCSSProperty_border_style,
                               borderTopStyle, borderBottomStyle,
                               borderLeftStyle, borderRightStyle,
                               PR_FALSE)) {
      borderPropertiesToSet |= B_BORDER_STYLE;
    }
    if ((borderPropertiesSet & B_BORDER_COLOR) != B_BORDER_COLOR ||
        !TryFourSidesShorthand(aString, eCSSProperty_border_color,
                               borderTopColor, borderBottomColor,
                               borderLeftColor, borderRightColor,
                               PR_FALSE)) {
      borderPropertiesToSet |= B_BORDER_COLOR;
    }
    if ((borderPropertiesSet & B_BORDER_WIDTH) != B_BORDER_WIDTH ||
        !TryFourSidesShorthand(aString, eCSSProperty_border_width,
                               borderTopWidth, borderBottomWidth,
                               borderLeftWidth, borderRightWidth,
                               PR_FALSE)) {
      borderPropertiesToSet |= B_BORDER_WIDTH;
    }
    borderPropertiesToSet &= borderPropertiesSet;
    if (borderPropertiesToSet) {
      if ((borderPropertiesSet & B_BORDER_TOP) != B_BORDER_TOP ||
          !TryBorderSideShorthand(aString, eCSSProperty_border_top,
                                  borderTopWidth, borderTopStyle, borderTopColor)) {
        finalBorderPropertiesToSet |= B_BORDER_TOP;
      }
      if ((borderPropertiesSet & B_BORDER_LEFT) != B_BORDER_LEFT ||
          !TryBorderSideShorthand(aString, eCSSProperty_border_left,
                                  borderLeftWidth, borderLeftStyle, borderLeftColor)) {
        finalBorderPropertiesToSet |= B_BORDER_LEFT;
      }
      if ((borderPropertiesSet & B_BORDER_RIGHT) != B_BORDER_RIGHT ||
          !TryBorderSideShorthand(aString, eCSSProperty_border_right,
                                  borderRightWidth, borderRightStyle, borderRightColor)) {
        finalBorderPropertiesToSet |= B_BORDER_RIGHT;
      }
      if ((borderPropertiesSet & B_BORDER_BOTTOM) != B_BORDER_BOTTOM ||
          !TryBorderSideShorthand(aString, eCSSProperty_border_bottom,
                                  borderBottomWidth, borderBottomStyle, borderBottomColor)) {
        finalBorderPropertiesToSet |= B_BORDER_BOTTOM;
      }
      finalBorderPropertiesToSet &= borderPropertiesToSet;
    }
  }

  TryFourSidesShorthand(aString, eCSSProperty_margin,
                        marginTop, marginBottom,
                        marginLeft, marginRight,
                        PR_TRUE);
  TryFourSidesShorthand(aString, eCSSProperty_padding,
                        paddingTop, paddingBottom,
                        paddingLeft, paddingRight,
                        PR_TRUE);
  TryBackgroundShorthand(aString,
                         bgColor, bgImage, bgRepeat, bgAttachment,
                         bgPositionX, bgPositionY);

  for (index = 0; index < count; index++) {
    nsCSSProperty property = OrderValueAt(index);
    switch (property) {

      NS_CASE_CONDITIONAL_OUTPUT_PROPERTY_VALUE(finalBorderPropertiesToSet & B_BORDER_TOP_STYLE,
                                                eCSSProperty_border_top_style, borderTopStyle)
      NS_CASE_CONDITIONAL_OUTPUT_PROPERTY_VALUE(finalBorderPropertiesToSet & B_BORDER_LEFT_STYLE,
                                                eCSSProperty_border_left_style, borderLeftStyle)
      NS_CASE_CONDITIONAL_OUTPUT_PROPERTY_VALUE(finalBorderPropertiesToSet & B_BORDER_RIGHT_STYLE,
                                                eCSSProperty_border_right_style, borderRightStyle)
      NS_CASE_CONDITIONAL_OUTPUT_PROPERTY_VALUE(finalBorderPropertiesToSet & B_BORDER_BOTTOM_STYLE,
                                                eCSSProperty_border_bottom_style, borderBottomStyle)

      NS_CASE_CONDITIONAL_OUTPUT_PROPERTY_VALUE(finalBorderPropertiesToSet & B_BORDER_TOP_COLOR,
                                                eCSSProperty_border_top_color, borderTopColor)
      NS_CASE_CONDITIONAL_OUTPUT_PROPERTY_VALUE(finalBorderPropertiesToSet & B_BORDER_LEFT_COLOR,
                                                eCSSProperty_border_left_color, borderLeftColor)
      NS_CASE_CONDITIONAL_OUTPUT_PROPERTY_VALUE(finalBorderPropertiesToSet & B_BORDER_RIGHT_COLOR,
                                                eCSSProperty_border_right_color, borderRightColor)
      NS_CASE_CONDITIONAL_OUTPUT_PROPERTY_VALUE(finalBorderPropertiesToSet & B_BORDER_BOTTOM_COLOR,
                                                eCSSProperty_border_bottom_color, borderBottomColor)

      NS_CASE_CONDITIONAL_OUTPUT_PROPERTY_VALUE(finalBorderPropertiesToSet & B_BORDER_TOP_WIDTH,
                                                eCSSProperty_border_top_width, borderTopWidth)
      NS_CASE_CONDITIONAL_OUTPUT_PROPERTY_VALUE(finalBorderPropertiesToSet & B_BORDER_LEFT_WIDTH,
                                                eCSSProperty_border_left_width, borderLeftWidth)
      NS_CASE_CONDITIONAL_OUTPUT_PROPERTY_VALUE(finalBorderPropertiesToSet & B_BORDER_RIGHT_WIDTH,
                                                eCSSProperty_border_right_width, borderRightWidth)
      NS_CASE_CONDITIONAL_OUTPUT_PROPERTY_VALUE(finalBorderPropertiesToSet & B_BORDER_BOTTOM_WIDTH,
                                                eCSSProperty_border_bottom_width, borderBottomWidth)

      NS_CASE_OUTPUT_PROPERTY_VALUE(eCSSProperty_margin_top, marginTop)
      NS_CASE_OUTPUT_PROPERTY_VALUE(eCSSProperty_margin_bottom, marginBottom)
      NS_CASE_OUTPUT_PROPERTY_VALUE_AS(eCSSProperty_margin_left_value,
                                       eCSSProperty_margin_left, marginLeft)
      NS_CASE_OUTPUT_PROPERTY_VALUE_AS(eCSSProperty_margin_right_value,
                                       eCSSProperty_margin_right, marginRight)

      NS_CASE_OUTPUT_PROPERTY_VALUE(eCSSProperty_padding_top, paddingTop)
      NS_CASE_OUTPUT_PROPERTY_VALUE(eCSSProperty_padding_bottom, paddingBottom)
      NS_CASE_OUTPUT_PROPERTY_VALUE_AS(eCSSProperty_padding_left_value,
                                       eCSSProperty_padding_left, paddingLeft)
      NS_CASE_OUTPUT_PROPERTY_VALUE_AS(eCSSProperty_padding_right_value,
                                       eCSSProperty_padding_right, paddingRight)

      NS_CASE_OUTPUT_PROPERTY_VALUE(eCSSProperty_background_color, bgColor)
      NS_CASE_OUTPUT_PROPERTY_VALUE(eCSSProperty_background_image, bgImage)
      NS_CASE_OUTPUT_PROPERTY_VALUE(eCSSProperty_background_repeat, bgRepeat)
      NS_CASE_OUTPUT_PROPERTY_VALUE(eCSSProperty_background_attachment, bgAttachment)

      case eCSSProperty_background_x_position:
      case eCSSProperty_background_y_position: {
        // 0 means not in the mOrder array; otherwise it's index+1
        PRBool isImportant;
        if (bgPositionX && bgPositionY &&
            AllPropertiesSameImportance(bgPositionX, bgPositionY,
                                        0, 0, 0, 0, isImportant)) {
          AppendASCIItoUTF16(nsCSSProps::GetStringValue(eCSSProperty_background_position), aString);
          aString.Append(NS_LITERAL_STRING(": "));
          UseBackgroundPosition(aString, bgPositionX, bgPositionY);
          AppendImportanceToString(isImportant, aString);
          aString.Append(NS_LITERAL_STRING("; "));
        }
        else if (eCSSProperty_background_x_position == property && bgPositionX) {
          AppendPropertyAndValueToString(eCSSProperty_background_x_position, aString);
          bgPositionX = 0;
        }
        else if (eCSSProperty_background_y_position == property && bgPositionY) {
          AppendPropertyAndValueToString(eCSSProperty_background_y_position, aString);
          bgPositionY = 0;
        }
        break;
      }

      case eCSSProperty_margin_left_ltr_source:
      case eCSSProperty_margin_left_rtl_source:
      case eCSSProperty_margin_right_ltr_source:
      case eCSSProperty_margin_right_rtl_source:
      case eCSSProperty_padding_left_ltr_source:
      case eCSSProperty_padding_left_rtl_source:
      case eCSSProperty_padding_right_ltr_source:
      case eCSSProperty_padding_right_rtl_source:
        break;

      case eCSSProperty_margin_start_value:
        AppendPropertyAndValueToString(property, eCSSProperty_margin_start,
                                       aString);
        break;
      case eCSSProperty_margin_end_value:
        AppendPropertyAndValueToString(property, eCSSProperty_margin_end,
                                       aString);
        break;
      case eCSSProperty_padding_start_value:
        AppendPropertyAndValueToString(property, eCSSProperty_padding_start,
                                       aString);
        break;
      case eCSSProperty_padding_end_value:
        AppendPropertyAndValueToString(property, eCSSProperty_padding_end,
                                       aString);
        break;

      default:
        if (0 <= property) {
          AppendPropertyAndValueToString(property, aString);
        }
        break;
    }
  }
  if (! aString.IsEmpty()) {
    // if the string is not empty, we have a trailing whitespace we should remove
    aString.Truncate(aString.Length() - 1);
  }
  return NS_OK;
}

#ifdef DEBUG
void nsCSSDeclaration::List(FILE* out, PRInt32 aIndent) const
{
  for (PRInt32 index = aIndent; --index >= 0; ) fputs("  ", out);

  fputs("{ ", out);
  fputs("nsCSSDeclaration::List not implemented", out);
  fputs("}", out);
}
#endif

PRUint32
nsCSSDeclaration::Count() const
{
  return (PRUint32)mOrder.Count();
}

nsresult
nsCSSDeclaration::GetNthProperty(PRUint32 aIndex, nsAString& aReturn) const
{
  aReturn.Truncate();
  if (aIndex < (PRUint32)mOrder.Count()) {
    nsCSSProperty property = OrderValueAt(aIndex);
    if (0 <= property) {
      AppendASCIItoUTF16(nsCSSProps::GetStringValue(property), aReturn);
    }
  }
  
  return NS_OK;
}

nsCSSDeclaration*
nsCSSDeclaration::Clone() const
{
  return new nsCSSDeclaration(*this);
}

PRBool
nsCSSDeclaration::InitializeEmpty()
{
  NS_ASSERTION(!mData && !mImportantData, "already initialized");
  mData = nsCSSCompressedDataBlock::CreateEmptyBlock();
  return mData != nsnull;
}
