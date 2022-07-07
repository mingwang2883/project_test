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
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <pthread.h>

#include <event.h>
#include <log.h>
#include <frame.h>
#include <stream.h>
#include <encoders.h>
#include <conf_parse.h>

/*
* u-law, A-law and linear PCM conversions.
*/
#define SIGN_BIT    (0x80)      /* Sign bit for a A-law byte. */
#define QUANT_MASK  (0xf)       /* Quantization field mask. */
#define NSEGS       (8)     /* Number of A-law segments. */
#define SEG_SHIFT   (4)     /* Left shift for segment number. */
#define SEG_MASK    (0x70)      /* Segment field mask. */
#define BIAS        (0x84)      /* Bias for linear code. */

struct alaw_encoder
{
    struct stream *output;
    struct stream_destination *input;
};

static short seg_end[8] = {0xFF, 0x1FF, 0x3FF, 0x7FF,
                           0xFFF, 0x1FFF, 0x3FFF, 0x7FFF};

static int search(int val, short	*table, int	size)
{
    int	i;

    for (i = 0; i < size; i++)
    {
        if (val <= *table++)
        {
            return (i);
        }
    }

    return (size);
}

/*
* alaw2linear() - Convert an A-law value to 16-bit linear PCM
*
*/
static int alaw2linear(unsigned char a_val)
{
    int	t;
    int	seg;

    a_val ^= 0x55;

    t = (a_val & QUANT_MASK) << 4;
    seg = ((unsigned)a_val & SEG_MASK) >> SEG_SHIFT;
    switch (seg)
    {
    case 0:
        t += 8;
        break;
    case 1:
        t += 0x108;
        break;
    default:
        t += 0x108;
        t <<= seg - 1;
    }
    return ((a_val & SIGN_BIT) ? t : -t);
}


/*
* ulaw2linear() - Convert a u-law value to 16-bit linear PCM
*
* First, a biased linear code is derived from the code word. An unbiased
* output can then be obtained by subtracting 33 from the biased code.
*
* Note that this function expects to be passed the complement of the
* original code word. This is in keeping with ISDN conventions.
*/
static int ulaw2linear(unsigned char u_val)
{
    int	t;

    /* Complement to obtain normal u-law value. */
    u_val = ~u_val;

    /*
    * Extract and bias the quantization bits. Then
    * shift up by the segment number and subtract out the bias.
    */
    t = ((u_val & QUANT_MASK) << 3) + BIAS;
    t <<= ((unsigned)u_val & SEG_MASK) >> SEG_SHIFT;

    return ((u_val & SIGN_BIT) ? (BIAS - t) : (t - BIAS));
}

/*
 * linear2alaw() - Convert a 16-bit linear PCM value to 8-bit A-law
 *
 */
static unsigned char linear2alaw(int pcm_val)	/* 2's complement (16-bit range) */
{
    int		mask;
    int		seg;
    unsigned char	aval;

    if (pcm_val >= 0)
    {
        mask = 0xD5;		/* sign (7th) bit = 1 */
    }
    else
    {
        mask = 0x55;		/* sign bit = 0 */
        pcm_val = -pcm_val - 8;
    }

    /* Convert the scaled magnitude to segment number. */
    seg = search(pcm_val, seg_end, 8);

    /* Combine the sign, segment, and quantization bits. */

    if (seg >= 8)		/* out of range, return maximum value. */
    {
        return (0x7F ^ mask);
    }
    else
    {
        aval = seg << SEG_SHIFT;
        if (seg < 2)
            aval |= (pcm_val >> 4) & QUANT_MASK;
        else
            aval |= (pcm_val >> (seg + 3)) & QUANT_MASK;
        return (aval ^ mask);
    }
}


/*
 * linear2ulaw() - Convert a linear PCM value to u-law
 *
 */
static unsigned char linear2ulaw(int pcm_val)	/* 2's complement (16-bit range) */
{
    int		mask;
    int		seg;
    unsigned char	uval;

    /* Get the sign and the magnitude of the value. */
    if (pcm_val < 0)
    {
        pcm_val = BIAS - pcm_val;
        mask = 0x7F;
    }
    else
    {
        pcm_val += BIAS;
        mask = 0xFF;
    }

    /* Convert the scaled magnitude to segment number. */
    seg = search(pcm_val, seg_end, 8);

    /*
     * Combine the sign, segment, quantization bits;
     * and complement the code word.
     */
    if (seg >= 8)		/* out of range, return maximum value. */
    {
        return (0x7F ^ mask);
    }
    else
    {
        uval = (seg << 4) | ((pcm_val >> (seg + 3)) & 0xF);
        return (uval ^ mask);
    }

}

