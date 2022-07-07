#include "anyka_media.h"

#define AUDIO_DEFAULT_INTERVAL        40
#define FIRST_ISP_PATH 		"/etc/jffs2/"		//first config path

/* encode this number frames, main index , sub index */
static int main_index, sub_index; //global resolution index
struct video_handle ak_venc[ENCODE_GRP_NUM];	//handle array

void *video_in_handle = NULL;			//global vi handle
void *ai_handle = NULL;

/* open video encode handle */
static void *venc_demo_open_encoder(int index)
{
    struct encode_param param = {0};

    switch (index)
    {
    case 0:
        param.width = resolutions[sub_index].width;
        param.height = resolutions[sub_index].height;
        param.minqp = 28;
        param.maxqp = 43;
        param.fps = 25;
        param.goplen = param.fps * 2;	   //current gop is stationary
        param.bps = 2000;				   //kbps
        param.profile = PROFILE_MAIN;	   //main profile
        param.use_chn = ENCODE_SUB_CHN;   //use main yuv channel data
        param.enc_grp = ENCODE_RECORD;     //assignment from enum encode_group_type
        param.br_mode = BR_MODE_VBR;	   //default is cbr
        param.enc_out_type = H264_ENC_TYPE;//h.264
        break;
    case 1:
        param.width = resolutions[main_index].width;
        param.height = resolutions[main_index].height;
        param.minqp = 28;
        param.maxqp = 43;
        param.fps = 25;
        param.goplen = param.fps * 2;
        param.bps = 1024;	//kbps
        param.profile = PROFILE_MAIN;		//same as above
        param.use_chn = ENCODE_MAIN_CHN;
        param.enc_grp = ENCODE_MAINCHN_NET;	//just this scope difference
        param.br_mode = BR_MODE_VBR;
        param.enc_out_type = H264_ENC_TYPE;
        break;
    case 2:
        param.width = resolutions[sub_index].width;
        param.height = resolutions[sub_index].height;
        param.minqp = 28;
        param.maxqp = 43;
        param.fps = 25;
        param.goplen = param.fps * 2;
        param.bps = 512;	//kbps
        param.profile = PROFILE_MAIN;
        param.use_chn = ENCODE_SUB_CHN;		//use sub yuv channel data
        param.enc_grp = ENCODE_SUBCHN_NET;	//same as above
        param.br_mode = BR_MODE_VBR;
        param.enc_out_type = H264_ENC_TYPE;
        break;
    case 3:
        param.width = resolutions[sub_index].width;
        param.height = resolutions[sub_index].height;
        param.minqp = 28;
        param.maxqp = 43;
        param.fps = 10;
        param.goplen = param.fps * 2;
        param.bps = 500;	//kbps
        param.profile = PROFILE_MAIN;
        param.use_chn = ENCODE_SUB_CHN;
        param.enc_grp = ENCODE_PICTURE;		//jpeg encode
        param.br_mode = BR_MODE_CBR;
        param.enc_out_type = MJPEG_ENC_TYPE;	//jpeg encode
        break;
    default:
        return NULL;
        break;
    }

    return ak_venc_open(&param);
}

