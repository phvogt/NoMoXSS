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
 * Portions created by the Initial Developer are Copyright (C) 1999
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

/******

  This file contains the list of all parsed CSS properties.  It is
  designed to be used as inline input through the magic of C
  preprocessing.  All entries must be enclosed in the appropriate
  CSS_PROP_* macro which will have cruel and unusual things done to it.
  It is recommended (but not strictly necessary) to keep all entries in
  alphabetical order.

  The arguments to CSS_PROP_* are:

  1. 'name' entries represent a CSS property name and *must* use only
  lowercase characters.

  2. 'id' should be the same as 'name' except that all hyphens ('-')
  in 'name' are converted to underscores ('_') in 'id'. This lets us
  do nice things with the macros without having to copy/convert strings
  at runtime.  These are the names used for the enum values of the
  nsCSSProperty enumeration defined in nsCSSProps.h.

  3. 'method' is designed to be as input for CSS2Properties and similar
  callers.  It must always be the same as 'name' except it must use
  InterCaps and all hyphens ('-') must be removed.

  4. 'datastruct' says which nsRuleData* struct this property goes in.

  5. 'member' gives the name of the member variable in the nsRuleData
  struct.

  6. 'type' gives the |nsCSSType| of the data in the nsRuleData struct
  and in the nsCSSDeclaration backend.

  7. 'iscoord' says whether the property is a coordinate property for
  which we use an explicit inherit value in the *style structs* (since
  inheritance requires knowledge of layout).

  Which CSS_PROP_* macro a property is in depends on which nsStyle* its
  computed value lives in (unless it is a shorthand, in which case it
  gets CSS_PROP_SHORTHAND).

  8. 'kwtable', which is either nsnull or the name of the appropriate
  keyword table member of class nsCSSProps, for use in
  nsCSSProps::LookupPropertyValue.

 ******/


/*************************************************************************/


// XXX Should we really be using CSS_PROP_SHORTHAND for 'border-spacing',
// 'background-position', and 'size'?


// All includers must explicitly define CSS_PROP_NOTIMPLEMENTED if they
// want this.  (Only the DOM cares.)
#ifndef CSS_PROP_NOTIMPLEMENTED
#define CSS_PROP_NOTIMPLEMENTED(name_, id_, method_) /* nothing */
#define DEFINED_CSS_PROP_NOTIMPLEMENTED
#endif

// All includers must explicitly define CSS_PROP_SHORTHAND if they
// want it.
#ifndef CSS_PROP_SHORTHAND
#define CSS_PROP_SHORTHAND(name_, id_, method_) /* nothing */
#define DEFINED_CSS_PROP_SHORTHAND
#endif


// Callers may define CSS_PROP_LIST_EXCLUDE_INTERNAL if they want to
// exclude internal properties that are not represented in the DOM (only
// the DOM style code defines this).

// A caller who wants all the properties can define the |CSS_PROP|
// macro.
#ifdef CSS_PROP

#define USED_CSS_PROP
#define CSS_PROP_FONT(name_, id_, method_, datastruct_, member_, type_, iscoord_, kwtable_) CSS_PROP(name_, id_, method_, datastruct_, member_, type_, iscoord_, kwtable_)
#define CSS_PROP_COLOR(name_, id_, method_, datastruct_, member_, type_, iscoord_, kwtable_) CSS_PROP(name_, id_, method_, datastruct_, member_, type_, iscoord_, kwtable_)
#define CSS_PROP_BACKGROUND(name_, id_, method_, datastruct_, member_, type_, iscoord_, kwtable_) CSS_PROP(name_, id_, method_, datastruct_, member_, type_, iscoord_, kwtable_)
#define CSS_PROP_LIST(name_, id_, method_, datastruct_, member_, type_, iscoord_, kwtable_) CSS_PROP(name_, id_, method_, datastruct_, member_, type_, iscoord_, kwtable_)
#define CSS_PROP_POSITION(name_, id_, method_, datastruct_, member_, type_, iscoord_, kwtable_) CSS_PROP(name_, id_, method_, datastruct_, member_, type_, iscoord_, kwtable_)
#define CSS_PROP_TEXT(name_, id_, method_, datastruct_, member_, type_, iscoord_, kwtable_) CSS_PROP(name_, id_, method_, datastruct_, member_, type_, iscoord_, kwtable_)
#define CSS_PROP_TEXTRESET(name_, id_, method_, datastruct_, member_, type_, iscoord_, kwtable_) CSS_PROP(name_, id_, method_, datastruct_, member_, type_, iscoord_, kwtable_)
#define CSS_PROP_DISPLAY(name_, id_, method_, datastruct_, member_, type_, iscoord_, kwtable_) CSS_PROP(name_, id_, method_, datastruct_, member_, type_, iscoord_, kwtable_)
#define CSS_PROP_VISIBILITY(name_, id_, method_, datastruct_, member_, type_, iscoord_, kwtable_) CSS_PROP(name_, id_, method_, datastruct_, member_, type_, iscoord_, kwtable_)
#define CSS_PROP_CONTENT(name_, id_, method_, datastruct_, member_, type_, iscoord_, kwtable_) CSS_PROP(name_, id_, method_, datastruct_, member_, type_, iscoord_, kwtable_)
#define CSS_PROP_QUOTES(name_, id_, method_, datastruct_, member_, type_, iscoord_, kwtable_) CSS_PROP(name_, id_, method_, datastruct_, member_, type_, iscoord_, kwtable_)
#define CSS_PROP_USERINTERFACE(name_, id_, method_, datastruct_, member_, type_, iscoord_, kwtable_) CSS_PROP(name_, id_, method_, datastruct_, member_, type_, iscoord_, kwtable_)
#define CSS_PROP_UIRESET(name_, id_, method_, datastruct_, member_, type_, iscoord_, kwtable_) CSS_PROP(name_, id_, method_, datastruct_, member_, type_, iscoord_, kwtable_)
#define CSS_PROP_TABLE(name_, id_, method_, datastruct_, member_, type_, iscoord_, kwtable_) CSS_PROP(name_, id_, method_, datastruct_, member_, type_, iscoord_, kwtable_)
#define CSS_PROP_TABLEBORDER(name_, id_, method_, datastruct_, member_, type_, iscoord_, kwtable_) CSS_PROP(name_, id_, method_, datastruct_, member_, type_, iscoord_, kwtable_)
#define CSS_PROP_MARGIN(name_, id_, method_, datastruct_, member_, type_, iscoord_, kwtable_) CSS_PROP(name_, id_, method_, datastruct_, member_, type_, iscoord_, kwtable_)
#define CSS_PROP_PADDING(name_, id_, method_, datastruct_, member_, type_, iscoord_, kwtable_) CSS_PROP(name_, id_, method_, datastruct_, member_, type_, iscoord_, kwtable_)
#define CSS_PROP_BORDER(name_, id_, method_, datastruct_, member_, type_, iscoord_, kwtable_) CSS_PROP(name_, id_, method_, datastruct_, member_, type_, iscoord_, kwtable_)
#define CSS_PROP_OUTLINE(name_, id_, method_, datastruct_, member_, type_, iscoord_, kwtable_) CSS_PROP(name_, id_, method_, datastruct_, member_, type_, iscoord_, kwtable_)
#define CSS_PROP_XUL(name_, id_, method_, datastruct_, member_, type_, iscoord_, kwtable_) CSS_PROP(name_, id_, method_, datastruct_, member_, type_, iscoord_, kwtable_)
#ifdef MOZ_SVG
#define CSS_PROP_SVG(name_, id_, method_, datastruct_, member_, type_, iscoord_, kwtable_) CSS_PROP(name_, id_, method_, datastruct_, member_, type_, iscoord_, kwtable_)
#define CSS_PROP_SVGRESET(name_, id_, method_, datastruct_, member_, type_, iscoord_, kwtable_) CSS_PROP(name_, id_, method_, datastruct_, member_, type_, iscoord_, kwtable_)
#endif

