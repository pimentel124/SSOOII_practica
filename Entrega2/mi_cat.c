#include "directorios.h"

int main(int argc,char *argv[]){
	
	if(argc != 3){
		printf("mi_cat, 6: Error en los argumentos.\n");
		printf("El uso del programa es:\n \t.");
		printf("/mi_cat [nombre del dispositivo] [camino]\n");
		exit(0);
	}
	
	if(bmount(argv[1]) == -1){
		printf("mi_cat, 13: Error al abrir el dispositivo: %s.\n", argv[1]);
		return -1;
	}
	
	unsigned int p_inodo_dir = 0;
	unsigned int p_inodo = 0;
	unsigned int p_entrada = 0;

	switch(buscar_entrada(argv[2], &p_inodo_dir, &p_inodo, &p_entrada, 0, 0)){
		case -1: printf("mi_cat, 22: Error en Extraer Camino.\n"); break; 
		case -2: printf("mi_cat, 23: Error en Leer Entrada.\n"); break; 
		case -3: printf("mi_cat, 24: Error: No existe la Entrada.\n"); break; 
		case -4: printf("mi_cat, 25: Error al escribir Entrada.\n"); break; 
		case -5: printf("mi_cat, 26: Error: Entrada ya existente.\n"); break;
		case -6: printf("mi_cat, 27: Error al encontrar el Directorio Intermedio.\n"); break; 
	}
	
	struct STAT p_stat;
	if(mi_stat_f(p_inodo, &p_stat) == -1){
		printf("mi_cat, 32: Error al obtener un STAT.\n");
		return -1;      
	}
	
	if(p_stat.tipo != 'f'){
	   printf("mi_cat, 37: El camino tiene que referenciar un fichero.\n");
	   return -1;
	}

	/* El que feim és un recorregut fins al final del fitxer i
	 * imprimim per pantalla tot el contingut, menys la brossa. */
	int offset = 0;
	int bytesLeidos = 0;
	unsigned char buffer[BLOCKSIZE];
	memset(buffer, 0, BLOCKSIZE);

	while(offset <= p_stat.tamEnBytesLog){
		bytesLeidos = mi_read_f(p_inodo, buffer, offset, BLOCKSIZE);
		write(1, buffer, bytesLeidos);
		offset += BLOCKSIZE;
		memset(buffer, 0, BLOCKSIZE);
	}
	printf("\n");
	bumount();
}