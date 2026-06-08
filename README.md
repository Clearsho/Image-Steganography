# Image Steganography

Hides secret text files inside BMP images using LSB (Least Significant Bit) technique in C.

## How it works

Every pixel in a BMP image has 3 bytes (R, G, B).
The program replaces the last bit of each byte with bits from the secret file.
The change is too small for the human eye to notice any difference in the image.

## Files

| File | Description |
|------|-------------|
| test_encode.c | main file, handles encode/decode flow |
| encode.c / encode.h | encoding logic |
| decode.c / decode.h | decoding logic |
| common.h | magic string definition |
| types.h | common typedefs and enums |

## Compile

make

or manually:

gcc test_encode.c encode.c decode.c -o stego

## Usage

Encode a secret file into an image:

./stego -e secret.txt src.bmp output.bmp

Decode and recover the hidden file:

./stego -d stego.bmp
