#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main(int argc, char* argv[]) {


	int pidHijo = fork();
	if(pidHijo != 0) {
		/* Código del padre */
		printf("[PADRE] Mi PID es %d y el valor de pidHijo es %d\n", getpid(), pidHijo);
		wait(NULL);
		printf("[PADRE] Termino ya que mi hijo ya ha terminado\n");
	} else {
		/* Código del hijo */
		printf("[HIJO] Mi PID es %d y el valor de pidHijo es %d\n", getpid(), pidHijo);
		printf("[HIJO] Termino.\n");
		/* Alternativa: ejecutar el hijo en un programa aparte 
			// Cambiar la ruta a tu programa
			char str[50];
			sprintf(str, "%d", pidHijo);
			execlp("/home/debian/ejercicios-SO/S3/EJ1/hijo", "hijo", str, NULL);
		*/
	}
	exit(0);
}
