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
//#include <ffmpeg_android/libavutil/time.h>
#include <time.h>

#include <gpac/internal/media_dev.h>
#include <gpac/internal/isomedia_dev.h>
#include <gpac/mpegts.h>
#include <gpac/constants.h>

#include <gpac/internal/avilib.h>
#include <gpac/internal/ogg.h>
#include <gpac/internal/vobsub.h>
#include <zlib.h>

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

/*QCP codec GUIDs*/
static const char *QCP_QCELP_GUID_1 = "\x41\x6D\x7F\x5E\x15\xB1\xD0\x11\xBA\x91\x00\x80\x5F\xB4\xB9\x7E";
static const char *QCP_EVRC_GUID = "\x8D\xD4\x89\xE6\x76\x90\xB5\x46\x91\xEF\x73\x6A\x51\x00\xCE\xB4";
static const char *QCP_SMV_GUID = "\x75\x2B\x7C\x8D\x97\xA7\x46\xED\x98\x5E\xD5\x3C\x8C\xC7\x5F\x84";

#define DUMP_AVCPARAM(_params) \
        count = gf_list_count(_params); \
        for (i=0;i<count;i++) { \
            GF_AVCConfigSlot *sl = (GF_AVCConfigSlot *)gf_list_get(_params, i); \
            gf_bs_write_u32(bs, 1); \
            gf_bs_write_data(bs, sl->data, sl->size); \
        } \

#define DUMP_HEVCPARAM(_params) \
    count = gf_list_count(_params->param_array); \
    for (i=0;i<count;i++) { \
        u32 j; \
        GF_HEVCParamArray *ar = (GF_HEVCParamArray *)gf_list_get(_params->param_array, i); \
        for (j=0; j<gf_list_count(ar->nalus); j++) { \
            GF_AVCConfigSlot *sl = (GF_AVCConfigSlot *)gf_list_get(ar->nalus, j); \
            gf_bs_write_u32(bs, 1); \
            gf_bs_write_data(bs, sl->data, sl->size); \
        } \
    } \

#define VIDEO_BUF_SIZE                 (1024*400)
GF_MemTrackerType mem_track = GF_MemTrackerNone;
char *inName, *outName;
TrackAction *tracks = NULL;
u32 nb_track_act;
u32 track_dump_type;
//Double interleaving_time;
GF_ISOFile *file;
Bool force_new,print_info;
u32 dump_isom;
char *tmpdir;
GF_Err e;
char outfile[5000];
Bool  needSave;
Bool verbose,open_edit;
#define DEFAULT_INTERLEAVING_IN_SEC 0.5
unsigned char buffer_video_playbck[VIDEO_BUF_SIZE]={0};

int jk_gf_bs_write_int(s32 _value, s32 nBits,unsigned char *buffer,int count);
int jk_gf_bs_write_u32(u32 value,unsigned char *buffer,int count);
int jk_gf_bs_write_u8(u32 value,unsigned char *buffer,int count);
int jk_gf_bs_write_data(const char *data, u32 nbBytes,unsigned char *buffer,int count);
void jk_gf_audio_write_data(const char *data, u32 nbBytes,unsigned char *buffer,int count);

//static GF_Err gf_export_message(GF_MediaExporter *dumper, GF_Err e, char *format, ...)
//{
//    if (dumper->flags & GF_EXPORT_PROBE_ONLY) return e;

//#ifndef GPAC_DISABLE_LOG
//    if (gf_log_tool_level_on(GF_LOG_AUTHOR, e ? GF_LOG_ERROR : GF_LOG_WARNING)) {
//        va_list args;
//        char szMsg[1024];
//        va_start(args, format);
//        vsnprintf(szMsg, 1024, format, args);
//        va_end(args);
//        GF_LOG((u32) (e ? GF_LOG_ERROR : GF_LOG_WARNING), GF_LOG_AUTHOR, ("%s\n", szMsg) );
//    }
//#endif
//    return e;
//}
//GF_EXPORT
//GF_Err gf_media_export_user(GF_MediaExporter *dumper,char *p_video,u32 time_scale)
//{
////    printf(">>>>>>>>>>[%s-%d]  dumper->flags:%s\n",__FUNCTION__,__LINE__,dumper->flags);
//    if (dumper->flags & GF_EXPORT_NATIVE) {
////#ifndef GPAC_DISABLE_MPEG2TS
////        if (dumper->in_name) {
////            char *ext = strrchr(dumper->in_name, '.');
////            if (ext && (!strnicmp(ext, ".ts", 3) || !strnicmp(ext, ".m2t", 4)) ) {
////                printf(">>>>>>>>>>[%s-%d]\n",__FUNCTION__,__LINE__);
////                return gf_media_export_ts_native(dumper);
////            }
////        }
////#endif /*GPAC_DISABLE_MPEG2TS*/
//        printf("111>>>>>>>>>>[%s-%d]\n",__FUNCTION__,__LINE__);
//        return gf_media_export_native(dumper,p_video ,time_scale);
//    }

