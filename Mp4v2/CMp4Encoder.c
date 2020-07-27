#include "CMp4Encoder.h"

#define JK_H264_LOAD_NAL_OK   0
#define JK_H264DEC_ERR_BADBS  0xa1218002        // bitstream is corrupted

#define AUDIO_TIME_SCALE 8000
#define VIDEO_TIME_SCALE 90000

#define PTS2TIME_SCALE(CurPTS, PrevPTS, timeScale) \
    ((MP4Duration)((CurPTS - PrevPTS) * 1.0 / (double)(1e+6) * timeScale))

#define INVALID_PTS 0xFFFFFFFFFFFFFFFF

/************************************************
 * This data structure describes the bitstream buffers.
 ************************************************/
typedef struct JK_H264_BITSTREAM_S 
{
    // global bitstream buffer.
    int iUsed;              // count of bytes to extract an rbsp/frame
    int iStreamLen;         // length of rbsp data/access unit
    unsigned char *pStart;  // start bitstream ptr
    unsigned char *pEnd;    // end bitstream ptr
    unsigned char *pBuffer; // buffer pointer from which we seek RBSP
    unsigned char *pStream; // rbsp/access unit start pointer
} H264_BITSTREAM_S;

static const char start_code[4] = {0x00, 0x00, 0x00, 0x01};

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
        {
            numZeros = 0;
        }

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
static void JK_BitStream_init_FM( H264_BITSTREAM_S *bs, unsigned char *pBitstream, 
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
static int JK_BitStreamLoadNALU_FM(H264_BITSTREAM_S *pBs)
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

    if(startCodeLen)
    {	// 开始找第二个起始码
        offset = startCodeLen;
        startCodeLen = 0;
        start_pos_2 = findStartCode_2(pBs->pBuffer + startCodeLen,
                                      pBs->iStreamLen,offset, &startCodeLen);

        //找到了第二个起始码
        if(startCodeLen)
        {
            pBs->iUsed = start_pos_2 - start_pos_1;
        }
        else
        {	// 只有一个起始码，即只剩一个NAL
            pBs->iUsed = pBs->iStreamLen;
        }

        memcpy(pBs->pStream, pBs->pBuffer, pBs->iUsed);		// 拷贝NAL
        pBs->iStreamLen  = pBs->iUsed;

        result = JK_H264_LOAD_NAL_OK;
    }

    return result;
}

void InitMp4Encoder(int width, int height, int frame,pMp4v2WriteMp4 mp4v2_write)
{
    mp4v2_write->b_isOpen = 0;
    mp4v2_write->state = invalid_state;
    mp4v2_write->m_vWidth  = width;
    mp4v2_write->m_vHeight = height;
    mp4v2_write->m_vFrateR = frame;
    mp4v2_write->m_vTimeScale = 90000;
    mp4v2_write->m_mp4FHandle = NULL;
    mp4v2_write->m_aTrackId = MP4_INVALID_TRACK_ID;
    mp4v2_write->m_vTrackId = MP4_INVALID_TRACK_ID;
    mp4v2_write->p_video_sample = NULL;
    mp4v2_write->p_audio_sample = NULL;
    mp4v2_write->m_bFirstVideo = true;
    mp4v2_write->m_bFirstAudio = true;
    mp4v2_write->m_u64VideoPTS = 0;
    mp4v2_write->m_u64FirstPTS = INVALID_PTS;

    memset(mp4v2_write->video_buffer,0,sizeof(mp4v2_write->video_buffer));
    mp4v2_write->m_ph264BSBuf = mp4v2_write->video_buffer;
}

void Mp4SetProp(int width,int height,int fps,int nSampleRate,int nChannal,int bitsPerSample,pMp4v2WriteMp4 mp4v2_write)
{
    mp4v2_write->m_vWidth = width;
    mp4v2_write->m_vHeight = height;
    mp4v2_write->m_vFrateR = fps;

    mp4v2_write->m_nSampleRate = nSampleRate;
    mp4v2_write->m_nChannal = nChannal;
    mp4v2_write->m_bitsPerSample = bitsPerSample;
}