int g711a_decode(short amp[], const unsigned char g711a_data[], int g711a_bytes)
{
    int i;
    int samples;
    unsigned char code;
    int sl;

    for (samples = i = 0;;)
    {
        if (i >= g711a_bytes)
        {
            break;
        }
        code = g711a_data[i++];

        sl = alaw2linear(code);

        amp[samples++] = (short) sl;
    }
    return samples * 2;
}

int g711u_decode(short amp[], const unsigned char g711u_data[], int g711u_bytes)
{
    int i;
    int samples;
    unsigned char code;
    int sl;

    for (samples = i = 0;;)
    {
        if (i >= g711u_bytes)
        {
            break;
        }
        code = g711u_data[i++];

        sl = ulaw2linear(code);

        amp[samples++] = (short) sl;
    }
    return samples*2;
}

int g711a_encode(unsigned char g711_data[], const short amp[], int len)
{
    int i;

    for (i = 0; i < len; i++)
    {
        g711_data[i] = linear2alaw(amp[i]);
    }

    return len;
}

int g711u_encode(unsigned char g711_data[], const short amp[], int len)
{
    int i;

    for (i = 0; i < len; i++)
    {
        g711_data[i] = linear2ulaw(amp[i]);
    }

    return len;
}

static void alaw_encode( struct spook_frame *pcm, void *d )
{
    struct alaw_encoder *en = (struct alaw_encoder *)d;
    struct spook_frame *alaw;

    alaw = new_frame();
    alaw->format = FORMAT_ALAW;
    alaw->width = 0;
    alaw->height = 0;

    alaw->length = pcm->length / 2;
    g711a_encode(alaw->d, pcm->d, pcm->length);

    alaw->key = 1;

    unref_frame( pcm );
    deliver_frame_to_stream( alaw, en->output );
}

static void get_framerate( struct stream *s, int *fincr, int *fbase )
{
    struct alaw_encoder *en = (struct alaw_encoder *)s->spook_private;

	en->input->stream->get_framerate( en->input->stream, fincr, fbase );
}

static void set_running( struct stream *s, int running )
{
    struct alaw_encoder *en = (struct alaw_encoder *)s->spook_private;

	set_waiting( en->input, running );
}

/************************ CONFIGURATION DIRECTIVES ************************/

static void *start_block(void)
{
	struct alaw_encoder *en;

	en = (struct alaw_encoder *)malloc( sizeof( struct alaw_encoder ) );
	en->input = NULL;
	en->output = NULL;

	return en;
}

static int end_block( void *d )
{
	struct alaw_encoder *en = (struct alaw_encoder *)d;

	if( ! en->input )
	{
		spook_log( SL_ERR, "alaw: missing input stream name" );
		return -1;
	}
	if( ! en->output )
	{
		spook_log( SL_ERR, "alaw: missing output stream name" );
		return -1;
	}

	return 0;
}

static int set_input( int num_tokens, struct token *tokens, void *d )
{
	struct alaw_encoder *en = (struct alaw_encoder *)d;
	int format = FORMAT_PCM;

	if( ! ( en->input = connect_to_stream( tokens[1].v.str, alaw_encode,
						en, &format, 1 ) ) )
	{
		spook_log( SL_ERR, "alaw: unable to connect to stream \"%s\"",
				tokens[1].v.str );
		return -1;
	}
	return 0;
}

static int set_output( int num_tokens, struct token *tokens, void *d )
{
	struct alaw_encoder *en = (struct alaw_encoder *)d;

	en->output = new_stream( tokens[1].v.str, FORMAT_ALAW, en );
	if( ! en->output )
	{
		spook_log( SL_ERR, "alaw: unable to create stream \"%s\"",
				tokens[1].v.str );
		return -1;
	}
	en->output->get_framerate = get_framerate;
	en->output->set_running = set_running;
	return 0;
}

static struct statement config_statements[] = {
	/* directive name, process function, min args, max args, arg types */
	{ "input", set_input, 1, 1, { TOKEN_STR } },
	{ "output", set_output, 1, 1, { TOKEN_STR } },

	/* empty terminator -- do not remove */
	{ NULL, NULL, 0, 0, {} }
};

int alaw_init(void)
{
	register_config_context( "encoder", "alaw", start_block, end_block,
					config_statements );
	return 0;
}
