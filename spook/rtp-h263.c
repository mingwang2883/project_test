/*
 * Copyright (C) 2004 Nathan Lutchansky <lutchann@litech.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <errno.h>
#include <event.h>
#include <log.h>
#include <frame.h>
#include <stream.h>
#include <outputs.h>
#include <rtp.h>
#include <rtp_media.h>
#include <conf_parse.h>

#define NALBUF_INITIAL_SIZE     4096
#define NALBUF_BLOCK_SIZE       512
#define MAX_RTP_PKT_LENGTH      1400

#define JK_H264_LOAD_NAL_OK     0
#define JK_H264DEC_ERR_BADBS    0xa1218002        // bitstream is corrupted
#define MAX_VIDEO_FRAM_SIZE                 (1024 * 300)
#define H264                    96

struct rtp_h263
{
	unsigned char *d;
	int len;
	int init_done;
	int ts_incr;
	unsigned int timestamp;
};

typedef struct JK_H264_BITSTREAM_S
{
    // global bitstream buffer.
    unsigned char *pStart;     // start bitstream ptr
    unsigned char *pEnd;       // end bitstream ptr
    unsigned char *pBuffer;    // buffer pointer from which we seek RBSP
    // rbsp/access unit layer
    unsigned char *pStream;    // rbsp/access unit start pointer
    int           iStreamLen;  // length of rbsp data/access unit
    int           iUsed;       // count of bytes to extract an rbsp/frame
} H264_BITSTREAM_S;

typedef struct
{
    /**//* byte 0 */
    unsigned char u4CSrcLen:4;		/**//* expect 0 */
    unsigned char u1Externsion:1;	/**//* expect 1, see RTP_OP below */
    unsigned char u1Padding:1;		/**//* expect 0 */
    unsigned char u2Version:2;		/**//* expect 2 */
    /**//* byte 1 */
    unsigned char u7Payload:7;		/**//* RTP_PAYLOAD_RTSP */
    unsigned char u1Marker:1;		/**//* expect 1 */
    /**//* bytes 2, 3 */
    unsigned short u16SeqNum;
    /**//* bytes 4-7 */
    unsigned long u32TimeStamp;
    /**//* bytes 8-11 */
    unsigned long u32SSrc;			/**//* stream number is used here. */
}StRtpFixedHdr;

typedef struct{
    //byte 0
    unsigned char u5Type:5;
    unsigned char u2Nri:2;
    unsigned char u1F:1;
}StNaluHdr; /**//* 1 BYTES */

typedef struct
{
    //byte 0
    unsigned char u5Type:5;
    unsigned char u2Nri:2;
    unsigned char u1F:1;
}StFuIndicator; /**//* 1 BYTES */

typedef struct
{
    //byte 0
    unsigned char u5Type:5;
    unsigned char u1R:1;
    unsigned char u1E:1;
    unsigned char u1S:1;
}StFuHdr; /**//* 1 BYTES */

typedef struct _tagStRtpHandle
{
    unsigned short      u16SeqNum;
    unsigned int        u32TimeStampInc;
    unsigned int        u32TimeStampCurr;
    unsigned int		u32CurrTime;
    unsigned int		u32PrevTime;
    unsigned int		u32SSrc;
    StRtpFixedHdr		*pRtpFixedHdr;
    StNaluHdr		    *pNaluHdr;
    StFuIndicator	    *pFuInd;
    StFuHdr				*pFuHdr;

}StRtpObj, *HndRtp;

typedef struct _byteStream_s
{
  FILE *fn;                     /* Bitstream file pointer */

  long bytesRead;               /* The number of bytes read from the file */
  unsigned char *bitbuf;        /* Buffer for stream bits */
  int bitbufLen;                /* Size of the bit buffer in bytes */
  int bitbufDataPos;
  int bitbufDataLen;            /* Length of all the data in the bit buffer in bytes */

  unsigned char *bitbufNalunit; /* Pointer to first NAL unit in the bitbuffer */
  int bitbufNalunitLen;         /* Length of the first NAL unit in the bit buffer in bytes,
                                   including variable size start code, nal head byte and the
                                   RBSP payload. It will help to find the RBSP length. */
} byteStream_s;