//    else return GF_NOT_SUPPORTED;
//}

static u32 mp4box_cleanup(u32 ret_code)
{
    printf("111>>>>>>>>>>[%s-%d]\n",__FUNCTION__,__LINE__);

    if (tracks) {
        for (int i = 0; i<nb_track_act; i++) {
            if (tracks[i].out_name)
                gf_free(tracks[i].out_name);
            if (tracks[i].src_name)
                gf_free(tracks[i].src_name);
            if (tracks[i].kind_scheme)
                gf_free(tracks[i].kind_scheme);
            if (tracks[i].kind_value)
                gf_free(tracks[i].kind_value);
        }
        gf_free(tracks);
        tracks = NULL;
    }
    gf_sys_close();
    return ret_code;
}

static GF_Err gf_media_export_native(GF_MediaExporter *dumper,unsigned char * p_video, u32 time_scale)
{
#ifdef GPAC_DISABLE_AV_PARSERS
    return GF_NOT_SUPPORTED;
#else
    GF_Err e = GF_OK;
    Bool add_ext;
    GF_DecoderConfig *dcfg;
    GF_GenericSampleDescription *udesc;
    char szName[1000], GUID[16];
    FILE *out;
    unsigned int *qcp_rates, rt_cnt;	/*contains constants*/
    GF_AVCConfig *avccfg, *svccfg, *mvccfg;
    GF_HEVCConfig *hevccfg, *lhvccfg;
    GF_M4ADecSpecInfo a_cfg;
    const char *stxtcfg;
    GF_BitStream *bs;
    u32 track, i, di, count, m_type, m_stype, dsi_size, qcp_type;
    Bool is_ogg, has_qcp_pad, is_vobsub;
    u32 aac_type, aac_mode;
    char *dsi;
//    QCPRateTable rtable[8];
    Bool is_stdout = GF_FALSE;
    Bool is_webvtt = GF_FALSE;
    dsi_size = 0;
    dsi = NULL;
    hevccfg = NULL;
    avccfg = NULL;
    svccfg = NULL;
    mvccfg = NULL;
    lhvccfg = NULL;
    stxtcfg = NULL;
    GF_HEVCParamArray *ar_t;
    GF_AVCConfigSlot *sl_t;

    int count_video = 0;
    int count_audio = 0;
    int test_cnt = 1;

    GF_HEVCConfig *bsaaa;

    if (!(track = gf_isom_get_track_by_id(dumper->file, dumper->trackID))) {
        printf("111>>>>>>>>>>[%s-%d]\n",__FUNCTION__,__LINE__);
        GF_LOG(GF_LOG_ERROR, GF_LOG_AUTHOR, ("Wrong track ID %d for file %s \n", dumper->trackID, gf_isom_get_filename(dumper->file)));
        return GF_BAD_PARAM;
    }

//    m_type = gf_isom_get_media_type(dumper->file, track);
    m_stype = gf_isom_get_media_subtype(dumper->file, track, 1);
    has_qcp_pad = GF_FALSE;
    if (dumper->out_name && !strcmp(dumper->out_name, "std")) {
        printf(">>>>>>>>>>[%s-%d]\n",__FUNCTION__,__LINE__);
        is_stdout = GF_TRUE;
    }

    aac_mode = aac_type = 0;
    qcp_type = 0;
    is_ogg = GF_FALSE;
    is_vobsub = GF_FALSE;
    udesc = NULL;

    /* check if the output file name needs an extension or already has one 检查输出文件名是否需要扩展名或已经有了扩展名*/
    if (dumper->out_name) {
        char *lastPathPart;
        printf("111>>>>>>>>>>[%s-%d]\n",__FUNCTION__,__LINE__);
        lastPathPart = strrchr(dumper->out_name , GF_PATH_SEPARATOR);
        if (!lastPathPart) {
            lastPathPart = dumper->out_name;
        } else {
            lastPathPart++;
        }
        if (strrchr(lastPathPart , '.')==NULL) {
            add_ext =  GF_TRUE;
        } else {
            add_ext = (dumper->flags & GF_EXPORT_FORCE_EXT) ? GF_TRUE : GF_FALSE;
        }
    } else {
        printf(">>>>>>>>>>[%s-%d]\n",__FUNCTION__,__LINE__);
        add_ext = GF_FALSE;
    }
    strcpy(szName, dumper->out_name ? dumper->out_name : "");

    /* Find the decoder configuration:
       - Check first if the stream is carried according to MPEG-4 Systems,
       i.e. using the Object Descriptor Framework, to get the Decoder Specific Info (dsi)
       - Or get it specifically depending on the media type
      找到解码器配置:-首先检查流是否按照MPEG-4系统传送，即使用对象描述符框架，以获取特定于解码器的信息(dsi)-或得到它具体取决于媒体类型 */
    dcfg = NULL;
    if ((m_stype==GF_ISOM_SUBTYPE_MPEG4) || (m_stype==GF_ISOM_SUBTYPE_MPEG4_CRYP)) {
        printf("222>>>>>>>>>>[%s-%d]\n",__FUNCTION__,__LINE__);
        dcfg = gf_isom_get_decoder_config(dumper->file, track, 1);
    }

    if (dcfg) {
        printf("222>>>>>>>>>>[%s-%d]dcfg->streamType:%d  dcfg->objectTypeIndication:%d\n",__FUNCTION__,__LINE__,dcfg->streamType,dcfg->objectTypeIndication);
        switch (dcfg->streamType)
        {
            case GF_STREAM_AUDIO:           ///////wzj 5
                switch (dcfg->objectTypeIndication) {       ///////wzj 64
                case GPAC_OTI_AUDIO_AAC_MPEG4:              ///wzj 64
                    if (!dcfg->decoderSpecificInfo) {
//                        gf_export_message(dumper, GF_OK, "Could not extract MPEG-4 AAC: descriptor specific info not found");
                        gf_odf_desc_del((GF_Descriptor *) dcfg);
                        return GF_NON_COMPLIANT_BITSTREAM;
                    }
                    dsi = dcfg->decoderSpecificInfo->data;
                    dcfg->decoderSpecificInfo->data = NULL;
                    dsi_size = dcfg->decoderSpecificInfo->dataLength;
                    aac_mode = 2;
                    if (add_ext)
                        strcat(szName, ".aac");
//                    gf_export_message(dumper, GF_OK, "Extracting MPEG-4 AAC");
                    break;
                /*fall through*/
                default:
                    gf_odf_desc_del((GF_Descriptor *) dcfg);
//                    return gf_export_message(dumper, GF_NOT_SUPPORTED, "Unknown audio in track ID %d - use NHNT", dumper->trackID);
                    return (GF_Err)-1;
                }
                break;
            default:
                gf_odf_desc_del((GF_Descriptor *) dcfg);
//                return gf_export_message(dumper, GF_NOT_SUPPORTED, "Cannot extract systems track ID %d in raw format - use NHNT", dumper->trackID);
            return (GF_Err)-1;
        }
        gf_odf_desc_del((GF_Descriptor *) dcfg);
    }
    else
    {
        // Not using the MPEG-4 Descriptor Framework 没有使用MPEG-4描述符框架/
            if ((m_stype==GF_ISOM_SUBTYPE_HEV1)
                   || (m_stype==GF_ISOM_SUBTYPE_HVC1)
                   || (m_stype==GF_ISOM_SUBTYPE_HVC2)
                   || (m_stype==GF_ISOM_SUBTYPE_HEV2)
                   || (m_stype==GF_ISOM_SUBTYPE_LHV1)
                   || (m_stype==GF_ISOM_SUBTYPE_LHE1)
                   || (m_stype==GF_ISOM_SUBTYPE_HVT1)
                  )
        {
            printf("333>>>>>>>>>>[%s-%d]  \n", __FUNCTION__, __LINE__);
            hevccfg = gf_isom_hevc_config_get(dumper->file, track, 1);
            lhvccfg = gf_isom_lhvc_config_get(dumper->file, track, 1);
            if (add_ext)
                strcat(szName, ".hvc");
//            gf_export_message(dumper, GF_OK, "Extracting MPEG-H HEVC stream to hevc");
        }
    }
    if (aac_mode) {
        printf("222>>>>>>>>>>[%s-%d]  \n", __FUNCTION__, __LINE__);
        gf_m4a_get_config(dsi, dsi_size, &a_cfg);
        if (aac_mode==2) aac_type = a_cfg.base_object_type - 1;
        gf_free(dsi);
        dsi = NULL;
    }

    out = gf_fopen(szName, "wb");

    bs = gf_bs_new(NULL, 0, GF_BITSTREAM_WRITE);
    unsigned char vps[32],sps[40],pps[16];
    int vps_len,sps_len,pps_len;
    int n;


    if (hevccfg) {
        printf("333>>>>>>>>>>[%s-%d]\n",__FUNCTION__,__LINE__);
        //DUMP_HEVCPARAM(hevccfg);
        count = gf_list_count(hevccfg->param_array);
//        printf("count : %d\n",count);
        for (i=0;i<count;i++) {
            u32 j;
            GF_HEVCParamArray *ar = (GF_HEVCParamArray *)gf_list_get(hevccfg->param_array, i);
            for (j=0; j<gf_list_count(ar->nalus); j++) {
                GF_AVCConfigSlot *sl = (GF_AVCConfigSlot *)gf_list_get(ar->nalus, j);
                gf_bs_write_u32(bs, 1);
                gf_bs_write_data(bs, sl->data, sl->size);
                if (i == 0){
                    vps_len = sl->size;
                    memcpy(vps, sl->data,sl->size);
                    printf("vps_len:%d\n",vps_len);
                    for(n = 0;n < vps_len;n++)
                    {
                        printf("dsi[%d]:%2hhx\n",n,vps[n]);
                    }
                }
                else if (i == 1){
                    sps_len = sl->size;
                    memcpy(sps, sl->data,sl->size);
                    for(n = 0;n < sps_len;n++)
                    {
                        printf("dsi[%d]:%2hhx\n",n,sps[n]);
                    }
                }
                else if (i == 2){
                    pps_len = sl->size;
                    memcpy(pps, sl->data,sl->size);
                    for(n = 0;n < pps_len;n++)
                    {
                        printf("dsi[%d]:%2hhx\n",n,pps[n]);
                    }
                }

//                printf("sl->size : % d\n",sl->size);
//                for(int n = 0;n < sl->size;n++)
//                {
//                    printf("dsi[%d]:%2hhx\n",n,sl->data[n]);
//                }
//                gf_bs_write_data_user(bs, sl->data, sl->size);
            }
        }
    }

    if (avccfg || hevccfg || lhvccfg) {
        printf("333>>>>>>>>>>[%s-%d]\n",__FUNCTION__,__LINE__);
        gf_bs_get_content(bs, &dsi, &dsi_size);
        gf_bs_del(bs);

        /* Start writing the stream out 开始写出流*/
        bs = gf_bs_from_file(out, GF_BITSTREAM_WRITE);
    } else {
        /* Start writing the stream out */
        printf("222>>>>>>>>>>[%s-%d]\n",__FUNCTION__,__LINE__);
        gf_bs_del(bs);
        bs = gf_bs_from_file(out, GF_BITSTREAM_WRITE);

        if (dsi) {
            printf(">>>>>>>>>>[%s-%d]\n",__FUNCTION__,__LINE__);
            gf_bs_write_data(bs, dsi, dsi_size);
            dsi=NULL;
        }
    }
    count = gf_isom_get_sample_count(dumper->file, track);

    /*QCP formatting QCP格式化*/
    qcp_rates = NULL;
    rt_cnt = 0;

    printf("111>>>>>>>>>>[%s-%d] count:%d \n", __FUNCTION__, __LINE__,count);

    /* Start exporting samples 开始输出采样*/
    for (i=time_scale; i<count; i++)
    {
        //printf("Start exporting samples\n");
        GF_ISOSample *samp = gf_isom_get_sample(dumper->file, track, i+1, &di);
//        FILE *test_fd = fopen("/home/wangming/gpac-0.8.0/bin/gcc/src.hvc","ab+");
//        fwrite(samp->data, samp->dataLength,1,test_fd);
//        fclose(test_fd);
        if (!samp) {
            printf(">>>>>>>>>>[%s-%d]\n",__FUNCTION__,__LINE__);
            e = gf_isom_last_error(dumper->file);
            break;
        }
        /*AVC sample to NALU  把AVC样品送到NALU*/
        if (avccfg || svccfg || mvccfg || hevccfg || lhvccfg)
        {
//            printf(">>>>>>>>>>[%s-%d]\n",__FUNCTION__,__LINE__);
            u32 j, nal_size, remain, nal_unit_size;
            Bool is_rap;
            Bool has_aud = GF_FALSE;
            Bool write_dsi = GF_FALSE;
            char *ptr = samp->data;
            nal_unit_size = 0;
            if (avccfg)
               nal_unit_size= avccfg->nal_unit_size;
            else if (svccfg)
                nal_unit_size = svccfg->nal_unit_size;
            else if (mvccfg)
                nal_unit_size = mvccfg->nal_unit_size;
            else if (hevccfg)
            {
//                printf("333>>>>>>>>>>[%s-%d]\n",__FUNCTION__,__LINE__);
                nal_unit_size = hevccfg->nal_unit_size;
            }
            else if (lhvccfg)
                nal_unit_size = lhvccfg->nal_unit_size;

            is_rap = (Bool)samp->IsRAP;
            //patch for opengop
            if (!is_rap) {
//               printf("333>>>>>>>>>>[%s-%d]\n",__FUNCTION__,__LINE__);
               gf_isom_get_sample_rap_roll_info(dumper->file, track, i+1, &is_rap, NULL, NULL);

                if (!is_rap) {
//                    printf("333>>>>>>>>>>[%s-%d]\n",__FUNCTION__,__LINE__);
                    u32 is_leading, dependsOn, dependedOn, redundant;
                    gf_isom_get_sample_flags(dumper->file, track, i+1, &is_leading, &dependsOn, &dependedOn, &redundant);
                    if (dependsOn==2) is_rap = GF_TRUE;
                }
            }

            if (is_rap) {
//                printf("333>>>>>>>>>>[%s-%d]\n",__FUNCTION__,__LINE__);
            }

            if (dsi && (is_rap || !i) ) {
//                printf("333>>>>>>>>>>[%s-%d]\n",__FUNCTION__,__LINE__);
                write_dsi = GF_TRUE;
            }

            remain = samp->dataLength;
            while (remain)
            {
                Bool is_aud = GF_FALSE;
                nal_size = 0;
                if (remain<nal_unit_size) {
                    GF_LOG(GF_LOG_ERROR, GF_LOG_AUTHOR, ("Sample %d (size %d): Corrupted NAL Unit: header size %d - bytes left %d\n", i+1, samp->dataLength, nal_unit_size, remain) );
                    break;
                }
                for (j=0; j<nal_unit_size; j++)
                {
                    nal_size |= ((u8) *ptr);
                    if (j+1<nal_unit_size) nal_size<<=8;
                    remain--;
                    ptr++;
                }

                if (remain < nal_size) {
                    GF_LOG(GF_LOG_ERROR, GF_LOG_AUTHOR, ("Sample %d (size %d): Corrupted NAL Unit: size %d - bytes left %d\n", i+1, samp->dataLength, nal_size, remain) );
                    nal_size = remain;
                }

                if (avccfg || svccfg || mvccfg) {
                    u8 nal_type = ptr[0] & 0x1F;
                    if (nal_type==GF_AVC_NALU_ACCESS_UNIT) is_aud = GF_TRUE;
                }
                else {
                    u8 nal_type = (ptr[0] & 0x7E) >> 1;
                    if (nal_type==GF_HEVC_NALU_ACCESS_UNIT) is_aud = GF_TRUE;
                }

                if (is_aud)
                {
                    if (!has_aud) {
                        printf(">>>>>>>>>>[%s-%d]\n",__FUNCTION__,__LINE__);
                        gf_bs_write_u32(bs, 1);
                        gf_bs_write_data(bs, ptr, nal_size);
//                        jk_gf_bs_write_u32(1,p_video,count_video);

                        has_aud = GF_TRUE;
                    }
                    ptr += nal_size;
                    remain -= nal_size;
                    continue;
                }

                if (!has_aud)
                {
                    has_aud = GF_TRUE;
                    gf_bs_write_u32(bs, 1);

                    if (avccfg || svccfg || mvccfg) {
                        printf(">>>>>>>>>>[%s-%d]\n",__FUNCTION__,__LINE__);
                        u32 hdr = ptr[0] & 0x60;
                        hdr |= GF_AVC_NALU_ACCESS_UNIT;
                        gf_bs_write_u8(bs, hdr);
                        gf_bs_write_u8(bs, 0xF0);
//                        memcpy(p_video + count_video, &hdr, sizeof(u32));
//                        count_video = count_video + 1;
//                        memcpy(p_video + count_video, "\0xF0", sizeof(char));
//                        count_video = count_video + 1;

                    }
                    else
                    {
                        //just copy the current nal header, patching the nal type to AU delim
                        //只需复制当前的nal头，将nal类型打补丁到AU delim
//                        printf(">>>>>>>>>>[%s-%d]\n",__FUNCTION__,__LINE__);
                        u32 hdr = ptr[0] & 0x81;
                        hdr |= GF_HEVC_NALU_ACCESS_UNIT << 1;
                        gf_bs_write_u8(bs, hdr);
                        gf_bs_write_u8(bs, ptr[1]);
//                        memcpy(p_video + count_video, &hdr, sizeof(u32));
//                        count_video = count_video + 1;
//                        memcpy(p_video + count_video, &ptr[1], sizeof(char));
//                        count_video = count_video + 1;

                        //pic-type - by default we signal all slice types possible
                        //pictype—默认情况下，我们标记所有可能的片类型//
                        gf_bs_write_int(bs, 2, 3);
                        gf_bs_write_int(bs, 0, 5);
//                        memcpy(p_video + count_video, &ptr[1], sizeof(char));
//                        count_video = count_video + sizeof(char);
//                        memcpy(p_video + count_video, &ptr[1], sizeof(char));
//                        count_video = count_video + sizeof(char);

                    }
                }

                if (write_dsi) {
                    write_dsi = GF_FALSE;
//                    printf(">>>>>>>>>>[%s-%d]\n",__FUNCTION__,__LINE__);
                    gf_bs_write_data(bs, dsi, dsi_size);
//                    for(int m = 0;m < 89;m++)
//                    {
//                        printf("dsi[%d]:%2hhx\n",m,dsi[m]);
//                    }
//                    printf("dsi : %x\n",dsi);
//                    printf("dsi_size : %d\n",dsi_size);
//                    gf_bs_write_data_user(bs, dsi, dsi_size);
//                    memcpy(p_video + count_video, dsi, dsi_size);
//                    count_video = count_video + dsi_size;

                }

                gf_bs_write_u32(bs, 1);
//                printf("nal_size:%d\n",nal_size);
                gf_bs_write_data(bs, ptr, nal_size);
//                gf_bs_write_data_user(bs, ptr, nal_size);
//                memcpy((p_video + count_video), "\x01", 1);
//                count_video = count_video + 1;
//                memcpy(p_video + count_video, ptr, nal_size);
//                count_video = count_video + nal_size;

                ptr += nal_size;
                remain -= nal_size;
            }
        }
        /*adts frame header 帧头*/
        else if (aac_mode > 0) {
           printf("222>>>>>>>>>>[%s-%d]\n",__FUNCTION__,__LINE__);
            gf_bs_write_int(bs, 0xFFF, 12);/*sync*/
            gf_bs_write_int(bs, (aac_mode==1) ? 1 : 0, 1);/*mpeg2 aac*/
            gf_bs_write_int(bs, 0, 2); /*layer*/
            gf_bs_write_int(bs, 1, 1); /* protection_absent*/
            gf_bs_write_int(bs, aac_type, 2);
            gf_bs_write_int(bs, a_cfg.base_sr_index, 4);
            gf_bs_write_int(bs, 0, 1);
            gf_bs_write_int(bs, a_cfg.nb_chan, 3);
            gf_bs_write_int(bs, 0, 4);
            gf_bs_write_int(bs, 7+samp->dataLength, 13);
        }
        /*fix rate octet for QCP        固定QCP的速率*/
        else if (qcp_type) {
            u32 j;
            printf(">>>>>>>>>>[%s-%d]\n",__FUNCTION__,__LINE__);
            for (j=0; j<rt_cnt; j++) {
                if (qcp_rates[2*j+1]==1+samp->dataLength) {
                    gf_bs_write_u8(bs, qcp_rates[2*j]);
                    break;
                }
            }
        }
        /*AV1: add Temporal Unit Delimiters     AV1:添加时态单元分隔符*/
        else if (m_stype == GF_ISOM_SUBTYPE_AV01) {
           printf(">>>>>>>>>>[%s-%d]\n",__FUNCTION__,__LINE__);
           gf_bs_write_u8(bs, 0x12);
            gf_bs_write_u8(bs, 0x00);
        }

        if (!avccfg && !svccfg && !mvccfg && !hevccfg && !lhvccfg &!is_webvtt) {
//            printf("222>>>>>>>>>>[%s-%d]\n",__FUNCTION__,__LINE__);
//            printf("count_audio =%d\n",count_audio);
//            if(count_audio == 8)
//            {
//                count_audio=0;
//            }
//            count_audio++;
            printf(">>>>>>>>>>[%s-%d]\n",__FUNCTION__,__LINE__);
            gf_bs_write_data(bs, samp->data, samp->dataLength);

            if (test_cnt == 1 ){
                int my_cnt = 0;
//                jk_gf_audio_write_data(samp->data,samp->dataLength,p_video,my_cnt);
//                printf("samp->data : %x \n",samp->data);
//                printf("samp->dataLength : %d \n",samp->dataLength);
                test_cnt = 0;
            }
        }
        if (samp->nb_pack)
            i += samp->nb_pack-1;

        gf_isom_sample_del(&samp);
        gf_set_progress("Media Export", i+1, count);
        if (dumper->flags & GF_EXPORT_DO_ABORT) break;
    }
    if (has_qcp_pad) {
        gf_bs_write_u8(bs, 0);
//        count_audio = jk_gf_bs_write_u8(0,p_video,count_audio);
        printf(">>>>>>>>>>[%s-%d]\n",__FUNCTION__,__LINE__);
    }
    printf("111>>>>>>>>>>[%s-%d]  \n", __FUNCTION__, __LINE__);
//    bsaaa = gf_odf_hevc_cfg_read(bs,sizeof(bs),GF_FALSE);//GF_FALSE GF_TRUE
exit:
    if (avccfg) gf_odf_avc_cfg_del(avccfg);
    if (svccfg) gf_odf_avc_cfg_del(svccfg);
    if (mvccfg) gf_odf_avc_cfg_del(mvccfg);
    if (hevccfg) gf_odf_hevc_cfg_del(hevccfg);
    if (lhvccfg) gf_odf_hevc_cfg_del(lhvccfg);
    gf_bs_del(bs);
    if (dsi) gf_free(dsi);
    if (!is_stdout)
        gf_fclose(out);
    return e;
#endif /*GPAC_DISABLE_AV_PARSERS*/
}