// For properties that are stored in the CSS backend but are not
// computed.  An includer may define this in addition to CSS_PROP, but
// otherwise we treat it as the same.
#ifndef CSS_PROP_BACKENDONLY
#define CSS_PROP_BACKENDONLY(name_, id_, method_, datastruct_, member_, type_, iscoord_, kwtable_) CSS_PROP(name_, id_, method_, datastruct_, member_, type_, iscoord_, kwtable_)
#define DEFINED_CSS_PROP_BACKENDONLY
#endif

#else /* !defined(CSS_PROP) */

// An includer who does not define CSS_PROP can define any or all of the
// per-struct macros that are equivalent to it, and the rest will be
// ignored.

#ifndef CSS_PROP_FONT
#define CSS_PROP_FONT(name_, id_, method_, datastruct_, member_, type_, iscoord_, kwtable_) /* nothing */
#define DEFINED_CSS_PROP_FONT
#endif
#ifndef CSS_PROP_COLOR
#define CSS_PROP_COLOR(name_, id_, method_, datastruct_, member_, type_, iscoord_, kwtable_) /* nothing */
#define DEFINED_CSS_PROP_COLOR
#endif
#ifndef CSS_PROP_BACKGROUND
#define CSS_PROP_BACKGROUND(name_, id_, method_, datastruct_, member_, type_, iscoord_, kwtable_) /* nothing */
#define DEFINED_CSS_PROP_BACKGROUND
#endif
#ifndef CSS_PROP_LIST
#define CSS_PROP_LIST(name_, id_, method_, datastruct_, member_, type_, iscoord_, kwtable_) /* nothing */
#define DEFINED_CSS_PROP_LIST
#endif
#ifndef CSS_PROP_POSITION
#define CSS_PROP_POSITION(name_, id_, method_, datastruct_, member_, type_, iscoord_, kwtable_) /* nothing */
#define DEFINED_CSS_PROP_POSITION
#endif
#ifndef CSS_PROP_TEXT
#define CSS_PROP_TEXT(name_, id_, method_, datastruct_, member_, type_, iscoord_, kwtable_) /* nothing */
#define DEFINED_CSS_PROP_TEXT
#endif
#ifndef CSS_PROP_TEXTRESET
#define CSS_PROP_TEXTRESET(name_, id_, method_, datastruct_, member_, type_, iscoord_, kwtable_) /* nothing */
#define DEFINED_CSS_PROP_TEXTRESET
#endif
#ifndef CSS_PROP_DISPLAY
#define CSS_PROP_DISPLAY(name_, id_, method_, datastruct_, member_, type_, iscoord_, kwtable_) /* nothing */
#define DEFINED_CSS_PROP_DISPLAY
#endif
#ifndef CSS_PROP_VISIBILITY
#define CSS_PROP_VISIBILITY(name_, id_, method_, datastruct_, member_, type_, iscoord_, kwtable_) /* nothing */
#define DEFINED_CSS_PROP_VISIBILITY
#endif
#ifndef CSS_PROP_CONTENT
#define CSS_PROP_CONTENT(name_, id_, method_, datastruct_, member_, type_, iscoord_, kwtable_) /* nothing */
#define DEFINED_CSS_PROP_CONTENT
#endif
#ifndef CSS_PROP_QUOTES
#define CSS_PROP_QUOTES(name_, id_, method_, datastruct_, member_, type_, iscoord_, kwtable_) /* nothing */
#define DEFINED_CSS_PROP_QUOTES
#endif
#ifndef CSS_PROP_USERINTERFACE
#define CSS_PROP_USERINTERFACE(name_, id_, method_, datastruct_, member_, type_, iscoord_, kwtable_) /* nothing */
#define DEFINED_CSS_PROP_USERINTERFACE
#endif
#ifndef CSS_PROP_UIRESET
#define CSS_PROP_UIRESET(name_, id_, method_, datastruct_, member_, type_, iscoord_, kwtable_) /* nothing */
#define DEFINED_CSS_PROP_UIRESET
#endif
#ifndef CSS_PROP_TABLE
#define CSS_PROP_TABLE(name_, id_, method_, datastruct_, member_, type_, iscoord_, kwtable_) /* nothing */
#define DEFINED_CSS_PROP_TABLE
#endif
#ifndef CSS_PROP_TABLEBORDER
#define CSS_PROP_TABLEBORDER(name_, id_, method_, datastruct_, member_, type_, iscoord_, kwtable_) /* nothing */
#define DEFINED_CSS_PROP_TABLEBORDER
#endif
#ifndef CSS_PROP_MARGIN
#define CSS_PROP_MARGIN(name_, id_, method_, datastruct_, member_, type_, iscoord_, kwtable_) /* nothing */
#define DEFINED_CSS_PROP_MARGIN
#endif
#ifndef CSS_PROP_PADDING
#define CSS_PROP_PADDING(name_, id_, method_, datastruct_, member_, type_, iscoord_, kwtable_) /* nothing */
#define DEFINED_CSS_PROP_PADDING
#endif
#ifndef CSS_PROP_BORDER
#define CSS_PROP_BORDER(name_, id_, method_, datastruct_, member_, type_, iscoord_, kwtable_) /* nothing */
#define DEFINED_CSS_PROP_BORDER
#endif
#ifndef CSS_PROP_OUTLINE
#define CSS_PROP_OUTLINE(name_, id_, method_, datastruct_, member_, type_, iscoord_, kwtable_) /* nothing */
#define DEFINED_CSS_PROP_OUTLINE
#endif
#ifndef CSS_PROP_XUL
#define CSS_PROP_XUL(name_, id_, method_, datastruct_, member_, type_, iscoord_, kwtable_) /* nothing */
#define DEFINED_CSS_PROP_XUL
#endif
#ifdef MOZ_SVG
#ifndef CSS_PROP_SVG
#define CSS_PROP_SVG(name_, id_, method_, datastruct_, member_, type_, iscoord_, kwtable_) /* nothing */
#define DEFINED_CSS_PROP_SVG
#endif
#ifndef CSS_PROP_SVGRESET
#define CSS_PROP_SVGRESET(name_, id_, method_, datastruct_, member_, type_, iscoord_, kwtable_) /* nothing */
#define DEFINED_CSS_PROP_SVGRESET
#endif
#endif /* defined(MOZ_SVG) */

#ifndef CSS_PROP_BACKENDONLY
#define CSS_PROP_BACKENDONLY(name_, id_, method_, datastruct_, member_, type_, iscoord_, kwtable_) /* nothing */
#define DEFINED_CSS_PROP_BACKENDONLY
#endif

#endif /* !defined(CSS_PROP) */

/*************************************************************************/

// For notes XXX bug 3935 below, the names being parsed do not correspond
// to the constants used internally.  It would be nice to bring the
// constants into line sometime.