bool OpenMp4Encoder(const char * fileName,pMp4v2WriteMp4 mp4v2_write)
{
    time(&mp4v2_write->t_start);
    mp4v2_write->tick_count = 0;
    mp4v2_write->b_isOpen = false;

    mp4v2_write->m_mp4FHandle = MP4Create(fileName,0);
    if (mp4v2_write->m_mp4FHandle == MP4_INVALID_FILE_HANDLE)
    {
        return false;
    }

    printf("creater file:%s\n", fileName);

    MP4SetTimeScale(mp4v2_write->m_mp4FHandle, mp4v2_write->m_vTimeScale);

    mp4v2_write->state = invalid_state;

    // 2.2 add audio track
    printf("m_nSampleRate:%d, m_bitsPerSample:%d\n", mp4v2_write->m_nSampleRate, mp4v2_write->m_bitsPerSample);

    mp4v2_write->m_aTrackId  = MP4AddAudioTrack(mp4v2_write->m_mp4FHandle,mp4v2_write->m_nSampleRate,
                                                mp4v2_write->m_bitsPerSample, MP4_MPEG4_AUDIO_TYPE );
    printf("m_aTrackId :%d\n", mp4v2_write->m_aTrackId);
    if (mp4v2_write->m_aTrackId == MP4_INVALID_TRACK_ID)
    {
        printf("failed file:%s\n", fileName);
        MP4Close(mp4v2_write->m_mp4FHandle, 0);
        return false;
    }

    // 2.3 set audio level  LC
    MP4SetAudioProfileLevel(mp4v2_write->m_mp4FHandle, 0x2 );

    // 2.4 get decoder info
    unsigned char  faacDecoderInfo[2] = {0};
    unsigned long  faacDecoderInfoSize = 0;

    faacDecoderInfo[0] = 0x15;//8k signal
    faacDecoderInfo[1] = 0x88;

    faacDecoderInfoSize = 2;

    // 2.5 set encoder info [16bit-8000hz-1channal->{ 0x15, 0x88 } ]
    bool bOk = MP4SetTrackESConfiguration(mp4v2_write->m_mp4FHandle,mp4v2_write->m_aTrackId,
                                          faacDecoderInfo, faacDecoderInfoSize );
    if( !bOk )
    {
        MP4Close(mp4v2_write->m_mp4FHandle, 0);
        return false;
    }

    printf("creater mp4 file:%s\n", fileName);

    mp4v2_write->b_isOpen = true;

    return true;
}

/* type=1 video, 2-audio */
uint64_t checkInvalidData(uint64_t cur_pts,uint64_t old_pts,int type,pMp4v2WriteMp4 mp4v2_write)
{
    uint64_t diff_duration = 0;
    if(type == 1)
    {
        diff_duration = PTS2TIME_SCALE(cur_pts, old_pts, VIDEO_TIME_SCALE);
        if(diff_duration < 0)
        {
            diff_duration = VIDEO_TIME_SCALE / mp4v2_write->m_vFrateR;
        }
        else if(diff_duration >= (7*VIDEO_TIME_SCALE))
        {
            diff_duration = VIDEO_TIME_SCALE / mp4v2_write->m_vFrateR;
        }
    }
    else if(type == 2)
    {
        diff_duration = PTS2TIME_SCALE(cur_pts, old_pts,AUDIO_TIME_SCALE);
        if(diff_duration < 0)
        {
            diff_duration = 320;
        }
        else if(diff_duration >= (6 * AUDIO_TIME_SCALE))
        {
            diff_duration = 320;
        }
    }
    else
    {
        diff_duration = type == 1 ? (VIDEO_TIME_SCALE / mp4v2_write->m_vFrateR) : 320;
    }

    return diff_duration;

}

