#include "directorios.h"


int main (int argc, char *argv[]) {
	if(argc != 5)
	{
		printf("Sintaxis: ./mi_escribir <disco> </ruta_fichero> <texto> <offset>\n");
		return -1;
	}
    if(bmount(argv[1])<0) return -1;
	int escritos = mi_write(argv[2], argv[3], atoi(argv[4]), strlen(argv[3]));
	if(escritos < 0)
	{
		if(escritos == ERROR_PERMISO_ESCRITURA)
		{
			printf("No hay permisos de escritura\n");
		} 
		else 
		{
        	fprintf(stderr,"(mi_escribir.c)Error al escribir los bytes\n");
		}
		escritos = 0;
	}
	printf("Longitud del texto: %lu \nBytes escritos: %d\n", strlen(argv[3]), escritos);
    bumount();
    return 0;
}