byteStream_s strByte;

static int GetSpsAndPpsFromStream(const char * buf, char * sps, int * spsSzie, char * pps, int * ppsSize);

static unsigned char b_Init = 0; // 是否获取了PPS,SPS的标志
static char spsArray[50];
static char ppsArray[50];
static char sps_pps_base64[256];
static int fSPSSize, fPPSSize;
static const char base64Char[] ="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static char *m_ph264DecodeBuf,  m_ph264BSBuf[MAX_VIDEO_FRAM_SIZE];

/*
 *
 * readBytesFromFile:
 *
 * Parameters:
 *      str                   Bitbuffer object
 *
 * Function:
 *      Read bytes from the bistream file
 *
 * Returns:
 *      Number of bytes read
 *
 */
static int readBytesFromFile(byteStream_s *str)
{
  int n;

  if (str->bitbufLen - str->bitbufDataLen < NALBUF_BLOCK_SIZE) {

    /* Buffer is too small -> allocate bigger buffer */
    str->bitbuf = (unsigned char *)realloc(str->bitbuf, str->bitbufLen+NALBUF_BLOCK_SIZE);

    if (str->bitbuf == NULL) {
      fprintf(stderr, "Cannot resize bitbuffer\n");
      printf("something wrong happend, exit now###\n");
    }
    str->bitbufLen += NALBUF_BLOCK_SIZE;
  }

  /* Read block of data */
  n = (int)fread(str->bitbuf + str->bitbufDataLen, 1, NALBUF_BLOCK_SIZE, str->fn);

  str->bytesRead     += n;
  str->bitbufDataLen += n;

  return n;
}

/*
 *
 * findStartCode:
 *
 * Parameters:
 *      ptr                   Pointer to byte stream
 *      len                   Length of byte stream
 *      offset                Search start position
 *      startCodeLen          Return pointer for start code langth
 *
 * Function:
 *      First next start code in byte stream
 *
 * Returns:
 *      Start code position if found or length of byte stream if
 *      start code was not found
 *
 */
static int findStartCode_2(unsigned char *ptr, int len, int offset, int *startCodeLen)
{
    int numZeros = 0;

    while (offset < len)
    {
        if (ptr[offset] == 0)
        {
            numZeros++;
        }
        else if (ptr[offset] == 1 && numZeros > 1)
        {
            *startCodeLen = numZeros + 1;
            return offset - numZeros;
        }
        else
            numZeros = 0;

        offset++;
    }

    *startCodeLen = 0;	// can not find startcode

    return len;
}

/************************************************
* Function Name  : JK_BitStream_init
* Description    : Initialize stream
* Parameters     : bs        stream, refer to H264_BITSTREAM_S
*                : pBitstream    H.264 stream data buffer address
*                : szBitstream   H.264 stream data length in byte
*                : pInStream   buffer to store NALU/AU extracted from H.264
*                    stream,  meanwhile, input buffer to H.264 decoder
* Return Type     : NULL
* Last Modified   : 2013-05-12
************************************************/
void JK_BitStream_init_FM( H264_BITSTREAM_S *bs, unsigned char *pBitstream,
                            unsigned int iBitstreamLen, unsigned char* pInStream)
{
    bs->pStart      = pBitstream;
    bs->pBuffer     = pBitstream;
    bs->pEnd        = pBitstream + iBitstreamLen;
    bs->iStreamLen  = 0;
    bs->iUsed       = 0;
    bs->pStream     = pInStream;
}