int Mp4VNaluEncode(UBYTE* _naluData,int _naluSize,unsigned long long pts_duration,pMp4v2WriteMp4 mp4v2_write)
{
    if (mp4v2_write->m_mp4FHandle == MP4_INVALID_FILE_HANDLE)
    {
        return -1;
    }

    /* read buffer */
    int index = -1;
    if(_naluData[0]==0 && _naluData[1]==0 && _naluData[2]==0 && _naluData[3]==1 && (_naluData[4]& 0x1f)==0x07)
    {
        index = _NALU_SPS_;  // 0
        //printf("%s[%d]====NALU_SPS\n",__FUNCTION__,__LINE__);
    }
    if(index!=_NALU_SPS_ && mp4v2_write->m_vTrackId == MP4_INVALID_TRACK_ID)
    {
        return 0;
    }
    if(_naluData[0]==0 && _naluData[1]==0 && _naluData[2]==0 && _naluData[3]==1 && (_naluData[4]& 0x1f)==0x08)
    {
        index = _NALU_PPS_; // 1
        //printf("%s[%d]====NALU_PPS\n",__FUNCTION__,__LINE__);
    }
    if(_naluData[0]==0 && _naluData[1]==0 && _naluData[2]==0 && _naluData[3]==1 && (_naluData[4]& 0x1f)==0x05)
    {
        index = _NALU_I_;  // 2
        //printf("%s[%d]====NALU_I\n",__FUNCTION__,__LINE__);
    }
    if(_naluData[0]==0 && _naluData[1]==0 && _naluData[2]==0 && _naluData[3]==1 && (_naluData[4]& 0x1f)==0x01)
    {
        index = _NALU_P_; // 3
        //printf("%s[%d]====NALU_P\n",__FUNCTION__,__LINE__);
    }
    if(_naluData[0]==0 && _naluData[1]==0 && _naluData[2]==0 && _naluData[3]==1 && ((_naluData[4] & 0x1f) ==0x06))
    {
        index = _NALU_SEI_; // 4
    }

    switch(index)
    {
    case _NALU_SPS_:
        if(mp4v2_write->m_vTrackId == MP4_INVALID_TRACK_ID)
        {
            mp4v2_write->m_vTrackId = MP4AddH264VideoTrack(mp4v2_write->m_mp4FHandle,mp4v2_write->m_vTimeScale,
                                              mp4v2_write->m_vTimeScale / mp4v2_write->m_vFrateR,
                                              mp4v2_write->m_vWidth,    // width
                                              mp4v2_write->m_vHeight,   // height
                                              _naluData[5],             // sps[1] AVCProfileIndication
                                              _naluData[6],             // sps[2] profile_compat
                                              _naluData[7],             // sps[3] AVCLevelIndication
                                              3);                       // 4 bytes length before each NAL unit
            if (mp4v2_write->m_vTrackId == MP4_INVALID_TRACK_ID)
            {
                printf("add video track failed.\n");
                return -1;
            }
            MP4SetVideoProfileLevel(mp4v2_write->m_mp4FHandle,0x7f);
        }
        mp4v2_write->state = sps_state;
        MP4AddH264SequenceParameterSet(mp4v2_write->m_mp4FHandle,mp4v2_write->m_vTrackId,_naluData+4,_naluSize-4);
        break;
    case _NALU_PPS_:
        mp4v2_write->state = pps_state;
        MP4AddH264PictureParameterSet(mp4v2_write->m_mp4FHandle,mp4v2_write->m_vTrackId,_naluData+4,_naluSize-4);
        break;
    case _NALU_SEI_ :
        break;
    case _NALU_I_: 
    case _NALU_P_:
        {
            _naluData[0] = (_naluSize - 4) >> 24;  
            _naluData[1] = (_naluSize - 4) >> 16;  
            _naluData[2] = (_naluSize - 4) >> 8;  
            _naluData[3] = (_naluSize - 4) & 0xff; 

            if (mp4v2_write->m_bFirstVideo)
            {
                if (mp4v2_write->m_u64FirstPTS > pts_duration)
                {
                    mp4v2_write->m_u64FirstPTS = pts_duration;
                }
                mp4v2_write->m_u64VideoPTS = pts_duration;
                mp4v2_write->m_bFirstVideo = false;
                printf("m_bFirstAudio:%d PTS2TIME_SCALE(pts_duration:%llu, m_u64VideoPTS:%lu, VIDEO_TIME_SCALE):%lu\n",
                       mp4v2_write->m_bFirstAudio,pts_duration,mp4v2_write->m_u64VideoPTS,
                       PTS2TIME_SCALE(pts_duration, mp4v2_write->m_u64VideoPTS, VIDEO_TIME_SCALE));
            }

            if(pts_duration == 0)
            {
                if ((_naluData[4] & 0x0F) == 5)
                {
                    MP4WriteSample(mp4v2_write->m_mp4FHandle,mp4v2_write->m_vTrackId,
                                   _naluData, _naluSize,MP4_INVALID_DURATION,0,true);
                }
                else
                {
                    MP4WriteSample(mp4v2_write->m_mp4FHandle,mp4v2_write->m_vTrackId,
                                   _naluData, _naluSize,MP4_INVALID_DURATION,0,false);
                }
            }
            else
            {
                uint64_t rel_duration = checkInvalidData(pts_duration,
                                                         mp4v2_write->m_u64VideoPTS, 1,mp4v2_write);
                if ((_naluData[4] & 0x0F) == 5)
                {
                    MP4WriteSample(mp4v2_write->m_mp4FHandle,mp4v2_write->m_vTrackId,
                                   _naluData, _naluSize,rel_duration,0,true);
                }
                else
                {
                    MP4WriteSample(mp4v2_write->m_mp4FHandle,mp4v2_write->m_vTrackId,
                                   _naluData, _naluSize,rel_duration,0,false);
                }
            }

            mp4v2_write->m_u64VideoPTS = pts_duration;
            break;
        }
    }

    return  0;
}

