#include "jk_mp4read_265.h"

using namespace std;

int JK_Gpac_H265::gf_audio_export_native(GF_MediaExporter *dumper,unsigned char * p_video, u32 current_count,u64 *time_scale)
{
    GF_Err e = GF_OK;

    GF_GenericSampleDescription *udesc;
    u32 i,m_stype, dsi_size;
    Bool has_qcp_pad;
    char *dsi;
    dsi_size = 0;
    dsi = NULL;
    int count_v = 0;
    u32 di;

    if (current_count == 0) {
        if (!(audio_track = gf_isom_get_track_by_id(dumper->file, dumper->trackID))) {
            GF_LOG(GF_LOG_ERROR, GF_LOG_AUTHOR, ("Wrong audio_track ID %d for file %s \n", dumper->trackID, gf_isom_get_filename(dumper->file)));
            return GF_BAD_PARAM;
        }

        m_stype = gf_isom_get_media_subtype(dumper->file, audio_track, 1);
        has_qcp_pad = GF_FALSE;

        aac_mode = aac_type = 0;
        udesc = NULL;
        dcfg = NULL;

        if ((m_stype==GF_ISOM_SUBTYPE_MPEG4) || (m_stype==GF_ISOM_SUBTYPE_MPEG4_CRYP)) {
            dcfg = gf_isom_get_decoder_config(dumper->file, audio_track, 1);
        }


        if (dcfg) {
            switch (dcfg->streamType)
            {
                case GF_STREAM_AUDIO:
                    switch (dcfg->objectTypeIndication) {
                    case GPAC_OTI_AUDIO_AAC_MPEG4:
                        if (!dcfg->decoderSpecificInfo) {
                            gf_odf_desc_del((GF_Descriptor *) dcfg);
                            return GF_NON_COMPLIANT_BITSTREAM;
                        }
                        dsi = dcfg->decoderSpecificInfo->data;
                        dcfg->decoderSpecificInfo->data = NULL;
                        dsi_size = dcfg->decoderSpecificInfo->dataLength;
                        aac_mode = 2;
                        if (add_ext){
                        }
                        break;
                    /*fall through*/
                    default:
                        gf_odf_desc_del((GF_Descriptor *) dcfg);
                        return (GF_Err)-1;
                    }
                    break;
                default:
                    gf_odf_desc_del((GF_Descriptor *) dcfg);
                    return (GF_Err)-1;
            }
            gf_odf_desc_del((GF_Descriptor *) dcfg);
        }
        if (aac_mode) {
            gf_m4a_get_config(dsi, dsi_size, &a_cfg);
            if (aac_mode==2) aac_type = a_cfg.base_object_type - 1;
            gf_free(dsi);
            dsi = NULL;
        }
        if (dsi) {
            jk_gf_audio_write_data(dsi,dsi_size,p_video,count_v);
            count_v += dsi_size*sizeof(u8);
            gf_free(dsi);
            dsi=NULL;
        }

        audio_total_count = gf_isom_get_sample_count(dumper->file, audio_track);
        printf("[%s-%d] audio_count:%d \n", __FUNCTION__, __LINE__,audio_total_count);
    }

    if (current_count >= audio_total_count)
    {
        if (dsi) gf_free(dsi);
        return 0;
    }

    /* Start exporting samples 开始输出采样*/
    for (i = current_count; i < current_count+1; i++)
    {
        GF_ISOSample *samp = gf_isom_get_sample(dumper->file, audio_track, current_count+1, &di);
        if (!samp) {
            e = gf_isom_last_error(dumper->file);
            printf("gf_audio_export_native e : %d\n",e);
            break;
        }

#if 0  //not need header
        /*adts frame header 帧头*/
        if (aac_mode > 0) {
             jk_gf_audio_write_int(0xFFF, 12,p_video,count_v);
             count_v += sizeof(u8);
             jk_gf_audio_write_int((aac_mode==1) ? 1 : 0, 1,p_video,count_v);
             jk_gf_audio_write_int(0, 2,p_video,count_v);
             jk_gf_audio_write_int(1, 1,p_video,count_v);
             count_v += sizeof(u8);
             jk_gf_audio_write_int(aac_type, 2,p_video,count_v);
             jk_gf_audio_write_int(a_cfg.base_sr_index, 4,p_video,count_v);
             jk_gf_audio_write_int(0, 1,p_video,count_v);
             jk_gf_audio_write_int(a_cfg.nb_chan, 3,p_video,count_v);
             count_v += sizeof(u8);
             jk_gf_audio_write_int(0, 4,p_video,count_v);
             jk_gf_audio_write_int(7+samp->dataLength, 13,p_video,count_v);
             count_v += 2*sizeof(u8);
        }
#endif

        jk_gf_audio_write_data(samp->data,samp->dataLength,p_video,count_v);
        count_v += samp->dataLength*sizeof(u8);

        *time_scale = samp->DTS;

        gf_isom_sample_del(&samp);
        gf_set_progress("Audio Export", current_count+1, audio_total_count);
        if (dumper->flags & GF_EXPORT_DO_ABORT) break;
    }

   if (has_qcp_pad) {
       jk_gf_bs_write_u8(0,p_video,count_v);
       count_v += sizeof(u8);
   }

   return count_v;
}

