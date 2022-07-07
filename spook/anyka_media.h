#ifndef _JIAKE_ANYKA_MEDIA_H
#define _JIAKE_ANYKA_MEDIA_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "jk_g_conf.h"
#include "ak_venc_common.h"
#include "jk_file_operation.h"
#include "ak_aenc.h"

#define START_RTSP_FLAG     "/tmp/start_jiake_rtsp"

int anyka_video_init(void);
int anyka_video_exit(void);
int mic_init(void);
int mic_exit(void);

#endif /* end of _JIAKE_ANYKA_MEDIA_H */
