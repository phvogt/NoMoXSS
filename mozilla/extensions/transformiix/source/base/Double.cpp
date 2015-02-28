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
 * The Initial Developer of the Original Code is The MITRE Corporation.
 * Portions created by MITRE are Copyright (C) 1999 The MITRE Corporation.
 *
 * Portions created by Keith Visco as a Non MITRE employee,
 * (C) 1999 Keith Visco. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Keith Visco, kvisco@ziplink.net
 *    -- original author.
 *
 * Larry Fitzpatrick, lef@opentext.com
 *
 * Eric Du, duxy@leyou.com.cn
 *   -- added fix for FreeBSD
 *
 * NaN/Infinity code copied from the JS-library with permission from
 * Netscape Communications Corporation: http://www.mozilla.org/js
 * http://lxr.mozilla.org/seamonkey/source/js/src/jsnum.h
 *
 */

#include "nsString.h"
#include "primitives.h"
#include "XMLUtils.h"
#include <math.h>
#include <stdlib.h>
#ifdef WIN32
#include <float.h>
#endif
#include "prdtoa.h"

/*
 * Utility class for doubles
 */

//A trick to handle IEEE floating point exceptions on FreeBSD - E.D.
#ifdef __FreeBSD__
#include <ieeefp.h>
#ifdef __alpha__
fp_except_t allmask = FP_X_INV|FP_X_OFL|FP_X_UFL|FP_X_DZ|FP_X_IMP;
#else
fp_except_t allmask = FP_X_INV|FP_X_OFL|FP_X_UFL|FP_X_DZ|FP_X_IMP|FP_X_DNML;
#endif
fp_except_t oldmask = fpsetmask(~allmask);
#endif

/**
 * Macros to workaround math-bugs bugs in various platforms
 */

/**
 * Stefan Hanske <sh990154@mail.uni-greifswald.de> reports:
 *  ARM is a little endian architecture but 64 bit double words are stored
 * differently: the 32 bit words are in little endian byte order, the two words
 * are stored in big endian`s way.
 */

#if defined(__arm) || defined(__arm32__) || defined(_arm26__) || defined(__arm__)
#define CPU_IS_ARM
#endif

#if (__GNUC__ == 2 && __GNUC_MINOR__ > 95) || __GNUC__ > 2
/**
 * This version of the macros is safe for the alias optimizations
 * that gcc does, but uses gcc-specific extensions.
 */

typedef union txdpun {
    PRFloat64 d;
    struct {
#if defined(IS_LITTLE_ENDIAN) && !defined(CPU_IS_ARM)
        PRUint32 lo, hi;
#else
        PRUint32 hi, lo;
#endif
    } s;
} txdpun;

#define TX_DOUBLE_HI32(x) (__extension__ ({ txdpun u; u.d = (x); u.s.hi; }))
#define TX_DOUBLE_LO32(x) (__extension__ ({ txdpun u; u.d = (x); u.s.lo; }))

#else // __GNUC__

/* We don't know of any non-gcc compilers that perform alias optimization,
 * so this code should work.
 */

#if defined(IS_LITTLE_ENDIAN) && !defined(CPU_IS_ARM)
#define TX_DOUBLE_HI32(x)        (((PRUint32 *)&(x))[1])
#define TX_DOUBLE_LO32(x)        (((PRUint32 *)&(x))[0])
#else
#define TX_DOUBLE_HI32(x)        (((PRUint32 *)&(x))[0])
#define TX_DOUBLE_LO32(x)        (((PRUint32 *)&(x))[1])
#endif

#endif // __GNUC__

#define TX_DOUBLE_HI32_SIGNBIT   0x80000000
#define TX_DOUBLE_HI32_EXPMASK   0x7ff00000
#define TX_DOUBLE_HI32_MANTMASK  0x000fffff

//-- Initialize Double related constants
#ifdef IS_BIG_ENDIAN
const PRUint32 nanMask[2] =    {TX_DOUBLE_HI32_EXPMASK | TX_DOUBLE_HI32_MANTMASK,
                                0xffffffff};
const PRUint32 infMask[2] =    {TX_DOUBLE_HI32_EXPMASK, 0};
const PRUint32 negInfMask[2] = {TX_DOUBLE_HI32_EXPMASK | TX_DOUBLE_HI32_SIGNBIT, 0};
#else
const PRUint32 nanMask[2] =    {0xffffffff,
                                TX_DOUBLE_HI32_EXPMASK | TX_DOUBLE_HI32_MANTMASK};
const PRUint32 infMask[2] =    {0, TX_DOUBLE_HI32_EXPMASK};
const PRUint32 negInfMask[2] = {0, TX_DOUBLE_HI32_EXPMASK | TX_DOUBLE_HI32_SIGNBIT};
#endif

const double Double::NaN = *((double*)nanMask);
const double Double::POSITIVE_INFINITY = *((double*)infMask);
const double Double::NEGATIVE_INFINITY = *((double*)negInfMask);

/*
 * Determines whether the given double represents positive or negative
 * inifinity
 */
MBool Double::isInfinite(double aDbl)
{
    return ((TX_DOUBLE_HI32(aDbl) & ~TX_DOUBLE_HI32_SIGNBIT) == TX_DOUBLE_HI32_EXPMASK &&
            !TX_DOUBLE_LO32(aDbl));
}

/*
 * Determines whether the given double is NaN
 */