/************************************************
* Function Name  : JK_BitStreamLoadNALU
* Description    : Read a nalu from stream
* Parameters     : bs        H.264 stream, refer to H264_BITSTREAM_S
* Return Type    : int		 JK_H264DEC_OK - success, HI_H264DEC_ERR_MBITS - fail
* Last Modified  : 2013-05-12
************************************************/
int JK_BitStreamLoadNALU_FM(H264_BITSTREAM_S *pBs)
{
    int start_pos_1, start_pos_2;
    int offset;
    int startCodeLen;
    int result = JK_H264DEC_ERR_BADBS;

    pBs->iUsed      = 0;
    pBs->iStreamLen = pBs->pEnd - pBs->pStart;

    start_pos_1 = start_pos_2 = 0;
    offset		 = 0;
    startCodeLen = 0;

    // 找到第一个起始码
    start_pos_1 = findStartCode_2(pBs->pBuffer, pBs->iStreamLen, offset, &startCodeLen);
    if(startCodeLen) {	// 开始找第二个起始码
        offset = startCodeLen;
        startCodeLen = 0;
        start_pos_2 = findStartCode_2(pBs->pBuffer + startCodeLen, pBs->iStreamLen,
                                offset, &startCodeLen);

        //找到了第二个起始码
        if(startCodeLen) {
            pBs->iUsed = start_pos_2 - start_pos_1;
        }
        else {	// 只有一个起始码，即只剩一个NAL
            pBs->iUsed = pBs->iStreamLen;
        }
        memcpy(pBs->pStream, pBs->pBuffer, pBs->iUsed);		// 拷贝NAL
        pBs->iStreamLen  = pBs->iUsed;

        result = JK_H264_LOAD_NAL_OK;
    }

    return result;
}

/*
 *
 * findStartCode:
 *
 * Parameters:
 *      str                   Bitbuffer object
 *
 * Function:
 *      First next start code in AVC byte stream
 *
 * Returns:
 *      Length of start code
 *
 *      str->bitbufDataPos will point to first byte that follows start code
 */
static int findStartCode(byteStream_s *str)
{
  int numZeros;
  int startCodeFound;
  int i;
  int currByte;

  numZeros       = 0;
  startCodeFound = 0;

  i = str->bitbufDataPos;

  while (!startCodeFound) {

    if (i == str->bitbufDataLen) {
      /* We are at the end of data -> read more from the bitstream file */

      int n = readBytesFromFile(str);

      if (n == 0) {
        /* End of bitstream -> stop search */
        //break;
    fseek(str->fn, 0, SEEK_SET);
    n = readBytesFromFile(str);
    if(n == 0) break;
      }
    }

    /* Find sequence of 0x00 ... 0x00 0x01 */

    while (i < str->bitbufDataLen) {

      currByte = str->bitbuf[i];
      i++;

      if (currByte > 1) /* If current byte is > 1, it cannot be part of a start code */
        numZeros = 0;
      else if (currByte == 0)  /* If current byte is 0, it could be part of a start code */
        numZeros++;
      else {                    /* currByte == 1 */
        if (numZeros > 1) {  /* currByte == 1. If numZeros > 1, we found a start code */
          startCodeFound = 1;
          break;
        }
        numZeros = 0;
      }
    }
  }

  str->bitbufDataPos = i;

  if (startCodeFound)
    return (numZeros + 1);
  else
    return 0;
}


/*
 *
 * getNalunitBits_ByteStream:
 *
 * Parameters:
 *      strIn                 Bytestream pointer
 *
 * Function:
 *      Read one NAL unit from bitstream file.
 *
 * Returns:
 *      1: No errors
 *      0: Could not read bytes (end of file)
 */
int getNalunitBits_ByteStream(void *strIn)
{
  byteStream_s *str;
  int numRemainingBytes;
  int startCodeLen;
  int nalUnitStartPos;

  str = (byteStream_s *)strIn;

  /*
   * Copy valid data to the beginning of the buffer
   */

  numRemainingBytes = str->bitbufDataLen - str->bitbufDataPos;

  if (numRemainingBytes > 0) {
    memcpy(str->bitbuf, str->bitbuf + str->bitbufDataPos, numRemainingBytes);
  }

  /* Update bitbuffer variables */
  str->bitbufDataLen = numRemainingBytes;
  str->bitbufDataPos = 0;


  /*
   * Find NAL unit boundaries
   */

  /* Find first start code */
  startCodeLen = findStartCode(str);

  if (startCodeLen == 0)
    return 0;

  /* Start address of the NAL unit */
  nalUnitStartPos = str->bitbufDataPos - startCodeLen;

  /* Find second start code */
  startCodeLen = findStartCode(str);

  /* Set data pointer to the beginning of the second start code */
  /* (i.e. to the end of the NAL unit)                          */
  if (startCodeLen != 0)
    str->bitbufDataPos -= startCodeLen;

  str->bitbufNalunit    = str->bitbuf + nalUnitStartPos;
  str->bitbufNalunitLen = str->bitbufDataPos;


  return 1;
}