static u32 create_new_track_action(int decode_style, TrackAction **actions, u32 *nb_track_act, u32 dump_type)
{
     printf("111>>>>>>>>>>[%s-%d]\n",__FUNCTION__,__LINE__);

    *actions = (TrackAction *)gf_realloc(*actions, sizeof(TrackAction) * (*nb_track_act+1));
    memset(&(*actions)[*nb_track_act], 0, sizeof(TrackAction) );
    (*actions)[*nb_track_act].act_type = TRAC_ACTION_RAW_EXTRACT;
    (*actions)[*nb_track_act].dump_type = dump_type;

    (*actions)[*nb_track_act].trackID = decode_style;
    if((*actions)[*nb_track_act].trackID == 3)
    {
        (*actions)[*nb_track_act].trackID = (u32) -1;
    }
//    parse_track_action_params(string, &(*actions)[*nb_track_act]);
    (*nb_track_act)++;
    return dump_type;
}



//file_name 需要解析的文件的名字   decode_style 解析类型（1：解析成hvc流 2：aac流 3:全部）
int read_mp4_h265(char *file_name,unsigned char *buffer_video_playbck,int decode_style, u32 time_scale)
{
    verbose = open_edit = GF_FALSE;
    tmpdir = NULL;
    needSave = GF_FALSE;
    inName = outName = NULL;

    gf_sys_init(mem_track);

//    char *arg_val = arg;
    inName = file_name;
    track_dump_type = create_new_track_action(decode_style, &tracks, &nb_track_act, GF_EXPORT_NATIVE);

//    printf("nb_track_act : %d \n",nb_track_act);
//    interleaving_time = DEFAULT_INTERLEAVING_IN_SEC;

    FILE *st = gf_fopen(inName, "rb");
    Bool file_exists = (Bool) 0;
    if (st) {
        file_exists = (Bool)1;
        gf_fclose(st);
    }
    file = gf_isom_open(inName, (u8) (force_new ? GF_ISOM_WRITE_EDIT : (open_edit ? GF_ISOM_OPEN_EDIT : ( ((dump_isom>0) || print_info) ? GF_ISOM_OPEN_READ_DUMP : GF_ISOM_OPEN_READ) ) ), tmpdir);

    strcpy(outfile, outName ? outName : inName);

    if (track_dump_type)
    {
        printf("111>>>>>>>>>>[%s-%d]\n",__FUNCTION__,__LINE__);
        char szFile[1024];
        GF_MediaExporter mdump;
        for (int i=0; i<nb_track_act; i++)
        {
            TrackAction *tka = &tracks[i];
            if (tka->act_type != TRAC_ACTION_RAW_EXTRACT) continue;
            memset(&mdump, 0, sizeof(mdump));
            mdump.file = file;
            mdump.flags = tka->dump_type;
            mdump.trackID = tka->trackID;
            mdump.sample_num = tka->sample_num;
//            printf("sample_num = %d\n",mdump.sample_num);
            if (tka->out_name) {
//                printf(">>>>>>>>>>[%s-%d]\n",__FUNCTION__,__LINE__);
                mdump.out_name = tka->out_name;
            }
            else if (outName) {
//                printf(">>>>>>>>>>[%s-%d]\n",__FUNCTION__,__LINE__);
                mdump.out_name = outName;
                mdump.flags |= GF_EXPORT_MERGE;
            }
            else {
                printf("111>>>>>>>>>>[%s-%d]\n",__FUNCTION__,__LINE__);
                sprintf(szFile, "%s_track%d", outfile, mdump.trackID);
                mdump.out_name = szFile;
                mdump.flags |= GF_EXPORT_FORCE_EXT;
            }
//            printf("tka->trackID : %d \n",tka->trackID);
            if (tka->trackID==(u32) -1) {
                u32 j;
                for (j=0; j<gf_isom_get_track_count(file); j++) {
//
                    mdump.trackID = gf_isom_get_track_id(file, j+1);
//                    printf("j : %d\n",j);
//                    printf("mdump.trackID : %d\n",mdump.trackID);
                    sprintf(szFile, "%s_track%d", outfile, mdump.trackID);
                    mdump.out_name = szFile;
                    mdump.flags |= GF_EXPORT_FORCE_EXT;
                    printf("111>>>>>>>>>>[%s-%d] go to printf ok \n",__FUNCTION__,__LINE__);
                    e = gf_media_export_native(&mdump,buffer_video_playbck,time_scale);
                    if (e)
                    {
                        printf(">>>>>>>>>>[%s-%d] go to err_exit\n",__FUNCTION__,__LINE__);
                        goto err_exit;
                    }
                }
            }
            else {
                printf("111>>>>>>>>>>[%s-%d] mdump.flags:%d\n",__FUNCTION__,__LINE__, mdump.flags);

                e = gf_media_export_native(&mdump,buffer_video_playbck,time_scale);
                if (e) goto err_exit;
            }
        }
    }

    if (!open_edit && !needSave) {
        if (file) gf_isom_delete(file);
        printf("111>>>>>>>>>>[%s-%d]\n",__FUNCTION__,__LINE__);

        goto exit;
    }

err_exit:
    /*close libgpac*/
    if (file) gf_isom_delete(file);
    fprintf(stderr, "\n\tError: %s\n", gf_error_to_string(e));
    printf("[%s-%d]Error: %s\n",__FUNCTION__,__LINE__,gf_error_to_string(e));
    return mp4box_cleanup(1);

exit:
    printf("[%s-%d]Error: %s\n",__FUNCTION__,__LINE__,gf_error_to_string(e));

    mp4box_cleanup(0);

    return 0;

}