int JK_Gpac_H265::gf_video_export_native(GF_MediaExporter *dumper,unsigned char * p_video, u32 current_count ,u64 *time_scale)
{
    GF_Err e = GF_OK;
    u32 di;
    GF_GenericSampleDescription *udesc;
    u32 i,m_stype;
    Bool has_qcp_pad;
    int count_v = 0;

    if (current_count == 0) {

        hevccfg = NULL;
        avccfg = NULL;
        svccfg = NULL;
        mvccfg = NULL;
        lhvccfg = NULL;

        if (!(video_track = gf_isom_get_track_by_id(dumper->file, dumper->trackID))) {
            GF_LOG(GF_LOG_ERROR, GF_LOG_AUTHOR, ("Wrong video_track ID %d for file %s \n", dumper->trackID, gf_isom_get_filename(dumper->file)));
            return GF_BAD_PARAM;
        }

        m_stype = gf_isom_get_media_subtype(dumper->file, video_track, 1);
        has_qcp_pad = GF_FALSE;
        udesc = NULL;

        /* check if the output file name needs an extension or already has one 检查输出文件名是否需要扩展名或已经有了扩展名*/

        /* Find the decoder configuration:
           - Check first if the stream is carried according to MPEG-4 Systems,
           i.e. using the Object Descriptor Framework, to get the Decoder Specific Info (dsi)
           - Or get it specifically depending on the media type
          找到解码器配置:-首先检查流是否按照MPEG-4系统传送，即使用对象描述符框架，以获取特定于解码器的信息(dsi)-或得到它具体取决于媒体类型 */

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
            hevccfg = gf_isom_hevc_config_get(dumper->file, video_track, 1);
            lhvccfg = gf_isom_lhvc_config_get(dumper->file, video_track, 1);
        }

        if (hevccfg) {
            video_total_count = gf_list_count(hevccfg->param_array);
            for (i=0;i<video_total_count;i++) {
                u32 j;
                GF_HEVCParamArray *ar = (GF_HEVCParamArray *)gf_list_get(hevccfg->param_array, i);
                for (j=0; j<gf_list_count(ar->nalus); j++) {
                    GF_AVCConfigSlot *sl = (GF_AVCConfigSlot *)gf_list_get(ar->nalus, j);
                    jk_gf_bs_write_u32(1,p_video,count_v);
                    count_v += sizeof(u32);
                    jk_gf_bs_write_data(sl->data,sl->size,p_video,count_v);
                    count_v += sl->size*sizeof(u8);
                    if (i == 0) {
                        vps_len = sl->size;
                        memcpy(vps_info, sl->data,sl->size);
                    }
                    else if (i == 1)
                    {
                        sps_len = sl->size;
                        memcpy(sps_info, sl->data,sl->size);
                    }
                    else if (i == 2)
                    {
                        pps_len = sl->size;
                        memcpy(pps_info, sl->data,sl->size);
                    }
                }
            }
        }
        video_total_count = gf_isom_get_sample_count(dumper->file, video_track);
        printf("[%s-%d] video_count:%d \n", __FUNCTION__, __LINE__,video_total_count);
    }

    if (current_count >= video_total_count)
    {
        if (avccfg) gf_odf_avc_cfg_del(avccfg);
        if (svccfg) gf_odf_avc_cfg_del(svccfg);
        if (mvccfg) gf_odf_avc_cfg_del(mvccfg);
        if (hevccfg) gf_odf_hevc_cfg_del(hevccfg);
        if (lhvccfg) gf_odf_hevc_cfg_del(lhvccfg);
        return 0;
    }

    /* Start exporting samples 开始输出采样*/
    for (i = current_count; i < current_count+1; i++)
    {
        GF_ISOSample *samp = gf_isom_get_sample(dumper->file, video_track, current_count+1, &di);
        if (!samp) {
            e = gf_isom_last_error(dumper->file);
            printf("gf_video_export_native e : %d\n",e);
            break;
        }
        /*AVC sample to NALU  把AVC样品送到NALU*/
        if (avccfg || svccfg || mvccfg || hevccfg || lhvccfg)
        {
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
                nal_unit_size = hevccfg->nal_unit_size;
            else if (lhvccfg)
                nal_unit_size = lhvccfg->nal_unit_size;

            is_rap = (Bool)samp->IsRAP;
            //patch for opengop
            if (!is_rap) {
               gf_isom_get_sample_rap_roll_info(dumper->file, video_track, current_count+1, &is_rap, NULL, NULL);

                if (!is_rap) {
                    u32 is_leading, dependsOn, dependedOn, redundant;
                    gf_isom_get_sample_flags(dumper->file, video_track, current_count+1, &is_leading, &dependsOn, &dependedOn, &redundant);
                    if (dependsOn==2) is_rap = GF_TRUE;
                }
            }
            if (is_rap) {
                write_dsi = GF_TRUE;
            }

            remain = samp->dataLength;
            while (remain)
            {
                Bool is_aud = GF_FALSE;
                nal_size = 0;
                if (remain<nal_unit_size) {
                    GF_LOG(GF_LOG_ERROR, GF_LOG_AUTHOR, ("Sample %d (size %d): Corrupted NAL Unit: header size %d - bytes left %d\n", current_count+1, samp->dataLength, nal_unit_size, remain) );
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
                    GF_LOG(GF_LOG_ERROR, GF_LOG_AUTHOR, ("Sample %d (size %d): Corrupted NAL Unit: size %d - bytes left %d\n", current_count+1, samp->dataLength, nal_size, remain) );
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
                        jk_gf_bs_write_u32(1,p_video,count_v);
                        count_v += sizeof(u32);
                        jk_gf_bs_write_data(ptr,nal_size,p_video,count_v);
                        count_v += nal_size*sizeof(u8);
                        has_aud = GF_TRUE;
                    }
                    ptr += nal_size;
                    remain -= nal_size;
                    continue;
                }

                if (!has_aud)
                {
                    has_aud = GF_TRUE;
//                    jk_gf_bs_write_u32(1,p_video,count_v);
//                    count_v += sizeof(u32);

                    if (avccfg || svccfg || mvccfg) {
                        u32 hdr = ptr[0] & 0x60;
                        hdr |= GF_AVC_NALU_ACCESS_UNIT;
                        jk_gf_bs_write_u8(hdr,p_video,count_v);
                        count_v += sizeof(u8);
                        jk_gf_bs_write_u8(0xF0,p_video,count_v);
                        count_v += sizeof(u8);
                    }
#if 0
                    else
                    {
                        //just copy the current nal header, patching the nal type to AU delim
                        //只需复制当前的nal头，将nal类型打补丁到AU delim
                        u32 hdr = ptr[0] & 0x81;
                        hdr |= GF_HEVC_NALU_ACCESS_UNIT << 1;
                        jk_gf_bs_write_u8(hdr,p_video,count_v);
                        count_v += sizeof(u8);
                        jk_gf_bs_write_u8(ptr[1],p_video,count_v);
                        count_v += sizeof(u8);

                        //pic-type - by default we signal all slice types possible
                        //pictype—默认情况下，我们标记所有可能的片类型//
                        jk_gf_bs_write_int(2,3,p_video,count_v);
                        jk_gf_bs_write_int(0,5,p_video,count_v);
                        count_v += sizeof(u8);
                    }
#endif
                }
                if (write_dsi) {
                    write_dsi = GF_FALSE;
                    if (current_count != 0) {
                        add_video_head(p_video,count_v);
                        count_v += 89*sizeof(u8);
                    }
                }
                jk_gf_bs_write_u32(1,p_video,count_v);
                count_v += sizeof(u32);
                jk_gf_bs_write_data(ptr,nal_size,p_video,count_v);
                count_v += nal_size*sizeof(u8);

                ptr += nal_size;
                remain -= nal_size;
            }

        }
        /*AV1: add Temporal Unit Delimiters     AV1:添加时态单元分隔符*/
        else if (m_stype == GF_ISOM_SUBTYPE_AV01) {
            jk_gf_bs_write_u8(0x12,p_video,count_v);
            count_v += sizeof(u8);
            jk_gf_bs_write_u8(0x00,p_video,count_v);
            count_v += sizeof(u8);
        }

        *time_scale = samp->DTS;

        gf_isom_sample_del(&samp);
        gf_set_progress("Video Export", current_count+1, video_total_count);
        if (dumper->flags & GF_EXPORT_DO_ABORT) break;
    }
    if (has_qcp_pad) {
        jk_gf_bs_write_u8(0,p_video,count_v);
        count_v += sizeof(u8);
    }

    return count_v;
}


