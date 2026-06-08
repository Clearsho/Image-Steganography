#ifndef DECODE_H
#define DECODE_H

#include "types.h"

#define MAX_SECRET_BUF_SIZE 1
#define MAX_IMAGE_BUF_SIZE  (MAX_SECRET_BUF_SIZE * 8)
#define MAX_FILE_SUFFIX     5

typedef struct _DecodeInfo
{
    /* Stego Image */
    char *stego_image_fname;
    FILE *fptr_stego_image;

    /* Output Secret File */
    char *secret_fname;
    FILE *fptr_secret;
    char  extn_secret_file[MAX_FILE_SUFFIX];
    int   extn_size;
    long  size_secret_file;

} DecodeInfo;

/* Read and validate decode args from argv */
Status read_and_validate_decode_args(char *argv[], DecodeInfo *decInfo);

/* Perform the decoding */
Status do_decoding(DecodeInfo *decInfo);

/* Open stego image file */
Status open_files_decode(DecodeInfo *decInfo);

/* Skip 54-byte BMP header */
Status skip_bmp_header(DecodeInfo *decInfo);

/* Read and validate magic string */
Status decode_magic_string(DecodeInfo *decInfo);

/* Decode extension size */
Status decode_extn_size(DecodeInfo *decInfo);

/* Decode secret file extension */
Status decode_secret_file_extn(DecodeInfo *decInfo);

/* Decode secret file size */
Status decode_secret_file_size(DecodeInfo *decInfo);

/* Decode and write secret file data */
Status decode_secret_file_data(DecodeInfo *decInfo);

/* Decode one byte from LSB of 8 image bytes */
char decode_byte_from_lsb(char *image_buffer);

/* Decode 4-byte int from LSB of 32 image bytes */
int decode_size_from_lsb(char *image_buffer);

#endif