#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>

void* multiplicar(void* param) {
	int valor = atoi(param);
	valor = valor * 2;
	printf("Soy el hilo que multiplica, el valor de mi variable es %d\n", valor);
	pthread_exit(0);
}

void* dividir(void* param) {
	int valor = atoi(param);
	valor = valor / 2;
	printf("Soy el hilo que divide, el valor de mi variable es %d\n", valor);
	pthread_exit(0);
}

int main(int argc, char* argv[]) {
	int variable = 10;
	char variable_str[sizeof(variable)];

	sprintf(variable_str, "%d", variable);

	pthread_t tid_1, tid_2;
	pthread_attr_t attr;

	pthread_attr_init(&attr);

	printf("Soy el hilo principal, el valor de mi variable es %d\n", variable);

	pthread_create(&tid_1, &attr, multiplicar, variable_str);
	pthread_create(&tid_2, &attr, dividir, variable_str);

	printf("Soy el hilo principal, quedo a la espera de que los hilos terminen\n");

	pthread_join(tid_1, NULL);
	pthread_join(tid_2, NULL);

}
