// combinador.c (modificado)
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>

#define SHM_NAME "/image_shm"

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Uso: %s <ruta_salida>\n", argv[0]);
        return 1;
    }

    printf("Vombinador: inicializando\n");

    char *output_path = argv[1];

    // Mapear la memoria compartida
    int shm_fd = shm_open(SHM_NAME, O_RDWR, 0666);
    // Mapear y combinar las dos mitades de la imagen

    // Guardar la imagen combinada en output_path

    printf("Imagen procesada guardada en: %s\n", output_path);
    return 0;
}