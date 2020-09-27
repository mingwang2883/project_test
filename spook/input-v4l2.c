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

#include <config.h>

#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#ifdef HAVE_ASM_TYPES_H
#include <asm/types.h>
#endif
#ifdef HAVE_LINUX_COMPILER_H
#include <linux/compiler.h>
#endif
#include <linux/videodev2.h>
#ifdef HAVE_GO7007_H
#include <linux/go7007.h>
#endif

#include <event.h>
#include <log.h>
#include <frame.h>
#include <stream.h>
#include <inputs.h>
#include <conf_parse.h>
#include "anyka_media.h"

#define	INPUTTYPE_WEBCAM	1
#define INPUTTYPE_NTSC		2
#define INPUTTYPE_PAL		3

extern int GetSpsAndPps(const unsigned char * p_buf);

struct v4l2_spook_input {
	struct stream *output;
	struct frame_exchanger *ex;
	char device[256];
	int format;
	int bitrate;
	int width;
	int height;
	int fincr;
	int fbase;
	int inputport;
	int inputtype;
	int fps;
	int fd;
	void *bufaddr[2];
	pthread_t thread;
	int running;
};

static void *capture_loop( void *d )
{
	struct v4l2_spook_input *conf = (struct v4l2_spook_input *)d;
	struct v4l2_buffer buf;
    struct spook_frame *f;
    struct video_stream stream = {0};
    int ret = -1;
    unsigned int IsKeyFrame=0;
    unsigned char buffer_read[300 * 1024] = {0};
    int buffer_size = 0;

    ret = anyka_video_init();
    if (ret != 0)
    {
        printf("anyka_video_init failed !\n");
        goto exit;
    }

    jk_creat_file(START_RTSP_FLAG);

    while(1)
	{
        f = get_next_frame( conf->ex, 0);
        if(f > 0)
		{
            f->length = buf.bytesused;
            f->format = conf->format;
            f->width = conf->width;
            f->height = conf->height;

            /* get stream */
            ret = ak_venc_get_stream(encode_mainchn_handle->stream_handle, &stream);
            if (ret != 0)
            {
                ak_sleep_ms(10);
                continue;
            }

            memset(buffer_read,0,sizeof(buffer_read));
            memcpy(buffer_read,stream.data, stream.len);
            buffer_size = stream.len;

            IsKeyFrame = (stream.frame_type == 1) ? 1 : 0;

            if(IsKeyFrame >= 1)
            {
                GetSpsAndPps((unsigned char*)buffer_read);
            }

            memcpy( f->d, buffer_read,buffer_size);
            f->length = buffer_size;

            f->key = IsKeyFrame;

            if(deliver_frame( conf->ex, f ) < 0)
            {
                printf("failed\n");
            }

            /* release frame */
            ak_venc_release_stream(encode_mainchn_handle->stream_handle, &stream);
        }
        else
        {
            usleep(10);
        }
    }

exit:
    ret = anyka_video_exit();
    if (ret != 0)
    {
        printf("anyka_video_exit failed\n");
        return NULL;
    }

	return NULL;
}

static void get_back_frame( struct spook_frame *f, void *d )
{
	struct v4l2_spook_input *conf = (struct v4l2_spook_input *)d;

	exchange_frame( conf->ex, new_frame() );
	deliver_frame_to_stream( f, conf->output );
}

static void get_framerate( struct stream *s, int *fincr, int *fbase )
{
    struct v4l2_spook_input *conf = (struct v4l2_spook_input *)s->spook_private;

	*fincr = conf->fincr;
	*fbase = conf->fbase;
}

static void set_running( struct stream *s, int running )
{
    struct v4l2_spook_input *conf = (struct v4l2_spook_input *)s->spook_private;

	conf->running = running;
}

/************************ CONFIGURATION DIRECTIVES ************************/

static void *start_block(void)
{
	struct v4l2_spook_input *conf;

	conf = (struct v4l2_spook_input *)malloc( sizeof( struct v4l2_spook_input ) );
	conf->output = NULL;
	conf->device[0] = 0;
	conf->format = FORMAT_RAW_UYVY;
	conf->bitrate = 0;
	conf->width = 0;
	conf->height = 0;
	conf->inputport = -1;
	conf->inputtype = 0;
	conf->fps = -1;
	conf->fd = -1;
	conf->running = 0;

	return conf;
}

static int end_block( void *d )
{
	struct v4l2_spook_input *conf = (struct v4l2_spook_input *)d;
	int i;

	if( ! conf->output )
	{
		spook_log( SL_ERR, "v4l2: missing output stream name" );
		return -1;
	}
	if( ! conf->device[0] )
	{
		spook_log( SL_ERR, "v4l2: missing V4L2 device name" );
		return -1;
	}
	switch( conf->inputtype )
	{
	case 0:
		spook_log( SL_ERR, "v4l2: input type not specified" );
		return -1;
	case INPUTTYPE_NTSC:
		conf->fincr = 1001;
		conf->fbase = 30000;
		if( conf->width == 0 )
		{
			conf->width = 320;
			conf->height = 240;
		}
		break;
	case INPUTTYPE_PAL:
		conf->fincr = 1;
		conf->fbase = 25;
		if( conf->width == 0 )
		{
			conf->width = 320;
			conf->height = 288;
		}
		break;
	case INPUTTYPE_WEBCAM:
		if( conf->fps < 0 )
		{
			spook_log( SL_ERR,
				"v4l2: framerate not specified for webcam" );
			return -1;
		} else if( conf->fps > 0 )
		{
			conf->fincr = 1;
			conf->fbase = conf->fps;
		} else spook_log( SL_INFO, "v4l2: must figure out framerate, this will take some time..." );
		if( conf->inputport < 0 ) conf->inputport = 0;
		if( conf->width == 0 )
		{
			conf->width = 352;
			conf->height = 288;
		}
		break;
	}

    conf->ex = new_exchanger( 10, get_back_frame, conf );
    for( i = 0; i < 10; ++i ) exchange_frame( conf->ex, new_frame() );
	pthread_create( &conf->thread, NULL, capture_loop, conf );

	return 0;
}

