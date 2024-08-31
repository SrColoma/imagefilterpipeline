#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <pthread.h>
#include <semaphore.h>
#include "bmp.h"

#define SHM_NAME "/image_shm"
#define SEM_READY "/sem_ready"
#define SEM_ENHANCE_DONE "/sem_enhance_done"

void* enhance_image(void* arg) {
    // Implementación del realce
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Uso: %s <ruta_salida>\n", argv[0]);
        return 1;
    }

    printf("Realzador: inicializando\n");

    char *output_path = argv[1];

    // Mapear la memoria compartida
    int shm_fd = shm_open(SHM_NAME, O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("Realzador: shm_open");
        return 1;
    }

    // Obtener el tamaño de la memoria compartida
    struct stat shm_stat;
    if (fstat(shm_fd, &shm_stat) == -1) {
        perror("Realzador: fstat");
        pclose(shm_fd);
        return 1;
    }
    size_t shm_size = shm_stat.st_size;

    // Mapear la memoria compartida
    void *ptr = mmap(0, shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (ptr == MAP_FAILED) {
        perror("Realzador: mmap");
        close(shm_fd);
        return 1;
    }

    // Crear hilos para procesar la imagen
    int num_threads = 4; // Número de hilos
    pthread_t threads[num_threads];
    for (int i = 0; i < num_threads; i++) {
        if (pthread_create(&threads[i], NULL, enhance_image, NULL) != 0) {
            perror("Realzador: pthread_create");
            munmap(ptr, shm_size);
            close(shm_fd);
            return 1;
        }
    }

    for (int i = 0; i < num_threads; i++) {
        if (pthread_join(threads[i], NULL) != 0) {
            perror("Realzador: pthread_join");
        }
    }

    printf("Realzador: Procesamiento completado, señalizando...\n");
    sem_t *sem_enhance_done = sem_open(SEM_ENHANCE_DONE, 0);
    if (sem_enhance_done == SEM_FAILED) {
        perror("Realzador: sem_open");
        munmap(ptr, shm_size);
        close(shm_fd);
        return 1;
    }
    sem_post(sem_enhance_done);
    munmap(ptr, shm_size);
    close(shm_fd);
    sem_close(sem_enhance_done);
    printf("Realzador: Tarea completada.\n");
    return 0;
}