static char* base64Encode(char const* origSigned, unsigned origLength)
{
  unsigned char const* orig = (unsigned char const*)origSigned; // in case any input bytes have the MSB set
  if (orig == NULL) return NULL;

  unsigned const numOrig24BitValues = origLength/3;
  unsigned char havePadding = origLength > numOrig24BitValues*3;
  unsigned char havePadding2 = origLength == numOrig24BitValues*3 + 2;
  unsigned const numResultBytes = 4*(numOrig24BitValues + havePadding);
  char* result = (char *)malloc(numResultBytes+1); // allow for trailing '\0'

  // Map each full group of 3 input bytes into 4 output base-64 characters:
  unsigned i;
  for (i = 0; i < numOrig24BitValues; ++i) {
    result[4*i+0] = base64Char[(orig[3*i]>>2)&0x3F];
    result[4*i+1] = base64Char[(((orig[3*i]&0x3)<<4) | (orig[3*i+1]>>4))&0x3F];
    result[4*i+2] = base64Char[((orig[3*i+1]<<2) | (orig[3*i+2]>>6))&0x3F];
    result[4*i+3] = base64Char[orig[3*i+2]&0x3F];
  }

  // Now, take padding into account.  (Note: i == numOrig24BitValues)
  if (havePadding) {
    result[4*i+0] = base64Char[(orig[3*i]>>2)&0x3F];
    if (havePadding2) {
      result[4*i+1] = base64Char[(((orig[3*i]&0x3)<<4) | (orig[3*i+1]>>4))&0x3F];
      result[4*i+2] = base64Char[(orig[3*i+1]<<2)&0x3F];
    } else {
      result[4*i+1] = base64Char[((orig[3*i]&0x3)<<4)&0x3F];
      result[4*i+2] = '=';
    }
    result[4*i+3] = '=';
  }

  result[numResultBytes] = '\0';
  return result;
}

char const* auxSDPLine()
{
  unsigned char * sps = (unsigned char *)spsArray;
  unsigned spsSize = fSPSSize;
  unsigned char * pps = (unsigned char *)ppsArray;
  unsigned ppsSize = fPPSSize;

  unsigned int profile_level_id;
  if (spsSize < 4) { // sanity check
    profile_level_id = 0;
  } else {
    profile_level_id = (sps[1]<<16)|(sps[2]<<8)|sps[3]; // profile_idc|constraint_setN_flag|level_idc
  }

  // Set up the "a=fmtp:" SDP line for this stream:
  char* sps_base64 = base64Encode((char*)sps, spsSize);
  char* pps_base64 = base64Encode((char*)pps, ppsSize);
  char const* fmtpFmt =
    "a=fmtp:%d packetization-mode=1"
    ";profile-level-id=%06X"
    ";sprop-parameter-sets=%s,%s\r\n";
  unsigned fmtpFmtSize = strlen(fmtpFmt)
    + 3 /* max char len */
    + 6 /* 3 bytes in hex */
    + strlen(sps_base64) + strlen(pps_base64);

    memset(sps_pps_base64, 0, sizeof(sps_pps_base64));

    printf("sdp fmtpFmtSize : %d\n", fmtpFmtSize);
    sprintf(sps_pps_base64, fmtpFmt,
       96,
    profile_level_id,
       sps_base64,
    pps_base64);

    printf("sps_pps_base64:%s\n", sps_pps_base64);
    free(sps_base64);
    free(pps_base64);

    return sps_pps_base64;
}

