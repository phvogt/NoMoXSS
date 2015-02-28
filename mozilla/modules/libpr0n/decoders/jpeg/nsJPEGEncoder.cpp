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
 *   Scott MacGregor <mscott@mozilla.org>
 *
 */

#ifdef MOZ_LOGGING
// sorry, this has to be before the pre-compiled header
#define FORCE_PR_LOG /* Allow logging in the release build */
#endif

#include <stdio.h>
#include "nsJPEGEncoder.h"
#include "nsIClipboard.h"
#include "nsIComponentManager.h"
#include "nspr.h"
#include "nsCRT.h"
#include "ImageLogging.h"
#include "jerror.h"

#include "nsILocalFile.h"
#include "nsIOutputStream.h"
#include "nsNetUtil.h"
#include "nsDirectoryServiceDefs.h"

PRLogModuleInfo *gJPEGEncoderLog = PR_NewLogModule("JPEGEncoder");

#define OUTPUT_BUF_SIZE  4096	/* choose an efficiently fwrite'able size */

NS_IMPL_ISUPPORTS1(nsJPEGEncoder, imgIEncoder)

METHODDEF(boolean) empty_output_buffer (j_compress_ptr jd);
METHODDEF(void) init_destination (j_compress_ptr jd);
METHODDEF(void) term_destination (j_compress_ptr cinfo);
METHODDEF(void) my_error_exit (j_common_ptr cinfo);

// helper declarations for converting a bitmap to 4 byte RGB data

nsresult ConvertColorBitMap(unsigned char * aBitmapBuffer, PBITMAPINFO pBitMapInfo, unsigned char * aOutputBuffer);nsJPEGEncoder * getEncoderForCompressionInfoStruct(jpeg_compress_struct * cInfo);

struct bitFields {
    PRUint32 red;
    PRUint32 green;
    PRUint32 blue;
    PRUint8 redLeftShift;
    PRUint8 redRightShift;
    PRUint8 greenLeftShift;
    PRUint8 greenRightShift;
    PRUint8 blueLeftShift;
    PRUint8 blueRightShift;
};

void CalcBitShift(bitFields * aColorMask);

/*
 *  Implementation of a JPEG destination manager object 
 */
typedef struct {
  /* public fields; must be first in this struct! */
  struct jpeg_destination_mgr pub;

  nsJPEGEncoder *encoder; // weak reference

} encoder_destination_mgr;

nsJPEGEncoder::nsJPEGEncoder()
{
  mBuffer = nsnull;
  mBufferLen = mBufferSize = 0;
  memset(&mInfo, 0, sizeof(jpeg_compress_struct));
}

nsJPEGEncoder::~nsJPEGEncoder()
{
}

nsresult nsJPEGEncoder::initCompressionInfo()
{
  /* We set up the normal JPEG error routines, then override error_exit. */
  mInfo.err = jpeg_std_error(&mErr.pub);
  mErr.pub.error_exit = my_error_exit;

  /* Establish the setjmp return context for my_error_exit to use. */
  if (setjmp(mErr.setjmp_buffer)) 
  {
    /* If we get here, the JPEG code has signaled an error.
     * We need to clean up the JPEG object, close the input file, and return.
     */
    return NS_ERROR_FAILURE;
  }

  jpeg_create_compress(&mInfo);

  encoder_destination_mgr * destinationManager;
  
  // first, setup the destination manager
  if (mInfo.dest == NULL) 
  {
    destinationManager = PR_NEWZAP(encoder_destination_mgr);
    if (!destinationManager) 
      return NS_ERROR_OUT_OF_MEMORY;

    mInfo.dest = NS_REINTERPRET_CAST(struct jpeg_destination_mgr *, destinationManager);
  }

  /* Setup callback functions. */
  destinationManager->pub.init_destination  = init_destination ;
  destinationManager->pub.empty_output_buffer = empty_output_buffer;
  destinationManager->pub.term_destination = term_destination;

  destinationManager->encoder = this; // note the lack of a reference here. We own the lifetime of the destinationManager object

  return NS_OK;
}