//------------------------------------------------------------------------------------------------- Mp4Encode说明
// 【x264编码出的NALU规律】
// 第一帧 SPS【0 0 0 1 0x67】 PPS【0 0 0 1 0x68】 SEI【0 0 1 0x6】 IDR【0 0 1 0x65】
// p帧      P【0 0 0 1 0x41】
// I帧    SPS【0 0 0 1 0x67】 PPS【0 0 0 1 0x68】 IDR【0 0 1 0x65】
// 【mp4v2封装函数MP4WriteSample】
// 此函数接收I/P nalu,该nalu需要用4字节的数据大小头替换原有的起始头，并且数据大小为big-endian格式
//-------------------------------------------------------------------------------------------------
int Mp4VEncode(UBYTE* FrameBuf,int FrameSize,int KeyFrame,unsigned long long pts_duration,pMp4v2WriteMp4 mp4v2_write)
{
    unsigned char type;
    unsigned char *m_ph264DecodeBuf = FrameBuf;

    int ret = 0;
    int count_temp = 0;

    unsigned int fResult;
    unsigned int uiDecodeUsed = 0;
    unsigned int uiDecodeLen = FrameSize;

    H264_BITSTREAM_S h264BS;

    while(1)
    {
        count_temp++;
        if(count_temp>= 20)
        {
            printf("while 1 is too long time!!\n");
            break;
        }
        if(uiDecodeUsed >= uiDecodeLen)
        {
            break;
        }

        JK_BitStream_init_FM(&h264BS, m_ph264DecodeBuf + uiDecodeUsed, uiDecodeLen - uiDecodeUsed, mp4v2_write->m_ph264BSBuf);

        fResult = JK_BitStreamLoadNALU_FM(&h264BS);

        if(fResult == JK_H264DEC_ERR_BADBS)
        {
            break;
        }

        if(h264BS.iUsed == 0)
        {
            break;
        }

        uiDecodeUsed +=  h264BS.iUsed;

        type = h264BS.pStream[4] & 0x1f;

        ret = Mp4VNaluEncode(h264BS.pStream, h264BS.iUsed,pts_duration,mp4v2_write);
        if(ret == -1)
        {
            break;
        }
    }

    return ret;
}

int Mp4AEncode(UBYTE* _aacData,int _aacSize,pMp4v2WriteMp4 mp4v2_write)
{
    if (mp4v2_write->m_mp4FHandle == MP4_INVALID_FILE_HANDLE)
    {
//        printf("m_mp4FHandle == MP4_INVALID_FILE_HANDLE\n");
        return -1;
    }
    if((mp4v2_write->m_vTrackId == MP4_INVALID_TRACK_ID))
    {
//        printf("m_vTrackId == MP4_INVALID_TRACK_ID\n");
        return -2;
    }
    int ret = 0;

    if(_aacSize > 0)
    {
        ret = MP4WriteSample(mp4v2_write->m_mp4FHandle,mp4v2_write->m_aTrackId,
                             (UBYTE*)&(_aacData[7]), _aacSize - 7 , 1024, 0, true);
        time(&mp4v2_write->t_end);
        mp4v2_write->tick_count += 2048;
        if(mp4v2_write->t_end - mp4v2_write->t_start >= 30)
        {
            mp4v2_write->t_start = mp4v2_write->t_end;
            mp4v2_write->tick_diff = mp4v2_write->tick_count - 30 * 16000;

            mp4v2_write->tick_count = 0;
        }
    }

    return ret;
}

