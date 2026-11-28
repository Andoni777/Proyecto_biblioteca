#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <mqueue.h>
#include <string.h>
#include <sys/shm.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <time.h>
#include <signal.h>

#define SHM_NAME "/shm_biblioteca"
#define MQ_NAME "/cola_solicitudes"
#define SEM1_NAME "/semaforo_1"
#define SEM2_NAME "/semaforo_2"

#define MAX_LIBROS 50 
#define SHM_SIZE (sizeof(int) * MAX_LIBROS)

// Creacion de semaforos
sem_t *sem1;
sem_t *sem2;
//
//Declaracion de la cola
mqd_t cola;
//
pid_t pid_mostrador, pid_deposito;

// Variables globales para el control de señales
volatile sig_atomic_t finalizar = 0;
volatile sig_atomic_t ejemplar_listo = 0;

// Señal de ejemplar listo
void tratamiento_sigusr1(int signum) {
    ejemplar_listo = 1;
}

// Para SIGINT (Ctrl + C)
void tratamiento_sigint(int signum) {
    signal(SIGINT, tratamiento_sigint);
    finalizar = 1;
}

int tiempo_aleatorio(int min, int max) {
    return rand() % (max - min + 1) + min;
}

void* busqueda(void* arg) {
    int num_solicitud = *(int *)arg;

    printf("[Hilo Búsqueda] Comienzo mi ejecucion...\n");

    printf("[Hilo Búsqueda] Buscando ejemplar nº%d...\n", num_solicitud);
    sleep(tiempo_aleatorio(1, 3));
    printf("[Hilo Busqueda] Ejemplar nº%d localizado. Enviando a extracción...\n", num_solicitud);
    
    sem_post(sem1); // Avisar al hilo de extracción

    return NULL;
}

void* extraccion(void* arg) {

    int num_solicitud = *(int *)arg;

    printf("[Hilo Extracción] Comienzo mi ejecución...\n");
    sem_wait(sem1); // Esperar a que búsqueda termine
    
    printf("[Hilo Extracción] Extrayendo ejemplar nº%d de la estantería...\n", num_solicitud);
    sleep(tiempo_aleatorio(2, 4));
    printf("[Hilo Extracción] Ejemplar nº%d extraído. Enviando a registro...\n", num_solicitud);
    
    sem_post(sem2); // Avisar al hilo de registro

    return NULL;
}

void* registro(void* arg) {
    
    int num_solicitud = *(int *)arg;

    printf("[Hilo Registro] Comienzo mi ejecución...\n");
    sem_wait(sem2); // Esperar a que extracción termine
    
    printf("[Hilo Registro] Registrando préstamo del ejemplar nº%d...\n", num_solicitud);
    sleep(tiempo_aleatorio(1, 2));
    printf("[Hilo Registro] Préstamo del ejemplar nº%d registrado\n", num_solicitud);

    return NULL;
}

