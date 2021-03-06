/* A lexical scanner generated by flex */

/* Scanner skeleton version:
 * $Header: /home/daffy/u0/vern/flex/RCS/flex.skl,v 2.91 96/09/10 16:58:48 vern Exp $
 */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <conf_parse.h>
#include "jk_g_conf.h"

static struct token *cur_token;
static int lineno;

struct token global_tokens[] =
{
    {TOKEN_STR, {.str="FrameHeap"}},{TOKEN_NUM,{25}}, {';',{0}},
    {TOKEN_STR, {.str="Port"}}, {TOKEN_NUM, {8554}}, {';', {0}},
    {TOKEN_STR, {.str="RtpRange"}}, {TOKEN_NUM, {50000}}, {TOKEN_NUM, {60000}},  {';',{0}},
    {TOKEN_STR, {.str="Input"}},{TOKEN_STR, {.str="V4L2"}},   {'{',{0}},
    {TOKEN_STR, {.str="Device"}}, {TOKEN_STR, {.str="/dev/video0"}}, {';',{0}},
    {TOKEN_STR, {.str="InputType"}}, {TOKEN_STR, {.str="webcam"}}, {';',{0}},
    {TOKEN_STR, {.str="InputPort"}}, {TOKEN_NUM, {0}}, {';',{0}},
    {TOKEN_STR, {.str="Format"}},{TOKEN_STR, {.str="mjpeg"}},   {';', {0}},
    {TOKEN_STR, {.str="FrameSize"}},
#ifdef SENSOR_MAX_1080P
    {TOKEN_NUM, {1920}}, {TOKEN_NUM, {1080}},
#endif /* end of SENSOR_MAX_1080P */
#ifdef SENSOR_MAX_720P
    {TOKEN_NUM, {1280}}, {TOKEN_NUM, {720}},
#endif /* end of SENSOR_MAX_720P */
    {';',  {0}},
    {TOKEN_STR, {.str="FrameRate"}}, {TOKEN_NUM, {FRAME_RATE}}, {';',  {0}},
    {TOKEN_STR, {.str="Output"}},{TOKEN_STR, {.str="compressed"}},   {';',{0}},  {'}',{0}},
    {TOKEN_STR, {.str="Input"}},{TOKEN_STR, {.str="OSS"}},{'{',  {0}},
    {TOKEN_STR, {.str="Device"}}, {TOKEN_STR, {.str="/dev/dsp"}}, {';',  {0}},
    {TOKEN_STR, {.str="SampleRate"}}, {TOKEN_NUM, {8000}}, {';',  {0}},
    {TOKEN_STR, {.str="Output"}}, {TOKEN_STR, {.str="pcm"}}, {';',  {0}}, {'}',  {0}},
    {TOKEN_STR, {.str="Encoder"}},{TOKEN_STR, {.str="ALAW"}},   {'{',  {0}},
    {TOKEN_STR, {.str="Input"}}, {TOKEN_STR, {.str="pcm"}}, {';',  {0}},
    {TOKEN_STR, {.str="Output"}}, {TOKEN_STR, {.str="alaw"}},   {';',  {0}},{'}',  {0}},
    {TOKEN_STR, {.str="RTSP-Handler"}},{TOKEN_STR, {.str="Live"}},   {'{',  {0}},
    {TOKEN_STR, {.str="Path"}}, {TOKEN_STR, {.str="/jkstream"}}, {';',  {0}},
    {TOKEN_STR, {.str="Path"}}, {TOKEN_STR, {.str="/webcam"}},{TOKEN_STR,{.str="Spook"}},
    {TOKEN_STR, {.str="admin"}},  {TOKEN_STR, {.str="123456"}}, {';', {0}},
    {TOKEN_STR, {.str="Track"}}, {TOKEN_STR, {.str="compressed"}}, {';',  {0}},
    {TOKEN_STR, {.str="Track"}}, {TOKEN_STR, {.str="alaw"}}, {';',  {0}},{'}',  {0}},
};

int start_conf_read(char *filename)
{
    lineno = 0;

    return 0;
}

int get_next_token( struct token *tok, int *line )
{
    int ret = 1;

	cur_token = tok;
    if((lineno+1) >= sizeof(global_tokens)/sizeof(struct token))
    {
//        printf("there is end of file !!\n");
        tok = NULL;
        return -1;
    }
    *cur_token = global_tokens[lineno];
    *line = lineno;
    lineno++;

	return ret;
}