// The parser will refuse to parse properties marked with -x-.

// Those marked XXX bug 48973 are CSS2 properties that we support
// differently from the spec for UI requirements.  If we ever
// support them correctly the old constants need to be renamed and
// new ones should be entered.

CSS_PROP_DISPLAY(-moz-appearance, appearance, MozAppearance, Display, mAppearance, eCSSType_Value, PR_FALSE, kAppearanceKTable)
CSS_PROP_SHORTHAND(-moz-border-radius, _moz_border_radius, MozBorderRadius)
CSS_PROP_BORDER(-moz-border-radius-topleft, _moz_border_radius_topLeft, MozBorderRadiusTopleft, Margin, mBorderRadius.mTop, eCSSType_Value, PR_TRUE, nsnull)
CSS_PROP_BORDER(-moz-border-radius-topright, _moz_border_radius_topRight, MozBorderRadiusTopright, Margin, mBorderRadius.mRight, eCSSType_Value, PR_TRUE, nsnull)
CSS_PROP_BORDER(-moz-border-radius-bottomleft, _moz_border_radius_bottomLeft, MozBorderRadiusBottomleft, Margin, mBorderRadius.mLeft, eCSSType_Value, PR_TRUE, nsnull)
CSS_PROP_BORDER(-moz-border-radius-bottomright, _moz_border_radius_bottomRight, MozBorderRadiusBottomright, Margin, mBorderRadius.mBottom, eCSSType_Value, PR_TRUE, nsnull)
CSS_PROP_SHORTHAND(-moz-outline-radius, _moz_outline_radius, MozOutlineRadius)
CSS_PROP_OUTLINE(-moz-outline-radius-topleft, _moz_outline_radius_topLeft, MozOutlineRadiusTopleft, Margin, mOutlineRadius.mTop, eCSSType_Value, PR_TRUE, nsnull)
CSS_PROP_OUTLINE(-moz-outline-radius-topright, _moz_outline_radius_topRight, MozOutlineRadiusTopright, Margin, mOutlineRadius.mRight, eCSSType_Value, PR_TRUE, nsnull)
CSS_PROP_OUTLINE(-moz-outline-radius-bottomleft, _moz_outline_radius_bottomLeft, MozOutlineRadiusBottomleft, Margin, mOutlineRadius.mLeft, eCSSType_Value, PR_TRUE, nsnull)
CSS_PROP_OUTLINE(-moz-outline-radius-bottomright, _moz_outline_radius_bottomRight, MozOutlineRadiusBottomright, Margin, mOutlineRadius.mBottom, eCSSType_Value, PR_TRUE, nsnull)
CSS_PROP_BACKENDONLY(azimuth, azimuth, Azimuth, Aural, mAzimuth, eCSSType_Value, PR_FALSE, kAzimuthKTable)
CSS_PROP_SHORTHAND(background, background, Background)
CSS_PROP_BACKGROUND(background-attachment, background_attachment, BackgroundAttachment, Color, mBackAttachment, eCSSType_Value, PR_FALSE, kBackgroundAttachmentKTable)
CSS_PROP_BACKGROUND(-moz-background-clip, _moz_background_clip, MozBackgroundClip, Color, mBackClip, eCSSType_Value, PR_FALSE, kBackgroundClipKTable)
CSS_PROP_BACKGROUND(background-color, background_color, BackgroundColor, Color, mBackColor, eCSSType_Value, PR_FALSE, kBackgroundColorKTable)
CSS_PROP_BACKGROUND(background-image, background_image, BackgroundImage, Color, mBackImage, eCSSType_Value, PR_FALSE, nsnull)
CSS_PROP_BACKGROUND(-moz-background-inline-policy, _moz_background_inline_policy, MozBackgroundInlinePolicy, Color, mBackInlinePolicy, eCSSType_Value, PR_FALSE, kBackgroundInlinePolicyKTable)
CSS_PROP_BACKGROUND(-moz-background-origin, _moz_background_origin, MozBackgroundOrigin, Color, mBackOrigin, eCSSType_Value, PR_FALSE, kBackgroundOriginKTable)
CSS_PROP_SHORTHAND(background-position, background_position, BackgroundPosition)
CSS_PROP_BACKGROUND(background-repeat, background_repeat, BackgroundRepeat, Color, mBackRepeat, eCSSType_Value, PR_FALSE, kBackgroundRepeatKTable)
#ifndef CSS_PROP_LIST_EXCLUDE_INTERNAL
CSS_PROP_BACKGROUND(-x-background-x-position, background_x_position, BackgroundXPosition, Color, mBackPositionX, eCSSType_Value, PR_FALSE, kBackgroundXPositionKTable) // XXX bug 3935
CSS_PROP_BACKGROUND(-x-background-y-position, background_y_position, BackgroundYPosition, Color, mBackPositionY, eCSSType_Value, PR_FALSE, kBackgroundYPositionKTable) // XXX bug 3935
#endif /* !defined (CSS_PROP_LIST_EXCLUDE_INTERNAL) */
CSS_PROP_DISPLAY(-moz-binding, binding, MozBinding, Display, mBinding, eCSSType_Value, PR_FALSE, nsnull) // XXX bug 3935
CSS_PROP_SHORTHAND(border, border, Border)
CSS_PROP_SHORTHAND(border-bottom, border_bottom, BorderBottom)
CSS_PROP_BORDER(border-bottom-color, border_bottom_color, BorderBottomColor, Margin, mBorderColor.mBottom, eCSSType_Value, PR_FALSE, kBorderColorKTable)
CSS_PROP_BORDER(-moz-border-bottom-colors, border_bottom_colors, MozBorderBottomColors, Margin, mBorderColors.mBottom, eCSSType_ValueList, PR_FALSE, nsnull)
CSS_PROP_BORDER(border-bottom-style, border_bottom_style, BorderBottomStyle, Margin, mBorderStyle.mBottom, eCSSType_Value, PR_FALSE, kBorderStyleKTable)  // on/off will need reflow
CSS_PROP_BORDER(border-bottom-width, border_bottom_width, BorderBottomWidth, Margin, mBorderWidth.mBottom, eCSSType_Value, PR_FALSE, kBorderWidthKTable)
CSS_PROP_TABLEBORDER(border-collapse, border_collapse, BorderCollapse, Table, mBorderCollapse, eCSSType_Value, PR_FALSE, kBorderCollapseKTable)
CSS_PROP_SHORTHAND(border-color, border_color, BorderColor)
CSS_PROP_SHORTHAND(border-left, border_left, BorderLeft)
CSS_PROP_BORDER(border-left-color, border_left_color, BorderLeftColor, Margin, mBorderColor.mLeft, eCSSType_Value, PR_FALSE, kBorderColorKTable)
CSS_PROP_BORDER(-moz-border-left-colors, border_left_colors, MozBorderLeftColors, Margin, mBorderColors.mLeft, eCSSType_ValueList, PR_FALSE, nsnull)
CSS_PROP_BORDER(border-left-style, border_left_style, BorderLeftStyle, Margin, mBorderStyle.mLeft, eCSSType_Value, PR_FALSE, kBorderStyleKTable)  // on/off will need reflow
CSS_PROP_BORDER(border-left-width, border_left_width, BorderLeftWidth, Margin, mBorderWidth.mLeft, eCSSType_Value, PR_FALSE, kBorderWidthKTable)
CSS_PROP_SHORTHAND(border-right, border_right, BorderRight)
CSS_PROP_BORDER(border-right-color, border_right_color, BorderRightColor, Margin, mBorderColor.mRight, eCSSType_Value, PR_FALSE, kBorderColorKTable)
CSS_PROP_BORDER(-moz-border-right-colors, border_right_colors, MozBorderRightColors, Margin, mBorderColors.mRight, eCSSType_ValueList, PR_FALSE, nsnull)
CSS_PROP_BORDER(border-right-style, border_right_style, BorderRightStyle, Margin, mBorderStyle.mRight, eCSSType_Value, PR_FALSE, kBorderStyleKTable)  // on/off will need reflow
CSS_PROP_BORDER(border-right-width, border_right_width, BorderRightWidth, Margin, mBorderWidth.mRight, eCSSType_Value, PR_FALSE, kBorderWidthKTable)
CSS_PROP_SHORTHAND(border-spacing, border_spacing, BorderSpacing)
CSS_PROP_SHORTHAND(border-style, border_style, BorderStyle)  // on/off will need reflow
CSS_PROP_SHORTHAND(border-top, border_top, BorderTop)
CSS_PROP_BORDER(border-top-color, border_top_color, BorderTopColor, Margin, mBorderColor.mTop, eCSSType_Value, PR_FALSE, kBorderColorKTable)
CSS_PROP_BORDER(-moz-border-top-colors, border_top_colors, MozBorderTopColors, Margin, mBorderColors.mTop, eCSSType_ValueList, PR_FALSE, nsnull)
CSS_PROP_BORDER(border-top-style, border_top_style, BorderTopStyle, Margin, mBorderStyle.mTop, eCSSType_Value, PR_FALSE, kBorderStyleKTable)  // on/off will need reflow
CSS_PROP_BORDER(border-top-width, border_top_width, BorderTopWidth, Margin, mBorderWidth.mTop, eCSSType_Value, PR_FALSE, kBorderWidthKTable)
CSS_PROP_SHORTHAND(border-width, border_width, BorderWidth)
#ifndef CSS_PROP_LIST_EXCLUDE_INTERNAL
CSS_PROP_TABLEBORDER(-x-border-x-spacing, border_x_spacing, BorderXSpacing, Table, mBorderSpacingX, eCSSType_Value, PR_FALSE, nsnull) // XXX bug 3935
CSS_PROP_TABLEBORDER(-x-border-y-spacing, border_y_spacing, BorderYSpacing, Table, mBorderSpacingY, eCSSType_Value, PR_FALSE, nsnull) // XXX bug 3935
#endif /* !defined (CSS_PROP_LIST_EXCLUDE_INTERNAL) */
CSS_PROP_POSITION(bottom, bottom, Bottom, Position, mOffset.mBottom, eCSSType_Value, PR_TRUE, nsnull)
CSS_PROP_POSITION(-moz-box-sizing, box_sizing, MozBoxSizing, Position, mBoxSizing, eCSSType_Value, PR_FALSE, kBoxSizingKTable) // XXX bug 3935
CSS_PROP_TABLEBORDER(caption-side, caption_side, CaptionSide, Table, mCaptionSide, eCSSType_Value, PR_FALSE, kCaptionSideKTable)
CSS_PROP_DISPLAY(clear, clear, Clear, Display, mClear, eCSSType_Value, PR_FALSE, kClearKTable)
CSS_PROP_DISPLAY(clip, clip, Clip, Display, mClip, eCSSType_Rect, PR_FALSE, nsnull)
CSS_PROP_COLOR(color, color, Color, Color, mColor, eCSSType_Value, PR_FALSE, nsnull)
CSS_PROP_CONTENT(content, content, Content, Content, mContent, eCSSType_ValueList, PR_FALSE, kContentKTable)
CSS_PROP_NOTIMPLEMENTED(counter-increment, counter_increment, CounterIncrement)
CSS_PROP_NOTIMPLEMENTED(counter-reset, counter_reset, CounterReset)
CSS_PROP_CONTENT(-moz-counter-increment, _moz_counter_increment, MozCounterIncrement, Content, mCounterIncrement, eCSSType_CounterData, PR_FALSE, nsnull) // XXX bug 137285
CSS_PROP_CONTENT(-moz-counter-reset, _moz_counter_reset, MozCounterReset, Content, mCounterReset, eCSSType_CounterData, PR_FALSE, nsnull) // XXX bug 137285
CSS_PROP_SHORTHAND(cue, cue, Cue)
CSS_PROP_BACKENDONLY(cue-after, cue_after, CueAfter, Aural, mCueAfter, eCSSType_Value, PR_FALSE, nsnull)
CSS_PROP_BACKENDONLY(cue-before, cue_before, CueBefore, Aural, mCueBefore, eCSSType_Value, PR_FALSE, nsnull)
CSS_PROP_USERINTERFACE(cursor, cursor, Cursor, UserInterface, mCursor, eCSSType_ValueList, PR_FALSE, kCursorKTable)
CSS_PROP_VISIBILITY(direction, direction, Direction, Display, mDirection, eCSSType_Value, PR_FALSE, kDirectionKTable)
CSS_PROP_DISPLAY(display, display, Display, Display, mDisplay, eCSSType_Value, PR_FALSE, kDisplayKTable)
CSS_PROP_BACKENDONLY(elevation, elevation, Elevation, Aural, mElevation, eCSSType_Value, PR_FALSE, kElevationKTable)
CSS_PROP_TABLEBORDER(empty-cells, empty_cells, EmptyCells, Table, mEmptyCells, eCSSType_Value, PR_FALSE, kEmptyCellsKTable)
CSS_PROP_DISPLAY(float, float, CssFloat, Display, mFloat, eCSSType_Value, PR_FALSE, kFloatKTable)
CSS_PROP_BORDER(-moz-float-edge, float_edge, MozFloatEdge, Margin, mFloatEdge, eCSSType_Value, PR_FALSE, kFloatEdgeKTable) // XXX bug 3935
CSS_PROP_SHORTHAND(font, font, Font)
CSS_PROP_FONT(font-family, font_family, FontFamily, Font, mFamily, eCSSType_Value, PR_FALSE, kFontKTable)
CSS_PROP_FONT(font-size, font_size, FontSize, Font, mSize, eCSSType_Value, PR_FALSE, kFontSizeKTable)
CSS_PROP_FONT(font-size-adjust, font_size_adjust, FontSizeAdjust, Font, mSizeAdjust, eCSSType_Value, PR_FALSE, nsnull)
CSS_PROP_BACKENDONLY(font-stretch, font_stretch, FontStretch, Font, mStretch, eCSSType_Value, PR_FALSE, kFontStretchKTable)
CSS_PROP_FONT(font-style, font_style, FontStyle, Font, mStyle, eCSSType_Value, PR_FALSE, kFontStyleKTable)
CSS_PROP_FONT(font-variant, font_variant, FontVariant, Font, mVariant, eCSSType_Value, PR_FALSE, kFontVariantKTable)
CSS_PROP_FONT(font-weight, font_weight, FontWeight, Font, mWeight, eCSSType_Value, PR_FALSE, kFontWeightKTable)
CSS_PROP_UIRESET(-moz-force-broken-image-icon, force_broken_image_icon, MozForceBrokenImageIcon, UserInterface, mForceBrokenImageIcon, eCSSType_Value, PR_FALSE, nsnull) // bug 58646
CSS_PROP_POSITION(height, height, Height, Position, mHeight, eCSSType_Value, PR_TRUE, nsnull)
CSS_PROP_LIST(-moz-image-region, image_region, MozImageRegion, List, mImageRegion, eCSSType_Rect, PR_TRUE, nsnull)
CSS_PROP_UIRESET(-moz-key-equivalent, key_equivalent, MozKeyEquivalent, UserInterface, mKeyEquivalent, eCSSType_ValueList, PR_FALSE, kKeyEquivalentKTable) // This will need some other notification, but what? // XXX bug 3935
CSS_PROP_POSITION(left, left, Left, Position, mOffset.mLeft, eCSSType_Value, PR_TRUE, nsnull)
CSS_PROP_TEXT(letter-spacing, letter_spacing, LetterSpacing, Text, mLetterSpacing, eCSSType_Value, PR_TRUE, nsnull)
CSS_PROP_TEXT(line-height, line_height, LineHeight, Text, mLineHeight, eCSSType_Value, PR_TRUE, nsnull)
CSS_PROP_SHORTHAND(list-style, list_style, ListStyle)
CSS_PROP_LIST(list-style-image, list_style_image, ListStyleImage, List, mImage, eCSSType_Value, PR_FALSE, nsnull)
CSS_PROP_LIST(list-style-position, list_style_position, ListStylePosition, List, mPosition, eCSSType_Value, PR_FALSE, kListStylePositionKTable)
CSS_PROP_LIST(list-style-type, list_style_type, ListStyleType, List, mType, eCSSType_Value, PR_FALSE, kListStyleKTable)
CSS_PROP_SHORTHAND(margin, margin, Margin)
CSS_PROP_MARGIN(margin-bottom, margin_bottom, MarginBottom, Margin, mMargin.mBottom, eCSSType_Value, PR_TRUE, nsnull)
CSS_PROP_SHORTHAND(-moz-margin-end, margin_end, MozMarginEnd)
#ifndef CSS_PROP_LIST_EXCLUDE_INTERNAL
CSS_PROP_MARGIN(margin-end-value, margin_end_value, X, Margin, mMarginEnd, eCSSType_Value, PR_TRUE, nsnull)
#endif
CSS_PROP_SHORTHAND(margin-left, margin_left, MarginLeft)
#ifndef CSS_PROP_LIST_EXCLUDE_INTERNAL
CSS_PROP_MARGIN(margin-left-value, margin_left_value, X, Margin, mMargin.mLeft, eCSSType_Value, PR_TRUE, nsnull)
CSS_PROP_MARGIN(margin-left-ltr-source, margin_left_ltr_source, X, Margin, mMarginLeftLTRSource, eCSSType_Value, PR_TRUE, kBoxPropSourceKTable)
CSS_PROP_MARGIN(margin-left-rtl-source, margin_left_rtl_source, X, Margin, mMarginLeftRTLSource, eCSSType_Value, PR_TRUE, kBoxPropSourceKTable)
#endif
CSS_PROP_SHORTHAND(margin-right, margin_right, MarginRight)
#ifndef CSS_PROP_LIST_EXCLUDE_INTERNAL
CSS_PROP_MARGIN(margin-right-value, margin_right_value, X, Margin, mMargin.mRight, eCSSType_Value, PR_TRUE, nsnull)
CSS_PROP_MARGIN(margin-right-ltr-source, margin_right_ltr_source, X, Margin, mMarginRightLTRSource, eCSSType_Value, PR_TRUE, kBoxPropSourceKTable)
CSS_PROP_MARGIN(margin-right-rtl-source, margin_right_rtl_source, X, Margin, mMarginRightRTLSource, eCSSType_Value, PR_TRUE, kBoxPropSourceKTable)
#endif
CSS_PROP_SHORTHAND(-moz-margin-start, margin_start, MozMarginStart)
#ifndef CSS_PROP_LIST_EXCLUDE_INTERNAL
CSS_PROP_MARGIN(margin-start-value, margin_start_value, X, Margin, mMarginStart, eCSSType_Value, PR_TRUE, nsnull)
#endif
CSS_PROP_MARGIN(margin-top, margin_top, MarginTop, Margin, mMargin.mTop, eCSSType_Value, PR_TRUE, nsnull)
CSS_PROP_CONTENT(marker-offset, marker_offset, MarkerOffset, Content, mMarkerOffset, eCSSType_Value, PR_TRUE, nsnull)
CSS_PROP_BACKENDONLY(marks, marks, Marks, Page, mMarks, eCSSType_Value, PR_FALSE, kPageMarksKTable)
CSS_PROP_POSITION(max-height, max_height, MaxHeight, Position, mMaxHeight, eCSSType_Value, PR_TRUE, nsnull)
CSS_PROP_POSITION(max-width, max_width, MaxWidth, Position, mMaxWidth, eCSSType_Value, PR_TRUE, nsnull)
CSS_PROP_POSITION(min-height, min_height, MinHeight, Position, mMinHeight, eCSSType_Value, PR_TRUE, nsnull)
CSS_PROP_POSITION(min-width, min_width, MinWidth, Position, mMinWidth, eCSSType_Value, PR_TRUE, nsnull)
CSS_PROP_DISPLAY(opacity, opacity, Opacity, Display, mOpacity, eCSSType_Value, PR_FALSE, nsnull) // XXX bug 3935
CSS_PROP_BACKENDONLY(orphans, orphans, Orphans, Breaks, mOrphans, eCSSType_Value, PR_FALSE, nsnull)
CSS_PROP_NOTIMPLEMENTED(outline, outline, Outline)
CSS_PROP_NOTIMPLEMENTED(outline-color, outline_color, OutlineColor)
CSS_PROP_NOTIMPLEMENTED(outline-style, outline_style, OutlineStyle)
CSS_PROP_NOTIMPLEMENTED(outline-width, outline_width, OutlineWidth)
CSS_PROP_SHORTHAND(-moz-outline, _moz_outline, MozOutline)  // XXX This is temporary fix for nsbeta3+ Bug 48973, turning outline into -moz-outline  XXX bug 48973
CSS_PROP_OUTLINE(-moz-outline-color, _moz_outline_color, MozOutlineColor, Margin, mOutlineColor, eCSSType_Value, PR_FALSE, kOutlineColorKTable) // XXX bug 48973
CSS_PROP_OUTLINE(-moz-outline-style, _moz_outline_style, MozOutlineStyle, Margin, mOutlineStyle, eCSSType_Value, PR_FALSE, kBorderStyleKTable) // XXX bug 48973
CSS_PROP_OUTLINE(-moz-outline-width, _moz_outline_width, MozOutlineWidth, Margin, mOutlineWidth, eCSSType_Value, PR_TRUE, kBorderWidthKTable) // XXX bug 48973
CSS_PROP_DISPLAY(overflow, overflow, Overflow, Display, mOverflow, eCSSType_Value, PR_FALSE, kOverflowKTable)
CSS_PROP_SHORTHAND(padding, padding, Padding)
CSS_PROP_PADDING(padding-bottom, padding_bottom, PaddingBottom, Margin, mPadding.mBottom, eCSSType_Value, PR_TRUE, nsnull)
CSS_PROP_SHORTHAND(-moz-padding-end, padding_end, MozPaddingEnd)
#ifndef CSS_PROP_LIST_EXCLUDE_INTERNAL
CSS_PROP_PADDING(padding-end-value, padding_end_value, X, Margin, mPaddingEnd, eCSSType_Value, PR_TRUE, nsnull)
#endif
CSS_PROP_SHORTHAND(padding-left, padding_left, PaddingLeft)
#ifndef CSS_PROP_LIST_EXCLUDE_INTERNAL
CSS_PROP_PADDING(padding-left-value, padding_left_value, X, Margin, mPadding.mLeft, eCSSType_Value, PR_TRUE, nsnull)
CSS_PROP_PADDING(padding-left-ltr-source, padding_left_ltr_source, X, Margin, mPaddingLeftLTRSource, eCSSType_Value, PR_TRUE, kBoxPropSourceKTable)
CSS_PROP_PADDING(padding-left-rtl-source, padding_left_rtl_source, X, Margin, mPaddingLeftRTLSource, eCSSType_Value, PR_TRUE, kBoxPropSourceKTable)
#endif
CSS_PROP_SHORTHAND(padding-right, padding_right, PaddingRight)
#ifndef CSS_PROP_LIST_EXCLUDE_INTERNAL
CSS_PROP_PADDING(padding-right-value, padding_right_value, X, Margin, mPadding.mRight, eCSSType_Value, PR_TRUE, nsnull)
CSS_PROP_PADDING(padding-right-ltr-source, padding_right_ltr_source, X, Margin, mPaddingRightLTRSource, eCSSType_Value, PR_TRUE, kBoxPropSourceKTable)
CSS_PROP_PADDING(padding-right-rtl-source, padding_right_rtl_source, X, Margin, mPaddingRightRTLSource, eCSSType_Value, PR_TRUE, kBoxPropSourceKTable)
#endif
CSS_PROP_SHORTHAND(-moz-padding-start, padding_start, MozPaddingStart)
#ifndef CSS_PROP_LIST_EXCLUDE_INTERNAL
CSS_PROP_PADDING(padding-start-value, padding_start_value, X, Margin, mPaddingStart, eCSSType_Value, PR_TRUE, nsnull)
#endif
CSS_PROP_PADDING(padding-top, padding_top, PaddingTop, Margin, mPadding.mTop, eCSSType_Value, PR_TRUE, nsnull)
CSS_PROP_BACKENDONLY(page, page, Page, Breaks, mPage, eCSSType_Value, PR_FALSE, nsnull)
CSS_PROP_DISPLAY(page-break-after, page_break_after, PageBreakAfter, Display, mBreakAfter, eCSSType_Value, PR_FALSE, kPageBreakKTable) // temp fix for bug 24000
CSS_PROP_DISPLAY(page-break-before, page_break_before, PageBreakBefore, Display, mBreakBefore, eCSSType_Value, PR_FALSE, kPageBreakKTable) // temp fix for bug 24000
CSS_PROP_BACKENDONLY(page-break-inside, page_break_inside, PageBreakInside, Breaks, mPageBreakInside, eCSSType_Value, PR_FALSE, kPageBreakInsideKTable)
CSS_PROP_SHORTHAND(pause, pause, Pause)
CSS_PROP_BACKENDONLY(pause-after, pause_after, PauseAfter, Aural, mPauseAfter, eCSSType_Value, PR_FALSE, nsnull)
CSS_PROP_BACKENDONLY(pause-before, pause_before, PauseBefore, Aural, mPauseBefore, eCSSType_Value, PR_FALSE, nsnull)
CSS_PROP_BACKENDONLY(pitch, pitch, Pitch, Aural, mPitch, eCSSType_Value, PR_FALSE, kPitchKTable)
CSS_PROP_BACKENDONLY(pitch-range, pitch_range, PitchRange, Aural, mPitchRange, eCSSType_Value, PR_FALSE, nsnull)
CSS_PROP_SHORTHAND(play-during, play_during, PlayDuring)
#ifndef CSS_PROP_LIST_EXCLUDE_INTERNAL
CSS_PROP_BACKENDONLY(-x-play-during-flags, play_during_flags, PlayDuringFlags, Aural, mPlayDuringFlags, eCSSType_Value, PR_FALSE, kPlayDuringKTable) // XXX why is this here?
CSS_PROP_BACKENDONLY(-x-play-during-uri, play_during_uri, PlayDuringURI, Aural, mPlayDuring, eCSSType_Value, PR_FALSE, nsnull) // XXX why is this here?
#endif /* !defined (CSS_PROP_LIST_EXCLUDE_INTERNAL) */
CSS_PROP_DISPLAY(position, position, Position, Display, mPosition, eCSSType_Value, PR_FALSE, kPositionKTable)
CSS_PROP_QUOTES(quotes, quotes, Quotes, Content, mQuotes, eCSSType_Quotes, PR_FALSE, nsnull)
CSS_PROP_UIRESET(-moz-resizer, resizer, MozResizer, UserInterface, mResizer, eCSSType_Value, PR_FALSE, kResizerKTable) // XXX bug 3935
CSS_PROP_BACKENDONLY(richness, richness, Richness, Aural, mRichness, eCSSType_Value, PR_FALSE, nsnull)
CSS_PROP_POSITION(right, right, Right, Position, mOffset.mRight, eCSSType_Value, PR_TRUE, nsnull)
CSS_PROP_SHORTHAND(size, size, Size)
#ifndef CSS_PROP_LIST_EXCLUDE_INTERNAL
CSS_PROP_BACKENDONLY(-x-size-height, size_height, SizeHeight, Page, mSizeHeight, eCSSType_Value, PR_FALSE, kPageSizeKTable) // XXX bug 3935
CSS_PROP_BACKENDONLY(-x-size-width, size_width, SizeWidth, Page, mSizeWidth, eCSSType_Value, PR_FALSE, kPageSizeKTable) // XXX bug 3935
#endif /* !defined (CSS_PROP_LIST_EXCLUDE_INTERNAL) */
CSS_PROP_BACKENDONLY(speak, speak, Speak, Aural, mSpeak, eCSSType_Value, PR_FALSE, kSpeakKTable)
CSS_PROP_BACKENDONLY(speak-header, speak_header, SpeakHeader, Aural, mSpeakHeader, eCSSType_Value, PR_FALSE, kSpeakHeaderKTable)
CSS_PROP_BACKENDONLY(speak-numeral, speak_numeral, SpeakNumeral, Aural, mSpeakNumeral, eCSSType_Value, PR_FALSE, kSpeakNumeralKTable)
CSS_PROP_BACKENDONLY(speak-punctuation, speak_punctuation, SpeakPunctuation, Aural, mSpeakPunctuation, eCSSType_Value, PR_FALSE, kSpeakPunctuationKTable)
CSS_PROP_BACKENDONLY(speech-rate, speech_rate, SpeechRate, Aural, mSpeechRate, eCSSType_Value, PR_FALSE, kSpeechRateKTable)
CSS_PROP_BACKENDONLY(stress, stress, Stress, Aural, mStress, eCSSType_Value, PR_FALSE, nsnull)
CSS_PROP_TABLE(table-layout, table_layout, TableLayout, Table, mLayout, eCSSType_Value, PR_FALSE, kTableLayoutKTable)
CSS_PROP_TEXT(text-align, text_align, TextAlign, Text, mTextAlign, eCSSType_Value, PR_FALSE, kTextAlignKTable)
CSS_PROP_TEXTRESET(text-decoration, text_decoration, TextDecoration, Text, mDecoration, eCSSType_Value, PR_FALSE, kTextDecorationKTable)
CSS_PROP_TEXT(text-indent, text_indent, TextIndent, Text, mTextIndent, eCSSType_Value, PR_TRUE, nsnull)
CSS_PROP_BACKENDONLY(text-shadow, text_shadow, TextShadow, Text, mTextShadow, eCSSType_Shadow, PR_FALSE, nsnull)
CSS_PROP_TEXT(text-transform, text_transform, TextTransform, Text, mTextTransform, eCSSType_Value, PR_FALSE, kTextTransformKTable)
CSS_PROP_POSITION(top, top, Top, Position, mOffset.mTop, eCSSType_Value, PR_TRUE, nsnull)
CSS_PROP_TEXTRESET(unicode-bidi, unicode_bidi, UnicodeBidi, Text, mUnicodeBidi, eCSSType_Value, PR_FALSE, kUnicodeBidiKTable)
CSS_PROP_USERINTERFACE(-moz-user-focus, user_focus, MozUserFocus, UserInterface, mUserFocus, eCSSType_Value, PR_FALSE, kUserFocusKTable) // XXX bug 3935
CSS_PROP_USERINTERFACE(-moz-user-input, user_input, MozUserInput, UserInterface, mUserInput, eCSSType_Value, PR_FALSE, kUserInputKTable) // XXX ??? // XXX bug 3935
CSS_PROP_USERINTERFACE(-moz-user-modify, user_modify, MozUserModify, UserInterface, mUserModify, eCSSType_Value, PR_FALSE, kUserModifyKTable) // XXX bug 3935
CSS_PROP_UIRESET(-moz-user-select, user_select, MozUserSelect, UserInterface, mUserSelect, eCSSType_Value, PR_FALSE, kUserSelectKTable) // XXX bug 3935
CSS_PROP_TEXTRESET(vertical-align, vertical_align, VerticalAlign, Text, mVerticalAlign, eCSSType_Value, PR_TRUE, kVerticalAlignKTable)
CSS_PROP_VISIBILITY(visibility, visibility, Visibility, Display, mVisibility, eCSSType_Value, PR_FALSE, kVisibilityKTable)  // reflow for collapse
CSS_PROP_BACKENDONLY(voice-family, voice_family, VoiceFamily, Aural, mVoiceFamily, eCSSType_Value, PR_FALSE, nsnull)
CSS_PROP_BACKENDONLY(volume, volume, Volume, Aural, mVolume, eCSSType_Value, PR_FALSE, kVolumeKTable)
CSS_PROP_TEXT(white-space, white_space, WhiteSpace, Text, mWhiteSpace, eCSSType_Value, PR_FALSE, kWhitespaceKTable)
CSS_PROP_BACKENDONLY(widows, widows, Widows, Breaks, mWidows, eCSSType_Value, PR_FALSE, nsnull)
CSS_PROP_POSITION(width, width, Width, Position, mWidth, eCSSType_Value, PR_TRUE, nsnull)
CSS_PROP_TEXT(word-spacing, word_spacing, WordSpacing, Text, mWordSpacing, eCSSType_Value, PR_TRUE, nsnull)
CSS_PROP_POSITION(z-index, z_index, ZIndex, Position, mZIndex, eCSSType_Value, PR_FALSE, nsnull)

