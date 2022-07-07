#ifndef _MP4_MUTEX_H_
#define _MP4_MUTEX_H_

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define G711_BUFFER_SIZE	(1200)
#define PCM_BUFFER_SIZE		(2400)

#define TEMP_BUFFER_SIZE	(4096)
#define ADTS_HEADER_LENGTH	(7)

typedef unsigned long   ULONG;
typedef unsigned int    UINT;
typedef unsigned char   UBYTE;
typedef char            _TCHAR;

/*
* u-law, A-law and linear PCM conversions.
*/
#define	SIGN_BIT	(0x80)		/* Sign bit for a A-law byte. */
#define	QUANT_MASK	(0xf)		/* Quantization field mask. */
#define	NSEGS		(8)		/* Number of A-law segments. */
#define	SEG_SHIFT	(4)		/* Left shift for segment number. */
#define	SEG_MASK	(0x70)		/* Segment field mask. */
#define	BIAS		(0x84)		/* Bias for linear code. */


int g711a_decode(short amp[], const unsigned char g711a_data[], int g711a_bytes);

int g711u_decode(short amp[], const unsigned char g711u_data[], int g711u_bytes);

int g711a_encode(unsigned char g711_data[], const short amp[], int len);

int g711u_encode(unsigned char g711_data[], const short amp[], int len);


#endif
