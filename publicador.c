#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <semaphore.h>
#include "bmp.h"

#define SHM_NAME "/image_shm"
#define SEM_READY "/sem_ready"

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Uso: %s <ruta_entrada>\n", argv[0]);
        return 1;
    }

    printf("Publicador: inicializando\n");

    char *input_path = argv[1];

    // Leer la imagen BMP
    FILE *srcFile = fopen(input_path, "rb");
    if (srcFile == NULL) {
        perror("Publicador: fopen");
        return 1;
    }

    BMP_Image *image = readImage(srcFile);
    fclose(srcFile);
    if (image == NULL) {
        fprintf(stderr, "Publicador: Error al leer la imagen BMP\n");
        return 1;
    }

    // Crear memoria compartida
    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("Publicador: shm_open");
        freeImage(image);
        return 1;
    }

    size_t shm_size = sizeof(BMP_Image) + image->norm_height * image->bytes_per_pixel * image->header.width_px;
    if (ftruncate(shm_fd, shm_size) == -1) {
        perror("Publicador: ftruncate");
        close(shm_fd);
        freeImage(image);
        return 1;
    }

    void *ptr = mmap(0, shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (ptr == MAP_FAILED) {
        perror("Publicador: mmap");
        close(shm_fd);
        freeImage(image);
        return 1;
    }

    // Copiar la imagen a la memoria compartida
    memcpy(ptr, image, shm_size);

    // Señalizar que la imagen está lista
    sem_t *sem_ready = sem_open(SEM_READY, O_CREAT, 0666, 0);
    if (sem_ready == SEM_FAILED) {
        perror("Publicador: sem_open");
        munmap(ptr, shm_size);
        close(shm_fd);
        freeImage(image);
        return 1;
    }
    sem_post(sem_ready);

    munmap(ptr, shm_size);
    close(shm_fd);
    sem_close(sem_ready);
    freeImage(image);
    printf("Publicador: Imagen cargada en memoria compartida.\n");
    return 0;
}