static int venc_start_stream(enum encode_group_type grp)
{
    if (grp == ENCODE_MAINCHN_NET) {
        encode_mainchn_handle = &ak_venc[grp];

        ak_print_normal_ex("stream encode mode, start encode group: %d\n", grp);

        /* init encode handle */
        encode_mainchn_handle->venc_handle = venc_demo_open_encoder(grp);
        if (!encode_mainchn_handle->venc_handle) {
            ak_print_error_ex("video encode open type: %d failed\n", grp);
            return -1;
        }

        encode_mainchn_handle->enc_type = grp;
        /* request stream, video encode module will start capture and encode */
        encode_mainchn_handle->stream_handle = ak_venc_request_stream(video_in_handle,
                encode_mainchn_handle->venc_handle);
        if (!encode_mainchn_handle->stream_handle) {
            ak_print_error_ex("request stream failed\n");
            return -1;
        }
    }
    else if (grp == ENCODE_SUBCHN_NET) {
        encode_subchn_handle = &ak_venc[grp];

        ak_print_normal_ex("stream encode mode, start encode group: %d\n", grp);

        /* init encode handle */
        encode_subchn_handle->venc_handle = venc_demo_open_encoder(grp);
        if (!encode_subchn_handle->venc_handle) {
            ak_print_error_ex("video encode open type: %d failed\n", grp);
            return -1;
        }

        encode_subchn_handle->enc_type = grp;
        /* request stream, video encode module will start capture and encode */
        encode_subchn_handle->stream_handle = ak_venc_request_stream(video_in_handle,
                encode_subchn_handle->venc_handle);
        if (!encode_subchn_handle->stream_handle) {
            ak_print_error_ex("request stream failed\n");
            return -1;
        }
    }

    return 0;
}


/*
 * venc_stop_stream - stop specific group's encode
 * grp[IN]: group index
 */
static void venc_stop_stream(enum encode_group_type grp)
{
    if (grp == ENCODE_MAINCHN_NET) {
        /* release resource */
        ak_venc_cancel_stream(encode_mainchn_handle->stream_handle);
        encode_mainchn_handle->stream_handle = NULL;
        ak_venc_close(encode_mainchn_handle->venc_handle);
        encode_mainchn_handle->venc_handle = NULL;
    }
    else if (grp == ENCODE_SUBCHN_NET) {
        /* release resource */
        ak_venc_cancel_stream(encode_subchn_handle->stream_handle);
        encode_subchn_handle->stream_handle = NULL;
        ak_venc_close(encode_subchn_handle->venc_handle);
        encode_subchn_handle->venc_handle = NULL;
    }
}


/*
 * venc_show_channel_res - check main and sub channel resolution
 *                         config, if select is invalid, it will
 *                         set global channel index to a invalid
 *                         value to avoid crash
 * main[IN]: main channel resolution selection index
 * sub[IN]:	sub channel resolution selection index
 */
static void venc_show_channel_res(int main, int sub)
{
    /* main channel value check */
    if (main < 7 || main > 14) {
        ak_print_error_ex("main channel resolution select out of range,"
                          " its value squre is [7, 14]\n");
        main_index = INVALID_INDEX;
    } else {
        ak_print_notice("main channel resolution: w: %d, h: %d\n",
                        resolutions[main].width, resolutions[main].height);
    }

    /* sub channel value check */
    if (sub < 0 || sub > 6) {
        ak_print_error_ex("sub channel resolution select out of range,"
                          " its value squre is [0, 6]\n");
        sub_index = INVALID_INDEX;
    } else {
        ak_print_notice("sub channel resolution: w: %d, h: %d\n",
                        resolutions[sub].width, resolutions[sub].height);
    }
}


