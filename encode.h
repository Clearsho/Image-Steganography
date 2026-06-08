#ifndef ENCODE_H
#define ENCODE_H

#include "types.h"

/*
 * Structure to store information required for
 * encoding secret file to source Image
 */

#define MAX_SECRET_BUF_SIZE 1
#define MAX_IMAGE_BUF_SIZE  (MAX_SECRET_BUF_SIZE * 8)
#define MAX_FILE_SUFFIX     5   /* ".txt" + '\0' = 5 */

typedef struct _EncodeInfo
{
    /* Source Image info */
    char *src_image_fname;
    FILE *fptr_src_image;
    uint  image_capacity;
    char  image_data[MAX_IMAGE_BUF_SIZE];

    /* Secret File Info */
    char *secret_fname;
    FILE *fptr_secret;
    char  extn_secret_file[MAX_FILE_SUFFIX];
    int   extn_size;
    long  size_secret_file;     /* single declaration, long to match ftell */
    char  secret_data[MAX_SECRET_BUF_SIZE];

    /* Stego Image Info */
    char *stego_image_fname;
    FILE *fptr_stego_image;

} EncodeInfo;


/* Check operation type */
OperationType check_operation_type(char *argv[]);

/* Read and validate encode args from argv */
Status read_and_validate_encode_args(char *argv[], EncodeInfo *encInfo);

/* Perform the encoding */
Status do_encoding(EncodeInfo *encInfo);

/* Get file pointers for i/p and o/p files */
Status open_files(EncodeInfo *encInfo);

/* Check image capacity against secret file size */
Status check_capacity(EncodeInfo *encInfo);

/* Get image size in bytes (width * height * 3) */
uint get_image_size_for_bmp(FILE *fptr_image);

/* Copy BMP header (54 bytes) from src to dest */
Status copy_bmp_header(FILE *fptr_src_image, FILE *fptr_dest_image);

/* Encode magic string into image */
Status encode_magic_string(const char *magic_string, EncodeInfo *encInfo);

/* Encode extension size as 4-byte int into image */
Status encode_extn_file_size(int extn_size, EncodeInfo *encInfo);

/* Encode secret file extension into image */
Status encode_secret_file_extn(const char *file_extn, EncodeInfo *encInfo);

/* Encode secret file size as 4-byte int into image */
Status encode_secret_file_size(long file_size, EncodeInfo *encInfo);

/* Encode secret file data byte by byte into image */
Status encode_secret_file_data(EncodeInfo *encInfo);

/* Encode a single byte into LSB of 8 image bytes */
Status encode_byte_to_lsb(char data, char *image_buffer);

/* Encode a 4-byte int into LSB of 32 image bytes */
Status encode_size_to_lsb(int data, char *image_buffer);

/* Copy remaining image bytes after encoding is done */
Status copy_remaining_img_data(FILE *fptr_src, FILE *fptr_dest);

#endif