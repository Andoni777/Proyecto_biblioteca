#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>

int main(int argc, char* argv[]) {
	int pidHijo;

	if (argc >= 2) {
		pidHijo = atoi(argv[1]);
	}

	printf("[HIJO] El valor de pidHijo es %d y mi PID es %d\n", pidHijo, getpid());
	printf("[HIJO] Termino.\n");
	exit(0);
}
