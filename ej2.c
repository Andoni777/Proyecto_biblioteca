#include <unistd.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdbool.h>

int main(int argc, char* argv[]) {
	int pid;

	if((pid = fork()) != 0) {
		/* Código del padre */
		printf("[PADRE - PID: %d] Espero a que mi hijo termine.\n", getpid());
		wait(NULL);
		printf("[PADRE] Termino ya que mi hijo ha terminado.\n");
	} else {
		/* Código del hijo */
		printf("[HIJO - PID: %d] Me quedo atascado en un bucle infinito...\n", getpid());
		while(true);
	}

}
