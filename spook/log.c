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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <time.h>
#include <pthread.h>

static int minlevel = 0;

int spook_log_init( int min )
{
    return 0;
}

void spook_log( int level, char *fmt, ... )
{
	va_list ap;
    char newfmt[128] = {0}, line[512] = {0};

	va_start( ap, fmt );
	if( level < minlevel )
	{
		va_end( ap );
		return;
	}

    sprintf( newfmt,"%s\n", fmt );

    vsnprintf( line, sizeof( line ), newfmt, ap );
	va_end( ap );

    printf("%s\n", line);

    return;
}

void send_log_buffer( int fd )
{
    return;
}