u32 JK_Gpac_H265::create_new_track_action(int decode_style, TrackAction **actions, u32 *nb_track_act, u32 dump_type)
{
    *actions = (TrackAction *)gf_realloc(*actions, sizeof(TrackAction) * (*nb_track_act+1));
    memset(&(*actions)[*nb_track_act], 0, sizeof(TrackAction) );
    (*actions)[*nb_track_act].act_type = TRAC_ACTION_RAW_EXTRACT;
    (*actions)[*nb_track_act].dump_type = dump_type;

    (*actions)[*nb_track_act].trackID = decode_style;
    if((*actions)[*nb_track_act].trackID == 3)
    {
        (*actions)[*nb_track_act].trackID = (u32) -1;
    }
    (*nb_track_act)++;
    return dump_type;
}

void JK_Gpac_H265::mp4box_cleanup()
{
    if (video_tracks != NULL) {
        gf_free(video_tracks);
        video_tracks = NULL;
    }
    if (audio_tracks != NULL) {
        gf_free(audio_tracks);
        audio_tracks = NULL;
    }
#ifdef GPAC_H265_SEI_INFO
    fclose(sei_info_fd);
#endif //end of GPAC_H265_SEI_INFO
}

//file_name 需要解析的文件的名字   decode_style 解析类型（1：解析成hvc流 2：aac流 3:全部）
int JK_Gpac_H265::read_mp4_h265(GF_ISOFile *file,unsigned char *buffer_video_playbck,int decode_style, u32 count,u64 *time_scale,char *sei_info_path)
{
    if (count == 0) {

        if (sei_info_path != NULL) {
            sei_info_fd = fopen(sei_info_path,"rb");
            if (sei_info_fd == NULL)
            {
                printf("fopen sei_info_fd error !\n");
            }
            fseek(sei_info_fd,2,SEEK_SET);
        }

        track_dump_type_v = create_new_track_action(decode_style, &video_tracks, &nb_track_act_v, GF_EXPORT_NATIVE);
        //Prevent continuous increase
        nb_track_act_v = 1;

        if (track_dump_type_v)
        {
            for (int i=0; i<nb_track_act_v; i++)
            {
                TrackAction *tka = &video_tracks[i];
                if (tka->act_type != TRAC_ACTION_RAW_EXTRACT) continue;
                memset(&video_mdump, 0, sizeof(video_mdump));
                video_mdump.file = file;
                video_mdump.flags = tka->dump_type;
                video_mdump.sample_num = tka->sample_num;
                printf("gf_video_export_native start\n");
                video_mdump.flags |= GF_EXPORT_FORCE_EXT;
                video_mdump.trackID = gf_isom_get_track_id(file, decode_style);
                ret_video = gf_video_export_native(&video_mdump,buffer_video_playbck,count,time_scale);
            }
        } else {
            printf("create_new_track_action error!\n");
        }
    } else {
        ret_video = gf_video_export_native(&video_mdump,buffer_video_playbck,count,time_scale);
    }

    return ret_video;
}

