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
#include <time.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <event.h>
#include <log.h>
#include <frame.h>
#include <stream.h>
#include <rtp.h>
#include <conf_parse.h>

static int log_fd = -1;

void write_access_log( char *path, struct sockaddr *addr, int code, char *req,
		int length, char *referer, char *user_agent )
{
    return;
}

/************************ CONFIGURATION DIRECTIVES ************************/

static int set_file( int num_tokens, struct token *tokens, void *d )
{
	if( log_fd >= 0 )
	{
		spook_log( SL_ERR, "log file: only one log file is allowed" );
		return -1;
	}
	if( ( log_fd = open( tokens[1].v.str,
				O_CREAT | O_APPEND | O_WRONLY, 0666 ) ) < 0 )
	{
		spook_log( SL_ERR,
			"can't open log file: %s", strerror( errno ) );
		return -1;
	}
	spook_log( SL_VERBOSE, "writing access logs to %s", tokens[1].v.str );
	return 0;
}

static struct statement config_statements[] = {
	/* directive name, process function, min args, max args, arg types */
	{ "file", set_file, 1, 1, { TOKEN_STR } },

	/* empty terminator -- do not remove */
	{ NULL, NULL, 0, 0, {} }
};

int access_log_init(void)
{
	register_config_context( "log", "file", NULL, NULL,
					config_statements );
	return 0;
}