void CloseMp4Encoder(pMp4v2WriteMp4 mp4v2_write)
{
    if(mp4v2_write->p_video_sample !=NULL)
    {
        free(mp4v2_write->p_video_sample);
        mp4v2_write->p_video_sample = NULL;
    }

    if(mp4v2_write->p_audio_sample != NULL)
    {
        free(mp4v2_write->p_audio_sample);
        mp4v2_write->p_audio_sample = NULL;
    }

    if(mp4v2_write->m_mp4FHandle)
    {
        MP4Close(mp4v2_write->m_mp4FHandle, MP4_CLOSE_DO_NOT_COMPUTE_BITRATE);
        mp4v2_write->m_mp4FHandle = NULL;

        mp4v2_write->b_isOpen = false;
    }
}

/* open mp4 file for read */
void InitMp4Reader(pMp4v2ReadMp4 mp4v2_read)
{
    mp4v2_read->m_mp4FHandle = NULL;
    mp4v2_read->p_video_sample = NULL;
    mp4v2_read->p_audio_sample = NULL;
}

bool OpenMp4Reader(const char *fileName,pMp4v2ReadMp4 mp4v2_read)
{
    int	i;
    int	b;
    char *video_name;
    char *audio_name;
    double video_frame_rate;

    u_int8_t **pp_sps = NULL;
    u_int8_t **pp_pps = NULL;
    u_int8_t *p_audio_config;
    u_int8_t *p_video_config;

    u_int32_t n_video_config_size;
    u_int32_t n_audio_config_size;
    u_int32_t *pn_sps,*pn_pps;

    /* track from 1 */
    mp4v2_read->sampleId_v = 0;
    mp4v2_read->sampleId_a = 0;

    mp4v2_read->m_mp4FHandle = MP4Read(fileName);
    if (!mp4v2_read->m_mp4FHandle)
    {
        printf("MP4Read failed\n");
        return FALSE;
    }

    /* 2. Identify the video & audio. */
    mp4v2_read->video_trId = MP4FindTrackId(mp4v2_read->m_mp4FHandle, 0, MP4_VIDEO_TRACK_TYPE, 0);
    if (mp4v2_read->video_trId == MP4_INVALID_TRACK_ID)
    {
        printf("No video track\n");
        video_name = NULL;
    }
    else
    {
        video_name = (char *)MP4GetTrackMediaDataName((char *)mp4v2_read->m_mp4FHandle,
                                                      mp4v2_read->video_trId);
        if (strcmp(video_name, "mp4v") == 0)
        {
            printf("Video = MPEG4\n");
        }
        else if (strcmp(video_name, "h263") == 0)
        {
            printf("Video = H.263\n");
        }
        else if (strcmp(video_name, "avc1") == 0)
        {
            printf("Video = H.264\n");
        }
        else
        {
            printf("Video = Unknown\n");
        }
    }

    mp4v2_read->audio_trId = MP4FindTrackId(mp4v2_read->m_mp4FHandle, 0, MP4_AUDIO_TRACK_TYPE, 0);
    if (mp4v2_read->audio_trId == MP4_INVALID_TRACK_ID)
    {
        printf("No audio track\n");
        audio_name = NULL;
    }
    else
    {
        audio_name = (char *)MP4GetTrackMediaDataName(mp4v2_read->m_mp4FHandle,mp4v2_read->audio_trId);
        if (strcmp(audio_name, "mp4a") == 0)
        {
            printf("Audio = MPEG4 AAC\n");
        }
        else
        {
            printf("Audio = Unknown\n");
        }
    }

    /* 3. Video Properties.  */
    if (mp4v2_read->video_trId != MP4_INVALID_TRACK_ID)
    {
        mp4v2_read->video_num_samples = MP4GetTrackNumberOfSamples(mp4v2_read->m_mp4FHandle, mp4v2_read->video_trId);
        mp4v2_read->m_vWidth = MP4GetTrackVideoWidth(mp4v2_read->m_mp4FHandle,mp4v2_read->video_trId);
        mp4v2_read->m_vHeight = MP4GetTrackVideoHeight(mp4v2_read->m_mp4FHandle,mp4v2_read->video_trId);
        video_frame_rate = MP4GetTrackVideoFrameRate(mp4v2_read->m_mp4FHandle,mp4v2_read->video_trId);

        mp4v2_read->video_timescale = MP4GetTrackTimeScale(mp4v2_read->m_mp4FHandle, mp4v2_read->video_trId);
        mp4v2_read->video_duration = MP4GetTrackDuration(mp4v2_read->m_mp4FHandle,mp4v2_read->video_trId);

        mp4v2_read->video_sample_max_size = MP4GetTrackMaxSampleSize(mp4v2_read->m_mp4FHandle,mp4v2_read->video_trId);

        mp4v2_read->p_video_sample = (u_int8_t *)malloc(mp4v2_read->video_sample_max_size);
        if(mp4v2_read->p_video_sample == NULL)
        {
            return FALSE;
        }

        mp4v2_read->n_video_sample = mp4v2_read->video_sample_max_size;
        mp4v2_read->m_vFrateR = (int)video_frame_rate;

//        printf("width:%d, height:%d, frameRate:%d,video_timescale:%d,"
//               " video_duration:%d video_num_samples:%d\n",mp4v2_read->m_vWidth,mp4v2_read->m_vHeight,
//               mp4v2_read->m_vFrateR, mp4v2_read->video_timescale, mp4v2_read->video_duration, mp4v2_read->video_num_samples);
    }

    /* 4. Audio Properties.  */
    if (mp4v2_read->audio_trId != MP4_INVALID_TRACK_ID)
    {
        mp4v2_read->audio_num_samples = MP4GetTrackNumberOfSamples(mp4v2_read->m_mp4FHandle,mp4v2_read->audio_trId);
        mp4v2_read->audio_num_channels = MP4GetTrackAudioChannels(mp4v2_read->m_mp4FHandle, mp4v2_read->audio_trId);

        mp4v2_read->audio_timescale = MP4GetTrackTimeScale(mp4v2_read->m_mp4FHandle,mp4v2_read->audio_trId);
        mp4v2_read->audio_duration = MP4GetTrackDuration(mp4v2_read->m_mp4FHandle,mp4v2_read->audio_trId);

        mp4v2_read->audio_sample_max_size = MP4GetTrackMaxSampleSize(mp4v2_read->m_mp4FHandle, mp4v2_read->audio_trId);

        printf("mp4v2_read->audio_sample_max_size %d\n",mp4v2_read->audio_sample_max_size);
        mp4v2_read->p_audio_sample = (u_int8_t *)malloc(mp4v2_read->audio_sample_max_size);
        if(mp4v2_read->p_audio_sample == NULL)
        {
            return FALSE;
        }

        mp4v2_read->n_audio_sample = mp4v2_read->audio_sample_max_size;
    }

    /*  4. Video Stream Header.  */
    if (mp4v2_read->video_trId != MP4_INVALID_TRACK_ID)
    {

        if (strcmp(video_name, "mp4v") == 0)
        {
            p_video_config = NULL;
            n_video_config_size = 0;

            b = MP4GetTrackESConfiguration(mp4v2_read->m_mp4FHandle, mp4v2_read->video_trId,
                                           &p_video_config, &n_video_config_size);
        }
        else if (strcmp(video_name, "h263") == 0)
        {

        }
        else if (strcmp(video_name, "avc1") == 0)
        {
            pp_sps = NULL;
            pn_sps = NULL;
            pp_pps = NULL;
            pn_pps = NULL;

            b = MP4GetTrackH264SeqPictHeaders(mp4v2_read->m_mp4FHandle, mp4v2_read->video_trId,
                                              &pp_sps, &pn_sps, &pp_pps, &pn_pps);
            if(!b)
            {
                return FALSE;
            }

            mp4v2_read->sps_num = *pn_sps;
            mp4v2_read->pps_num = *pn_pps;

            //printf("pn_sps:%d, pn_pps:%d\n", *pn_sps, *pn_pps);
            //printf("pn_sps:%d, pn_pps:%d\n", *(pn_sps+1), *(pn_pps+1));

            pp_sps = pp_pps = NULL;
            pn_sps = pn_pps = NULL;

            b = MP4GetTrackH264SeqPictHeaders(mp4v2_read->m_mp4FHandle, mp4v2_read->video_trId,
                                              &pp_sps, &pn_sps, &pp_pps, &pn_pps);
            if (!b)
            {
                return FALSE;
            }

            // SPS memcpy
            if (pp_sps)
            {
                for (i=0; pn_sps[i] != 0; i++)
                {
                    memcpy(mp4v2_read->sps + 4,pp_sps[i], *(pn_sps + i));
                    free((pp_sps[i]));
                }
                free(pp_sps);
                free(pn_sps);
            }
            // PPS memcpy
            if (pp_pps)
            {
                for (i=0; *(pp_pps + i); i++)
                {
                    memcpy(mp4v2_read->pps + 4,*(pp_pps + i),*(pn_pps + i));
                    free(*(pp_pps + i));
                }
                free(pp_pps);
                free(pn_pps);
            }
        }
        else
        {
            printf("\nVideo = Unknown");
        }
    }

    /* 5. Audio Stream Header.  */
    if (mp4v2_read->audio_trId != MP4_INVALID_TRACK_ID)
    {
        if (strcmp(audio_name, "mp4a") == 0)
        {
            p_audio_config = NULL;
            n_audio_config_size = 0;

            b = MP4GetTrackESConfiguration(mp4v2_read->m_mp4FHandle, mp4v2_read->audio_trId,
                                           &p_audio_config, &n_audio_config_size);

            int i;
            for(i = 0;i < n_audio_config_size;i++)
            {
                mp4v2_read->audio_config[i] = p_audio_config[i];
            }

            mp4v2_read->audio_config_size = n_audio_config_size;
        }
        else
        {
            printf("\nAideo = Unknown");
        }
    }

    mp4v2_read->pps[0] = 0x00;
    mp4v2_read->pps[1] = 0x00;
    mp4v2_read->pps[2] = 0x00;
    mp4v2_read->pps[3] = 0x01;

    mp4v2_read->sps[0] = 0x00;
    mp4v2_read->sps[1] = 0x00;
    mp4v2_read->sps[2] = 0x00;
    mp4v2_read->sps[3] = 0x01;

    /* track from 1 */
    mp4v2_read->sampleId_v = 1;
    mp4v2_read->sampleId_a = 1;

    return TRUE;
}