int main(int argc, char* argv[]) {
    // Registrar captura de SIGINT antes del fork para que la hereden los hijos
    signal(SIGINT, tratamiento_sigint);
    // Apertura de semaforos
    sem1 = sem_open(SEM1_NAME, O_CREAT, 0644, 0);
    if (sem1 == SEM_FAILED) {
        perror("Error al crear sem1");
        exit(EXIT_FAILURE);
    }

    sem2 = sem_open(SEM2_NAME, O_CREAT, 0644, 0);
    if (sem2 == SEM_FAILED) {
        perror("Error al crear sem2");
        exit(EXIT_FAILURE);
    }
    //

    //Creacion de la cola
    struct mq_attr atributos;

    atributos.mq_flags = 0;
    atributos.mq_maxmsg = 10;
    atributos.mq_msgsize = sizeof(int);
    atributos.mq_curmsgs = 0;

    cola = mq_open(MQ_NAME, O_CREAT | O_RDWR, 0644, &atributos);

    if (cola == (mqd_t)-1) {
        perror("Error al crear la cola");
        exit(EXIT_FAILURE);
    }
    //

    
    //creación de memoria compartida
    int fd_shm = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0644);

    if (fd_shm == -1) {
        perror("Error al crear la memoria compartida");
        exit(EXIT_FAILURE);
    }

    if (ftruncate(fd_shm, SHM_SIZE) == -1) {
        perror("Error al truncar la memoria compartida");
        exit(EXIT_FAILURE);
    }
    int *shm_ejemplar = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd_shm, 0);

    if (shm_ejemplar == MAP_FAILED) {
        perror("Error al mapear la memoria compartida");
        exit(EXIT_FAILURE);
    } else {
        close(fd_shm);
    }

    // Creación de procesos hijos
    pid_mostrador = fork();

    if (pid_mostrador != 0) {
        pid_deposito = fork();
        if (pid_deposito != 0) {
            /* Proceso padre */

            printf("[Proceso Padre] Esperando a que terminen mis hijos...\n");
            
            // Si waitpid se interrumpe al llegar SIGINT, vuelve a esperar a que termine cada hijo
            while (waitpid(pid_mostrador, NULL, 0) == -1) {
                if(!finalizar){
                    perror("Error esperando al proceso Mostrador");
                    break;
                }
            }
            while (waitpid(pid_deposito, NULL, 0) == -1) {
                if(!finalizar){
                    perror("Error esperando al proceso Deposito");
                    break;
                }
            }

            printf("[Proceso Padre] Ejecución finalizada.\n");

            
            //Después de que ambos hijos hayan terminado, el padre debe llamar a munmap 
            //para quitar su propio mapeo y a shm_unlink para eliminar el nombre del objeto 
            //compartido.
            munmap(shm_ejemplar, SHM_SIZE);
            shm_unlink(SHM_NAME);

            // Cerrar y eliminar semáforos
            sem_close(sem1);
            sem_close(sem2);
            sem_unlink(SEM1_NAME);
            sem_unlink(SEM2_NAME);

            // Cerrar y eliminar cola de mensajes
            mq_close(cola);
            mq_unlink(MQ_NAME);

        } else {
            /* Proceso Depósito */
            printf("[Proceso Depósito] Comienzo mi ejecución...\n");
            
            while(!finalizar) {
                int num_solicitud;
                // Esperar una solicitud del mostrador y comprobar interrupciones
                if (mq_receive(cola, (char *)&num_solicitud, sizeof(num_solicitud), NULL) == -1) {
                    if (finalizar) break;
                    perror("Error en mq_receive");
                    continue;
                }

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

                // Escribir en memoria compartida 
                shm_ejemplar[num_solicitud % MAX_LIBROS] = num_solicitud;

                printf("[Proceso Depósito] Escrito en memoria compartida: %d\n", num_solicitud);
                kill(pid_mostrador, SIGUSR1);

                printf("[Proceso Depósito] Solicitud nº %d finalizada.\n", num_solicitud);
            }
            // Limpieza local del hijo antes de salir
            munmap(shm_ejemplar, SHM_SIZE);
            sem_close(sem1);
            sem_close(sem2);
            mq_close(cola);
        }
    } else {
        /* Proceso Mostrador */
        
        signal(SIGUSR1, tratamiento_sigusr1);   //Capturamos señal
        printf("[Proceso Mostrador] Comienzo mi ejecución...\n");
        int num_solicitud = 0;

        while (!finalizar) {
            
            sleep(tiempo_aleatorio(3, 8));
            printf("[Proceso Mostrador] Recibida solicitud de préstamo nº %d. Enviando al depósito...\n", num_solicitud);
            mq_send(cola, (char *)&num_solicitud,
            sizeof(num_solicitud), 0);

            while (!ejemplar_listo && !finalizar) {
                pause();
            }

            if (finalizar){
                break;
            }

            ejemplar_listo = 0;

            // Esperar a que el depósito le notifique que la solicitud ha sido procesada

            printf("[Proceso Mostrador] Solicitud nº %d procesada. Ejemplar listo para entrega.\n", shm_ejemplar[num_solicitud % MAX_LIBROS]);

            
            num_solicitud++;

        }
        
        munmap(shm_ejemplar, SHM_SIZE);
        sem_close(sem1);
        sem_close(sem2);
        mq_close(cola);
    }

    exit(0);
}
