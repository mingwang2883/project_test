#ifndef __JK_CMP4_ENCODE_H__
#define __JK_CMP4_ENCODE_H__

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <stddef.h>
#include "mp4v2.h"
#include "CAudioAac.h"
#include "file.h"

#define MAX_VIDEO_RECORD_FRAM_SIZE  (1024 * 350)

#define _NALU_SPS_  0
#define _NALU_PPS_  1
#define _NALU_I_    2
#define _NALU_P_    3
#define _NALU_SEI_  4

#define SPS_5   0x42 
#define SPS_6   0x00
#define SPS_7   0x14

typedef unsigned int  u_int32_t;
typedef unsigned char u_int8_t;

enum
{
    invalid_state,
    sps_state,
    pps_state,
    record_state,
    sps_frame,
    pps_frame,
    i_frame,
};

typedef struct
{
    unsigned char *m_ph264BSBuf;
    unsigned char video_buffer[MAX_VIDEO_RECORD_FRAM_SIZE];

    bool b_isOpen;
    bool m_bFirstAudio;
    bool m_bFirstVideo;

    int state;
    int m_vWidth;
    int m_vHeight;
    int m_vFrateR;
    int	tick_diff;
    int m_nChannal;
    int tick_count;
    int m_vTimeScale;
    int m_nSampleRate;
    int m_bitsPerSample;

    time_t t_start;
    time_t t_end;

    u_int8_t *p_video_sample;
    u_int8_t *p_audio_sample;

    uint64_t m_u64VideoPTS;
    uint64_t m_u64FirstPTS;

    MP4TrackId m_vTrackId;
    MP4TrackId m_aTrackId;

    MP4FileHandle m_mp4FHandle;
}Mp4v2WriteMp4,*pMp4v2WriteMp4;

typedef struct
{
    char sps[100];
    char pps[100];

    int	sps_num;
    int pps_num;
    int m_vWidth;
    int m_vHeight;
    int m_vFrateR;
    int	audio_num_channels;

    u_int8_t audio_config[10];
    u_int8_t *p_video_sample;
    u_int8_t *p_audio_sample;

    u_int32_t audio_config_size;
    u_int32_t n_video_sample;
    u_int32_t n_audio_sample;
    u_int32_t video_timescale;
    u_int32_t audio_timescale;
    u_int32_t video_num_samples;
    u_int32_t audio_num_samples;
    u_int32_t video_sample_max_size;
    u_int32_t audio_sample_max_size;

    MP4SampleId	sampleId_v;
    MP4SampleId	sampleId_a;

    MP4TrackId video_trId;
    MP4TrackId audio_trId;

    MP4Duration video_duration;
    MP4Duration audio_duration;

    MP4FileHandle m_mp4FHandle;
}Mp4v2ReadMp4,*pMp4v2ReadMp4;

/* write mp4 file */
int Mp4VEncode(UBYTE* FrameBuf,int FrameSize,int KeyFrame,unsigned long long pts_duration,pMp4v2WriteMp4 mp4v2_write);
int Mp4AEncode(UBYTE* _aacData,int _aacSize,pMp4v2WriteMp4 mp4v2_write);
bool OpenMp4Encoder(const char * fileName,pMp4v2WriteMp4 mp4v2_write);
void InitMp4Encoder(int width, int height, int frame,pMp4v2WriteMp4 mp4v2_write);
void Mp4SetProp(int width, int height, int fps, int nSampleRate, int nChannal, int bitsPerSample,pMp4v2WriteMp4 mp4v2_write);
void CloseMp4Encoder(pMp4v2WriteMp4 mp4v2_write);

/* read mp4 file */
bool OpenMp4Reader(const char *fileName,pMp4v2ReadMp4 mp4v2_read);
bool Mp4ReadVideo(char *p_video,int *frameLen,int *KeyFrame,MP4Timestamp *time_stamp,pMp4v2ReadMp4 mp4v2_read);
bool Mp4ReadAudio(char *p_audio,int *frameLen,MP4Timestamp *time_stamp,pMp4v2ReadMp4 mp4v2_read);
bool Mp4SeekMedia(int total_second,int ops_second,pMp4v2ReadMp4 mp4v2_read);
bool CloseMp4Reader(pMp4v2ReadMp4 mp4v2_read);

#endif  /* end of __JK_CMP4_ENCODE_H__ */
