#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>

int main(int argc, char* argv[]) {
	int variable = 10;
	int pid1, pid2;
	int pidFin;
	bool fin1=false, fin2=false;

	if((pid1 = fork()) != 0) {
		/* Proceso padre */
		if((pid2 = fork()) != 0) {
			/* Proceso padre */
			printf("[PADRE] Quedo a la espera de mis hijos...\n");
			while(!fin1 || !fin2) {
				pidFin = wait(NULL);
				if (pidFin == pid1) {
					printf("[PADRE] Ha terminado hijo 1\n");
					fin1 = true;
				} else if (pidFin = pid2) {
					printf("[PADRE] Ha terminado hijo 2\n");
					fin2 = true;
				}
			}
			printf("[PADRE] Todos mis hijos han terminado, ¡adiós!\n");
		} else {
			/* Proceso hijo2 */
			float variable2 = variable / 2;
			printf("[HIJO 2] El valor de mi variable es %f\n", variable2);
			exit(0);
		}
	} else {
		/* Proceso hijo1 */
		float variable1 = variable * 2;
		printf("[HIJO 1] El valor de mi variable es %f\n", variable1);
		exit(0);
	}
}
