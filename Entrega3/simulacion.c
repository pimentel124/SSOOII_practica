/**
 * @file simulacion.c
 * @author Álvaro Pimentel, Andreu Marqués
 *
 */
#include "simulacion.h"

char dirPrueba[100];
char dir[100];
char dir2[200];
static int acabados = 0;

int main(int argc, char **argv) {
    struct tm *ts;
    char nomIniFichero[8] = "/simul_";
    char fecha[16];
    // Mostramos sintaxis correcta
    if (argc != 2) {
        fprintf(stderr, COLOR_ERROR "Error Sintaxis: ./simulacion <disco>\n" COLOR_RESET);
        return -1;
    }
    // Montamos disco
    if (bmount(argv[1]) == -1) {
        return -1;
    }
    fprintf(stderr, "*** Simulación de %d procesos realizando cada uno %d escrituras ***\n", NUMPROCESOS, NUMESCRITURAS);
    // Creamos directorio
    memset(dirPrueba, 0, sizeof(dirPrueba));
    strcpy(dirPrueba, nomIniFichero);
    memset(fecha, 0, sizeof(fecha));
    time_t t = time(NULL);
    ts = localtime(&t);
    strftime(fecha, sizeof(fecha), "%Y%m%d%H%M%S/", ts);
    strcat(dirPrueba, fecha);

    if (mi_creat(dirPrueba, '7') < 0) {
        fprintf(stderr, "El directorio '%s' no se ha podido crear\n", dirPrueba);
        bumount();
        return -1;
    } else {
        fprintf(stderr, "Directorio simulación: %s\n", dirPrueba);
    }

    // asociar la señal SIGCHLD al enterrador
    signal(SIGCHLD, reaper);
    for (int i = 1; i <= NUMPROCESOS; i++) {
        if (fork() == 0) {
            proceso(getpid(), argv[1], i);
        }
        // Esperamos 0.15 s
        usleep(150000);
    }
    // Esperamos a que acaben todos los procesos
    while (acabados < NUMPROCESOS) {
        pause();
    }
    fprintf(stderr, "Total de procesos terminados: %d.\n", acabados);
    bumount();
    exit(0);
}

void reaper() {
    pid_t ended;
    signal(SIGCHLD, reaper);
    while ((ended = waitpid(-1, NULL, WNOHANG)) > 0) {
        acabados++;
        // Mostramos los procesos que han acabado:
        // fprintf(stderr, "acabado: %d total acabados: %d\n", ended, acabados);
        // fflush(stderr);
    }
}

/* Creamos un directorio y un fichero para cada proceso,
 *  guardamos en el fichero las correspondientes escrituras.
 */
void proceso(int pid, char *disco, int numProceso) {
    char pidDirectorio[16];
    struct REGISTRO reg;
    if (bmount(disco) == -1) {
        fprintf(stderr, "Error al montar el disco para el proceso con PID %d\n", pid);
        bumount();
        exit(1);
    }
    sprintf(pidDirectorio, "proceso_%d/", pid);
    // fprintf(stderr, "proceso_%d/", pid);
    memset(dir, 0, sizeof(dir));
    strcpy(dir, dirPrueba);
    strcat(dir, pidDirectorio);
    // Creamos el directorio del proceso
    if (mi_creat(dir, '7') != 0) {
        bumount();
        exit(1);
    }
    memset(dir2, 0, sizeof(dir2));
    strcpy(dir2, dir);
    fprintf(stderr, "Proceso %d: Completadas %d escrituras en %sprueba.dat\n", numProceso, NUMESCRITURAS, dir);
    fflush(stderr);
    snprintf(dir2, sizeof(dir2), "%sprueba.dat", dir);

    // fprintf(stderr, "**DEBUG - camino completo: %s**\n",dir2);

    // Creamos pureba.dat
    if (mi_creat(dir2, '6') != 0) {
        fprintf(stderr, "simulacion.c - Error en la creación del fichero \"prueba.dat\"\n");
        bumount();
        exit(1);
    }

    // Escribimos los registros
    srand(time(NULL) + getpid());
    for (int i = 0; i < NUMESCRITURAS; i++) {
        reg.fecha = time(NULL);
        reg.pid = getpid();
        reg.nEscritura = i + 1;
        reg.nRegistro = rand() % REGMAX;
        if (mi_write(dir2, &reg, reg.nRegistro * sizeof(struct REGISTRO), sizeof(struct REGISTRO)) < 0) {
            fprintf(stderr, "Fallo en la escritura en %s\n", dir2);
            exit(1);
        }
        usleep(50000);
    }
    bumount();
    exit(0);
}