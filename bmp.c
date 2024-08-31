#include <stdlib.h>
#include <stdio.h>
#include "bmp.h"

/* Función para imprimir mensajes de error */
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

BMP_Image* createBMPImage(FILE* fptr) {
    // Asignar memoria para BMP_Image
    BMP_Image* image = (BMP_Image*)malloc(sizeof(BMP_Image));
    if (image == NULL) {
        printError(MEMORY_ERROR);
        return NULL;
    }

    // Leer los primeros 54 bytes del archivo fuente en el header
    if (fread(&image->header, sizeof(BMP_Header), 1, fptr) != 1) {
        free(image);
        printError(FILE_ERROR);
        return NULL;
    }

    // Imprimir información del encabezado BMP para depuración
    printBMPHeader(&image->header);

    // Verificar si el archivo BMP es válido
    if (!checkBMPValid(&image->header)) {
        free(image);
        printError(VALID_ERROR);
        return NULL;
    }

    // Calcular el ancho, alto y bytes por pixel
    int width = image->header.width_px;
    int height = abs(image->header.height_px);
    int bytes_per_pixel = image->header.bits_per_pixel / 8;

    image->norm_height = height;
    image->bytes_per_pixel = bytes_per_pixel;

    // Asignar memoria para los datos de la imagen
    image->pixels = (Pixel**)malloc(height * sizeof(Pixel*));
    if (image->pixels == NULL) {
        free(image);
        printError(MEMORY_ERROR);
        return NULL;
    }

    for (int i = 0; i < height; i++) {
        image->pixels[i] = (Pixel*)malloc(width * bytes_per_pixel); // Ajustado para 32 bits por píxel
        if (image->pixels[i] == NULL) {
            for (int j = 0; j < i; j++) {
                free(image->pixels[j]);
            }
            free(image->pixels);
            free(image);
            printError(MEMORY_ERROR);
            return NULL;
        }
    }

    // Leer los datos de la imagen con padding
    int padding = (4 - (width * bytes_per_pixel) % 4) % 4;

    printf("Leyendo datos de imagen...\n");
    for (int i = 0; i < height; i++) {
        if (fread(image->pixels[i], bytes_per_pixel, width, fptr) != width) {
            for (int j = 0; j < height; j++) {
                free(image->pixels[j]);
            }
            free(image->pixels);
            free(image);
            printError(FILE_ERROR);
            return NULL;
        }
        // Saltar el padding
        fseek(fptr, padding, SEEK_CUR);
    }

    return image;
}

void readImageData(FILE* srcFile, BMP_Image* image, int dataSize) {
    fread(image->pixels[0], sizeof(Pixel), dataSize, srcFile);
}

BMP_Image* readImage(FILE* srcFile) {
    printf("Llamando a createBMPImage...\n");
    BMP_Image* image = createBMPImage(srcFile);
    if (image == NULL) {
        printf("Error: createBMPImage devolvió NULL\n");
    }
    return image;
}

void writeImage(char* destFileName, BMP_Image* dataImage) {
    
    printf("Abriendo archivo de salida para escritura: %s\n", destFileName);
    FILE* destFile = fopen(destFileName, "wb");
    if (destFile == NULL) {
        printError(FILE_ERROR);
        return;
    }

    printf("Escribiendo el encabezado BMP...\n");
    if (fwrite(&dataImage->header, sizeof(BMP_Header), 1, destFile) != 1) {
        fclose(destFile);
        printError(FILE_ERROR);
        return;
    }

    int width = dataImage->header.width_px;
    int height = abs(dataImage->header.height_px);
    int bytes_per_pixel = dataImage->header.bits_per_pixel / 8;
    int padding = (4 - (width * bytes_per_pixel) % 4) % 4;

    printf("Escribiendo datos de imagen...\n");
    for (int i = 0; i < height; i++) {
        if (fwrite(dataImage->pixels[i], bytes_per_pixel, width, destFile) != width) {
            fclose(destFile);
            printError(FILE_ERROR);
            return;
        }
        // Escribir el padding
        uint8_t paddingBytes[3] = {0, 0, 0};
        if (fwrite(paddingBytes, 1, padding, destFile) != padding) {
            fclose(destFile);
            printError(FILE_ERROR);
            return;
        }
    }

    printf("Cerrando archivo de salida...\n");
    fclose(destFile);
}

void freeImage(BMP_Image* image) {
    for (int i = 0; i < image->norm_height; i++) {
        free(image->pixels[i]);
    }
    free(image->pixels);
    free(image);
}

int checkBMPValid(BMP_Header* header) {
    if (header->type != 0x4d42) {
        return FALSE;
    }
    if (header->bits_per_pixel != 24 && header->bits_per_pixel != 32) {
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
