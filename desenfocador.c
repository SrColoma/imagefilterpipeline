#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <pthread.h>
#include <semaphore.h>
#include "bmp.h"

#define SHM_NAME "/image_shm"
#define SEM_READY "/sem_ready"
#define SEM_BLUR_DONE "/sem_blur_done"

void* blur_image(void* arg) {
    // Implementación del desenfoque
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Uso: %s <ruta_salida>\n", argv[0]);
        return 1;
    }

    printf("Desenfocador: inicializando\n");

    char *output_path = argv[1];

    // Mapear la memoria compartida
    int shm_fd = shm_open(SHM_NAME, O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("Desenfocador: shm_open");
        return 1;
    }

    // Obtener el tamaño de la memoria compartida
    struct stat shm_stat;
    if (fstat(shm_fd, &shm_stat) == -1) {
        perror("Desenfocador: fstat");
        close(shm_fd);
        return 1;
    }
    size_t shm_size = shm_stat.st_size;

    // Mapear la memoria compartida
    void *ptr = mmap(0, shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (ptr == MAP_FAILED) {
        perror("Desenfocador: mmap");
        close(shm_fd);
        return 1;
    }

    // Crear hilos para procesar la imagen
    int num_threads = 4; // Número de hilos
    pthread_t threads[num_threads];
    for (int i = 0; i < num_threads; i++) {
        if (pthread_create(&threads[i], NULL, blur_image, NULL) != 0) {
            perror("Desenfocador: pthread_create");
            munmap(ptr, shm_size);
            close(shm_fd);
            return 1;
        }
    }

    for (int i = 0; i < num_threads; i++) {
        if (pthread_join(threads[i], NULL) != 0) {
            perror("Desenfocador: pthread_join");
        }
    }

    printf("Desenfocador: Procesamiento completado, señalizando...\n");
    sem_t *sem_blur_done = sem_open(SEM_BLUR_DONE, 0);
    if (sem_blur_done == SEM_FAILED) {
        perror("Desenfocador: sem_open");
        munmap(ptr, shm_size);
        close(shm_fd);
        return 1;
    }
    sem_post(sem_blur_done);
    munmap(ptr, shm_size);
    close(shm_fd);
    sem_close(sem_blur_done);
    printf("Desenfocador: Tarea completada.\n");
    return 0;
}