void NeedGetNewSpsPps()
{
    b_Init = 0;   // 需要重新获取PPS, SPS
}

int GetSpsAndPps(const unsigned char * p_buf)
{
    int ret = 0;
    if(b_Init == 0)
    {
        memset(spsArray, 0, sizeof(spsArray));
        memset(ppsArray, 0, sizeof(ppsArray));

        ret = GetSpsAndPpsFromStream((char *)p_buf, spsArray, &fSPSSize, ppsArray, &fPPSSize);
        if(ret == 0)  // 得到了SPS, PPS
        {
            b_Init = 1;
            auxSDPLine();
            return 0;
        }
    }

    return -1;
}

static int GetSpsAndPpsFromStream(const char * buf, char * sps, int * spsSzie, char * pps, int * ppsSize)
{
    unsigned char m_ph264DecodeBuf[150];
    unsigned char m_ph264BSBuf[150];
    H264_BITSTREAM_S h264BS;
    unsigned int uiDecodeUsed = 0;
    unsigned int uiDecodeLen;
    int fResult;
    unsigned char type;
    unsigned char flag_sps = 0, flag_pps = 0;

    uiDecodeLen = 100;
    uiDecodeUsed = 0;

    memcpy(m_ph264DecodeBuf, buf, uiDecodeLen);

    while(1)
    {
        if(uiDecodeUsed >= uiDecodeLen) break;

        JK_BitStream_init_FM(&h264BS, m_ph264DecodeBuf + uiDecodeUsed, uiDecodeLen - uiDecodeUsed, m_ph264BSBuf);
        fResult = JK_BitStreamLoadNALU_FM(&h264BS);
        if(fResult == JK_H264DEC_ERR_BADBS) break;
        uiDecodeUsed +=  h264BS.iUsed;

        type = h264BS.pStream[4] & 0x1f;

        switch(type)
        {
        case 1:  // P
        case 5:   // IDR
        case 6:   // SEI
            break;
        case 7:	 // SPS
            flag_sps = 1;
            memcpy(spsArray, (char *)h264BS.pStream + 4, h264BS.iUsed);
            *spsSzie = h264BS.iUsed - 4;
            break;
        case 8:   // PPS
            flag_pps = 1;
            memcpy(ppsArray, (char *)h264BS.pStream + 4, h264BS.iUsed);
            *ppsSize = h264BS.iUsed - 4;
            break;
        case 12:	// FILL
            break;
        default:
            break;
        }
    }

    if(flag_pps && flag_sps)
        return 0;

    return -1;
}

static int h263_process_frame( struct spook_frame *f, void *d )
{
    struct rtp_h263 *out = (struct rtp_h263 *)d;

    out->d = f->d;
    out->len = f->length;

    out->timestamp +=  90000 / FRAME_RATE;

    return out->init_done;
}

static int h263_get_sdp( char *dest, int len, int payload, int port, void *d )
{    
    return snprintf(dest,len,"a=range:npt=0-\r\nm=video %d RTP/AVP %d\r\nc=IN IP4 0.0.0.0\r\nb=AS:500\r\na=rtpmap:%d H264/90000\r\n",
                        port, payload, payload);
}