static int set_device( int num_tokens, struct token *tokens, void *d )
{
	struct v4l2_spook_input *conf = (struct v4l2_spook_input *)d;

	strcpy( conf->device, tokens[1].v.str );
	return 0;
}

static int set_output( int num_tokens, struct token *tokens, void *d )
{
	struct v4l2_spook_input *conf = (struct v4l2_spook_input *)d;

	conf->output = new_stream( tokens[1].v.str, conf->format, conf );
	if( ! conf->output )
	{
		spook_log( SL_ERR, "v4l2: unable to create stream \"%s\"",
				tokens[1].v.str );
		return -1;
	}
	conf->output->get_framerate = get_framerate;
	conf->output->set_running = set_running;
	return 0;
}

static int set_format( int num_tokens, struct token *tokens, void *d )
{
	struct v4l2_spook_input *conf = (struct v4l2_spook_input *)d;

	if( conf->output )
	{
		spook_log( SL_ERR, "v4l2: output format must be specified "
				"before output stream name" );
		return -1;
	}
	if( ! strcasecmp( tokens[1].v.str, "raw" ) )
		conf->format = FORMAT_RAW_UYVY;
	else if( ! strcasecmp( tokens[1].v.str, "mpeg4" ) )
		conf->format = FORMAT_MPEG4;
	else if( ! strcasecmp( tokens[1].v.str, "mjpeg" ) )
		conf->format = FORMAT_JPEG;
	else
	{
		spook_log( SL_ERR, "v4l2: format \"%s\" is unsupported; try "
				"RAW, MJPEG, or MPEG4", tokens[1].v.str );
		return -1;
	}

	return 0;
}

static int set_bitrate( int num_tokens, struct token *tokens, void *d )
{
	struct v4l2_spook_input *conf = (struct v4l2_spook_input *)d;

	if( tokens[1].v.num < 10 || tokens[1].v.num > 10000 )
	{
		spook_log( SL_ERR,
			"v4l2: bitrate must be between 10 and 10000" );
		return -1;
	}
	conf->bitrate = tokens[1].v.num;
	return 0;
}

static int set_framesize( int num_tokens, struct token *tokens, void *d )
{
	struct v4l2_spook_input *conf = (struct v4l2_spook_input *)d;

	conf->width = tokens[1].v.num;
	conf->height = tokens[2].v.num;

	return 0;
}

static int set_inputport( int num_tokens, struct token *tokens, void *d )
{
	struct v4l2_spook_input *conf = (struct v4l2_spook_input *)d;

	conf->inputport = tokens[1].v.num;

	return 0;
}

static int set_inputtype( int num_tokens, struct token *tokens, void *d )
{
	struct v4l2_spook_input *conf = (struct v4l2_spook_input *)d;

	if( ! strcasecmp( tokens[1].v.str, "webcam" ) )
		conf->inputtype = INPUTTYPE_WEBCAM;
	else if( ! strcasecmp( tokens[1].v.str, "ntsc" ) )
		conf->inputtype = INPUTTYPE_NTSC;
	else if( ! strcasecmp( tokens[1].v.str, "pal" ) )
		conf->inputtype = INPUTTYPE_PAL;
	else
	{
		spook_log( SL_ERR,
			"v4l2: video mode \"%s\" is unsupported; try NTSC, PAL or WEBCAM",
			tokens[1].v.str );
		return -1;
	}

	return 0;
}

static int set_framerate_num( int num_tokens, struct token *tokens, void *d )
{
	struct v4l2_spook_input *conf = (struct v4l2_spook_input *)d;

	if( conf->fps >= 0 )
	{
		spook_log( SL_ERR, "v4l2: frame rate has already been set!" );
		return -1;
	}
	conf->fps = tokens[1].v.num;

	return 0;
}

static int set_framerate_str( int num_tokens, struct token *tokens, void *d )
{
	struct v4l2_spook_input *conf = (struct v4l2_spook_input *)d;

	if( conf->fps >= 0 )
	{
		spook_log( SL_ERR, "v4l2: frame rate has already been set!" );
		return -1;
	}
	if( strcasecmp( tokens[1].v.str, "auto" ) )
	{
		spook_log( SL_ERR,
			"v4l2: frame rate should be a number or \"auto\"" );
		return -1;
	}

	conf->fps = 0;

	return 0;
}

static struct statement config_statements[] = {
	/* directive name, process function, min args, max args, arg types */
	{ "device", set_device, 1, 1, { TOKEN_STR } },
	{ "output", set_output, 1, 1, { TOKEN_STR } },
	{ "format", set_format, 1, 1, { TOKEN_STR } },
	{ "bitrate", set_bitrate, 1, 1, { TOKEN_NUM } },
	{ "framesize", set_framesize, 2, 2, { TOKEN_NUM, TOKEN_NUM } },
	{ "inputport", set_inputport, 1, 1, { TOKEN_NUM } },
	{ "inputtype", set_inputtype, 1, 1, { TOKEN_STR } },
	{ "framerate", set_framerate_num, 1, 1, { TOKEN_NUM } },
	{ "framerate", set_framerate_str, 1, 1, { TOKEN_STR } },

	/* empty terminator -- do not remove */
	{ NULL, NULL, 0, 0, {} }
};

void v4l2_init(void)
{
	register_config_context( "input", "v4l2", start_block, end_block,
					config_statements );
}