nsJPEGEncoder * getEncoderForCompressionInfoStruct(jpeg_compress_struct * cInfo)
{
  if (cInfo && cInfo->dest)
  {
    encoder_destination_mgr *destinationManager = NS_REINTERPRET_CAST(encoder_destination_mgr *, cInfo->dest);
    return destinationManager->encoder;
  }

  return NULL;
}


/** imgIEncoder methods **/

NS_IMETHODIMP nsJPEGEncoder::EncodeClipboardImage(nsIClipboardImage * aClipboardImage, nsIFile ** aImageFile)
{
  // this is windows only....
  STGMEDIUM stm;
  nsresult rv = aClipboardImage->GetNativeImage( (void *) &stm);

  if (NS_FAILED(rv)) 
    return rv;

  // test...try writing the bitmap to a file
  nsCOMPtr<nsIFile> fileToUse;
  nsAutoString fileName;
  fileName.AssignWithConversion("moz-screenshot.jpg");
  NS_GetSpecialDirectory(NS_OS_TEMP_DIR, getter_AddRefs(fileToUse));

  fileToUse->Append(fileName);
  nsCOMPtr<nsILocalFile> path = do_QueryInterface(fileToUse);
  path->CreateUnique(nsIFile::NORMAL_FILE_TYPE, 0600);

  rv = NS_NewLocalFileOutputStream(getter_AddRefs(mOutputStream), path);

  if (NS_FAILED(rv))
    return rv;

  // FROM HERE ON OUT, it is critical that we do not return early in this routine.
  // we must release our lock and release stm before exiting
  HGLOBAL hGlobal = stm.hGlobal;
  BYTE  * pGlobal = (BYTE  *)::GlobalLock (hGlobal) ;
  
  BITMAPINFO * bitMapInfo = (BITMAPINFO *) pGlobal;

  rv = initCompressionInfo();
  if (NS_FAILED(rv))
    return rv;

  mInfo.in_color_space = JCS_RGB;
  mInfo.input_components = 3;
  mInfo.data_precision = 8;
  mInfo.image_width = (JDIMENSION) bitMapInfo->bmiHeader.biWidth;
  mInfo.image_height = (JDIMENSION) bitMapInfo->bmiHeader.biHeight;

  unsigned char * rgbData = (unsigned char *) malloc (bitMapInfo->bmiHeader.biWidth * bitMapInfo->bmiHeader.biHeight * 3 /* RGB */);

  if (rgbData)
  {
    rv = ConvertColorBitMap((unsigned char *) (pGlobal + bitMapInfo->bmiHeader.biSize), bitMapInfo, rgbData);
    if (NS_SUCCEEDED(rv))
    {
      jpeg_set_defaults(&mInfo);

      if ( bitMapInfo->bmiHeader.biXPelsPerMeter > 0 &&  bitMapInfo->bmiHeader.biYPelsPerMeter > 0) 
      {
        /* Set JFIF density parameters from the BMP data */
        mInfo.X_density = (UINT16) ( bitMapInfo->bmiHeader.biXPelsPerMeter/100); /* 100 cm per meter */
        mInfo.Y_density = (UINT16) ( bitMapInfo->bmiHeader.biYPelsPerMeter/100);
        mInfo.density_unit = 2;	/* dots/cm */
      }

      jpeg_start_compress(&mInfo, TRUE);

      PRInt32 row_stride = bitMapInfo->bmiHeader.biWidth * 3;
      JSAMPROW row_pointer[1];

      while (mInfo.next_scanline < mInfo.image_height) 
      {
        row_pointer[0] = &rgbData[mInfo.next_scanline * row_stride];
        jpeg_write_scanlines(&mInfo, row_pointer, 1);
      }

      jpeg_finish_compress(&mInfo);
    }

    free(rgbData);
  } // if rgbdata

  jpeg_destroy_compress(&mInfo);

  // close the output file stream. we are done with it
  mOutputStream->Close();

  // return the file URL to the JPG
  NS_IF_ADDREF(*aImageFile = fileToUse);

  ::GlobalUnlock (hGlobal);  // release our lock on the bitmap 

  aClipboardImage->ReleaseNativeImage( (void *) &stm);

  return NS_OK;
}

