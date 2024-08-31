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

    printf("Combinador: memoria compartida mapeada\n");

    // Aquí se combinarían las dos porciones procesadas de la imagen
    BMP_Image *image = (BMP_Image *)ptr;
    int half_height = image->norm_height / 2;

    // Verificar que la imagen esté correctamente cargada
    if (image->header.width_px == 0 || image->norm_height == 0) {
        fprintf(stderr, "Combinador: Error en los datos de la imagen\n");
        munmap(ptr, shm_size);
        close(shm_fd);
        return 1;
    }

    printf("Combinador: imagenes correctamente cargadas\n");

    // Verificar que los punteros a los píxeles estén correctamente inicializados
    if (image->pixels == NULL) {
        fprintf(stderr, "Combinador: Puntero a los píxeles es NULL\n");
        munmap(ptr, shm_size);
        close(shm_fd);
        return 1;
    }

    // Combinar las dos mitades de la imagen
    for (int i = 0; i < half_height; i++) {
        for (int j = 0; j < image->header.width_px; j++) {
            // Verificar los índices antes de acceder a los píxeles
            if (i + half_height >= image->norm_height) {
                fprintf(stderr, "Combinador: Índice fuera de rango: i=%d, half_height=%d, norm_height=%d\n", i, half_height, image->norm_height);
                munmap(ptr, shm_size);
                close(shm_fd);
                return 1;
            }
            // Verificar que los punteros a las filas de píxeles estén correctamente inicializados
            if (image->pixels[i] == NULL || image->pixels[i + half_height] == NULL) {
                fprintf(stderr, "Combinador: Puntero a la fila de píxeles es NULL: i=%d, half_height=%d\n", i, half_height);
                munmap(ptr, shm_size);
                close(shm_fd);
                return 1;
            }
            // Agregar mensajes de depuración para verificar los valores de los índices y los punteros
            fprintf(stderr, "Combinador: Accediendo a píxeles: i=%d, j=%d\n", i, j);
            fprintf(stderr, "Combinador: Punteros: pixels[%d]=%p, pixels[%d]=%p\n", i, (void*)image->pixels[i], i + half_height, (void*)image->pixels[i + half_height]);

            // Copiar los píxeles de la segunda mitad a la primera mitad
            image->pixels[i][j] = image->pixels[i + half_height][j];
        }
    }

    printf("Combinador: combinando\n");

    // Guardar la imagen combinada en output_path
    printf("Combinador: guardando las imagenes\n");
    if (!writeImage(output_path, image)) {
        fprintf(stderr, "Combinador: Error al guardar la imagen\n");
        munmap(ptr, shm_size);
        close(shm_fd);
        return 1;
    }

    printf("Combinador: Imagen procesada guardada en: %s\n", output_path);

    // Desmapear la memoria compartida
    munmap(ptr, shm_size);
    close(shm_fd);

    return 0;
}