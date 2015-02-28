/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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
 * Copyright (C) 2001 Netscape Communications Corporation.
 * All Rights Reserved.
 * 
 * Contributor(s):
 *   Stuart Parmenter <pavlov@netscape.com>
 */

#ifndef nsJPEGEncoder_h_
#define nsJPEGEncoder_h_

#include "imgIEncoder.h"
#include "nsIOutputStream.h"

#include <ole2.h> // stgmedium

#include "nsCOMPtr.h"

extern "C" {
#include "jpeglib.h"
}

#include <setjmp.h>

#define NS_JPEGENCODER_CID \
{ /* 13784154-1C73-4635-B75F-F1F21755E1C1 */         \
     0x13784154,                                     \
     0x1c73,                                         \
     0x4635,                                         \
    {0xb7, 0x5f, 0xf1, 0xf2, 0x17, 0x55, 0xe1, 0xc1} \
}

// helper structs for interacting with the jpeg library
typedef struct {
    struct jpeg_error_mgr pub;  /* "public" fields for IJG library*/
    jmp_buf setjmp_buffer;      /* For handling catastropic errors */
} encoder_error_mgr;


class nsJPEGEncoder : public imgIEncoder
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_IMGIENCODER

  nsJPEGEncoder();
  virtual ~nsJPEGEncoder();

public:
  encoder_error_mgr mErr;
  struct jpeg_compress_struct mInfo;

  JOCTET *mBuffer;      // temporary buffer the jpeg library writes the compressed data to before we write it out to disk
  PRUint32 mBufferLen;  // amount of data currently in mBuffer
  PRUint32 mBufferSize; // size in bytes what mBuffer was created with
  nsCOMPtr<nsIOutputStream> mOutputStream;

protected:
  nsresult initCompressionInfo(); 
};

#endif // nsJPEGEncoder_h_