int jk_gf_bs_write_int(s32 _value, s32 nBits,unsigned char *buffer,int count)
{
    u32 value, nb_shift;
    //move to unsigned to avoid sanitizer warnings when we pass a value not codable on the given number of bits
    //we do this when setting bit fileds to all 1's
    value = (u32) _value;
    nb_shift = sizeof (s32) * 8 - nBits;
    if (nb_shift)
        value <<= nb_shift;

    char buf[4] = {0};

    while (--nBits >= 0) {
        //but check value as signed
        sprintf(buf,"%d",(s32)value < 0);
        memcpy(buffer+count,buf,sizeof(char));
        count++;
        value <<= 1;
    }

    return count;
}

int jk_gf_bs_write_u32(u32 value,unsigned char *buffer,int count)
{

    char buf[16] = {0};
    sprintf(buf,"%d%d%d%d",(u8)((value>>24)&0xff),(u8)((value>>16)&0xff),(u8)((value>>8)&0xff),(u8)((value>>0)&0xff));
    memcpy(buffer+count,buf,sizeof(u32));
    return count += sizeof(u32);
}

int jk_gf_bs_write_u8(u32 value,unsigned char *buffer,int count)
{
    char buf[4] = {0};
    sprintf(buf,"%d",value);
    memcpy(buffer+count,buf,sizeof(u32));
    return count += sizeof(u8);
}


