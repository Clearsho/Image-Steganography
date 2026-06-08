#include <stdio.h>
#include "encode.h"
#include "decode.h"
#include "types.h"

int main(int argc __attribute__((unused)), char *argv[])
{
    OperationType op = check_operation_type(argv);

    if (op == e_encode)
    {
        EncodeInfo encInfo;

        if (read_and_validate_encode_args(argv, &encInfo) == e_failure)
        {
            printf("ERROR: Invalid arguments for encoding\n");
            return 1;
        }

        if (do_encoding(&encInfo) == e_failure)
        {
            printf("ERROR: Encoding failed\n");
            return 1;
        }

        printf("Encoding successful\n");
        return 0;
    }
    else if (op == e_decode)
    {
        DecodeInfo decInfo;

        if (read_and_validate_decode_args(argv, &decInfo) == e_failure)
        {
            printf("ERROR: Invalid arguments for decoding\n");
            return 1;
        }

        if (do_decoding(&decInfo) == e_failure)
        {
            printf("ERROR: Decoding failed\n");
            return 1;
        }

        printf("Decoding successful\n");
        return 0;
    }
    else
    {
        printf("Usage:\n");
        printf("  Encode: %s -e <secret.txt> <src.bmp> [stego.bmp]\n", argv[0]);
        printf("  Decode: %s -d <stego.bmp> [output_file]\n", argv[0]);
        return 1;
    }
}