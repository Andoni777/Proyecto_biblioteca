#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdbool.h>

int main(int argc, char* argv[]) {


	int pidHijo = fork();
	if(pidHijo != 0) {
		/* Código del padre */
		printf("[PADRE] Mi PID es %d y el valor de pidHijo es %d\n", getpid(), pidHijo);
		while(true);
		wait(NULL);
		printf("[PADRE] Termino ya que mi hijo ya ha terminado\n");
	} else {
		/* Código del hijo */
		printf("[HIJO] Mi PID es %d y el valor de pidHijo es %d\n", getpid(), pidHijo);
		printf("[HIJO] Termino.\n");
		exit(0);
	}

}