int jk_gf_bs_write_data(const char *data, u32 nbBytes,unsigned char *buffer,int count)
{
    /*we need some feedback for this guy...*/
    while (nbBytes) {
        count = jk_gf_bs_write_int((s32) *data,8,buffer,count);
        data++;
        nbBytes--;
    }
    return count;
}

u32 audio_current = 0;
u32 audio_nbBits = 0;
u32 audio_write_cnt = 0;
u32 write_total_cnt = 0;
static void jk_audio_WriteBit(unsigned char *buffer, u32 value,int count)
{
    audio_current <<= 1;
    printf("value : %x \n",value);
    audio_current |= value;
    if (++audio_nbBits == 8) {
        audio_nbBits = 0;
//        printf("jk_audio_WriteBit cnt : %d\n",count);
        printf("(u8)audio_current : %x \n",audio_current);
        buffer[count] = (u8)audio_current;
        printf("buffer : %x \n",buffer[count]);
        audio_current = 0;
    }
}

void jk_gf_audio_write_int(s32 _value, s32 nBits,unsigned char *buffer,int count)
{
    u32 value, nb_shift;
    //move to unsigned to avoid sanitizer warnings when we pass a value not codable on the given number of bits
    //we do this when setting bit fileds to all 1's
    value = (u32) _value;
    nb_shift = sizeof (s32) * 8 - nBits;
    if (nb_shift)
        value <<= nb_shift;

    while (--nBits >= 0) {
        //but check value as signed
        jk_audio_WriteBit(buffer,((s32)value)<0,count);
        value <<= 1;
        if(++write_total_cnt == 8)
        {
            count += sizeof(u8);
            write_total_cnt = 0;
        }
//        printf("jk_gf_audio_write_int cnt : %d\n",count);
    }
}