bool Mp4ReadVideo(char *p_video, int *frameLen,int *KeyFrame,MP4Timestamp *time_stamp,pMp4v2ReadMp4 mp4v2_read)
{
    bool b = FALSE;
    int count = 0;

    *frameLen = 0;
    *KeyFrame = 0;
    mp4v2_read->n_video_sample = mp4v2_read->video_sample_max_size;

    /*  6. Read Video Samples.  */
    if((mp4v2_read->video_trId != MP4_INVALID_TRACK_ID)
            && (mp4v2_read->sampleId_v < mp4v2_read->video_num_samples))
    {
        b = MP4ReadSample(mp4v2_read->m_mp4FHandle,mp4v2_read->video_trId,
                          mp4v2_read->sampleId_v,&mp4v2_read->p_video_sample,
                          &mp4v2_read->n_video_sample,time_stamp, NULL, NULL, NULL);
        mp4v2_read->sampleId_v++;
        if ((!b) || (mp4v2_read->n_video_sample == 0))
        {
            printf("%d ERROR [%d] \n",__LINE__,mp4v2_read->sampleId_v);
            return FALSE;
        }

        if((mp4v2_read->p_video_sample[4]&0X1F) == 0x05)  // I frame
        {
            memcpy(p_video, mp4v2_read->sps, mp4v2_read->sps_num + 4);
            count += (mp4v2_read->sps_num + 4);

            memcpy(p_video + count, mp4v2_read->pps, mp4v2_read->pps_num + 4);
            count += (mp4v2_read->pps_num + 4);

            *KeyFrame = 1;
        }

        memcpy(p_video + count,start_code,4);
        count += 4;

        memcpy(p_video + count, mp4v2_read->p_video_sample + 4, mp4v2_read->n_video_sample - 4);
        count += (mp4v2_read->n_video_sample - 4);
        *frameLen = count;
        *time_stamp = MP4GetSampleTime(mp4v2_read->m_mp4FHandle,mp4v2_read->video_trId,
                                       mp4v2_read->sampleId_v);

        return TRUE;
    }

    return FALSE;
}

