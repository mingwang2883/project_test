#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>

#define TAB44 "    "
#define PRINTF_DEBUG

#define PRTNTF_STR_LEN 10

typedef enum e_hevc_nalu_type {
   HEVC_NAL_TRAIL_N    = 0,
   HEVC_NAL_TRAIL_R    = 1,
   HEVC_NAL_TSA_N      = 2,
   HEVC_NAL_TSA_R      = 3,
   HEVC_NAL_STSA_N     = 4,
   HEVC_NAL_STSA_R     = 5,
   HEVC_NAL_RADL_N     = 6,
   HEVC_NAL_RADL_R     = 7,
   HEVC_NAL_RASL_N     = 8,
   HEVC_NAL_RASL_R     = 9,
   HEVC_NAL_VCL_N10    = 10,
   HEVC_NAL_VCL_R11    = 11,
   HEVC_NAL_VCL_N12    = 12,
   HEVC_NAL_VCL_R13    = 13,
   HEVC_NAL_VCL_N14    = 14,
   HEVC_NAL_VCL_R15    = 15,
   HEVC_NAL_BLA_W_LP   = 16,
   HEVC_NAL_BLA_W_RADL = 17,
   HEVC_NAL_BLA_N_LP   = 18,
   HEVC_NAL_IDR_W_RADL = 19,
   HEVC_NAL_IDR_N_LP   = 20,
   HEVC_NAL_CRA_NUT    = 21,
   HEVC_NAL_IRAP_VCL22 = 22,
   HEVC_NAL_IRAP_VCL23 = 23,
   HEVC_NAL_RSV_VCL24  = 24,
   HEVC_NAL_RSV_VCL25  = 25,
   HEVC_NAL_RSV_VCL26  = 26,
   HEVC_NAL_RSV_VCL27  = 27,
   HEVC_NAL_RSV_VCL28  = 28,
   HEVC_NAL_RSV_VCL29  = 29,
   HEVC_NAL_RSV_VCL30  = 30,
   HEVC_NAL_RSV_VCL31  = 31,
   HEVC_NAL_VPS        = 32,
   HEVC_NAL_SPS        = 33,
   HEVC_NAL_PPS        = 34,
   HEVC_NAL_AUD        = 35,
   HEVC_NAL_EOS_NUT    = 36,
   HEVC_NAL_EOB_NUT    = 37,
   HEVC_NAL_FD_NUT     = 38,
   HEVC_NAL_SEI_PREFIX = 39,
   HEVC_NAL_SEI_SUFFIX = 40
} E_HEVC_NALU_TYPE;

/********************************************************************************
typedef struct t_h264_nalu_header
{
   unsigned char forbidden_bit:1, nal_reference_idc:2, nal_unit_type:5;
} T_H264_NALU_HEADER; (1个字节, hevc header为2个字节)
*********************************************************************************/
typedef struct t_h265_nalu_header
{
   unsigned short forbidden_zero_bit:1, nuh_reserved_zero_6bits:6, nal_unit_type:6, nuh_temporal_id_plus1:3;
} T_H265_NALU_HEADER;

typedef struct t_h265_nalu
{
   int startCodeLen;

   T_H265_NALU_HEADER h265NaluHeader;

   unsigned int bodyLen;

   unsigned char *bodyData;
} T_H265_NALU;

/**********************************************************************************
1. h265的起始码: 0x000001(3 Bytes)或0x00000001(4 Bytes);
2. 文件流中用起始码来区分NALU;
3. 如果NALU类型为vps, sps, pps, 或者解码顺序为第一个AU的第一个NALU, 起始码前面再加一个0x00
   视频流的首个NALU的起始码前加入0x00(4 Bytes的由来).
***********************************************************************************/
static int FindStartCode3Bytes(unsigned char *scData)
{
   int isFind = 0;

   if ((0==scData[0]) && (0==scData[1]) && (1==scData[2]))
   {
       isFind = 1;
   }

   return isFind;
}

static int FindStartCode4Bytes(unsigned char *scData)
{
   int isFind = 0;

   if ((0==scData[0]) && (0==scData[1]) && (0==scData[2]) && (1 == scData[3]))
   {
       isFind = 1;
   }

   return isFind;
}

static int GetNaluDataLen(int startPos, int h265BitsSize, unsigned char *h265Bits)
{
   int parsePos = 0;

   parsePos = startPos;

   while (parsePos < h265BitsSize)
   {
       if (FindStartCode3Bytes(&h265Bits[parsePos]))
       {
           return parsePos - startPos;
       }
       else if (FindStartCode4Bytes(&h265Bits[parsePos]))
       {
           return parsePos - startPos;
       }
       else
       {
           parsePos++;
       }
   }

   return parsePos - startPos; // if file is end
}