//file_name 需要解析的文件的名字   decode_style 解析类型（1：解析成hvc流 2：aac流 3:全部）
int JK_Gpac_H265::read_mp4_audio(GF_ISOFile *file,unsigned char *buffer_video_playbck,int decode_style, u32 count,u64 *time_scale)
{
    if (count == 0) {
        track_dump_type_a = create_new_track_action(decode_style, &audio_tracks, &nb_track_act_a, GF_EXPORT_NATIVE);
        //Prevent continuous increase
        nb_track_act_a = 1;

        if (track_dump_type_a)
        {
            for (int i=0; i<nb_track_act_a; i++)
            {
                TrackAction *tka = &audio_tracks[i];
                if (tka->act_type != TRAC_ACTION_RAW_EXTRACT) continue;
                memset(&audio_mdump, 0, sizeof(audio_mdump));
                audio_mdump.file = file;
                audio_mdump.flags = tka->dump_type;
                audio_mdump.sample_num = tka->sample_num;
                printf("gf_audio_export_native start\n");
                audio_mdump.flags |= GF_EXPORT_FORCE_EXT;
                audio_mdump.trackID = gf_isom_get_track_id(file, decode_style);
                ret_audio = gf_audio_export_native(&audio_mdump,buffer_video_playbck,count,time_scale);
            }
        } else {
            printf("create_new_track_action error!\n");
        }
    } else {
        ret_audio = gf_audio_export_native(&audio_mdump,buffer_video_playbck,count,time_scale);
    }

    return ret_audio;
}