bool Mp4ReadAudio(char *p_audio, int *frameLen, MP4Timestamp *time_stamp,pMp4v2ReadMp4 mp4v2_read)
{
    bool b = FALSE;
    *frameLen = 0;
    mp4v2_read->n_audio_sample = mp4v2_read->audio_sample_max_size;

    if ((mp4v2_read->audio_trId != MP4_INVALID_TRACK_ID)
            && (mp4v2_read->sampleId_a < mp4v2_read->audio_num_samples))
    {
        b = MP4ReadSample(mp4v2_read->m_mp4FHandle, mp4v2_read->audio_trId, mp4v2_read->sampleId_a,
                          &mp4v2_read->p_audio_sample, &mp4v2_read->n_audio_sample,
                          time_stamp, NULL, NULL, NULL);
        mp4v2_read->sampleId_a++;
        if ((!b) || (mp4v2_read->n_audio_sample == 0))
        {
            printf("%d ERROR [%d] \n",__LINE__,mp4v2_read->sampleId_a);
            return FALSE;
        }

        memcpy(p_audio,mp4v2_read->p_audio_sample,mp4v2_read->n_audio_sample);
        *frameLen = mp4v2_read->n_audio_sample;
        *time_stamp = MP4GetSampleTime(mp4v2_read->m_mp4FHandle, mp4v2_read->audio_trId,
                                       mp4v2_read->sampleId_a);

        return TRUE;
    }

    return FALSE;
}