void ParseNaluData(const unsigned int naluLen,
                          unsigned char* const nuluData)
{
   static int naluNum = 0;

   unsigned char *data = NULL;
   char typeStr[PRTNTF_STR_LEN+1] = {0};

   T_H265_NALU_HEADER h265NaluHeader = {0};

   data = nuluData;

   memset(&h265NaluHeader, 0x0, sizeof(T_H265_NALU_HEADER));

   h265NaluHeader.nal_unit_type = ((data[0]>>1) & 0x3f);

   naluNum++;

#ifdef PRINTF_DEBUG
   switch (h265NaluHeader.nal_unit_type)
   {
       case HEVC_NAL_TRAIL_N:
           sprintf(typeStr, "B SLICE");
           break;

       case HEVC_NAL_TRAIL_R:
           sprintf(typeStr, "P SLICE");
           break;

       case HEVC_NAL_IDR_W_RADL:
           sprintf(typeStr, "IDR");
           break;

       case HEVC_NAL_VPS:
           sprintf(typeStr, "VPS");
           break;

       case HEVC_NAL_SPS:
           sprintf(typeStr, "SPS");
           break;

       case HEVC_NAL_PPS:
           sprintf(typeStr, "PPS");
           break;

       case HEVC_NAL_SEI_PREFIX:
           sprintf(typeStr, "SEI");
           break;

       default:
           sprintf(typeStr, "NTYPE(%d)", h265NaluHeader.nal_unit_type);
           break;
   }

//   printf("%5d| %7s| %8d|\n", naluNum, typeStr, naluLen);
#endif
}

#if 0

int main(int argc, char *argv[])
{
   int fileLen = 0;
   int naluLen = 0;
   int h265BitsPos = 0; /* h265, hevc; h264, avc系列, Advanced Video Coding */

   unsigned char *h265Bits = NULL;
   unsigned char *naluData = NULL;

   FILE *fp = NULL;

   if (2 != argc)
   {
       printf("Usage: flvparse **.flv\n");

       return -1;
   }

   fp = fopen(argv[1], "rb");
   if (!fp)
   {
       printf("open file[%s] error!\n", argv[1]);

       return -1;
   }

   fseek(fp, 0, SEEK_END);

   fileLen = ftell(fp);

   fseek(fp, 0, SEEK_SET);

   h265Bits = (unsigned char*)malloc(fileLen);
   if (!h265Bits)
   {
       printf("maybe file is too long, or memery is not enough!\n");

       fclose(fp);

       return -1;
   }

   memset(h265Bits, 0x0, fileLen);

   if (fread(h265Bits, 1, fileLen, fp) < 0)
   {
       printf("read file data to h265Bits error!\n");

       fclose(fp);
       free(h265Bits);

       h265Bits = NULL;

       return -1;
   }

   fclose(fp);

//   printf("-----+--- NALU Table --+\n");
//   printf(" NUM |  TYPE |   LEN   |\n");
//   printf("-----+-------+---------+\n");

   while (h265BitsPos < (fileLen-4))
   {
       if (FindStartCode3Bytes(&h265Bits[h265BitsPos]))
       {
           naluLen = GetNaluDataLen(h265BitsPos+3, fileLen, h265Bits);

           naluData = (unsigned char*)malloc(naluLen);
           if (naluData)
           {
               memset(naluData, 0x0, naluLen);

               memcpy(naluData, h265Bits+h265BitsPos+3, naluLen);

               ParseNaluData(naluLen, naluData);

               free(naluData);
               naluData = NULL;
           }

           h265BitsPos += (naluLen+3);
       }
       else if (FindStartCode4Bytes(&h265Bits[h265BitsPos]))
       {
           naluLen = GetNaluDataLen(h265BitsPos+4, fileLen, h265Bits);

           naluData = (unsigned char*)malloc(naluLen);
           if (naluData)
           {
               memset(naluData, 0x0, naluLen);

               memcpy(naluData, h265Bits+h265BitsPos+4, naluLen);

               ParseNaluData(naluLen, naluData);

               free(naluData);
               naluData = NULL;
           }

           h265BitsPos += (naluLen+4);
       }
       else
       {
           h265BitsPos++;
       }
   }

   return 0;
}
#endif
