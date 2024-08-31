#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include "bmp.h"

#define SHM_NAME "/image_shm"

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Uso: %s <ruta_salida>\n", argv[0]);
        return 1;
    }

    printf("Combinador: inicializando\n");

    char *output_path = argv[1];

    // Mapear la memoria compartida
    int shm_fd = shm_open(SHM_NAME, O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("Combinador: shm_open");
        return 1;
    }

    // Obtener el tamaño de la memoria compartida
    struct stat shm_stat;
    if (fstat(shm_fd, &shm_stat) == -1) {
        perror("Combinador: fstat");
        close(shm_fd);
        return 1;
    }
    size_t shm_size = shm_stat.st_size;

    // Mapear la memoria compartida
    void *ptr = mmap(0, shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (ptr == MAP_FAILED) {
        perror("Combinador: mmap");
        close(shm_fd);
        return 1;
    }

    // Aquí se combinarían las dos porciones procesadas de la imagen
    BMP_Image *image = (BMP_Image *)ptr;
    int half_height = image->norm_height / 2;

    // Combinar las dos mitades de la imagen
    for (int i = 0; i < half_height; i++) {
        for (int j = 0; j < image->header.width_px; j++) {
            image->pixels[i][j] = image->pixels[i + half_height][j];
        }
    }

    // Guardar la imagen combinada en output_path
    writeImage(output_path, image);

    printf("Combinador: Imagen procesada guardada en: %s\n", output_path);

    // Desmapear la memoria compartida
    munmap(ptr, shm_size);
    close(shm_fd);

    return 0;
}