CSS_PROP_XUL(-moz-box-align, box_align, MozBoxAlign, XUL, mBoxAlign, eCSSType_Value, PR_FALSE, kBoxAlignKTable) // XXX bug 3935
CSS_PROP_XUL(-moz-box-direction, box_direction, MozBoxDirection, XUL, mBoxDirection, eCSSType_Value, PR_FALSE, kBoxDirectionKTable) // XXX bug 3935
CSS_PROP_XUL(-moz-box-flex, box_flex, MozBoxFlex, XUL, mBoxFlex, eCSSType_Value, PR_FALSE, nsnull) // XXX bug 3935
CSS_PROP_XUL(-moz-box-orient, box_orient, MozBoxOrient, XUL, mBoxOrient, eCSSType_Value, PR_FALSE, kBoxOrientKTable) // XXX bug 3935
CSS_PROP_XUL(-moz-box-pack, box_pack, MozBoxPack, XUL, mBoxPack, eCSSType_Value, PR_FALSE, kBoxPackKTable) // XXX bug 3935
CSS_PROP_XUL(-moz-box-ordinal-group, box_ordinal_group, MozBoxOrdinalGroup, XUL, mBoxOrdinal, eCSSType_Value, PR_FALSE, nsnull)

#ifdef MOZ_SVG
// XXX treat SVG's CSS Properties as internal for now.
// Do we want to create an nsIDOMSVGCSS2Properties interface?
#ifndef CSS_PROP_LIST_EXCLUDE_INTERNAL
CSS_PROP_SVGRESET(dominant-baseline, dominant_baseline, DominantBaseline, SVG, mDominantBaseline, eCSSType_Value, PR_FALSE, kDominantBaselineKTable)
CSS_PROP_SVG(fill, fill, Fill, SVG, mFill, eCSSType_Value, PR_FALSE, nsnull)
CSS_PROP_SVG(fill-opacity, fill_opacity, FillOpacity, SVG, mFillOpacity, eCSSType_Value, PR_FALSE, nsnull)
CSS_PROP_SVG(fill-rule, fill_rule, FillRule, SVG, mFillRule, eCSSType_Value, PR_FALSE, kFillRuleKTable)
CSS_PROP_SVG(pointer-events, pointer_events, PointerEvents, SVG, mPointerEvents, eCSSType_Value, PR_FALSE, kPointerEventsKTable)
CSS_PROP_SVG(shape-rendering, shape_rendering, ShapeRendering, SVG, mShapeRendering, eCSSType_Value, PR_FALSE, kShapeRenderingKTable)
CSS_PROP_SVG(stroke, stroke, Stroke, SVG, mStroke, eCSSType_Value, PR_FALSE, nsnull)
CSS_PROP_SVG(stroke-dasharray, stroke_dasharray, StrokeDasharray, SVG, mStrokeDasharray, eCSSType_Value, PR_FALSE, nsnull)
CSS_PROP_SVG(stroke-dashoffset, stroke_dashoffset, StrokeDashoffset, SVG, mStrokeDashoffset, eCSSType_Value, PR_FALSE, nsnull)
CSS_PROP_SVG(stroke-linecap, stroke_linecap, StrokeLinecap, SVG, mStrokeLinecap, eCSSType_Value, PR_FALSE, kStrokeLinecapKTable)
CSS_PROP_SVG(stroke-linejoin, stroke_linejoin, StrokeLinejoin, SVG, mStrokeLinejoin, eCSSType_Value, PR_FALSE, kStrokeLinejoinKTable)
CSS_PROP_SVG(stroke-miterlimit, stroke_miterlimit, StrokeMiterlimit, SVG, mStrokeMiterlimit, eCSSType_Value, PR_FALSE, nsnull)
CSS_PROP_SVG(stroke-opacity, stroke_opacity, StrokeOpacity, SVG, mStrokeOpacity, eCSSType_Value, PR_FALSE, nsnull)
CSS_PROP_SVG(stroke-width, stroke_width, StrokeWidth, SVG, mStrokeWidth, eCSSType_Value, PR_FALSE, nsnull)
CSS_PROP_SVG(text-anchor, text_anchor, TextAnchor, SVG, mTextAnchor, eCSSType_Value, PR_FALSE, kTextAnchorKTable)
CSS_PROP_SVG(text-rendering, text_rendering, TextRendering, SVG, mTextRendering, eCSSType_Value, PR_FALSE, kTextRenderingKTable)
#endif /* !defined (CSS_PROP_LIST_EXCLUDE_INTERNAL) */
#endif

