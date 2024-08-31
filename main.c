#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <fcntl.h>

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

    // Crear semáforos
    sem_t *sem_ready = sem_open(SEM_READY, O_CREAT, 0666, 0);
    if (sem_ready == SEM_FAILED) {
        perror("Main: sem_open");
        return 1;
    }
    sem_t *sem_blur_done = sem_open(SEM_BLUR_DONE, O_CREAT, 0666, 0);
    if (sem_blur_done == SEM_FAILED) {
        perror("Main: sem_open");
        sem_close(sem_ready);
        return 1;
    }
    sem_t *sem_enhance_done = sem_open(SEM_ENHANCE_DONE, O_CREAT, 0666, 0);
    if (sem_enhance_done == SEM_FAILED) {
        perror("Main: sem_open");
        sem_close(sem_ready);
        sem_close(sem_blur_done);
        return 1;
    }

    printf("Main: Esperando Publicador\n");

    // Lanzar el proceso publicador
    pid_t pub_pid = fork();
    if (pub_pid == 0) {
        execl("./publicador", "publicador", input_path, NULL);
        perror("Main: execl");
        return 1;
    }

    sem_wait(sem_ready);
    sem_close(sem_ready);

    printf("Main: Publicador completado, iniciando Desenfocador\n");

    pid_t pid_blur = fork();
    if (pid_blur == 0) {
        execl("./desenfocador", "desenfocador", output_path, NULL);
        perror("Main: execl");
        return 1;
    }

    sem_wait(sem_blur_done);
    sem_close(sem_blur_done);

    printf("Main: Desenfocador completado, iniciando Realzador\n");

    pid_t pid_enhance = fork();
    if (pid_enhance == 0) {
        execl("./realzador", "realzador", output_path, NULL);
        perror("Main: execl");
        return 1;
    }

    sem_wait(sem_enhance_done);
    sem_close(sem_enhance_done);

    printf("Main: Realzador completado, iniciando Combinador\n");

    pid_t pid_combine = fork();
    if (pid_combine == 0) {
        execl("./combinador", "combinador", output_path, NULL);
        perror("Main: execl");
        return 1;
    }

    // Esperar a que el proceso combinador termine
    waitpid(pid_combine, NULL, 0);

    // Aquí se combinarían las dos porciones procesadas de la imagen
    // y se guardaría la imagen final en disco
    sem_unlink(SEM_READY);
    sem_unlink(SEM_BLUR_DONE);
    sem_unlink(SEM_ENHANCE_DONE);

    printf("Main: Tarea completada.\n");
    return 0;
}