/* set capture resolution */
static int venc_set_resolution(void *handle)
{
    struct video_resolution resolution = {0};
    /* get origin resolution */
    if (ak_vi_get_sensor_resolution(handle, &resolution))
        ak_print_error_ex("get sensor resolution failed\n");
    else
        ak_print_normal("\tcurrent sensor resolution: [W: %d], [H: %d]\n",
                resolution.width, resolution.height);

    /* set channel attribute */
    struct video_channel_attr attr;
    memset(&attr, 0, sizeof(struct video_channel_attr));

    attr.res[VIDEO_CHN_MAIN].width = resolutions[main_index].width;
    attr.res[VIDEO_CHN_MAIN].height = resolutions[main_index].height;
    attr.res[VIDEO_CHN_SUB].width = resolutions[sub_index].width;
    attr.res[VIDEO_CHN_SUB].height = resolutions[sub_index].height;

#ifdef SENSOR_MAX_1080P
    attr.res[VIDEO_CHN_MAIN].max_width = 1920;
    attr.res[VIDEO_CHN_MAIN].max_height = 1080;
#endif

#ifdef SENSOR_MAX_720P
    attr.res[VIDEO_CHN_MAIN].max_width = 1280;
    attr.res[VIDEO_CHN_MAIN].max_height = 720;
#endif
    attr.res[VIDEO_CHN_SUB].max_width = 640;
    attr.res[VIDEO_CHN_SUB].max_height = 360;

    attr.crop.left = 0;
    attr.crop.top = 0;
    attr.crop.width = resolutions[main_index].width;
    attr.crop.height = resolutions[main_index].height;

    ak_print_normal("mw: %d, mh: %d, sw: %d, sh: %d\n",
        attr.res[VIDEO_CHN_MAIN].width, attr.res[VIDEO_CHN_MAIN].height,
        attr.res[VIDEO_CHN_SUB].width, attr.res[VIDEO_CHN_SUB].height);

    /* modify resolution */
    if (ak_vi_set_channel_attr(handle, &attr)) {
        ak_print_error_ex("set channel attribute failed\n");
        return -1;
    }

    return 0;
}

/* init video input */
static void *venc_init_video_in(void)
{
    /* open video input device */
    void *handle = ak_vi_open(VIDEO_DEV0);
    if (handle == NULL) {
        ak_print_error_ex("vi open failed\n");
        return NULL;
    }
    ak_print_normal("\tvi open ok\n");

    /* set capture resolution */
    if (venc_set_resolution(handle)) {
        ak_print_error_ex("\tset resolution failed\n");
        ak_vi_close(handle);
        handle = NULL;
        return NULL;
    }

    /* set capture fps */
    ak_vi_set_fps(handle, 25);
    ak_print_normal("\tcapture fps: %d\n", ak_vi_get_fps(handle));

    return handle;
}

int anyka_video_init(void)
{
    int ret;

#ifdef SENSOR_MAX_1080P
    main_index = VIDED_SIZE_1920x1080;
#endif

#ifdef SENSOR_MAX_720P
    main_index = VIDED_SIZE_1280x720;
#endif
    sub_index = VIDED_SIZE_640x360;

    venc_show_channel_res(main_index, sub_index);
    if (main_index == INVALID_INDEX || sub_index == INVALID_INDEX) {
        ak_print_error_ex("invalid argument\n");
        return -1;
    }

    /* match sensor, if success, open vi */
    ret = ak_vi_match_sensor(FIRST_ISP_PATH);
    /* first, only when match sensor ok you can do next things */
    if (ret != 0) {
        ak_print_error_ex("vi match config failed\n");
        return -1;
    }

    /* one video input device, only open one time vi for encode */
    video_in_handle = venc_init_video_in();
    if (!video_in_handle) {
        ak_print_error_ex("video input init faild, exit\n");
        return -1;
    }

    /* start capture and encode */
    if(ak_vi_capture_on(video_in_handle)) {
        ak_print_error_ex("video in capture on failed\n");
        return -1;
    }

    ret = ak_vi_set_flip_mirror(video_in_handle,1,1);
    if (ret != 0) {
        ak_print_error_ex("ak_vi_set_flip_mirror failed\n");
        return -1;
    }

    /* start */
    venc_start_stream(ENCODE_MAINCHN_NET);
//    venc_start_stream(ENCODE_SUBCHN_NET);

    return 0;
}

int anyka_video_exit(void)
{
    /* stop video encode */
    venc_stop_stream(encode_mainchn_handle->enc_type);
//    venc_stop_stream(encode_subchn_handle->enc_type);

    ak_print_normal_ex("### videoCaptureThread exit ###\n");

    /* close video input device */
    ak_print_normal("closing video input ...\n");
    ak_vi_capture_off(video_in_handle);
    ak_vi_close(video_in_handle);
    video_in_handle = NULL;
    ak_sleep_ms(10);

    ak_print_normal("video encode demo exit ...\n");
    return 0;
}

