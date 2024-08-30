#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <unistd.h>
#include "bmp.h"

#define SHM_NAME "/image_shm"
#define SEM_READY "/sem_ready"
#define SEM_EDGE_DONE "/sem_edge_done"

void apply_edge_detection(BMP_Image *image, Pixel *pixels) {
    int kernel[3][3] = {{-1, -1, -1}, {-1, 8, -1}, {-1, -1, -1}};
    Pixel *temp_pixels = malloc(image->norm_height * image->header.width_px * sizeof(Pixel));
    
    for (int row = 1; row < image->norm_height - 1; row++) {
        for (int col = image->header.width_px / 2; col < image->header.width_px - 1; col++) {
            int red_sum = 0, green_sum = 0, blue_sum = 0;
            
            for (int i = -1; i <= 1; i++) {
                for (int j = -1; j <= 1; j++) {
                    Pixel pixel = pixels[(row + i) * image->header.width_px + (col + j)];
                    red_sum += pixel.red * kernel[i+1][j+1];
                    green_sum += pixel.green * kernel[i+1][j+1];
                    blue_sum += pixel.blue * kernel[i+1][j+1];
                }
            }
            
            temp_pixels[row * image->header.width_px + col].red = (uint8_t)(abs(red_sum) > 255 ? 255 : abs(red_sum));
            temp_pixels[row * image->header.width_px + col].green = (uint8_t)(abs(green_sum) > 255 ? 255 : abs(green_sum));
            temp_pixels[row * image->header.width_px + col].blue = (uint8_t)(abs(blue_sum) > 255 ? 255 : abs(blue_sum));
        }
    }
    
    // Copy the processed pixels back to the shared memory
    for (int row = 1; row < image->norm_height - 1; row++) {
        for (int col = image->header.width_px / 2; col < image->header.width_px - 1; col++) {
            pixels[row * image->header.width_px + col] = temp_pixels[row * image->header.width_px + col];
        }
    }
    
    free(temp_pixels);
}

int main() {
    printf("Realzador: Esperando señal del publicador...\n");
    sem_t *sem_ready = sem_open(SEM_READY, 0);
    if (sem_ready == SEM_FAILED) {
        perror("sem_open");
        return 1;
    }
    sem_wait(sem_ready);
    printf("Realzador: Señal recibida, procesando imagen...\n");

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

    apply_edge_detection(image, pixels);

    printf("Realzador: Procesamiento completado, señalizando...\n");
    sem_t *sem_edge_done = sem_open(SEM_EDGE_DONE, 0);
    if (sem_edge_done == SEM_FAILED) {
        perror("sem_open");
        munmap(ptr, shm_size);
        close(shm_fd);
        sem_close(sem_ready);
        return 1;
    }
    sem_post(sem_edge_done);

    munmap(ptr, shm_size);
    close(shm_fd);
    sem_close(sem_ready);
    sem_close(sem_edge_done);

    printf("Realzador: Tarea completada.\n");
    return 0;
}