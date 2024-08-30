#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "bmp.h"

void printError(int error){
  switch(error){
  case ARGUMENT_ERROR:
    printf("Usage:ex5 <source> <destination>\n");
    break;
  case FILE_ERROR:
    printf("Unable to open file!\n");
    break;
  case MEMORY_ERROR:
    printf("Unable to allocate memory!\n");
    break;
  case VALID_ERROR:
    printf("BMP file not valid!\n");
    break;
  default:
    break;
  }
}

BMP_Image* createBMPImage(FILE *file) {
    printf("createBMPImage: Inicio de la función\n");
    BMP_Image *image = (BMP_Image *)malloc(sizeof(BMP_Image));
    if (image == NULL) {
        printf("createBMPImage: Error al asignar memoria para BMP_Image\n");
        return NULL;
    }
    printf("createBMPImage: Memoria asignada para BMP_Image\n");
    // Aquí se debería inicializar la estructura BMP_Image
    // Leer el encabezado BMP
    printf("createBMPImage: Intentando leer el encabezado BMP\n");
    size_t read_size = fread(&(image->header), sizeof(BMP_Header), 1, file);
    if (read_size != 1) {
        printf("createBMPImage: Error al leer el encabezado BMP\n");
        free(image);
        return NULL;
    }
    printf("createBMPImage: Encabezado BMP leído\n");

    // Validar el encabezado BMP
    if (!checkBMPValid(&(image->header))) {
        free(image);
        return NULL;
    }
    
    // Calcular el tamaño de la imagen y asignar memoria para los píxeles
    image->norm_height = abs(image->header.height_px);
    image->bytes_per_pixel = image->header.bits_per_pixel / 8;
    int dataSize = image->norm_height * image->header.width_px * image->bytes_per_pixel;
    image->pixels = (Pixel **)malloc(image->norm_height * sizeof(Pixel *));
    if (image->pixels == NULL) {
        free(image);
        return NULL;
    }
    for (int i = 0; i < image->norm_height; i++) {
        image->pixels[i] = (Pixel *)malloc(image->header.width_px * sizeof(Pixel));
        if (image->pixels[i] == NULL) {
            for (int j = 0; j < i; j++) {
                free(image->pixels[j]);
            }
            free(image->pixels);
            free(image);
            return NULL;
        }
    }
    // Leer los datos de la imagen
    readImageData(file, image, dataSize);
    return image;
}

void readImageData(FILE* srcFile, BMP_Image* image, int dataSize) {
    fseek(srcFile, image->header.offset, SEEK_SET);
    
    int padding = (4 - (image->header.width_px * image->bytes_per_pixel) % 4) % 4;
    
    for (int i = 0; i < image->norm_height; i++) {
        fread(image->pixels[i], sizeof(Pixel), image->header.width_px, srcFile);
        fseek(srcFile, padding, SEEK_CUR);  // Skip padding
    }
}

void readImage(FILE *srcFile, BMP_Image* dataImage) {
    dataImage = createBMPImage(srcFile);
    if (dataImage == NULL) {
        return;
    }
    
    int dataSize = dataImage->norm_height * dataImage->header.width_px * dataImage->bytes_per_pixel;
    readImageData(srcFile, dataImage, dataSize);
}

void writeImage(char* destFileName, BMP_Image* dataImage) {
    FILE* destFile = fopen(destFileName, "wb");
    if (destFile == NULL) {
        printError(FILE_ERROR);
        return;
    }

    fwrite(&dataImage->header, sizeof(BMP_Header), 1, destFile);

    int padding = (4 - (dataImage->header.width_px * dataImage->bytes_per_pixel) % 4) % 4;
    uint8_t paddingBytes[3] = {0, 0, 0};

    for (int i = 0; i < dataImage->norm_height; i++) {
        fwrite(dataImage->pixels[i], sizeof(Pixel), dataImage->header.width_px, destFile);
        fwrite(paddingBytes, 1, padding, destFile);
    }

    fclose(destFile);
}

void freeImage(BMP_Image* image) {
    if (image != NULL) {
        if (image->pixels != NULL) {
            for (int i = 0; i < image->norm_height; i++) {
                free(image->pixels[i]);
            }
            free(image->pixels);
        }
        free(image);
    }
}

int checkBMPValid(BMP_Header* header) {
    if (header->type != 0x4d42) {
        return FALSE;
    }
    if (header->bits_per_pixel != 24) {
        return FALSE;
    }
    if (header->planes != 1) {
        return FALSE;
    }
    if (header->compression != 0) {
        return FALSE;
    }
    return TRUE;
}

void printBMPHeader(BMP_Header* header) {
    printf("file type (should be 0x4d42): %x\n", header->type);
    printf("file size: %d\n", header->size);
    printf("offset to image data: %d\n", header->offset);
    printf("header size: %d\n", header->header_size);
    printf("width_px: %d\n", header->width_px);
    printf("height_px: %d\n", header->height_px);
    printf("planes: %d\n", header->planes);
    printf("bits: %d\n", header->bits_per_pixel);
}

void printBMPImage(BMP_Image* image) {
    printf("data size is %ld\n", sizeof(image->pixels));
    printf("norm_height size is %d\n", image->norm_height);
    printf("bytes per pixel is %d\n", image->bytes_per_pixel);
}