#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "decode.h"
#include "types.h"
#include "common.h"

/*
 * Rebuild one byte from the LSB of 8 consecutive image bytes
 * Mirrors encode_byte_to_lsb exactly — bit 7 is in buffer[0]
 */
char decode_byte_from_lsb(char *image_buffer)
{
    char data = 0;
    int n = 7;
    for (int i = 0; i <= 7; i++)
    {
        data = data | ((image_buffer[i] & 0x01) << n);
        n--;
    }
    return data;
}

/*
 * Rebuild a 4-byte int from the LSB of 32 consecutive image bytes
 * Mirrors encode_size_to_lsb exactly — bit 31 is in buffer[0]
 */
int decode_size_from_lsb(char *image_buffer)
{
    int data = 0;
    int n = 31;
    for (int i = 0; i <= 31; i++)
    {
        data = data | ((image_buffer[i] & 0x01) << n);
        n--;
    }
    return data;
}

/*
 * Open the stego image in binary read mode
 */
Status open_files_decode(DecodeInfo *decInfo)
{
    decInfo->fptr_stego_image = fopen(decInfo->stego_image_fname, "rb");
    if (decInfo->fptr_stego_image == NULL)
    {
        perror("fopen");
        fprintf(stderr, "ERROR: Unable to open file %s\n", decInfo->stego_image_fname);
        return e_failure;
    }
    return e_success;
}

/*
 * Skip the 54-byte BMP header — encoded data starts right after
 */
Status skip_bmp_header(DecodeInfo *decInfo)
{
    fseek(decInfo->fptr_stego_image, 54, SEEK_SET);
    return e_success;
}

/*
 * Read MAGIC_STRING_LEN chars from image and compare against MAGIC_STRING
 * If they don't match the image carries no hidden data
 */
Status decode_magic_string(DecodeInfo *decInfo)
{
    char buffer[8];
    char magic[MAGIC_STRING_LEN + 1];

    for (int i = 0; i < (int)MAGIC_STRING_LEN; i++)
    {
        if (fread(buffer, sizeof(char), 8, decInfo->fptr_stego_image) != 8)
        {
            printf("ERROR: Unexpected end of file while reading magic string\n");
            return e_failure;
        }
        magic[i] = decode_byte_from_lsb(buffer);
    }
    magic[MAGIC_STRING_LEN] = '\0';

    if (strcmp(magic, MAGIC_STRING) != 0)
    {
        printf("ERROR: No hidden data found in this image\n");
        return e_failure;
    }

    printf("Magic string check passed\n");
    return e_success;
}

/*
 * Read 32 image bytes and decode the extension length from their LSBs
 */
Status decode_extn_size(DecodeInfo *decInfo)
{
    char buffer[32];

    if (fread(buffer, sizeof(char), 32, decInfo->fptr_stego_image) != 32)
    {
        printf("ERROR: Unexpected end of file while reading extension size\n");
        return e_failure;
    }

    decInfo->extn_size = decode_size_from_lsb(buffer);
    printf("Extension size : %d\n", decInfo->extn_size);
    return e_success;
}

/*
 * Decode extn_size chars from image to recover the file extension (e.g. ".txt")
 * If no output filename was given, auto-generate one using the decoded extension
 */
Status decode_secret_file_extn(DecodeInfo *decInfo)
{
    char buffer[8];

    for (int i = 0; i < decInfo->extn_size; i++)
    {
        if (fread(buffer, sizeof(char), 8, decInfo->fptr_stego_image) != 8)
        {
            printf("ERROR: Unexpected end of file while reading extension\n");
            return e_failure;
        }
        decInfo->extn_secret_file[i] = decode_byte_from_lsb(buffer);
    }
    decInfo->extn_secret_file[decInfo->extn_size] = '\0';

    printf("Secret file extension : %s\n", decInfo->extn_secret_file);

    if (decInfo->secret_fname == NULL)
    {
        char *name = malloc(16);
        if (name == NULL)
        {
            printf("ERROR: Memory allocation failed\n");
            return e_failure;
        }
        strcpy(name, "output");
        strcat(name, decInfo->extn_secret_file);
        decInfo->secret_fname = name;
    }

    return e_success;
}

/*
 * Decode 32 image bytes to recover the secret file size
 */
Status decode_secret_file_size(DecodeInfo *decInfo)
{
    char buffer[32];

    if (fread(buffer, sizeof(char), 32, decInfo->fptr_stego_image) != 32)
    {
        printf("ERROR: Unexpected end of file while reading file size\n");
        return e_failure;
    }

    decInfo->size_secret_file = (long)decode_size_from_lsb(buffer);
    printf("Secret file size : %ld bytes\n", decInfo->size_secret_file);
    return e_success;
}

/*
 * Decode size_secret_file bytes from image and write them to the output file
 * 8 image bytes are consumed per secret byte — exact mirror of encode_secret_file_data
 */
Status decode_secret_file_data(DecodeInfo *decInfo)
{
    char buffer[8];
    char ch;

    decInfo->fptr_secret = fopen(decInfo->secret_fname, "w");
    if (decInfo->fptr_secret == NULL)
    {
        perror("fopen");
        fprintf(stderr, "ERROR: Unable to create output file %s\n", decInfo->secret_fname);
        return e_failure;
    }

    for (long i = 0; i < decInfo->size_secret_file; i++)
    {
        if (fread(buffer, sizeof(char), 8, decInfo->fptr_stego_image) != 8)
        {
            printf("ERROR: Unexpected end of file while reading secret data\n");
            fclose(decInfo->fptr_secret);
            return e_failure;
        }
        ch = decode_byte_from_lsb(buffer);
        fwrite(&ch, sizeof(char), 1, decInfo->fptr_secret);
    }

    fclose(decInfo->fptr_secret);
    printf("Secret data decoded\n");
    return e_success;
}

/*
 * Validate decode arguments:
 * argv[2] = stego image (.bmp)  — required
 * argv[3] = output filename     — optional
 */
Status read_and_validate_decode_args(char *argv[], DecodeInfo *decInfo)
{
    if (argv[2] == NULL)
    {
        printf("ERROR: Stego image file not provided\n");
        return e_failure;
    }

    if (strstr(argv[2], ".bmp") == NULL)
    {
        printf("ERROR: Stego image must be a .bmp file\n");
        return e_failure;
    }
    decInfo->stego_image_fname = argv[2];

    decInfo->secret_fname = (argv[3] != NULL) ? argv[3] : NULL;

    return e_success;
}

/*
 * Master decode function — runs all steps in the same order they were encoded
 */
Status do_decoding(DecodeInfo *decInfo)
{
    if (open_files_decode(decInfo) == e_failure)
        return e_failure;
    printf("Files opened successfully\n");

    skip_bmp_header(decInfo);

    if (decode_magic_string(decInfo) == e_failure)
    {
        fclose(decInfo->fptr_stego_image);
        return e_failure;
    }

    if (decode_extn_size(decInfo) == e_failure)
    {
        fclose(decInfo->fptr_stego_image);
        return e_failure;
    }

    if (decode_secret_file_extn(decInfo) == e_failure)
    {
        fclose(decInfo->fptr_stego_image);
        return e_failure;
    }

    if (decode_secret_file_size(decInfo) == e_failure)
    {
        fclose(decInfo->fptr_stego_image);
        return e_failure;
    }

    if (decode_secret_file_data(decInfo) == e_failure)
    {
        fclose(decInfo->fptr_stego_image);
        return e_failure;
    }

    fclose(decInfo->fptr_stego_image);
    printf("Decoding done -> %s\n", decInfo->secret_fname);
    return e_success;
}