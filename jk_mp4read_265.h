#ifndef _MP4READ_265_H_
#define _MP4READ_265_H_

#include <gpac/media_tools.h>
#include <gpac/download.h>
#include <gpac/network.h>
#include <gpac/utf.h>
#include <gpac/scene_manager.h>
#include <gpac/ietf.h>
#include <gpac/ismacryp.h>
#include <gpac/constants.h>

#include <gpac/internal/mpd.h>
#include <gpac/tools.h>
#include <time.h>

#include <gpac/setup.h>
#include <time.h>

#include <gpac/internal/media_dev.h>
#include <gpac/internal/isomedia_dev.h>
#include <gpac/mpegts.h>
#include <gpac/constants.h>

#include <gpac/internal/avilib.h>
#include <gpac/internal/ogg.h>
#include <gpac/internal/vobsub.h>
#include <zlib.h>
#include <gpac/isomedia.h>


#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    TRAC_ACTION_REM_TRACK		= 0,
    TRAC_ACTION_SET_LANGUAGE	= 1,
    TRAC_ACTION_SET_DELAY		= 2,
    TRAC_ACTION_SET_KMS_URI		= 3,
    TRAC_ACTION_SET_PAR			= 4,
    TRAC_ACTION_SET_HANDLER_NAME= 5,
    TRAC_ACTION_ENABLE			= 6,
    TRAC_ACTION_DISABLE			= 7,
    TRAC_ACTION_REFERENCE		= 8,
    TRAC_ACTION_RAW_EXTRACT		= 9,
    TRAC_ACTION_REM_NON_RAP		= 10,
    TRAC_ACTION_SET_KIND		= 11,
    TRAC_ACTION_REM_KIND		= 12,
    TRAC_ACTION_SET_ID			= 13,
    TRAC_ACTION_SET_UDTA		= 14,
    TRAC_ACTION_SWAP_ID			= 15,
    TRAC_ACTION_REM_NON_REFS	= 16,
} TrackActionType;

typedef struct
{
    TrackActionType act_type;
    u32 trackID;
    char lang[10];
    s32 delay_ms;
    const char *kms;
    const char *hdl_name;
    s32 par_num, par_den;
    u32 dump_type, sample_num;
    char *out_name;
    char *src_name;
    u32 udta_type;
    char *kind_scheme, *kind_value;
    u32 newTrackID;
} TrackAction;

class JK_Gpac_H265
{
public:
    JK_Gpac_H265()
    {
        printf("JK_Gpac_H265 init\n");
        ret_video = 0;
        ret_audio = 0;
        audio_total_count = 0;
        audio_total_count = 0;
        video_current = 0;
        audio_current = 0;
        video_nbBits = 0;
        audio_nbBits = 0;
        track_dump_type_v = 0;
        track_dump_type_a = 0;
        nb_track_act_v = 0;
        nb_track_act_a = 0;
        audio_write_cnt = 0;
        vps_info[32] = {0};
        sps_info[40] = {0};
        pps_info[16] = {0};
        vps_len = 0;
        sps_len = 0;
        pps_len = 0;
        video_tracks = NULL;
        audio_tracks = NULL;
    }
    void mp4box_cleanup();
    int gf_audio_export_native(GF_MediaExporter *dumper,unsigned char * p_video, u32 current_count,u64 *time_scale);
    int gf_video_export_native(GF_MediaExporter *dumper,unsigned char * p_video,u32 current_count,u64 *time_scale);
    u32 create_new_track_action(int decode_style, TrackAction **actions, u32 *nb_track_act, u32 dump_type);
    int read_mp4_h265(GF_ISOFile *file,unsigned char *buffer_video_playbck,int decode_style,u32 count, u64 *time_scale,char *sei_info_path);
    int read_mp4_audio(GF_ISOFile *file,unsigned char *buffer_video_playbck,int decode_style, u32 count,u64 *time_scale);
    void jk_BS_WriteBit(unsigned char *buffer, u32 value,int count);
    void jk_gf_bs_write_int(s32 _value, s32 nBits,unsigned char *buffer,int count);
    void jk_gf_bs_write_u32(u32 value,unsigned char *buffer,int count);
    void jk_gf_bs_write_u8(u32 value,unsigned char *buffer,int count);
    void add_video_head(unsigned char *buffer,int count);
    void jk_gf_bs_write_data(const char *data, u32 nbBytes,unsigned char *buffer,int count);
    void jk_audio_WriteBit(unsigned char *buffer, u32 value,int count);
    void jk_gf_audio_write_int(s32 _value, s32 nBits,unsigned char *buffer,int count);
    void jk_gf_audio_write_data(const char *data, u32 nbBytes,unsigned char *buffer,int count);

private:
    TrackAction *video_tracks;
    TrackAction *audio_tracks;
    u32 nb_track_act_v,nb_track_act_a;
    u32 track_dump_type_v,track_dump_type_a;
    u32 ret_video,ret_audio;
    u32 video_total_count,audio_total_count;
    GF_MediaExporter video_mdump,audio_mdump;
    u32 video_track,audio_track;
    u32 video_current,audio_current;
    u32 video_nbBits,audio_nbBits;
    GF_M4ADecSpecInfo a_cfg;
    GF_DecoderConfig *dcfg;
    GF_AVCConfig *avccfg, *svccfg, *mvccfg;
    GF_HEVCConfig *hevccfg, *lhvccfg;
    u32 aac_type, aac_mode;
    Bool add_ext;
    u32 audio_write_cnt;
    char vps_info[32],sps_info[40],pps_info[16];
    int vps_len,sps_len,pps_len;
    FILE *sei_info_fd;
};

#ifdef __cplusplus
}
#endif

#endif