MBool Double::isNaN(double aDbl)
{
    return ((TX_DOUBLE_HI32(aDbl) & TX_DOUBLE_HI32_EXPMASK) == TX_DOUBLE_HI32_EXPMASK &&
            (TX_DOUBLE_LO32(aDbl) || (TX_DOUBLE_HI32(aDbl) & TX_DOUBLE_HI32_MANTMASK)));
}

/*
 * Determines whether the given double is negative
 */
MBool Double::isNeg(double aDbl)
{
    return (TX_DOUBLE_HI32(aDbl) & TX_DOUBLE_HI32_SIGNBIT) != 0;
}

/*
 * Converts the given String to a double, if the String value does not
 * represent a double, NaN will be returned
 */
class txStringToDouble
{
public:
    typedef PRUnichar input_type;
    typedef PRUnichar value_type;
    txStringToDouble(): mState(eWhitestart), mSign(ePositive) {}

    PRUint32
    write(const input_type* aSource, PRUint32 aSourceLength)
    {
        if (mState == eIllegal) {
            return aSourceLength;
        }
        PRUint32 i = 0;
        PRUnichar c;
        for ( ; i < aSourceLength; ++i) {
            c = aSource[i];
            switch (mState) {
                case eWhitestart:
                    if (c == '-') {
                        mState = eDecimal;
                        mSign = eNegative;
                    }
                    else if (c >= '0' && c <= '9') {
                        mState = eDecimal;
                        mBuffer.Append((char)c);
                    }
                    else if (c == '.') {
                        mState = eMantissa;
                        mBuffer.Append((char)c);
                    }
                    else if (!XMLUtils::isWhitespace(c)) {
                        mState = eIllegal;
                        return aSourceLength;
                    }
                    break;
                case eDecimal:
                    if (c >= '0' && c <= '9') {
                        mBuffer.Append((char)c);
                    }
                    else if (c == '.') {
                        mState = eMantissa;
                        mBuffer.Append((char)c);
                    }
                    else if (XMLUtils::isWhitespace(c)) {
                        mState = eWhiteend;
                    }
                    else {
                        mState = eIllegal;
                        return aSourceLength;
                    }
                    break;
                case eMantissa:
                    if (c >= '0' && c <= '9') {
                        mBuffer.Append((char)c);
                    }
                    else if (XMLUtils::isWhitespace(c)) {
                        mState = eWhiteend;
                    }
                    else {
                        mState = eIllegal;
                        return aSourceLength;
                    }
                    break;
                case eWhiteend:
                    if (!XMLUtils::isWhitespace(c)) {
                        mState = eIllegal;
                        return aSourceLength;
                    }
                    break;
                default:
                    break;
            }
        }
        return aSourceLength;
    }

    double
    getDouble()
    {
        if (mState == eIllegal || mBuffer.IsEmpty() ||
            (mBuffer.Length() == 1 && mBuffer[0] == '.')) {
            return Double::NaN;
        }
        return mSign*PR_strtod(mBuffer.get(), 0);
    }
private:
    nsCAutoString mBuffer;
    enum {
        eWhitestart,
        eDecimal,
        eMantissa,
        eWhiteend,
        eIllegal
    } mState;
    enum {
        eNegative = -1,
        ePositive = 1
    } mSign;
};

double Double::toDouble(const nsAString& aSrc)
{
    txStringToDouble sink;
    nsAString::const_iterator fromBegin, fromEnd;
    copy_string(aSrc.BeginReading(fromBegin), aSrc.EndReading(fromEnd), sink);
    return sink.getDouble();
}

/*
 * Converts the value of the given double to a String, and places
 * The result into the destination String.
 * @return the given dest string
 */
void Double::toString(double aValue, nsAString& aDest)
{

    // check for special cases

    if (isNaN(aValue)) {
        aDest.Append(NS_LITERAL_STRING("NaN"));
        return;
    }
    if (isInfinite(aValue)) {
        if (aValue < 0)
            aDest.Append(PRUnichar('-'));
        aDest.Append(NS_LITERAL_STRING("Infinity"));
        return;
    }

    // Mantissa length is 17, so this is plenty
    const int buflen = 20;
    char buf[buflen];

    PRIntn intDigits, sign;
    char* endp;
    PR_dtoa(aValue, 0, 0, &intDigits, &sign, &endp, buf, buflen - 1);

    // compute length
    PRInt32 length = endp - buf;
    if (length > intDigits) {
        // decimal point needed
        ++length;
        if (intDigits < 1) {
            // leading zeros, -intDigits + 1
            length += 1 - intDigits;
        }
    }
    else {
        // trailing zeros, total length given by intDigits
        length = intDigits;
    }
    if (aValue < 0)
        ++length;
    PRUint32 oldlength = aDest.Length();
    aDest.SetLength(oldlength + length); // grow the string
    nsAString::iterator dest;
    aDest.BeginWriting(dest).advance(PRInt32(oldlength));
    if (aValue < 0) {
        *dest = '-'; ++dest;
    }
    int i;
    // leading zeros
    if (intDigits < 1) {
        *dest = '0'; ++dest;
        *dest = '.'; ++dest;
        for (i = 0; i > intDigits; --i) {
            *dest = '0'; ++dest;
        }
    }
    // mantissa
    int firstlen = PR_MIN(intDigits, endp - buf);
    for (i = 0; i < firstlen; i++) {
        *dest = buf[i]; ++dest;
    }
    if (i < endp - buf) {
        if (i > 0) {
            *dest = '.'; ++dest;
        }
        for (; i < endp - buf; i++) {
            *dest = buf[i]; ++dest;
        }
    }
    // trailing zeros
    for (; i < intDigits; i++) {
        *dest = '0'; ++dest;
    }
}
