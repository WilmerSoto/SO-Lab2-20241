#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/wait.h>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Uso: %s <comando>\n", argv[0]);
        return 1;
    }


struct timeval inicio, final;

gettimeofday(&inicio, NULL);

pid_t pid = fork();

if (pid < 0) {
    perror("Error creando el fork");
    return 1;
} else if (pid == 0) {
    execvp(argv[1], &argv[1]);
    perror("Error ejecutando el comando");
    exit(1);
} else{
    wait(NULL);
    gettimeofday(&final, NULL);

    double segundos_final = final.tv_sec - inicio.tv_sec;
    double microsegundos_final = (final.tv_usec - inicio.tv_usec)/1000000.0;

    double tiempo_final = segundos_final + microsegundos_final;

    printf("Tiempo final: %.5f segundos\n", tiempo_final);
    }
    return 0;
}