void InvertRows(unsigned char * aInitialBuffer, PRUint32 sizeOfBuffer, PRUint32 numBytesPerRow)
{
  if (!numBytesPerRow) return; 

  PRUint32 numRows = sizeOfBuffer / numBytesPerRow;
  void * temporaryRowHolder = (void *) nsMemory::Alloc(numBytesPerRow);

  PRUint32 currentRow = 0;
  PRUint32 lastRow = (numRows - 1) * numBytesPerRow;
  while (currentRow < lastRow)
  {
    // store the current row into a temporary buffer
    memcpy(temporaryRowHolder, (void *) &aInitialBuffer[currentRow], numBytesPerRow);
    memcpy((void *) &aInitialBuffer[currentRow], (void *)&aInitialBuffer[lastRow], numBytesPerRow);
    memcpy((void *) &aInitialBuffer[lastRow], temporaryRowHolder, numBytesPerRow);
    lastRow -= numBytesPerRow;
    currentRow += numBytesPerRow;
  }

  nsMemory::Free(temporaryRowHolder);
}

nsresult ConvertColorBitMap(unsigned char * buffer, PBITMAPINFO pBitMapInfo, unsigned char * outBuffer)
{
  PR_LOG(gJPEGEncoderLog, PR_LOG_ALWAYS, ("nsJPEGEncoder::ConvertColorBitMap"));

  PRUint8 bitCount = pBitMapInfo->bmiHeader.biBitCount; 
  PRUint32 imageSize = pBitMapInfo->bmiHeader.biSizeImage; // may be zero for BI_RGB bitmaps which means we need to calculate by hand
  PRUint32 bytesPerPixel = bitCount / 8;
  
  if (bitCount <= 4)
    bytesPerPixel = 1;

  // rows are DWORD aligned. Calculate how many real bytes are in each row in the bitmap. This number won't 
  // correspond to biWidth.
  PRUint32 rowSize = (bitCount * pBitMapInfo->bmiHeader.biWidth + 7) / 8; // +7 to round up
  if (rowSize % 4)
    rowSize += (4 - (rowSize % 4)); // Pad to DWORD Boundary
  
  // if our buffer includes a color map, skip over it 
  if (bitCount <= 8)
  {
    PRInt32 bytesToSkip = (pBitMapInfo->bmiHeader.biClrUsed ? pBitMapInfo->bmiHeader.biClrUsed : (1 << bitCount) ) * sizeof(RGBQUAD);
    buffer +=  bytesToSkip;
  }

  bitFields colorMasks; // only used if biCompression == BI_BITFIELDS

  if (pBitMapInfo->bmiHeader.biCompression == BI_BITFIELDS)
  {
    // color table consists of 3 DWORDS containing the color masks...
    colorMasks.red = (*((PRUint32*)&(pBitMapInfo->bmiColors[0]))); 
    colorMasks.green = (*((PRUint32*)&(pBitMapInfo->bmiColors[1]))); 
    colorMasks.blue = (*((PRUint32*)&(pBitMapInfo->bmiColors[2]))); 
    CalcBitShift(&colorMasks);
    buffer += 3 * sizeof(DWORD);
  } else if (pBitMapInfo->bmiHeader.biCompression == BI_RGB && !imageSize)  // BI_RGB can have a size of zero which means we figure it out
  {
    // XXX: note use rowSize here and not biWidth. rowSize accounts for the DWORD padding for each row
    imageSize = rowSize * pBitMapInfo->bmiHeader.biHeight;
  }

  // dump out some log information about the bit map info struct
  PR_LOG(gJPEGEncoderLog, PR_LOG_ALWAYS, ("nsJPEGEncoder::ConvertColorBitMap biBitCount: %u", bitCount));
  PR_LOG(gJPEGEncoderLog, PR_LOG_ALWAYS, ("nsJPEGEncoder::ConvertColorBitMap rowSize: %u", rowSize));
  PR_LOG(gJPEGEncoderLog, PR_LOG_ALWAYS, ("nsJPEGEncoder::ConvertColorBitMap biCompression: %u", pBitMapInfo->bmiHeader.biCompression));
  PR_LOG(gJPEGEncoderLog, PR_LOG_ALWAYS, ("nsJPEGEncoder::ConvertColorBitMap biWidth: %u", pBitMapInfo->bmiHeader.biWidth));
  PR_LOG(gJPEGEncoderLog, PR_LOG_ALWAYS, ("nsJPEGEncoder::ConvertColorBitMap biHeight: %u", pBitMapInfo->bmiHeader.biHeight));
  PR_LOG(gJPEGEncoderLog, PR_LOG_ALWAYS, ("nsJPEGEncoder::ConvertColorBitMap biClrUsed: %u", pBitMapInfo->bmiHeader.biClrUsed));

  InvertRows(buffer, imageSize, rowSize);

  PR_LOG(gJPEGEncoderLog, PR_LOG_ALWAYS, ("nsJPEGEncoder::ConvertColorBitMap bytesPerPixel: %u", bytesPerPixel));
  PR_LOG(gJPEGEncoderLog, PR_LOG_ALWAYS, ("nsJPEGEncoder::ConvertColorBitMap calculated imageSize: %u", imageSize));
  PR_LOG(gJPEGEncoderLog, PR_LOG_ALWAYS, ("nsJPEGEncoder::ConvertColorBitMap biSizeImage: %u", pBitMapInfo->bmiHeader.biSizeImage));

  if (!pBitMapInfo->bmiHeader.biCompression || pBitMapInfo->bmiHeader.biCompression == BI_BITFIELDS) 
  {  
    PRUint32 index = 0;
    PRUint32 writeIndex = 0;
     
    unsigned char redValue, greenValue, blueValue;
    PRUint8 colorTableEntry = 0;
    PRInt8 bit; // used for grayscale bitmaps where each bit is a pixel
    PRUint32 numPixelsLeftInRow = pBitMapInfo->bmiHeader.biWidth; // how many more pixels do we still need to read for the current row
    PRUint32 pos = 0;

    while (index < imageSize)
    {
      switch (bitCount) 
      {
        case 1:
          for (bit = 7; bit >= 0 && numPixelsLeftInRow; bit--)
          {
            colorTableEntry = (buffer[index] >> bit) & 1;
            outBuffer[writeIndex++] = pBitMapInfo->bmiColors[colorTableEntry].rgbRed;
            outBuffer[writeIndex++] = pBitMapInfo->bmiColors[colorTableEntry].rgbGreen;
            outBuffer[writeIndex++] = pBitMapInfo->bmiColors[colorTableEntry].rgbBlue;
            numPixelsLeftInRow--;
          }
          pos += 1;
          break;
        case 4:
          {
            // each buffer[index] entry contains data for two pixels.
            // read the first pixel
            colorTableEntry = buffer[index] >> 4;
            outBuffer[writeIndex++] = pBitMapInfo->bmiColors[colorTableEntry].rgbRed;
            outBuffer[writeIndex++] = pBitMapInfo->bmiColors[colorTableEntry].rgbGreen;
            outBuffer[writeIndex++] = pBitMapInfo->bmiColors[colorTableEntry].rgbBlue;
            numPixelsLeftInRow--;

            if (numPixelsLeftInRow) // now read the second pixel
            {
              colorTableEntry = buffer[index] & 0xF;
              outBuffer[writeIndex++] = pBitMapInfo->bmiColors[colorTableEntry].rgbRed;
              outBuffer[writeIndex++] = pBitMapInfo->bmiColors[colorTableEntry].rgbGreen;
              outBuffer[writeIndex++] = pBitMapInfo->bmiColors[colorTableEntry].rgbBlue;
              numPixelsLeftInRow--;
            }
            pos += 1;
          }
          break;
        case 8:
          outBuffer[writeIndex++] = pBitMapInfo->bmiColors[buffer[index]].rgbRed;
          outBuffer[writeIndex++] = pBitMapInfo->bmiColors[buffer[index]].rgbGreen;
          outBuffer[writeIndex++] = pBitMapInfo->bmiColors[buffer[index]].rgbBlue;
          numPixelsLeftInRow--;
          pos += 1;    
          break;
        case 16:
          {
            PRUint16 num = 0;
            num = (PRUint8) buffer[index+1];
            num <<= 8;
            num |= (PRUint8) buffer[index];

            redValue = ((PRUint32) (((float)(num & 0xf800) / 0xf800) * 0xFF0000) & 0xFF0000)>> 16;
            greenValue =  ((PRUint32)(((float)(num & 0x07E0) / 0x07E0) * 0x00FF00) & 0x00FF00)>> 8;
            blueValue =  ((PRUint32)(((float)(num & 0x001F) / 0x001F) * 0x0000FF) & 0x0000FF);

            // now we have the right RGB values...
            outBuffer[writeIndex++] = redValue;
            outBuffer[writeIndex++] = greenValue;
            outBuffer[writeIndex++] = blueValue;        
            numPixelsLeftInRow--;
            pos += 2;          
          }
          break;
        case 32:
        case 24:
          if (pBitMapInfo->bmiHeader.biCompression == BI_BITFIELDS)
          {
            PRUint32 val = *((PRUint32*) (buffer + index) );
            outBuffer[writeIndex++] = (val & colorMasks.red) >> colorMasks.redRightShift << colorMasks.redLeftShift;
            outBuffer[writeIndex++] =  (val & colorMasks.green) >> colorMasks.greenRightShift << colorMasks.greenLeftShift;
            outBuffer[writeIndex++] = (val & colorMasks.blue) >> colorMasks.blueRightShift << colorMasks.blueLeftShift;
            numPixelsLeftInRow--;
            pos += 4; // we read in 4 bytes of data in order to process this pixel
          }
          else
          {
            outBuffer[writeIndex++] = buffer[index+2];
            outBuffer[writeIndex++] =  buffer[index+1];
            outBuffer[writeIndex++] = buffer[index];
            numPixelsLeftInRow--;
            pos += bytesPerPixel; // 3 bytes for 24 bit data, 4 bytes for 32 bit data (we skip over the 4th byte)...
          }
          break;
        default:
          // This is probably the wrong place to check this...
          return NS_ERROR_FAILURE;
      }

      index += bytesPerPixel; // increment our loop counter

      if (!numPixelsLeftInRow)
      {
        if (rowSize != pos)
        {
          // advance index to skip over remaining padding bytes
          index += (rowSize - pos);

        }
        numPixelsLeftInRow = pBitMapInfo->bmiHeader.biWidth;
        pos = 0; 
      }

    } // while we still have bytes to process
  }

  PR_LOG(gJPEGEncoderLog, PR_LOG_ALWAYS, ("exiting nsJPEGEncoder::ConvertColorBitMap"));

  return NS_OK;
}