int mic_init(void)
{
    int ret = AK_FAILED;
    int volume = 6;					// set volume,volume is from 0~12

    struct pcm_param param;
    memset(&param, 0, sizeof(struct pcm_param));

    param.sample_rate = 8000;				// set sample rate
    param.sample_bits = 16;					// sample bits only support 16 bit
    param.channel_num = AUDIO_CHANNEL_MONO;	// channel number

    /*
     * step 1: open ai
     */
    ai_handle = ak_ai_open(&param);
    if (NULL == ai_handle)
    {
        ak_print_normal("*** ak_ai_open failed. ***\n");
        return -1;
    }

    /* volume is from 0 to 12,volume 0 is mute */
    ret = ak_ai_set_volume(ai_handle, volume);
    if (ret) {
        ak_print_normal("*** set ak_ai_set_volume failed. ***\n");
        ak_ai_close(ai_handle);
        return -1;
    }

    ret = ak_ai_set_aec(ai_handle, AUDIO_FUNC_ENABLE);
    if (ret) {
        ak_print_normal("*** ak_ai_set_aec failed. ***\n");
        ak_ai_close(ai_handle);
        return -1;
    }

    /* enable noise reduction and automatic gain control ,
     * nr&agc only support 8K sample
     */
    ret = ak_ai_set_nr_agc(ai_handle, AUDIO_FUNC_ENABLE);
    if (ret) {
        ak_print_normal("*** set ak_ai_set_nr_agc failed. ***\n");
        ak_ai_close(ai_handle);
        return -1;
    }

    /* set resample */
    ret = ak_ai_set_resample(ai_handle, AUDIO_FUNC_DISABLE);
    if (ret) {
        ak_print_normal("*** set ak_ai_set_resample  failed. ***\n");
        ak_ai_close(ai_handle);
        return -1;
    }

    /* set source, source include mic and linein */
    ret = ak_ai_set_source(ai_handle, AI_SOURCE_MIC);
    if (ret) {
        ak_print_normal("*** set ak_ai_open failed. ***\n");
        ak_ai_close(ai_handle);
        return -1;
    }

    /* clear ai buffer */
    ret = ak_ai_clear_frame_buffer(ai_handle);
    if (ret) {
        ak_print_normal("*** set ak_ai_clear_frame_buffer failed. ***\n");
        ak_ai_close(ai_handle);
        return -1;
    }

    /*
     * step 2: set ai working configuration,
     * configuration include:
     * frame interval, source, volume,
     * enable noise reduction and automatic gain control,
     * resample, clear frame buffer
     */
    ret = ak_ai_set_frame_interval(ai_handle, AUDIO_DEFAULT_INTERVAL);
    if (ret) {
        ak_print_normal("*** set ak_ai_set_frame_interval failed. ***\n");
        ak_ai_close(ai_handle);
        return -1;
    }

    /*
     * step 3: start capture frames
     */
    ret = ak_ai_start_capture(ai_handle);
    if (ret)
    {
        ak_print_normal("*** ak_ai_start_capture failed. ***\n");
        ak_ai_close(ai_handle);
        return -1;
    }

    return 0;
}

int mic_exit(void)
{
    int ret = -1;

    /*
     * step 5: stop capture frames
     */
    ret = ak_ai_stop_capture(ai_handle);
    if (ret != 0)
    {
        ak_print_normal("*** ak_ai_stop_capture failed. ***\n");
        ak_ai_close(ai_handle);
        return -1;
    }

    /*
     * step 6: close ai
     */
    ret = ak_ai_close(ai_handle);
    if (ret)
    {
        ak_print_normal("*** ak_ai_close failed. ***\n");
        return -1;
    }

    return 0;
}