void JK_Gpac_H265::jk_BS_WriteBit(unsigned char *buffer, u32 value,int count)
{
    video_current <<= 1;
    video_current |= value;
    if (++video_nbBits == 8) {
        video_nbBits = 0;
        buffer[count] = (u8)video_current;
        video_current = 0;
    }
}

void JK_Gpac_H265::jk_gf_bs_write_int(s32 _value, s32 nBits,unsigned char *buffer,int count)
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
        jk_BS_WriteBit(buffer,((s32)value) < 0,count);
        value <<= 1;
    }
}

void JK_Gpac_H265::jk_gf_bs_write_u32(u32 value,unsigned char *buffer,int count)
{
    buffer[count] = (u8) ((value>>24)&0xff);
    count += sizeof(u8);
    buffer[count] = (u8) ((value>>16)&0xff);
    count += sizeof(u8);
    buffer[count] = (u8) ((value>>8)&0xff);
    count += sizeof(u8);
    buffer[count] = (u8) ((value>>0)&0xff);
    count += sizeof(u8);
}

void JK_Gpac_H265::jk_gf_bs_write_u8(u32 value,unsigned char *buffer,int count)
{
    buffer[count] = (u8)value;
}

void JK_Gpac_H265::jk_gf_bs_write_data(const char *data, u32 nbBytes,unsigned char *buffer,int count)
{
    if (!nbBytes) return;
    /*we need some feedback for this guy...*/
    memcpy(buffer+count,data,nbBytes);
}

