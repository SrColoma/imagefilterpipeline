#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <sys/stat.h>

#define SHM_NAME "/image_shm"
#define SEM_READY "/sem_ready"
#define SEM_BLUR_DONE "/sem_blur_done"
#define SEM_ENHANCE_DONE "/sem_enhance_done"

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Uso: %s <ruta_entrada> <ruta_salida>\n", argv[0]);
        return 1;
    }

    char *input_path = argv[1];
    char *output_path = argv[2];

    // Crear memoria compartida y semáforos
    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("Main: shm_open");
        return 1;
    }

    // Ajustar tamaño de la memoria compartida según el tamaño de la imagen
    // Aquí asumimos un tamaño máximo para la memoria compartida
    size_t shm_size = 10 * 1024 * 1024; // 10 MB
    if (ftruncate(shm_fd, shm_size) == -1) {
        perror("Main: ftruncate");
        close(shm_fd);
        return 1;
    }

    // Mapear la memoria compartida
    void *shm_ptr = mmap(0, shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shm_ptr == MAP_FAILED) {
        perror("Main: mmap");
        close(shm_fd);
        return 1;
    }

    // Inicializar la memoria compartida si es necesario
    memset(shm_ptr, 0, shm_size);

    sem_t *sem_ready = sem_open(SEM_READY, O_CREAT, 0666, 0);
    if (sem_ready == SEM_FAILED) {
        perror("Main: sem_open SEM_READY");
        munmap(shm_ptr, shm_size);
        close(shm_fd);
        return 1;
    }
    sem_t *sem_blur_done = sem_open(SEM_BLUR_DONE, O_CREAT, 0666, 0);
    if (sem_blur_done == SEM_FAILED) {
        perror("Main: sem_open SEM_BLUR_DONE");
        sem_close(sem_ready);
        munmap(shm_ptr, shm_size);
        close(shm_fd);
        return 1;
    }
    sem_t *sem_enhance_done = sem_open(SEM_ENHANCE_DONE, O_CREAT, 0666, 0);
    if (sem_enhance_done == SEM_FAILED) {
        perror("Main: sem_open SEM_ENHANCE_DONE");
        sem_close(sem_ready);
        sem_close(sem_blur_done);
        munmap(shm_ptr, shm_size);
        close(shm_fd);
        return 1;
    }

    printf("Main: Esperando Publicador\n");

    pid_t pub_pid = fork();
    if (pub_pid == 0) {
        execl("./publicador", "publicador", input_path, NULL);
        perror("execl");
        return 1;
    }

    sem_wait(sem_ready);
    sem_close(sem_ready);

    printf("Main: Publicador completado, iniciando Desenfocador\n");

    pid_t blur_pid = fork();
    if (blur_pid == 0) {
        execl("./desenfocador", "desenfocador", output_path, NULL);
        perror("execl");
        return 1;
    }

    pid_t enhance_pid = fork();
    if (enhance_pid == 0) {
        execl("./realzador", "realzador", output_path, NULL);
        perror("execl");
        return 1;
    }

    sem_wait(sem_blur_done);
    sem_close(sem_blur_done);

    sem_wait(sem_enhance_done);
    sem_close(sem_enhance_done);

    printf("Main: Desenfocador y Realzador completados, combinando imagen\n");

    // Llamar al combinador para combinar las dos porciones procesadas de la imagen
    pid_t combine_pid = fork();
    if (combine_pid == 0) {
        execl("./combinador", "combinador", output_path, NULL);
        perror("execl");
        return 1;
    }

    // Esperar a que el proceso combinador termine
    int status;
    waitpid(combine_pid, &status, 0);
    if (WIFEXITED(status)) {
        printf("Main: Combinador terminó con estado %d\n", WEXITSTATUS(status));
    } else {
        printf("Main: Combinador terminó anormalmente\n");
    }

    printf("Main: Tarea completada.\n");

    // Limpiar recursos
    sem_unlink(SEM_READY);
    sem_unlink(SEM_BLUR_DONE);
    sem_unlink(SEM_ENHANCE_DONE);
    munmap(shm_ptr, shm_size);
    close(shm_fd);
    shm_unlink(SHM_NAME);

    return 0;
}