static int h263_send( struct rtp_endpoint *ep, void *d)
{
    struct rtp_h263 *out = (struct rtp_h263 *)d;
    struct iovec v[3];
    unsigned char vhdr[2] = { 0x04, 0x00 }; /* Set P bit */
    int flag;
    unsigned char u8NaluBytes;
    int	s32Bytes;
    int s32NalBufSize;
    int s32NaluRemain;
    char *pNaluCurr;
    H264_BITSTREAM_S h264BS;
    unsigned int uiDecodeUsed = 0;
    unsigned int uiDecodeLen;

    m_ph264DecodeBuf = (char *)out->d;
    uiDecodeLen = out->len;
    uiDecodeUsed = 0;

    while(1)
    {
        if(uiDecodeUsed >= uiDecodeLen) break;

        JK_BitStream_init_FM(&h264BS, (unsigned char*)m_ph264DecodeBuf + uiDecodeUsed,
                             uiDecodeLen - uiDecodeUsed, (unsigned char*)m_ph264BSBuf);
        if(JK_BitStreamLoadNALU_FM(&h264BS) == JK_H264DEC_ERR_BADBS)
        {
            fprintf(stderr, "bads NAUL\n");
            break;;
        }

        uiDecodeUsed += h264BS.iUsed;

        u8NaluBytes = *((unsigned char *)h264BS.pStream + 4);
        s32Bytes = 0;
        s32NalBufSize = h264BS.iUsed - 4;
        s32NaluRemain = h264BS.iUsed - 5;
        pNaluCurr = (char *)h264BS.pStream + 5;

        if(s32NaluRemain <= MAX_RTP_PKT_LENGTH)
        {
            StNaluHdr    *   pNaluHdr = (StNaluHdr    *)&vhdr[0];
            pNaluHdr->u1F				= (u8NaluBytes & 0x80) >> 7;
            pNaluHdr->u2Nri			= (u8NaluBytes & 0x60) >> 5;
            pNaluHdr->u5Type			= u8NaluBytes & 0x1f;

            v[1].iov_base = (unsigned char *)&vhdr[0];
            v[1].iov_len = 1;

            // remove NALU
            v[2].iov_base = pNaluCurr;
            v[2].iov_len = s32NaluRemain;

            if((u8NaluBytes & 0x1f) == 7 ||  ((u8NaluBytes & 0x1f) == 6) || (u8NaluBytes & 0x1f) == 8) flag = 0; // SPS or PPS
            else flag = 1;							     // P or I frame

            if( send_rtp_packet( ep, v, 3, out->timestamp, flag ) < 0 )
            {
                return -1;
            }
        }
        else
        {
            StFuIndicator	* pFuInd = (StFuIndicator  *)&vhdr[0];

            pFuInd->u1F		= (u8NaluBytes & 0x80) >> 7;
            pFuInd->u2Nri		= (u8NaluBytes & 0x60) >> 5;
            pFuInd->u5Type		= 28;

            StFuHdr	* pFuHdr = (StFuHdr *)&vhdr[1];
            pFuHdr->u1R       = 0;
            pFuHdr->u5Type    = u8NaluBytes & 0x1f;

            while(s32NaluRemain > 0)
            {
                pFuHdr->u1E       = (s32NaluRemain <= MAX_RTP_PKT_LENGTH) ? 1 : 0;
                pFuHdr->u1S       = (s32NaluRemain == (s32NalBufSize - 1)) ? 1 : 0;

                s32Bytes = (s32NaluRemain < MAX_RTP_PKT_LENGTH) ? s32NaluRemain : MAX_RTP_PKT_LENGTH;

                v[1].iov_base = (unsigned char *)&vhdr[0];
                v[1].iov_len = 2;

                v[2].iov_base = pNaluCurr;  //out->d + 1;
                v[2].iov_len = s32Bytes; //plen;
                if( send_rtp_packet( ep, v, 3, out->timestamp, (s32NaluRemain <= MAX_RTP_PKT_LENGTH) ? 1 : 0 ) < 0 )
                {
                    return -1;
                }

                pNaluCurr += MAX_RTP_PKT_LENGTH;
                s32NaluRemain -= MAX_RTP_PKT_LENGTH;
            }
        }
    }

    return 0;
}

struct rtp_media *new_rtp_media_h263_stream( struct stream *stream )
{
	struct rtp_h263 *out;
	int fincr, fbase;

	stream->get_framerate( stream, &fincr, &fbase );
	out = (struct rtp_h263 *)malloc( sizeof( struct rtp_h263 ) );
	out->init_done = 1;
	out->timestamp = 0;
    out->ts_incr = 90000 / FRAME_RATE;

	return new_rtp_media( h263_get_sdp, NULL,
					h263_process_frame, h263_send, out );
}