void JK_Gpac_H265::add_video_head(unsigned char *buffer,int count)
{
#if 0
    jk_gf_bs_write_u8(0,buffer,count);
    count += sizeof(u8);
    jk_gf_bs_write_u8(0,buffer,count);
    count += sizeof(u8);
    jk_gf_bs_write_u8(0,buffer,count);
    count += sizeof(u8);
    jk_gf_bs_write_u8(1,buffer,count);
    count += sizeof(u8);
    jk_gf_bs_write_u8(64,buffer,count);
    count += sizeof(u8);
    jk_gf_bs_write_u8(1,buffer,count);
    count += sizeof(u8);
    jk_gf_bs_write_u8(12,buffer,count);
    count += sizeof(u8);
    jk_gf_bs_write_u8(1,buffer,count);
    count += sizeof(u8);
    jk_gf_bs_write_u8(255,buffer,count);
    count += sizeof(u8);
    jk_gf_bs_write_u8(255,buffer,count);
    count += sizeof(u8);
    jk_gf_bs_write_u8(1,buffer,count);
    count += sizeof(u8);
    jk_gf_bs_write_u8(96,buffer,count);
    count += sizeof(u8);
    jk_gf_bs_write_u8(0,buffer,count);
    count += sizeof(u8);
    jk_gf_bs_write_u8(0,buffer,count);
    count += sizeof(u8);
    jk_gf_bs_write_u8(3,buffer,count);
    count += sizeof(u8);
    jk_gf_bs_write_u8(0,buffer,count);
    count += sizeof(u8);
    jk_gf_bs_write_u8(176,buffer,count);
    count += sizeof(u8);
    jk_gf_bs_write_u8(0,buffer,count);
    count += sizeof(u8);
    jk_gf_bs_write_u8(0,buffer,count);
    count += sizeof(u8);
    jk_gf_bs_write_u8(3,buffer,count);
    count += sizeof(u8);
    jk_gf_bs_write_u8(0,buffer,count);
    count += sizeof(u8);
    jk_gf_bs_write_u8(0,buffer,count);
    count += sizeof(u8);
    jk_gf_bs_write_u8(3,buffer,count);
    count += sizeof(u8);
    jk_gf_bs_write_u8(0,buffer,count);
    count += sizeof(u8);
    jk_gf_bs_write_u8(123,buffer,count);
    count += sizeof(u8);
    jk_gf_bs_write_u8(172,buffer,count);
    count += sizeof(u8);
    jk_gf_bs_write_u8(9,buffer,count);
    count += sizeof(u8);
    jk_gf_bs_write_u8(0,buffer,count);
    count += sizeof(u8);
    jk_gf_bs_write_u8(0,buffer,count);
    count += sizeof(u8);
    jk_gf_bs_write_u8(0,buffer,count);
    count += sizeof(u8);
    jk_gf_bs_write_u8(1,buffer,count);
    count += sizeof(u8);
    jk_gf_bs_write_u8(66,buffer,count);
    count += sizeof(u8);
    jk_gf_bs_write_u8(1,buffer,count);
    count += sizeof(u8);
    jk_gf_bs_write_u8(1,buffer,count);
    count += sizeof(u8);
    jk_gf_bs_write_u8(1,buffer,count);
    count += sizeof(u8);
    jk_gf_bs_write_u8(96,buffer,count);
    count += sizeof(u8);
    jk_gf_bs_write_u8(0,buffer,count);
    count += sizeof(u8);
    jk_gf_bs_write_u8(0,buffer,count);
    count += sizeof(u8);
    jk_gf_bs_write_u8(3,buffer,count);
    count += sizeof(u8);
    jk_gf_bs_write_u8(0,buffer,count);
    count += sizeof(u8);
    jk_gf_bs_write_u8(176,buffer,count);
    count += sizeof(u8);
    jk_gf_bs_write_u8(0,buffer,count);
    count += sizeof(u8);
    jk_gf_bs_write_u8(0,buffer,count);
    count += sizeof(u8);
    jk_gf_bs_write_u8(3,buffer,count);
    count += sizeof(u8);
    jk_gf_bs_write_u8(0,buffer,count);
    count += sizeof(u8);
    jk_gf_bs_write_u8(0,buffer,count);
    count += sizeof(u8);
    jk_gf_bs_write_u8(3,buffer,count);
    count += sizeof(u8);
    jk_gf_bs_write_u8(0,buffer,count);
    count += sizeof(u8);
    jk_gf_bs_write_u8(123,buffer,count);
    count += sizeof(u8);
    jk_gf_bs_write_u8(160,buffer,count);
    count += sizeof(u8);
    jk_gf_bs_write_u8(3,buffer,count);
    count += sizeof(u8);
    jk_gf_bs_write_u8(192,buffer,count);
    count += sizeof(u8);
    jk_gf_bs_write_u8(128,buffer,count);
    count += sizeof(u8);
    jk_gf_bs_write_u8(16,buffer,count);
    count += sizeof(u8);
    jk_gf_bs_write_u8(229,buffer,count);
    count += sizeof(u8);
    jk_gf_bs_write_u8(141,buffer,count);
    count += sizeof(u8);
    jk_gf_bs_write_u8(174,buffer,count);
    count += sizeof(u8);
    jk_gf_bs_write_u8(228,buffer,count);
    count += sizeof(u8);
    jk_gf_bs_write_u8(203,buffer,count);
    count += sizeof(u8);
    jk_gf_bs_write_u8(243,buffer,count);
    count += sizeof(u8);
    jk_gf_bs_write_u8(112,buffer,count);
    count += sizeof(u8);
    jk_gf_bs_write_u8(16,buffer,count);
    count += sizeof(u8);
    jk_gf_bs_write_u8(16,buffer,count);
    count += sizeof(u8);
    jk_gf_bs_write_u8(16,buffer,count);
    count += sizeof(u8);
    jk_gf_bs_write_u8(8,buffer,count);
    count += sizeof(u8);
    jk_gf_bs_write_u8(0,buffer,count);
    count += sizeof(u8);
    jk_gf_bs_write_u8(0,buffer,count);
    count += sizeof(u8);
    jk_gf_bs_write_u8(0,buffer,count);
    count += sizeof(u8);
    jk_gf_bs_write_u8(1,buffer,count);
    count += sizeof(u8);
    jk_gf_bs_write_u8(68,buffer,count);
    count += sizeof(u8);
    jk_gf_bs_write_u8(1,buffer,count);
    count += sizeof(u8);
    jk_gf_bs_write_u8(192,buffer,count);
    count += sizeof(u8);
    jk_gf_bs_write_u8(242,buffer,count);
    count += sizeof(u8);
    jk_gf_bs_write_u8(176,buffer,count);
    count += sizeof(u8);
    jk_gf_bs_write_u8(59,buffer,count);
    count += sizeof(u8);
    jk_gf_bs_write_u8(36,buffer,count);
    count += sizeof(u8);
#endif
    jk_gf_bs_write_u32(1,buffer,count);
    count += sizeof(u32);
    jk_gf_bs_write_data(vps_info,vps_len,buffer,count);
    count += vps_len * sizeof(u8);
    jk_gf_bs_write_u32(1,buffer,count);
    count += sizeof(u32);
    jk_gf_bs_write_data(sps_info,sps_len,buffer,count);
    count += sps_len * sizeof(u8);
    jk_gf_bs_write_u32(1,buffer,count);
    count += sizeof(u32);
    jk_gf_bs_write_data(pps_info,pps_len,buffer,count);
    count += pps_len * sizeof(u8);
    jk_gf_bs_write_u32(1,buffer,count);
    count += sizeof(u32);
    jk_gf_bs_write_u8(0x4E,buffer,count);
    count += sizeof(u8);
    jk_gf_bs_write_u8(1,buffer,count);
    count += sizeof(u8);
    jk_gf_bs_write_u8(0xE5,buffer,count);
    count += sizeof(u8);
    jk_gf_bs_write_u8(4,buffer,count);
    count += sizeof(u8);
#ifdef GPAC_H265_SEI_INFO
    fread(buffer+count,2,1,sei_info_fd);
    count += 2*sizeof(u8);
#else
    jk_gf_bs_write_u8(0x69,buffer,count);
    count += sizeof(u8);
    jk_gf_bs_write_u8(0xAA,buffer,count);
    count += sizeof(u8);
#endif //end of GPAC_H265_SEI_INFO
    jk_gf_bs_write_u8(0,buffer,count);
    count += sizeof(u8);
    jk_gf_bs_write_u8(0,buffer,count);
    count += sizeof(u8);
    jk_gf_bs_write_u8(128,buffer,count);
    count += sizeof(u8);
}

void JK_Gpac_H265::jk_audio_WriteBit(unsigned char *buffer, u32 value,int count)
{
    audio_current <<= 1;
    audio_current |= value;
    if (++audio_nbBits == 8) {
        audio_nbBits = 0;
        buffer[count] = (u8)audio_current;
        audio_current = 0;
    }
}

void JK_Gpac_H265::jk_gf_audio_write_int(s32 _value, s32 nBits,unsigned char *buffer,int count)
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
        jk_audio_WriteBit(buffer,((s32)value) < 0,count);
        value <<= 1;
    }
}

void JK_Gpac_H265::jk_gf_audio_write_data(const char *data, u32 nbBytes,unsigned char *buffer,int count)
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
        }
    }
}