static void calcBitmask(PRUint32 aMask, PRUint8& aBegin, PRUint8& aLength)
{
    // find the rightmost 1
    PRUint8 pos;
    PRBool started = PR_FALSE;
    aBegin = aLength = 0;
    for (pos = 0; pos <= 31; pos++) {
        if (!started && (aMask & (1 << pos))) {
            aBegin = pos;
            started = PR_TRUE;
        }
        else if (started && !(aMask & (1 << pos))) {
            aLength = pos - aBegin;
            break;
        }
    }
}

void CalcBitShift(bitFields * aColorMask)
{
    PRUint8 begin, length;
    // red
    calcBitmask(aColorMask->red, begin, length);
    aColorMask->redRightShift = begin;
    aColorMask->redLeftShift = 8 - length;
    // green
    calcBitmask(aColorMask->green, begin, length);
    aColorMask->greenRightShift = begin;
    aColorMask->greenLeftShift = 8 - length;
    // blue
    calcBitmask(aColorMask->blue, begin, length);
    aColorMask->blueRightShift = begin;
    aColorMask->blueLeftShift = 8 - length;
}

/******************************************************************************/
/* data destination manager method 
        Initialize the destination buffer the JPEG library should write compressed bits into
*/
METHODDEF(void)
init_destination  (j_compress_ptr jd)
{
  nsJPEGEncoder * encoder = getEncoderForCompressionInfoStruct(jd);
  if (!encoder) 
    return;

  if (!encoder->mBuffer)
  {
    encoder->mBuffer = (JOCTET *)PR_Malloc(OUTPUT_BUF_SIZE);
    encoder->mBufferSize = OUTPUT_BUF_SIZE;
  }

  jd->dest->next_output_byte = encoder->mBuffer;
  jd->dest->free_in_buffer = OUTPUT_BUF_SIZE;

}

