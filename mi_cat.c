#include "directorios.h"


int main (int argc, char **argv) {
	
	if(argv[1] == NULL || argv[2] == NULL)
	{
		fprintf(stderr, "Sintaxis: ./mi_cat <disco> </ruta_fichero>\n");
		return -1;
	}
	
    int nbytes;
	unsigned int bytes_leidos = 0;
	const unsigned int BUFFER_SIZE = 1024;
	unsigned char buf[BUFFER_SIZE];
	unsigned int offset = 0;
	struct STAT stat;
	bmount(argv[1]);
	
	if(argv[2][strlen(argv[2])-1]=='/'){ 
		fprintf(stderr, "Se ha introducido un directorio, no un fichero\n");
		return -1;
    }
	
	memset(buf, 0, BUFFER_SIZE);
	nbytes = mi_read(argv[2], buf, offset, BUFFER_SIZE);
    
	while(nbytes > 0)
	{
        bytes_leidos += nbytes;
		write(1, buf, nbytes);
		offset += nbytes;
		memset(buf, 0, BUFFER_SIZE);
		nbytes = mi_read(argv[2], buf, offset, BUFFER_SIZE);
	}
	if(nbytes < 0)
	{
		fprintf(stderr,"(mi_cat) Error al leer\n");
	}
    mi_stat(argv[2],&stat);
	fprintf(stderr, "\n\ntotal_leidos: %d\n", bytes_leidos);
	bumount(argv[1]);
	return 0;
}