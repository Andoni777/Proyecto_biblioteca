#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <mqueue.h>
#include <string.h>
#include <sys/shm.h>
#include <sys/mman.h>

#define SHM_NAME "/shm_biblioteca"

int *shm_ejemplar;
int fd_shm;

// Creacion de semaforos
sem_t *sem1;
sem_t *sem2;
sem_t *sem3;
//
//Declaracion de la cola
mqd_t cola;
//
pid_t pid_mostrador, pid_deposito;

int tiempo_aleatorio(int min, int max) {
    return rand() % (max - min + 1) + min;
}

void* busqueda(void* arg) {
    int num_solicitud = *(int *)arg;

    printf("[Hilo Búsqueda] Comienzo mi ejecucion...\n");

    printf("[Hilo Búsqueda] Buscando ejemplar nº%d...\n", num_solicitud);
    sleep(tiempo_aleatorio(1, 3));
    printf("[Hilo Busqueda] Ejemplar nº%d localizado. Enviando a extracción...\n", num_solicitud);
    sem_post(sem1);

    return NULL;
}

void* extraccion(void* arg) {
    sem_wait(sem1);
    int num_solicitud = *(int *)arg;

    printf("[Hilo Extracción] Comienzo mi ejecución...\n");

    printf("[Hilo Extracción] Extrayendo ejemplar nº%d de la estantería...\n", num_solicitud);
    sleep(tiempo_aleatorio(2, 4));
    printf("[Hilo Extracción] Ejemplar nº%d extraído. Enviando a registro...\n", num_solicitud);
    sem_post(sem2);

    return NULL;
}

void* registro(void* arg) {
    sem_wait(sem2);
    int num_solicitud = *(int *)arg;

    printf("[Hilo Registro] Comienzo mi ejecución...\n");

    printf("[Hilo Registro] Registrando préstamo del ejemplar nº%d...\n", num_solicitud);
    sleep(tiempo_aleatorio(1, 2));
    printf("[Hilo Registro] Préstamo del ejemplar nº%d registrado. Notificando al mostrador...\n", num_solicitud);
    sem_post(sem3);

    return NULL;
}

int main(int argc, char* argv[]) {
    // Apertura de semaforos
    sem1 = sem_open("/semaforo_1", O_CREAT, 0644, 0);
    if (sem1 == SEM_FAILED) {
        perror("Error al crear sem1");
        exit(EXIT_FAILURE);
    }

    sem2 = sem_open("/semaforo_2", O_CREAT, 0644, 0);
    if (sem2 == SEM_FAILED) {
        perror("Error al crear sem2");
        exit(EXIT_FAILURE);
    }

    sem3 = sem_open("/semaforo_3", O_CREAT, 0644, 0);
    if (sem3 == SEM_FAILED) {
        perror("Error al crear sem3");
        exit(EXIT_FAILURE);
    }
    //

    //Creacion de la cola
    struct mq_attr atributos;

    atributos.mq_flags = 0;
    atributos.mq_maxmsg = 10;
    atributos.mq_msgsize = sizeof(int);
    atributos.mq_curmsgs = 0;

    cola = mq_open("/cola_solicitudes", O_CREAT | O_RDWR, 0644, &atributos);

    if (cola == (mqd_t)-1) {
        perror("Error al crear la cola");
        exit(EXIT_FAILURE);
    }
    //

    
    //creación de memoria compartida
    fd_shm = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0644);
    ftruncate(fd_shm, sizeof(int));
    shm_ejemplar = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, fd_shm, 0);

    sigset_t set_urs1;
    sigemptyset(&set_urs1);
    sigaddset(&set_urs1, SIGUSR1);
    sigprocmask(SIG_BLOCK, &set_urs1, NULL);

    pid_mostrador = fork();

    if (pid_mostrador != 0) {
        pid_deposito = fork();
        if (pid_deposito != 0) {
            /* Proceso padre */

            printf("[Proceso Padre] Esperando a que terminen mis hijos...\n");


            printf("[Proceso Padre] Ejecución finalizada.\n");
        } else {
            /* Proceso Depósito */
            printf("[Proceso Depósito] Comienzo mi ejecución...\n");
            
            while(1) {
                int num_solicitud;
                
                // Esperar una solicitud del mostrador
                mq_receive(cola, (char *)&num_solicitud, sizeof(num_solicitud), NULL);

                printf("[Proceso Depósito] Recibida solicitud nº %d\n", num_solicitud);

                pthread_t hilo_busqueda;
                pthread_t hilo_extraccion;
                pthread_t hilo_registro;

                // Pasar el numero a los tres hilos
                pthread_create(&hilo_busqueda, NULL, busqueda, &num_solicitud);
                pthread_create(&hilo_extraccion, NULL, extraccion, &num_solicitud);
                pthread_create(&hilo_registro, NULL, registro, &num_solicitud);

                // Esperar a que terminen
                pthread_join(hilo_busqueda, NULL);
                pthread_join(hilo_extraccion, NULL);
                pthread_join(hilo_registro, NULL);

                *shm_ejemplar = num_solicitud; //escribe el ID en la memoria compartida
                kill(pid_mostrador, SIGUSR1);

                printf("[Proceso Depósito] Solicitud nº %d finalizada.\n", num_solicitud);
            }
        }
    } else {
        /* Proceso Mostrador */
        printf("[Proceso Mostrador] Comienzo mi ejecución...\n");
        int num_solicitud = 0;

        while (1) {
            
            sleep(tiempo_aleatorio(3, 8));
            printf("[Proceso Mostrador] Recibida solicitud de préstamo nº %d. Enviando al depósito...\n", num_solicitud);
            mq_send(cola, (char *)&num_solicitud,
            sizeof(num_solicitud), 0);

            // Esperar a que el depósito le notifique que la solicitud ha sido procesada

            int sig;
            sigwait(&set_urs1, &sig);   // Espera a que el depósito le notifique que la solicitud ha sido procesada
            printf("[Proceso Mostrador] Solicitud nº %d procesada. Ejemplar listo para entrega.\n", num_solicitud);

            sem_wait(sem3);

            num_solicitud++;

        }
    }

    exit(0);
}