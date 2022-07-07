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
#include <sys/ioctl.h>
#include <errno.h>
#include <pthread.h>

#include <linux/soundcard.h>

#include <event.h>
#include <log.h>
#include <frame.h>
#include <stream.h>
#include <inputs.h>
#include <conf_parse.h>
#include "anyka_media.h"

#define AUDIO_SEND_BUF_SIZE 1024

extern void *ai_handle;

struct oss_input {
	struct stream *output;
	char device[256];
	struct soft_queue *queue;
	struct audio_ring *ring;
	int fd;
	int rate;
	int channels;
	pthread_t thread;
	int running;
};

static void *capture_loop( void *d )
{
    int ret = -1;
    int buffer_size = 0;
	struct oss_input *conf = (struct oss_input *)d;
    struct frame audio_frame = {0};
    unsigned char buffer_read[AUDIO_SEND_BUF_SIZE * 2] = {0};

    ret= mic_init();
    if (ret != 0)
    {
        printf("mic_init failed\n");
        goto exit;
    }

    while(1)
	{
        ret = ak_ai_get_frame(ai_handle, &audio_frame, 0);
        if (ret < 0)
        {
            ak_sleep_ms(10);
            continue;
        }

        buffer_size = audio_frame.len;

        memset(buffer_read, 0, sizeof(buffer_read));
        memcpy(buffer_read, audio_frame.data, buffer_size);

        audio_ring_input( conf->ring,buffer_read,buffer_size);

        ak_ai_release_frame(ai_handle, &audio_frame);
	}

exit:
    ret = mic_exit();
    if (ret != 0)
    {
        printf("mic_exit failed\n");
        return NULL;
    }

	return NULL;
}

static void get_back_frame( struct event_info *ei, void *d )
{
	struct oss_input *conf = (struct oss_input *)d;
    struct spook_frame *f = (struct spook_frame *)ei->data;

	deliver_frame_to_stream( f, conf->output );
}

static void get_framerate( struct stream *s, int *fincr, int *fbase )
{
    struct oss_input *conf = (struct oss_input *)s->spook_private;

	*fincr = conf->channels;
	*fbase = conf->rate * conf->channels;
}

static void set_running( struct stream *s, int running )
{
    struct oss_input *conf = (struct oss_input *)s->spook_private;

	conf->running = running;
}

/************************ CONFIGURATION DIRECTIVES ************************/

static void *start_block(void)
{
	struct oss_input *conf;

	conf = (struct oss_input *)malloc( sizeof( struct oss_input ) );
	conf->output = NULL;
	conf->device[0] = 0;
	conf->rate = 0;
    conf->channels = 1;
	conf->running = 0;

	return conf;
}

static int end_block( void *d )
{
	struct oss_input *conf = (struct oss_input *)d;

	if( ! conf->output )
	{
		spook_log( SL_ERR, "oss: missing output stream name" );
		return -1;
	}
	if( ! conf->device[0] )
	{
		spook_log( SL_ERR, "oss: missing DSP device name" );
		return -1;
	}
	if( conf->rate == 0 )
	{
		spook_log( SL_ERR, "oss: sample rate not specified" );
		return -1;
	}

	conf->queue = new_soft_queue( 16 );
	add_softqueue_event( conf->queue, 0, get_back_frame, conf );
	/* Set frame length to 4608, which is the size of the blocks that
	 * the MP2 encoder will need.  This is just temporary... */
	conf->ring = new_audio_ring( 2 * conf->channels, conf->rate,
					4608, conf->queue );
	pthread_create( &conf->thread, NULL, capture_loop, conf );

	return 0;
}

static int set_device( int num_tokens, struct token *tokens, void *d )
{
	struct oss_input *conf = (struct oss_input *)d;

	strcpy( conf->device, tokens[1].v.str );
	return 0;
}

static int set_output( int num_tokens, struct token *tokens, void *d )
{
	struct oss_input *conf = (struct oss_input *)d;

	conf->output = new_stream( tokens[1].v.str, FORMAT_PCM, conf );
	if( ! conf->output )
	{
		spook_log( SL_ERR, "oss: unable to create stream \"%s\"",
				tokens[1].v.str );
		return -1;
	}
	conf->output->get_framerate = get_framerate;
	conf->output->set_running = set_running;
	return 0;
}

static int set_samplerate( int num_tokens, struct token *tokens, void *d )
{
	struct oss_input *conf = (struct oss_input *)d;

	if( conf->rate > 0 )
	{
		spook_log( SL_ERR, "oss: sample rate has already been set!" );
		return -1;
	}
	conf->rate = tokens[1].v.num;

	return 0;
}

static struct statement config_statements[] = {
	/* directive name, process function, min args, max args, arg types */
	{ "output", set_output, 1, 1, { TOKEN_STR } },
	{ "device", set_device, 1, 1, { TOKEN_STR } },
	{ "samplerate", set_samplerate, 1, 1, { TOKEN_NUM } },

	/* empty terminator -- do not remove */
	{ NULL, NULL, 0, 0, {} }
};

void oss_init(void)
{
	register_config_context( "input", "oss", start_block, end_block,
					config_statements );
}
