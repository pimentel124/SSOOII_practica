/**
 * @file mi_stat.c
 * @author Álvaro Pimentel, Andreu Marqués
 * @brief Programa (comando) que muestra la información acerca del inodo de un fichero o directorio,
 * llamando a la función mi_stat() de la capa de directorios
 */
#include "directorios.h"

int main(int argc, char **argv){
	if(argc != 3){
		fprintf(stderr, COLOR_ERROR "Error Sintaxis: ./mi_stat <disco> </ruta>\n" COLOR_RESET);
		return -1;
	}
	if(bmount(argv[1])==-1){
        printf("Error (mi_stat.c) en montar el disco \n");
        return -1;
    }
	struct STAT stat;
	struct tm *ts;
	char atime[80];
	char mtime[80];
	char ctime[80];
	int ninodo = mi_stat(argv[2], &stat);
	if(ninodo >= 0){
		printf("Nº de inodo: %d\n", ninodo);
		printf("tipo: %c\n", stat.tipo);
		printf("permisos: %d\n", stat.permisos);
		ts = localtime(&stat.atime);
		strftime(atime, sizeof(atime), "%a %Y-%m-%d %H:%M:%S", ts);
		ts = localtime(&stat.ctime);
		strftime(ctime, sizeof(ctime), "%a %Y-%m-%d %H:%M:%S", ts);
		ts = localtime(&stat.mtime);
		strftime(mtime, sizeof(mtime), "%a %Y-%m-%d %H:%M:%S", ts);
		printf("atime: %s\n", atime);
		printf("ctime: %s\n", ctime);
		printf("mtime: %s\n", mtime);
		printf("nlinks: %d\n", stat.nlinks);
		printf("tamEnBytesLog: %d\n", stat.tamEnBytesLog);
		printf("numBloquesOcupados: %d\n\n", stat.numBloquesOcupados);
	}
	bumount();
	return 0;
}