#include <stdio.h>
#include <string.h>
#include "encode.h"
#include "types.h"
#include "common.h"

/*
 * Get image capacity in bytes
 * Width is at offset 18, height at offset 22, each 4 bytes
 * Capacity = width * height * 3 (RGB, 1 byte per channel)
 */
uint get_image_size_for_bmp(FILE *fptr_image)
{
    uint width, height;

    fseek(fptr_image, 18, SEEK_SET);
    fread(&width,  sizeof(uint), 1, fptr_image);
    fread(&height, sizeof(uint), 1, fptr_image);

    printf("width = %u, height = %u\n", width, height);
    return width * height * 3;
}

/*
 * Open source image, secret file and stego image
 * BMP files must be opened in binary mode
 */
Status open_files(EncodeInfo *encInfo)
{
    encInfo->fptr_src_image = fopen(encInfo->src_image_fname, "rb");
    if (encInfo->fptr_src_image == NULL)
    {
        perror("fopen");
        fprintf(stderr, "ERROR: Unable to open file %s\n", encInfo->src_image_fname);
        return e_failure;
    }

    encInfo->fptr_secret = fopen(encInfo->secret_fname, "r");
    if (encInfo->fptr_secret == NULL)
    {
        perror("fopen");
        fprintf(stderr, "ERROR: Unable to open file %s\n", encInfo->secret_fname);
        return e_failure;
    }

    encInfo->fptr_stego_image = fopen(encInfo->stego_image_fname, "wb");
    if (encInfo->fptr_stego_image == NULL)
    {
        perror("fopen");
        fprintf(stderr, "ERROR: Unable to open file %s\n", encInfo->stego_image_fname);
        return e_failure;
    }

    return e_success;
}

/*
 * Check argv[1] to decide encode / decode / unsupported
 */
OperationType check_operation_type(char *argv[])
{
    if (argv[1] == NULL)
        return e_unsupported;

    if (strcmp(argv[1], "-e") == 0)
        return e_encode;

    if (strcmp(argv[1], "-d") == 0)
        return e_decode;

    return e_unsupported;
}

/*
 * Validate encode arguments:
 * argv[2] = secret file (.txt)
 * argv[3] = source image (.bmp)
 * argv[4] = output stego image (.bmp) — optional
 */
Status read_and_validate_encode_args(char *argv[], EncodeInfo *encInfo)
{
    if (argv[2] == NULL)
    {
        printf("ERROR: Secret file not provided\n");
        return e_failure;
    }

    if (strstr(argv[2], ".txt") == NULL)
    {
        printf("ERROR: Secret file must be a .txt file\n");
        return e_failure;
    }
    encInfo->secret_fname = argv[2];

    if (argv[3] == NULL)
    {
        printf("ERROR: Source image file not provided\n");
        return e_failure;
    }

    if (strstr(argv[3], ".bmp") == NULL)
    {
        printf("ERROR: Source image must be a .bmp file\n");
        return e_failure;
    }
    encInfo->src_image_fname = argv[3];

    if (argv[4] == NULL)
    {
        encInfo->stego_image_fname = "stego.bmp";
        return e_success;
    }

    if (strstr(argv[4], ".bmp") != NULL)
    {
        encInfo->stego_image_fname = argv[4];
        return e_success;
    }

    printf("ERROR: Output file must be a .bmp file\n");
    return e_failure;
}

/*
 * Copy first 54 bytes (BMP header) from src to dest image
 */
Status copy_bmp_header(FILE *fptr_src_image, FILE *fptr_dest_image)
{
    char header[54];

    fseek(fptr_src_image, 0, SEEK_SET);
    fread(header,  sizeof(char), 54, fptr_src_image);
    fwrite(header, sizeof(char), 54, fptr_dest_image);

    return e_success;
}

/*
 * Encode a single byte into the LSB of 8 image bytes
 * Bit 7 of data goes into image_buffer[0], bit 0 into image_buffer[7]
 */
Status encode_byte_to_lsb(char data, char *image_buffer)
{
    int n = 7;
    for (int i = 0; i <= 7; i++)
    {
        image_buffer[i] = image_buffer[i] & 0xFE;              /* clear LSB */
        char bit = (data & (1 << n)) >> n;                     /* get nth bit */
        image_buffer[i] = image_buffer[i] | bit;               /* set LSB */
        n--;
    }
    return e_success;
}

/*
 * Encode a 4-byte integer into the LSB of 32 image bytes
 * Bit 31 goes into image_buffer[0], bit 0 into image_buffer[31]
 */
Status encode_size_to_lsb(int data, char *image_buffer)
{
    int n = 31;
    for (int i = 0; i <= 31; i++)
    {
        image_buffer[i] = image_buffer[i] & 0xFE;              /* clear LSB */
        int bit = (data & (1 << n)) >> n;                      /* get nth bit */
        image_buffer[i] = image_buffer[i] | bit;               /* set LSB */
        n--;
    }
    return e_success;
}

/*
 * Encode magic string ("#*") into image
 * Each char uses 8 image bytes
 */
Status encode_magic_string(const char *magic_string, EncodeInfo *encInfo)
{
    char buffer[8];

    for (int i = 0; i < (int)MAGIC_STRING_LEN; i++)
    {
        fread(buffer, sizeof(char), 8, encInfo->fptr_src_image);
        encode_byte_to_lsb(magic_string[i], buffer);
        fwrite(buffer, sizeof(char), 8, encInfo->fptr_stego_image);
    }

    return e_success;
}