// Callers that want information on the properties that are in
// the style structs but not in the nsCSS* structs should define
// |CSS_PROP_INCLUDE_NOT_CSS|.  (Some of these are also in nsRuleData*,
// and a distinction might be needed at some point.)
// The first 3 parameters don't matter, but some compilers don't like
// empty arguments to macros.
#ifdef CSS_PROP_INCLUDE_NOT_CSS
CSS_PROP_VISIBILITY(X, X, X, Display, mLang, eCSSType_Value, PR_FALSE, nsnull)
CSS_PROP_TABLE(X, X, X, Table, mFrame, eCSSType_Value, PR_FALSE, nsnull)
CSS_PROP_TABLE(X, X, X, Table, mRules, eCSSType_Value, PR_FALSE, nsnull)
CSS_PROP_TABLE(X, X, X, Table, mCols, eCSSType_Value, PR_FALSE, nsnull)
CSS_PROP_TABLE(X, X, X, Table, mSpan, eCSSType_Value, PR_FALSE, nsnull)
#endif /* defined(CSS_PROP_INCLUDE_NOT_CSS) */

#ifdef USED_CSS_PROP

#undef USED_CSS_PROP
#undef CSS_PROP_FONT
#undef CSS_PROP_COLOR
#undef CSS_PROP_BACKGROUND
#undef CSS_PROP_LIST
#undef CSS_PROP_POSITION
#undef CSS_PROP_TEXT
#undef CSS_PROP_TEXTRESET
#undef CSS_PROP_DISPLAY
#undef CSS_PROP_VISIBILITY
#undef CSS_PROP_CONTENT
#undef CSS_PROP_QUOTES
#undef CSS_PROP_USERINTERFACE
#undef CSS_PROP_UIRESET
#undef CSS_PROP_TABLE
#undef CSS_PROP_TABLEBORDER
#undef CSS_PROP_MARGIN
#undef CSS_PROP_PADDING
#undef CSS_PROP_BORDER
#undef CSS_PROP_OUTLINE
#undef CSS_PROP_XUL
#ifdef MOZ_SVG
#undef CSS_PROP_SVG
#undef CSS_PROP_SVGRESET
#endif
#ifdef DEFINED_CSS_PROP_BACKENDONLY
#undef CSS_PROP_BACKENDONLY
#undef DEFINED_CSS_PROP_BACKENDONLY
#endif

