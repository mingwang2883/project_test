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

#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <errno.h>

#include <event.h>
#include <log.h>
#include <frame.h>
#include <stream.h>
#include <inputs.h>
#include <encoders.h>
#include <filters.h>
#include <outputs.h>
#include <rtp.h>
#include <conf_parse.h>
#include <config.h>

extern int alaw_init(void);
extern void oss_init(void);

int read_config_file( char *config_file );
int spook_log_init( int min );

void random_bytes( unsigned char *dest, int len )
{
	int i;

	for( i = 0; i < len; ++i )
		dest[i] = random() & 0xff;
}

void random_id( unsigned char *dest, int len )
{
	int i;

	for( i = 0; i < len / 2; ++i )
        sprintf( (char *)dest + i * 2, "%02X",
                (unsigned int)( random() & 0xff ) );
	dest[len] = 0;
}

int main( int argc, char **argv )
{
	enum { DB_NONE, DB_FOREGROUND, DB_DEBUG } debug_mode = DB_NONE;
    char *config_file = NULL;

	switch( debug_mode )
	{
	case DB_NONE:
		spook_log_init( SL_INFO );
		break;
	case DB_FOREGROUND:
		spook_log_init( SL_VERBOSE );
		break;
	case DB_DEBUG:
		spook_log_init( SL_DEBUG );
		break;
	}

	oss_init();

	v4l2_init();

	alaw_init();

	live_init();

    if( read_config_file( config_file ) < 0 )
    {
        return 1;
    }

	event_loop( 0 );

	return 0;
}

/********************* GLOBAL CONFIGURATION DIRECTIVES ********************/

int config_frameheap( int num_tokens, struct token *tokens, void *d )
{
	int size, count;

	signal( SIGPIPE, SIG_IGN );

	count = tokens[1].v.num;
    if( num_tokens == 3)
    {
        size = tokens[2].v.num;
    }
    else
    {
        size = 400 * 1024;
    }

	spook_log( SL_DEBUG, "frame size is %d", size );

	init_frame_heap( size, count );

	return 0;
}