void jk_gf_audio_write_data(const char *data, u32 nbBytes,unsigned char *buffer,int count)
{
    if (!nbBytes) return;
    /*we need some feedback for this guy...*/

    if(++audio_write_cnt == 8)
    {
        audio_write_cnt = 0;
        memcpy(buffer+count,data,nbBytes);
    } else {
        while (nbBytes) {
            jk_gf_audio_write_int((s32) *data, 8,buffer,count);
            count += sizeof(u8);
            data++;
            nbBytes--;
//            printf("jk_gf_audio_write_data cnt : %d\n",count);
        }
    }
}

int main()
{
    read_mp4_h265("No4.mp4",buffer_video_playbck ,1, 10);
//    unsigned char buffer[VIDEO_BUF_SIZE];
//    char a[9] = "10001110" ;
//    memset(buffer,0,sizeof(buffer));
//    count_v = jk_gf_bs_write_u32(2,buffer,count_v);
//    count_v = jk_gf_bs_write_int(0xfff,12,buffer,count_v);
//    count_v = jk_gf_bs_write_u8(3,buffer,count_v);
//    count_v = jk_gf_bs_write_int(2,3,buffer,count_v);
//    count_v = jk_gf_bs_write_int(2,5,buffer,count_v);

//    count_v =jk_gf_bs_write_u32(4,buffer,count_v);
//    count_v = jk_gf_bs_write_data(a,1,buffer,count_v);
//    count_v = jk_gf_bs_write_u8(3,buffer,count_v);

//    printf("count_v = %d\n",count_v);
//    printf("buffer : %s\n",buffer);
//    printf("buffer : %s\n",buffer_video_playbck);

    return 0;
}


