// main.c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <semaphore.h>

#define SHM_NAME "/image_shm"
#define SEM_READY "/sem_ready"
#define SEM_BLUR_DONE "/sem_blur_done"
#define SEM_EDGE_DONE "/sem_edge_done"

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Uso: %s <ruta_entrada> <ruta_salida> <num_hilos>\n", argv[0]);
        return 1;
    }

    char *input_path = argv[1];
    char *output_path = argv[2];
    char *num_threads = argv[3];

    // Crear memoria compartida y semáforos
    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    // Ajustar tamaño de la memoria compartida según el tamaño de la imagen

    sem_t *sem_ready = sem_open(SEM_READY, O_CREAT, 0666, 0);
    sem_t *sem_blur_done = sem_open(SEM_BLUR_DONE, O_CREAT, 0666, 0);
    sem_t *sem_edge_done = sem_open(SEM_EDGE_DONE, O_CREAT, 0666, 0);

    // Lanzar Publicador
    pid_t pub_pid = fork();
    if (pub_pid == 0) {
        execlp("./publicador", "publicador", input_path, NULL);
        perror("Error al ejecutar publicador");
        exit(1);
    }

    // Esperar a que el Publicador cargue la imagen
    sem_wait(sem_ready);
    printf("Main: Esperando Publicador\n");

    // Lanzar Desenfocador
    pid_t blur_pid = fork();
    if (blur_pid == 0) {
        execlp("./desenfocador", "desenfocador", num_threads, NULL);
        perror("Error al ejecutar desenfocador");
        exit(1);
    }

    // Lanzar Realzador
    pid_t edge_pid = fork();
    if (edge_pid == 0) {
        execlp("./realzador", "realzador", NULL);
        perror("Error al ejecutar realzador");
        exit(1);
    }

    // Esperar a que terminen Desenfocador y Realzador
    sem_wait(sem_blur_done);
    printf("Main: Esperando Desenfocador\n");
    sem_wait(sem_edge_done);
    printf("Main: Esperando Realzador\n");

    // Lanzar Combinador
    pid_t comb_pid = fork();
    if (comb_pid == 0) {
        execlp("./combinador", "combinador", output_path, NULL);
        perror("Error al ejecutar combinador");
        exit(1);
    }

    // Esperar a que todos los procesos hijos terminen
    waitpid(pub_pid, NULL, 0);
    waitpid(blur_pid, NULL, 0);
    waitpid(edge_pid, NULL, 0);
    waitpid(comb_pid, NULL, 0);

    // Limpiar recursos
    sem_close(sem_ready);
    sem_close(sem_blur_done);
    sem_close(sem_edge_done);
    sem_unlink(SEM_READY);
    sem_unlink(SEM_BLUR_DONE);
    sem_unlink(SEM_EDGE_DONE);
    shm_unlink(SHM_NAME);

    printf("Main: Procesamiento de imagen completado.\n");
    return 0;
}