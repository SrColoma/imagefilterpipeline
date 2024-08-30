#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <unistd.h>
#include <string.h>
#include "bmp.h"

#define SHM_NAME "/image_shm"
#define SEM_READY "/sem_ready"

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Uso: %s <ruta_imagen>\n", argv[0]);
        return 1;
    }

    printf("Publicador: inicializando\n");

    char *filepath = argv[1];
    FILE *file = fopen(filepath, "rb");
    if (file == NULL) {
        fprintf(stderr, "Error al abrir el archivo: %s\n", filepath);
        return FILE_ERROR;
    }
    printf("Publicador: archivo abierto\n");

    BMP_Image *image = createBMPImage();
    if (image == NULL) {
        fclose(file);
        printf("Publicador: BMP_Image MEMORY_ERROR\n");
        return MEMORY_ERROR;
    }


    readImage(file, image);
    fclose(file);

    printf("Publicador: Imagen BMP leída correctamente\n");

    // Calcular el tamaño total de la memoria compartida
    size_t pixels_size = image->norm_height * image->header.width_px * sizeof(Pixel);
    size_t shm_size = sizeof(BMP_Image) + pixels_size;

    // Crear y mapear la memoria compartida
    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    printf("Publicador: Memoria compartida creada\n");
    if (shm_fd == -1) {
        perror("shm_open");
        freeImage(image);
        return 1;
    }

    if (ftruncate(shm_fd, shm_size) == -1) {
        perror("ftruncate");
        freeImage(image);
        close(shm_fd);
        shm_unlink(SHM_NAME);
        return 1;
    }

    void *ptr = mmap(NULL, shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (ptr == MAP_FAILED) {
        perror("mmap");
        freeImage(image);
        close(shm_fd);
        shm_unlink(SHM_NAME);
        return 1;
    }

    // Copiar la imagen a la memoria compartida
    memcpy(ptr, image, sizeof(BMP_Image));
    
    // Copiar los pixels
    Pixel *pixels_ptr = (Pixel *)((char *)ptr + sizeof(BMP_Image));
    for (int i = 0; i < image->norm_height; i++) {
        memcpy(pixels_ptr + i * image->header.width_px, image->pixels[i], image->header.width_px * sizeof(Pixel));
    }

    printf("Publicador: Imagen copiada a la memoria compartida\n");

    // Señalizar que la imagen está lista
    sem_t *sem_ready = sem_open(SEM_READY, 0);
    if (sem_ready == SEM_FAILED) {
        perror("sem_open");
        munmap(ptr, shm_size);
        close(shm_fd);
        shm_unlink(SHM_NAME);
        freeImage(image);
        return 1;
    }

    sem_post(sem_ready);
    printf("Publicador: Señal enviada, imagen lista para procesar\n");

    // Limpiar recursos
    freeImage(image);
    munmap(ptr, shm_size);
    close(shm_fd);
    sem_close(sem_ready);

    return 0;
}