bool Mp4SeekMedia(int total_second, int ops_second,pMp4v2ReadMp4 mp4v2_read)
{
    mp4v2_read->sampleId_a = ops_second * mp4v2_read->audio_num_samples / total_second;
    mp4v2_read->sampleId_v = ops_second * mp4v2_read->video_num_samples / total_second;
    printf("FUNCTION:%s sampleId_a:%d, sampleId_v:%d, total_second:%d, ops_second:%d\n",
           __FUNCTION__, mp4v2_read->sampleId_a, mp4v2_read->sampleId_v,total_second,ops_second);
}

bool CloseMp4Reader(pMp4v2ReadMp4 mp4v2_read)
{
    if(mp4v2_read->p_video_sample !=NULL)
    {
        free(mp4v2_read->p_video_sample);
        mp4v2_read->p_video_sample = NULL;
    }

    if(mp4v2_read->p_audio_sample != NULL)
    {
        free(mp4v2_read->p_audio_sample);
        mp4v2_read->p_audio_sample = NULL;
    }

    if(mp4v2_read->m_mp4FHandle)
    {	
        MP4Close(mp4v2_read->m_mp4FHandle, 0);
        mp4v2_read->m_mp4FHandle = NULL;
    }

    return TRUE;
}

void read_mp4_to_buff(void)
{
    unsigned char v_buff[400 * 1024] = {0};
    unsigned char a_buff[50 * 1024] = {0};

    bool v_ret;
    bool a_ret;

    int v_len;
    int a_len;
    int key_frame = 0;

    MP4Timestamp v_time;
    MP4Timestamp a_time;

    Mp4v2WriteMp4 mp4_write;
    Mp4v2ReadMp4  mp4_read;

    FILE *fp_v = NULL;
    FILE *fp_a = NULL;

    fp_v = fopen("./v.264","wb");
    if(fp_v == NULL)
    {
        printf("fp_v open failed\n");
    }

    fp_a = fopen("./a.aac","wb");
    if(fp_a == NULL)
    {
        printf("fp_a open failed\n");
    }

    InitMp4Reader(&mp4_read);

    OpenMp4Reader("./test.mp4",&mp4_read);

    InitMp4Encoder(1280,720,mp4_read.m_vFrateR,&mp4_write);

    Mp4SetProp(1280,720,mp4_read.m_vFrateR,8000,1,16,&mp4_write);

    OpenMp4Encoder("./mp4v2.mp4",&mp4_write);

    while(1)
    {
        memset(v_buff,0,sizeof(v_buff));
        v_ret = Mp4ReadVideo(v_buff,&v_len,&key_frame,&v_time,&mp4_read);
        if(v_ret)
        {
            fwrite(v_buff, 1, v_len, fp_v);
//            printf("v_len %d\n",v_len);
//            printf("v_time %llu\n",v_time);
            Mp4VEncode(v_buff,v_len,key_frame,v_time * 10,&mp4_write);
        }

        memset(a_buff,0,sizeof(a_buff));
        a_ret = Mp4ReadAudio(a_buff,&a_len,&a_time,&mp4_read);
        if(a_ret)
        {
            fwrite(a_buff, 1, a_len, fp_a);
            printf("a_len %d\n",a_len);
//            printf("a_time %llu\n",a_time);
            Mp4AEncode(a_buff,a_len,&mp4_write);
        }

        if((a_ret == false) && (v_ret == false))
        {
            printf("end of read mp4\n");
            break;
        }
    }

    fclose(fp_v);
    fclose(fp_a);
    CloseMp4Reader(&mp4_read);
    CloseMp4Encoder(&mp4_write);
}

int main()
{
    read_mp4_to_buff();
    return 0;
}