// called by the JPEG library when our working output buffer is full. We need to take the bits and write
// them to our destination stream. return true if buffer was dumped successfully otherwise return 
METHODDEF(boolean) empty_output_buffer (j_compress_ptr cinfo)
{
  nsJPEGEncoder * encoder = getEncoderForCompressionInfoStruct(cinfo);
  if (!encoder) 
    return PR_TRUE;
  
  PRUint32 written;
  encoder->mOutputStream->Write((const char*) encoder->mBuffer, encoder->mBufferSize, &written);

  // XXX: what do we do if we did write the # of bytes we thought we should write?

  // now reset the jpeg pointers to the start of the buffer
  cinfo->dest->next_output_byte = encoder->mBuffer;
  cinfo->dest->free_in_buffer = OUTPUT_BUF_SIZE;

  return PR_TRUE;
}


/* Terminate destination --- called by jpeg_finish_compress()() after all
 * data has been written to clean up JPEG destination manager. NOT called by 
 * jpeg_abort() or jpeg_destroy().
*/
METHODDEF(void) term_destination (j_compress_ptr cinfo)
{
  nsJPEGEncoder * encoder = getEncoderForCompressionInfoStruct(cinfo);
  if (!encoder) 
    return;

  // flush whatever is left in the buffer
  PRUint32 written;
  encoder->mOutputStream->Write((const char*) encoder->mBuffer, encoder->mBufferSize - cinfo->dest->free_in_buffer , &written);  

  // if we were an asynch process making calls back to an observer, we would 
  // make the notification saying we were all done in this method. Since we are blocking, 
  // do nothing.
}

/* Override the standard error method in the IJG JPEG decoder code. 
 */
METHODDEF(void) my_error_exit (j_common_ptr cinfo)
{
  nsresult error_code;
  encoder_error_mgr *err = (encoder_error_mgr *) cinfo->err;

  /* Convert error to a browser error code */
  switch (cinfo->err->msg_code) {
  case JERR_OUT_OF_MEMORY:
      error_code = NS_ERROR_OUT_OF_MEMORY;
  default:
      error_code = NS_ERROR_FAILURE;
  }

#ifdef DEBUG
  char buffer[JMSG_LENGTH_MAX];

  /* Create the message */
  (*cinfo->err->format_message) (cinfo, buffer);

  fprintf(stderr, "JPEG decoding error:\n%s\n", buffer);
#endif

  /* Return control to the setjmp point. */
  longjmp(err->setjmp_buffer, error_code);
}