#else /* !defined(USED_CSS_PROP) */

#ifdef DEFINED_CSS_PROP_FONT
#undef CSS_PROP_FONT
#undef DEFINED_CSS_PROP_FONT
#endif
#ifdef DEFINED_CSS_PROP_COLOR
#undef CSS_PROP_COLOR
#undef DEFINED_CSS_PROP_COLOR
#endif
#ifdef DEFINED_CSS_PROP_BACKGROUND
#undef CSS_PROP_BACKGROUND
#undef DEFINED_CSS_PROP_BACKGROUND
#endif
#ifdef DEFINED_CSS_PROP_LIST
#undef CSS_PROP_LIST
#undef DEFINED_CSS_PROP_LIST
#endif
#ifdef DEFINED_CSS_PROP_POSITION
#undef CSS_PROP_POSITION
#undef DEFINED_CSS_PROP_POSITION
#endif
#ifdef DEFINED_CSS_PROP_TEXT
#undef CSS_PROP_TEXT
#undef DEFINED_CSS_PROP_TETEXTRESETT
#endif
#ifdef DEFINED_CSS_PROP_TEXTRESET
#undef CSS_PROP_TEXTRESET
#undef DEFINED_CSS_PROP_TEDISPLAYTRESET
#endif
#ifdef DEFINED_CSS_PROP_DISPLAY
#undef CSS_PROP_DISPLAY
#undef DEFINED_CSS_PROP_DISPLAY
#endif
#ifdef DEFINED_CSS_PROP_VISIBILITY
#undef CSS_PROP_VISIBILITY
#undef DEFINED_CSS_PROP_VISIBILITY
#endif
#ifdef DEFINED_CSS_PROP_CONTENT
#undef CSS_PROP_CONTENT
#undef DEFINED_CSS_PROP_CONTENT
#endif
#ifdef DEFINED_CSS_PROP_QUOTES
#undef CSS_PROP_QUOTES
#undef DEFINED_CSS_PROP_QUOTES
#endif
#ifdef DEFINED_CSS_PROP_USERINTERFACE
#undef CSS_PROP_USERINTERFACE
#undef DEFINED_CSS_PROP_USERINTERFACE
#endif
#ifdef DEFINED_CSS_PROP_UIRESET
#undef CSS_PROP_UIRESET
#undef DEFINED_CSS_PROP_UIRESET
#endif
#ifdef DEFINED_CSS_PROP_TABLE
#undef CSS_PROP_TABLE
#undef DEFINED_CSS_PROP_TABLE
#endif
#ifdef DEFINED_CSS_PROP_TABLEBORDER
#undef CSS_PROP_TABLEBORDER
#undef DEFINED_CSS_PROP_TABLEBORDER
#endif
#ifdef DEFINED_CSS_PROP_MARGIN
#undef CSS_PROP_MARGIN
#undef DEFINED_CSS_PROP_MARGIN
#endif
#ifdef DEFINED_CSS_PROP_PADDING
#undef CSS_PROP_PADDING
#undef DEFINED_CSS_PROP_PADDING
#endif
#ifdef DEFINED_CSS_PROP_BORDER
#undef CSS_PROP_BORDER
#undef DEFINED_CSS_PROP_BORDER
#endif
#ifdef DEFINED_CSS_PROP_OUTLINE
#undef CSS_PROP_OUTLINE
#undef DEFINED_CSS_PROP_OUTLINE
#endif
#ifdef DEFINED_CSS_PROP_XUL
#undef CSS_PROP_XUL
#undef DEFINED_CSS_PROP_XUL
#endif
#ifdef MOZ_SVG
#ifdef DEFINED_CSS_PROP_SVG
#undef CSS_PROP_SVG
#undef DEFINED_CSS_PROP_SVG
#endif
#ifdef DEFINED_CSS_PROP_SVGRESET
#undef CSS_PROP_SVGRESET
#undef DEFINED_CSS_PROP_SVGRESET
#endif
#endif /* defined(MOZ_SVG) */
#ifdef DEFINED_CSS_PROP_BACKENDONLY
#undef CSS_PROP_BACKENDONLY
#undef DEFINED_CSS_PROP_BACKENDONLY
#endif

#endif /* !defined(USED_CSS_PROP) */

#ifdef DEFINED_CSS_PROP_NOTIMPLEMENTED
#undef CSS_PROP_NOTIMPLEMENTED
#undef DEFINED_CSS_PROP_NOTIMPLEMENTED
#endif

#ifdef DEFINED_CSS_PROP_SHORTHAND
#undef CSS_PROP_SHORTHAND
#undef DEFINED_CSS_PROP_SHORTHAND
#endif
