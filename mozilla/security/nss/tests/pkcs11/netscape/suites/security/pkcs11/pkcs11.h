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
 * Copyright (C) 1994-2000 Netscape Communications Corporation.  All
 * Rights Reserved.
 * 
 * Contributor(s):
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
 */
#ifndef PKCS11_H
#define PKCS11_H

#define NULL_OP		0x00
#define ENCRYPT_OP	0x01
#define SIGN_OP		0x02
#define KEYGEN_OP	0x04
#define DIGEST_OP	0x08
#define HMAC_OP		0x10

typedef struct {
	CK_MECHANISM_TYPE	type;
	int			op;
	CK_MECHANISM_TYPE	keygenMech;
} MechInfo;

static int numMechs=118;
static MechInfo mechInfo[] = {
	{CKM_RSA_PKCS_KEY_PAIR_GEN,
				KEYGEN_OP,	CKM_RSA_PKCS_KEY_PAIR_GEN},
	{CKM_RSA_PKCS,		ENCRYPT_OP | SIGN_OP,
						CKM_RSA_PKCS_KEY_PAIR_GEN},
	{CKM_RSA_9796,		SIGN_OP,	CKM_RSA_PKCS_KEY_PAIR_GEN},
	{CKM_RSA_X_509,		ENCRYPT_OP | SIGN_OP,
						CKM_RSA_PKCS_KEY_PAIR_GEN},
	{CKM_MD2_RSA_PKCS,	SIGN_OP,	CKM_RSA_PKCS_KEY_PAIR_GEN},
	{CKM_MD5_RSA_PKCS,	SIGN_OP,	CKM_RSA_PKCS_KEY_PAIR_GEN},
	{CKM_SHA1_RSA_PKCS,	SIGN_OP,	CKM_RSA_PKCS_KEY_PAIR_GEN},
	{CKM_DSA_KEY_PAIR_GEN,	KEYGEN_OP,	CKM_DSA_KEY_PAIR_GEN},
	{CKM_DSA,		SIGN_OP,	CKM_DSA_KEY_PAIR_GEN},
	{CKM_DSA_SHA1,		SIGN_OP,	CKM_DSA_KEY_PAIR_GEN},
	{CKM_DH_PKCS_KEY_PAIR_GEN,
				KEYGEN_OP,	CKM_DH_PKCS_KEY_PAIR_GEN},
	{CKM_DH_PKCS_DERIVE,	NULL_OP,	0},
	{CKM_RC2_KEY_GEN,	KEYGEN_OP,	CKM_RC2_KEY_GEN},
	{CKM_RC2_ECB,		ENCRYPT_OP,	CKM_RC2_KEY_GEN},
	{CKM_RC2_CBC,		ENCRYPT_OP,	CKM_RC2_KEY_GEN},
	{CKM_RC2_MAC,		NULL_OP,	CKM_RC2_KEY_GEN},
	{CKM_RC2_MAC_GENERAL,	NULL_OP,	CKM_RC2_KEY_GEN},
	{CKM_RC2_CBC_PAD,	NULL_OP,	CKM_RC2_KEY_GEN},
	{CKM_RC4_KEY_GEN,	KEYGEN_OP,	CKM_RC4_KEY_GEN},
	{CKM_RC4,		ENCRYPT_OP,	CKM_RC4_KEY_GEN},
	{CKM_DES_KEY_GEN,	KEYGEN_OP,	CKM_DES_KEY_GEN},
	{CKM_DES_ECB,		ENCRYPT_OP,	CKM_DES_KEY_GEN},
	{CKM_DES_CBC,		ENCRYPT_OP,	CKM_DES_KEY_GEN},
	{CKM_DES_MAC,		NULL_OP,	CKM_DES_KEY_GEN},
	{CKM_DES_MAC_GENERAL,	NULL_OP,	CKM_DES_KEY_GEN},
	{CKM_DES_CBC_PAD,	NULL_OP,	CKM_DES_KEY_GEN},
	{CKM_DES2_KEY_GEN,	KEYGEN_OP,	CKM_DES2_KEY_GEN},
	{CKM_DES3_KEY_GEN,	KEYGEN_OP,	CKM_DES3_KEY_GEN},
	{CKM_DES3_ECB,		ENCRYPT_OP,	CKM_DES3_KEY_GEN},
	{CKM_DES3_CBC,		ENCRYPT_OP,	CKM_DES3_KEY_GEN},
	{CKM_DES3_MAC,		NULL_OP,	CKM_DES3_KEY_GEN},
	{CKM_DES3_MAC_GENERAL,	NULL_OP,	CKM_DES3_KEY_GEN},
	{CKM_DES3_CBC_PAD,	NULL_OP,	CKM_DES3_KEY_GEN},
	{CKM_CDMF_KEY_GEN,	KEYGEN_OP,	CKM_CDMF_KEY_GEN},
	{CKM_CDMF_ECB,		ENCRYPT_OP,	CKM_CDMF_KEY_GEN},
	{CKM_CDMF_CBC,		ENCRYPT_OP,	CKM_CDMF_KEY_GEN},
	{CKM_CDMF_MAC,		NULL_OP,	CKM_CDMF_KEY_GEN},
	{CKM_CDMF_MAC_GENERAL,	NULL_OP,	CKM_CDMF_KEY_GEN},
	{CKM_CDMF_CBC_PAD,	NULL_OP,	CKM_CDMF_KEY_GEN},
	{CKM_MD2,		DIGEST_OP,	0},
	{CKM_MD2_HMAC,		HMAC_OP,	0},
	{CKM_MD2_HMAC_GENERAL,	HMAC_OP,	0},
	{CKM_MD5,		DIGEST_OP,	0},
	{CKM_MD5_HMAC,		HMAC_OP,	0},
	{CKM_MD5_HMAC_GENERAL,	HMAC_OP,	0},
	{CKM_SHA_1,		DIGEST_OP,	0},
	{CKM_SHA_1_HMAC,	HMAC_OP,	0},
	{CKM_SHA_1_HMAC_GENERAL,HMAC_OP,	0},
	{CKM_CAST_KEY_GEN,	KEYGEN_OP,	CKM_CAST_KEY_GEN},
	{CKM_CAST_ECB,		ENCRYPT_OP,	CKM_CAST_KEY_GEN},
	{CKM_CAST_CBC,		ENCRYPT_OP,	CKM_CAST_KEY_GEN},
	{CKM_CAST_MAC,		NULL_OP,	CKM_CAST_KEY_GEN},
	{CKM_CAST_MAC_GENERAL,	NULL_OP,	CKM_CAST_KEY_GEN},
	{CKM_CAST_CBC_PAD,	NULL_OP,	CKM_CAST_KEY_GEN},
	{CKM_CAST3_KEY_GEN,	KEYGEN_OP,	CKM_CAST3_KEY_GEN},
	{CKM_CAST3_ECB,		ENCRYPT_OP,	CKM_CAST3_KEY_GEN},
	{CKM_CAST3_CBC,		ENCRYPT_OP,	CKM_CAST3_KEY_GEN},
	{CKM_CAST3_MAC,		NULL_OP,	CKM_CAST3_KEY_GEN},
	{CKM_CAST3_MAC_GENERAL,	NULL_OP,	CKM_CAST3_KEY_GEN},
	{CKM_CAST3_CBC_PAD,	NULL_OP,	CKM_CAST3_KEY_GEN},
	{CKM_CAST5_KEY_GEN,	KEYGEN_OP,	CKM_CAST5_KEY_GEN},
	{CKM_CAST5_ECB,		ENCRYPT_OP,	CKM_CAST5_KEY_GEN},
	{CKM_CAST5_CBC,		ENCRYPT_OP,	CKM_CAST5_KEY_GEN},
	{CKM_CAST5_MAC,		NULL_OP,	CKM_CAST5_KEY_GEN},
	{CKM_CAST5_MAC_GENERAL,	NULL_OP,	CKM_CAST5_KEY_GEN},
	{CKM_CAST5_CBC_PAD,	NULL_OP,	CKM_CAST5_KEY_GEN},
	{CKM_RC5_KEY_GEN,	KEYGEN_OP,	CKM_RC5_KEY_GEN},
	{CKM_RC5_ECB,		ENCRYPT_OP,	CKM_RC5_KEY_GEN},
	{CKM_RC5_CBC,		ENCRYPT_OP,	CKM_RC5_KEY_GEN},
	{CKM_RC5_MAC,		NULL_OP,	CKM_RC5_KEY_GEN},
	{CKM_RC5_MAC_GENERAL,	NULL_OP,	CKM_RC5_KEY_GEN},
	{CKM_RC5_CBC_PAD,	NULL_OP,	CKM_RC5_KEY_GEN},
	{CKM_IDEA_KEY_GEN,	KEYGEN_OP,	CKM_IDEA_KEY_GEN},
	{CKM_IDEA_ECB,		ENCRYPT_OP,	CKM_IDEA_KEY_GEN},
	{CKM_IDEA_CBC,		ENCRYPT_OP,	CKM_IDEA_KEY_GEN},
	{CKM_IDEA_MAC,		NULL_OP,	CKM_IDEA_KEY_GEN},
	{CKM_IDEA_MAC_GENERAL,	NULL_OP,	CKM_IDEA_KEY_GEN},
	{CKM_IDEA_CBC_PAD,	NULL_OP,	CKM_IDEA_KEY_GEN},
	{CKM_GENERIC_SECRET_KEY_GEN,
				KEYGEN_OP,	CKM_GENERIC_SECRET_KEY_GEN},
/* SSL mechanisms?
	{CKM_SSL3_PRE_MASTER_KEY_GEN},
	{CKM_SSL3_MASTER_KEY_DERIVE},
	{CKM_SSL3_KEY_AND_MAC_DERIVE},
	{CKM_SSL3_MD5_MAC},
	{CKM_SSL3_SHA1_MAC},
*/
	{CKM_PBE_MD2_DES_CBC,	KEYGEN_OP,	0},
	{CKM_PBE_MD5_DES_CBC,	KEYGEN_OP,	0},
	{CKM_PBE_MD5_CAST_CBC,	KEYGEN_OP,	0},
	{CKM_PBE_MD5_CAST3_CBC,	KEYGEN_OP,	0},
	{CKM_PBE_MD5_CAST5_CBC,	KEYGEN_OP,	0},
	{CKM_PBE_MD5_CAST128_CBC,
				KEYGEN_OP,	0},
	{CKM_PBE_SHA1_CAST5_CBC,KEYGEN_OP,	0},
	{CKM_PBE_SHA1_CAST128_CBC,
				KEYGEN_OP,	0},
	{CKM_PBE_SHA1_RC4_128,	KEYGEN_OP,	0},
	{CKM_PBE_SHA1_RC4_40,	KEYGEN_OP,	0},
	{CKM_PBE_SHA1_DES3_EDE_CBC,
				KEYGEN_OP,	0},
	{CKM_PBE_SHA1_DES2_EDE_CBC,
				KEYGEN_OP,	0},
	{CKM_PBE_SHA1_RC2_128_CBC,
				KEYGEN_OP,	0},
	{CKM_PBE_SHA1_RC2_40_CBC,
				KEYGEN_OP,	0},
	{CKM_PBA_SHA1_WITH_SHA1_HMAC,
				KEYGEN_OP,	0},
	{CKM_SKIPJACK_KEY_GEN,	KEYGEN_OP,	CKM_SKIPJACK_KEY_GEN},
	{CKM_SKIPJACK_ECB64,	ENCRYPT_OP,	CKM_SKIPJACK_KEY_GEN},
	{CKM_SKIPJACK_CBC64,	ENCRYPT_OP,	CKM_SKIPJACK_KEY_GEN},
	{CKM_SKIPJACK_OFB64,	ENCRYPT_OP,	CKM_SKIPJACK_KEY_GEN},
	{CKM_SKIPJACK_CFB64,	ENCRYPT_OP,	CKM_SKIPJACK_KEY_GEN},
	{CKM_SKIPJACK_CFB32,	ENCRYPT_OP,	CKM_SKIPJACK_KEY_GEN},
	{CKM_SKIPJACK_CFB16,	ENCRYPT_OP,	CKM_SKIPJACK_KEY_GEN},
	{CKM_SKIPJACK_CFB8,	ENCRYPT_OP,	CKM_SKIPJACK_KEY_GEN},
	{CKM_KEA_KEY_PAIR_GEN,	KEYGEN_OP,	0},
	{CKM_BATON_KEY_GEN,	KEYGEN_OP,	CKM_BATON_KEY_GEN},
	{CKM_BATON_ECB128,	ENCRYPT_OP,	CKM_BATON_KEY_GEN},
	{CKM_BATON_ECB96,	ENCRYPT_OP,	CKM_BATON_KEY_GEN},
	{CKM_BATON_CBC128,	ENCRYPT_OP,	CKM_BATON_KEY_GEN},
	{CKM_BATON_COUNTER,	ENCRYPT_OP,	CKM_BATON_KEY_GEN},
	{CKM_BATON_SHUFFLE,	ENCRYPT_OP,	CKM_BATON_KEY_GEN},
	{CKM_ECDSA_KEY_PAIR_GEN,KEYGEN_OP,	CKM_ECDSA_KEY_PAIR_GEN},
	{CKM_ECDSA,		SIGN_OP,	CKM_ECDSA_KEY_PAIR_GEN},
	{CKM_ECDSA_SHA1,	SIGN_OP,	CKM_ECDSA_KEY_PAIR_GEN},
	{CKM_JUNIPER_KEY_GEN,	KEYGEN_OP,	CKM_JUNIPER_KEY_GEN},
	{CKM_JUNIPER_ECB128,	ENCRYPT_OP,	CKM_JUNIPER_KEY_GEN},
	{CKM_JUNIPER_CBC128,	ENCRYPT_OP,	CKM_JUNIPER_KEY_GEN},
	{CKM_JUNIPER_COUNTER,	ENCRYPT_OP,	CKM_JUNIPER_KEY_GEN},
	{CKM_JUNIPER_SHUFFLE,	ENCRYPT_OP,	CKM_JUNIPER_KEY_GEN},
	{CKM_FASTHASH,		DIGEST_OP,	0}
};

#endif