/*
 * Encode extension length as a 4-byte int (uses 32 image bytes)
 */
Status encode_extn_file_size(int extn_size, EncodeInfo *encInfo)
{
    char buffer[32];

    fread(buffer,  sizeof(char), 32, encInfo->fptr_src_image);
    encode_size_to_lsb(extn_size, buffer);
    fwrite(buffer, sizeof(char), 32, encInfo->fptr_stego_image);

    return e_success;
}

/*
 * Encode secret file extension char by char (e.g. ".txt")
 * Each char uses 8 image bytes
 */
Status encode_secret_file_extn(const char *file_extn, EncodeInfo *encInfo)
{
    char buffer[8];
    int len = strlen(file_extn);

    for (int i = 0; i < len; i++)
    {
        fread(buffer, sizeof(char), 8, encInfo->fptr_src_image);
        encode_byte_to_lsb(file_extn[i], buffer);
        fwrite(buffer, sizeof(char), 8, encInfo->fptr_stego_image);
    }

    return e_success;
}

/*
 * Encode secret file size as a 4-byte int (uses 32 image bytes)
 */
Status encode_secret_file_size(long file_size, EncodeInfo *encInfo)
{
    char buffer[32];

    fread(buffer,  sizeof(char), 32, encInfo->fptr_src_image);
    encode_size_to_lsb((int)file_size, buffer);
    fwrite(buffer, sizeof(char), 32, encInfo->fptr_stego_image);

    return e_success;
}

/*
 * Encode secret file data byte by byte
 * Each byte of secret data uses 8 image bytes
 */
Status encode_secret_file_data(EncodeInfo *encInfo)
{
    char ch;
    char buffer[8];

    while (fread(&ch, sizeof(char), 1, encInfo->fptr_secret) == 1)
    {
        fread(buffer, sizeof(char), 8, encInfo->fptr_src_image);
        encode_byte_to_lsb(ch, buffer);
        fwrite(buffer, sizeof(char), 8, encInfo->fptr_stego_image);
    }

    return e_success;
}

/*
 * Copy leftover image bytes (after encoded region) to stego image
 */
Status copy_remaining_img_data(FILE *fptr_src, FILE *fptr_dest)
{
    char ch;

    while (fread(&ch, sizeof(char), 1, fptr_src) == 1)
        fwrite(&ch, sizeof(char), 1, fptr_dest);

    return e_success;
}

/*
 * Check image has enough capacity to hold all encoded data
 * Also populates extension, extension size, and secret file size
 */
Status check_capacity(EncodeInfo *encInfo)
{
    encInfo->image_capacity = get_image_size_for_bmp(encInfo->fptr_src_image);

    strcpy(encInfo->extn_secret_file, strstr(encInfo->secret_fname, "."));
    encInfo->extn_size = strlen(encInfo->extn_secret_file);

    fseek(encInfo->fptr_secret, 0, SEEK_END);
    encInfo->size_secret_file = ftell(encInfo->fptr_secret);
    fseek(encInfo->fptr_secret, 0, SEEK_SET);

    /* magic(16) + extn_size(32) + extn(extn_size*8) + file_size(32) + data(size*8) */
    int bytes_needed = MAGIC_STRING_LEN * 8
                     + 32
                     + encInfo->extn_size * 8
                     + 32
                     + encInfo->size_secret_file * 8;

    if (bytes_needed <= (int)(encInfo->image_capacity - 54))
        return e_success;

    return e_failure;
}

/*
 * Master encoding function
 */
Status do_encoding(EncodeInfo *encInfo)
{
    Status ret;

    ret = open_files(encInfo);
    if (ret == e_failure)
    {
        printf("ERROR: Failed to open files\n");
        return e_failure;
    }
    printf("Files opened successfully\n");

    if (check_capacity(encInfo) == e_failure)
    {
        printf("ERROR: Image does not have enough capacity\n");
        return e_failure;
    }
    printf("Capacity check passed\n");

    copy_bmp_header(encInfo->fptr_src_image, encInfo->fptr_stego_image);
    printf("BMP header copied\n");

    if (encode_magic_string(MAGIC_STRING, encInfo) == e_failure)
    {
        printf("ERROR: Failed to encode magic string\n");
        return e_failure;
    }
    printf("Magic string encoded\n");

    if (encode_extn_file_size(encInfo->extn_size, encInfo) == e_failure)
    {
        printf("ERROR: Failed to encode extension size\n");
        return e_failure;
    }
    printf("Extension size encoded\n");

    if (encode_secret_file_extn(encInfo->extn_secret_file, encInfo) == e_failure)
    {
        printf("ERROR: Failed to encode extension\n");
        return e_failure;
    }
    printf("Extension encoded\n");

    if (encode_secret_file_size(encInfo->size_secret_file, encInfo) == e_failure)
    {
        printf("ERROR: Failed to encode file size\n");
        return e_failure;
    }
    printf("Secret file size encoded\n");

    if (encode_secret_file_data(encInfo) == e_failure)
    {
        printf("ERROR: Failed to encode secret data\n");
        return e_failure;
    }
    printf("Secret data encoded\n");

    copy_remaining_img_data(encInfo->fptr_src_image, encInfo->fptr_stego_image);
    printf("Remaining image data copied\n");

    fclose(encInfo->fptr_src_image);
    fclose(encInfo->fptr_secret);
    fclose(encInfo->fptr_stego_image);

    return e_success;
}