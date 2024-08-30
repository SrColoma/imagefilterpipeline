#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <pthread.h>
#include <unistd.h>
#include "bmp.h"

#define SHM_NAME "/image_shm"
#define SEM_READY "/sem_ready"
#define SEM_BLUR_DONE "/sem_blur_done"

typedef struct {
    BMP_Image *image;
    Pixel *pixels;
    int start_row;
    int end_row;
} ThreadArgs;

void apply_blur(BMP_Image *image, Pixel *pixels, int row, int col) {
    int kernel[3][3] = {{1, 1, 1}, {1, 1, 1}, {1, 1, 1}};
    int kernel_sum = 9;
    int red_sum = 0, green_sum = 0, blue_sum = 0;

    for (int i = -1; i <= 1; i++) {
        for (int j = -1; j <= 1; j++) {
            int cur_row = row + i;
            int cur_col = col + j;
            if (cur_row >= 0 && cur_row < image->norm_height && cur_col >= 0 && cur_col < image->header.width_px) {
                Pixel pixel = pixels[cur_row * image->header.width_px + cur_col];
                red_sum += pixel.red * kernel[i+1][j+1];
                green_sum += pixel.green * kernel[i+1][j+1];
                blue_sum += pixel.blue * kernel[i+1][j+1];
            }
        }
    }

    Pixel *target_pixel = &pixels[row * image->header.width_px + col];
    target_pixel->red = red_sum / kernel_sum;
    target_pixel->green = green_sum / kernel_sum;
    target_pixel->blue = blue_sum / kernel_sum;
}

void *blur_thread(void *arg) {
    ThreadArgs *args = (ThreadArgs *)arg;
    for (int row = args->start_row; row < args->end_row; row++) {
        for (int col = 0; col < args->image->header.width_px / 2; col++) {
            apply_blur(args->image, args->pixels, row, col);
        }
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Uso: %s <num_hilos>\n", argv[0]);
        return 1;
    }

    printf("Desenfocador: inicializando\n");

    int num_threads = atoi(argv[1]);

    printf("Desenfocador: Esperando señal del publicador...\n");
    sem_t *sem_ready = sem_open(SEM_READY, 0);
    if (sem_ready == SEM_FAILED) {
        perror("sem_open");
        return 1;
    }
    sem_wait(sem_ready);
    printf("Desenfocador: Señal recibida, procesando imagen...\n");

    int shm_fd = shm_open(SHM_NAME, O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open");
        sem_close(sem_ready);
        return 1;
    }

    struct stat sb;
    if (fstat(shm_fd, &sb) == -1) {
        perror("fstat");
        close(shm_fd);
        sem_close(sem_ready);
        return 1;
    }
    size_t shm_size = sb.st_size;

    void *ptr = mmap(NULL, shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (ptr == MAP_FAILED) {
        perror("mmap");
        close(shm_fd);
        sem_close(sem_ready);
        return 1;
    }

    BMP_Image *image = (BMP_Image *)ptr;
    Pixel *pixels = (Pixel *)((char *)ptr + sizeof(BMP_Image));

    pthread_t threads[num_threads];
    ThreadArgs thread_args[num_threads];
    int rows_per_thread = image->norm_height / num_threads;

    for (int i = 0; i < num_threads; i++) {
        thread_args[i].image = image;
        thread_args[i].pixels = pixels;
        thread_args[i].start_row = i * rows_per_thread;
        thread_args[i].end_row = (i == num_threads - 1) ? image->norm_height : (i + 1) * rows_per_thread;
        if (pthread_create(&threads[i], NULL, blur_thread, &thread_args[i]) != 0) {
            perror("pthread_create");
            munmap(ptr, shm_size);
            close(shm_fd);
            sem_close(sem_ready);
            return 1;
        }
    }

    for (int i = 0; i < num_threads; i++) {
        if (pthread_join(threads[i], NULL) != 0) {
            perror("pthread_join");
        }
    }

    printf("Desenfocador: Procesamiento completado, señalizando...\n");
    sem_t *sem_blur_done = sem_open(SEM_BLUR_DONE, 0);
    if (sem_blur_done == SEM_FAILED) {
        perror("sem_open");
        munmap(ptr, shm_size);
        close(shm_fd);
        sem_close(sem_ready);
        return 1;
    }
    sem_post(sem_blur_done);

    munmap(ptr, shm_size);
    close(shm_fd);
    sem_close(sem_ready);
    sem_close(sem_blur_done);

    printf("Desenfocador: Tarea completada.\n");
    return 0;
}