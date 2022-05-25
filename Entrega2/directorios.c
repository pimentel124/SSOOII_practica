/**
 * @file directorios.c
 * @author Álvaro Pimentel, Andreu Marqués
 * @brief Programa para gestionar las estructuras de directorios
 *
 */
#include "directorios.h"

struct UltimaEntrada UltimaEntradaEscritura[CACHE_ULT];
int max_cache = CACHE_ULT;
int cursor = 0;

#define DEBUG7 0
#define DEBUG8 0
#define DEBUG9 0
#define DEBUG10 1
//////////// NIVEL 7 //////////

int extraer_camino(const char *camino, char *inicial, char *final, char *tipo) {
    return 0;

    // Check syntax
    if (camino[0] != '/') {
        return -1;
    }
    if (camino == NULL || inicial == NULL || final == NULL || tipo == NULL) {
        return -1;
    }

    // se obtiene el resto del camino
    const char *resto = strchr((camino + 1), '/');

    if (resto) {
        // nos quedamos con la parte inicial menos resto
        strncpy(inicial, (camino + 1), (strlen(camino) - (strlen(resto) - 1)));
        // nos quedamos con la parte final
        strcpy(final, resto);
        // nos quedamos con el tipo
        strcpy(tipo, "d");
    } else {  // si no tiene parte final, significa que estamos trabajando con un fichero
        strcpy(inicial, (camino + 1));
        strcpy(final, "");
        strcpy(tipo, "f");
    }

#if DEBUG7
    fprintf(stderr, "Camino: %s\nInicio: %s\nFinal: %s\nTipo: %s\n", camino, inicial, final, tipo);
#endif
    return 0;
}

int buscar_entrada(const char *camino_parcial, unsigned int *p_inodo_dir, unsigned int *p_inodo,
                   unsigned int *p_entrada, char reservar, unsigned char permisos) {
    struct entrada entrada;
    struct inodo inodo_dir;
    char inicial[sizeof(entrada.nombre)];
    char final[strlen(camino_parcial)];
    char tipo;
    int cant_entradas_inodo, num_entrada_inodo;

    memset(inicial, 0, sizeof(entrada.nombre));
    memset(final, 0, strlen(camino_parcial));
    memset(entrada.nombre, 0, sizeof(entrada.nombre));

    // el camino_parcial es “/”
    if (!strcmp(camino_parcial, "/")) {  // camino_parcial es “/”
        // se lee el superbloque
        struct superbloque SB;

        if (bread(posSB, &SB) == -1) {
            fprintf(stderr, "Directorios.c --> buscar_entrada() -- Error al leer el superbloque\n");
            return -1;
        }

        *p_inodo = SB.posInodoRaiz;  // la raiz siempre está asociada al inodo 0
        *p_entrada = 0;
        return 0;
    }
    if (extraer_camino(camino_parcial, inicial, final, &tipo) == -1) {
        return ERROR_CAMINO_INCORRECTO;  // el camino no es correcto  == -1
    }
#if DEBUG7
    printf("[buscar_entrada()-->Inicial: %s, Final: %s, Reservar: %d]\n", inicial, final, reservar);
#endif
    // buscamos la entrada cuyo nombre se encuentra en inicial
    if (leer_inodo(*(p_inodo_dir), &inodo_dir) == -1) {
        return ERROR_PERMISO_LECTURA;
    }
    // array de tipo struct entrada de las entradas que caben en un bloque, para optimizar la lectura en RAM
    struct entrada buffer_lectura[BLOCKSIZE / sizeof(struct entrada)];
    memset(buffer_lectura, 0, (BLOCKSIZE / sizeof(struct entrada)) * sizeof(struct entrada));
    // cantidad de entradas que contiene el inodo
    cant_entradas_inodo = inodo_dir.tamEnBytesLog / sizeof(struct entrada);
    // nº de entrada inicial
    num_entrada_inodo = 0;
    int aux_lectura = 0;
    if (cant_entradas_inodo > 0) {

        aux_lectura += mi_read_f(*p_inodo_dir, &buffer_lectura, aux_lectura, BLOCKSIZE);

        while ((num_entrada_inodo < cant_entradas_inodo) &&
               (strcmp(inicial, buffer_lectura[num_entrada_inodo].nombre) != 0)) {

            num_entrada_inodo++;

            if ((num_entrada_inodo % (BLOCKSIZE / sizeof(struct entrada))) == 0) {
                aux_lectura += mi_read_f(*p_inodo_dir, &buffer_lectura, aux_lectura, BLOCKSIZE);
            }
        }
    }

    // inicial != entrada.nombre --> la entrada no existe
    if (strcmp(buffer_lectura[num_entrada_inodo].nombre, inicial) != 0) {
        // seleccionar(reserva)
        switch (reservar) {
            // modo consulta. Como no existe retornamos error
            case 0:
                return ERROR_NO_EXISTE_ENTRADA_CONSULTA;  //error -3
                break;
            
            // modo escritura
            case 1:
                // Creamos la entrada en el directorio referenciado por *p_inodo_dir
                // si es fichero no permitir escritura
                if (inodo_dir.tipo == 'f') {
                    return ERROR_NO_SE_PUEDE_CREAR_ENTRADA_EN_UN_FICHERO;  //error -7
                }
                // si es directorio comprobar que tiene permiso de escritura
                if ((inodo_dir.permisos & 2) != 2) {
                    return ERROR_PERMISO_ESCRITURA;  //error -5
                } else {
                    // copiar *inicial en el nombre de la entrada
                    strcpy(entrada.nombre, inicial);
                    if (tipo == 'd') {  // se trada de un directorio
                        if (strcmp(final, "/") == 0) {
                            // se reserva un inodo en modo directorio y se asigna a la entrada
                            entrada.ninodo = reservar_inodo('d', 6);
#if DEBUG7
                            printf("[buscar_entrada()->reservado inodo: %d tipo %c con permisos %d para '%s']\n", entrada.ninodo, tipo, permisos, entrada.nombre);
#endif
                        } else {
                            // cuelgan más diretorios o ficheros
                            return ERROR_NO_EXISTE_DIRECTORIO_INTERMEDIO;  //error -4
                        }
                    } else { // se trata de un fichero

                        // se reserva un inodo en modo fichero y se asigna a la entrada
                        entrada.ninodo = reservar_inodo('f', 6);
#if DEBUG7
                        printf("[buscar()->reservado inodo: %d tipo %c con permisos %d para '%s']\n", entrada.ninodo, tipo, permisos, entrada.nombre);
#endif
                    }
#if DEBUG7
                    fprintf(stderr, "[buscar_entrada()->creada entrada: %s, %d] \n", inicial, entrada.ninodo);
#endif
                    // se escribe la entrada en el directorio padre
                    if (mi_write_f(*p_inodo_dir, &entrada, num_entrada_inodo * sizeof(struct entrada), sizeof(struct entrada)) == -1) {
                        // se había reservado un inodo para la entrada
                        if (entrada.ninodo != -1) {

                            //se libera el inodo reservado
                            liberar_inodo(entrada.ninodo);
#if DEBUG7
                            fprintf(stderr, "[buscar_entrada()-> liberado inodo %i, reservado a %s\n", num_entrada_inodo, inicial);
#endif
                        }
                        return -1;
                    }
                }
        }
    }
    // Si ya se ha llegado al final del camino
    if (!strcmp(final, "/") || !strcmp(final, "")) {
        if ((num_entrada_inodo < cant_entradas_inodo) && (reservar == 1)) {
            //la entrada ya existe
            return ERROR_ENTRADA_YA_EXISTENTE;   //error -6
        }
        // se asigna a *p_inodo el número de inodo del directorio o fichero que se ha creado o se ha leído
        *(p_inodo) = buffer_lectura[num_entrada_inodo].ninodo;
        // se asigna a *p_entrada el número de su entrada dentro del último directorio que lo contiene
        *(p_entrada) = num_entrada_inodo;
        // finazamos el bucle recursivo
        return 0;
    } else {
        // se asigna a *p_inodo_dir el puntero al inodo que se indica en la entrada encontrada;
        *p_inodo_dir = buffer_lectura[num_entrada_inodo].ninodo;
        return buscar_entrada(final, p_inodo_dir, p_inodo, p_entrada, reservar, permisos);
    }
    return 0;
}

void mostrar_error_buscar_entrada(int error){
    switch(error){
        case ERROR_CAMINO_INCORRECTO:
            fprintf(stderr, COLOR_ERROR "Error: Camino incorrecto\n" COLOR_RESET);
            break;
        case ERROR_PERMISO_LECTURA:
            fprintf(stderr, COLOR_ERROR "Error: Permiso denegado de lectura\n" COLOR_RESET);
            break;
        case ERROR_NO_EXISTE_ENTRADA_CONSULTA:
            fprintf(stderr, COLOR_ERROR "Error: No existe el archivo o directorio\n" COLOR_RESET);
            break;
        case ERROR_NO_EXISTE_DIRECTORIO_INTERMEDIO:
            fprintf(stderr, COLOR_ERROR "Error: No existe algún directorio intermedip\n" COLOR_RESET);
            break;
        case ERROR_PERMISO_ESCRITURA:
            fprintf(stderr, COLOR_ERROR "Error: Permiso denegado de escritura\n" COLOR_RESET);
            break;
        case ERROR_ENTRADA_YA_EXISTENTE:
            fprintf(stderr, COLOR_ERROR "Error: El archivo ya existe\n" COLOR_RESET);
            break;
        case ERROR_NO_SE_PUEDE_CREAR_ENTRADA_EN_UN_FICHERO:
            fprintf(stderr, COLOR_ERROR "Error: No es un directorio\n" COLOR_RESET);
            break;
    }

}

//////////////////NIVEL 8////////////////////////////

int mi_creat(const char *camino, unsigned char permisos)
{

    unsigned int p_inodo_dir, p_inodo, p_entrada;
    p_inodo_dir = 0;

    int error = buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, 1, permisos);
    if (error < 0)
    {
        mostrar_error_buscar_entrada(error);

        return -1;
    }
    
    return 0;
}

int mi_dir(const char *camino, char *buffer, char tipo)
{
    unsigned int p_inodo_dir, p_inodo, p_entrada;

    char longitud[TAMFILA];
    struct entrada entrada;
    struct inodo inodo;
    struct tm *tm;

    p_inodo_dir = 0;

    int error = buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, 0, 6);
    if (error < 0)
    {
        mostrar_error_buscar_entrada(error);
        return -1;
    }
    if (leer_inodo(p_inodo, &inodo) == -1)
    {
        fprintf(stderr, "Error: (mi_dir) al leer el inodo\n");
        return -1;
    }

    if ((inodo.permisos & 4) != 4)
    {
        fprintf(stderr, "Error: (mi_dir) no hay permisos de lectura\n");
        return -1;
    }

    
    if (inodo.tipo != tipo)
    {
        fprintf(stderr, "Error: (mi_dir) la sintaxis no concuerda con el tipo\n");
        return -1;
    }

    int num_entrada_inodo;
    int cant_entradas_inodo;
    
    if (inodo.tipo == 'd')
    {
        num_entrada_inodo = 0;
        
        cant_entradas_inodo = inodo.tamEnBytesLog / sizeof(entrada);

        while (num_entrada_inodo < cant_entradas_inodo)
        {
            
            if (mi_read_f(p_inodo, &entrada, num_entrada_inodo * sizeof(entrada), sizeof(entrada)) < 0)
            {
                fprintf(stderr, "Error: (mi_dir) ejecutando mi_read_f");
                return -1;
            }
            
            if (entrada.ninodo >= 0)
            {
                
                struct inodo inodoAux;
                if (leer_inodo(entrada.ninodo, &inodoAux) == -1)
                {
                    fprintf(stderr, "Error: (mi_dir) al leer el inodoAux");
                    return -1;
                }

                
                if (inodoAux.tipo == 'd')
                {
                    strcat(buffer, "d"); 
                }
                else if (inodoAux.tipo == 'f')
                {
                    strcat(buffer, "f"); 
                }
                strcat(buffer, "\t");

                
                if (inodoAux.permisos & 4) strcat(buffer, "r");
                else strcat(buffer, "-");

                if (inodoAux.permisos & 2) strcat(buffer, "w");
                else strcat(buffer, "-");

                if (inodoAux.permisos & 1) strcat(buffer, "x");
                else strcat(buffer, "-");
                
                strcat(buffer, "\t");

               
                
                char tmp[100];
                tm = localtime(&inodoAux.mtime);
                sprintf(tmp, "%d-%02d-%02d %02d:%02d:%02d", tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);
                strcat(buffer, tmp);
                strcat(buffer, "\t");

                
                memset(longitud, '\0', sizeof(longitud));
                sprintf(longitud, " %d bytes", inodoAux.tamEnBytesLog);
                strcat(buffer, longitud);
                strcat(buffer, "\t");

                
                strcat(buffer, entrada.nombre);
                strcat(buffer, "\t");
                strcat(buffer, "\n");

                num_entrada_inodo++;
            }
            else
            {
                return 0;
            }
        }
    }
    
    else if (inodo.tipo == 'f')
    {
        num_entrada_inodo = 1;
        if (mi_read_f(p_inodo, &entrada, num_entrada_inodo * sizeof(entrada), sizeof(entrada)) < 0)
        {
            fprintf(stderr, "Error: (mi_dir) ejecutando mi_read_f");
            return -1;
        }

        struct inodo inodoAux;
        if (leer_inodo(p_inodo, &inodoAux) == -1)
        {
            perror("Error al leer_inodo en mi_dir");
            return -1;
        }

        
        strcat(buffer, "f");
        strcat(buffer, "\t");

        
        if (inodoAux.permisos & 4) strcat(buffer, "r");
        else strcat(buffer, "-");

        if (inodoAux.permisos & 2) strcat(buffer, "w");
        else strcat(buffer, "-");

        if (inodoAux.permisos & 1) strcat(buffer, "x");
        else strcat(buffer, "-");
        
        strcat(buffer, "\t");

        
        struct tm *tm;
        char tmp[100];
        tm = localtime(&inodoAux.mtime);
        sprintf(tmp, "%d-%02d-%02d %02d:%02d:%02d", tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);
        strcat(buffer, tmp);
        strcat(buffer, "\t");

        
        memset(longitud, '\0', sizeof(longitud));
        sprintf(longitud, " %d bytes", inodoAux.tamEnBytesLog);
        strcat(buffer, longitud);
        strcat(buffer, "\t");

        
        strcat(buffer, entrada.nombre);
        strcat(buffer, "\t");
        strcat(buffer, "\n");
    }

    return num_entrada_inodo;
}



int mi_chmod(const char *camino, unsigned char permisos)
{
    unsigned int p_inodo_dir, p_inodo, p_entrada;
    struct inodo inodo;

    p_inodo_dir = 0;

    int err_entrada = buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, 0, permisos);
    if (err_entrada < 0)
    {
        mostrar_error_buscar_entrada(err_entrada);
        return -1;
    }

    if (leer_inodo(p_inodo, &inodo) == -1)
      return -1;
    
    return mi_chmod_f(p_inodo, permisos);
}




int mi_stat(const char *camino, struct STAT *p_stat)
{
    struct inodo inodo;
    unsigned int p_inodo_dir, p_inodo, p_entrada;
    p_inodo_dir = 0;
    
    int err_entrada = buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, 0, 0);
    if (err_entrada < 0)
    {
        mostrar_error_buscar_entrada(err_entrada);
        return -1;
    }
   
    if (mi_stat_f(p_inodo, p_stat) < 0)
    {
        perror("Error en mi_stat_f, en mi_stat");
        return -1;
    }

    if (leer_inodo(p_inodo, &inodo) == -1)
      return -1;

    return mi_stat_f(p_inodo, stat);
}

//////////////////NIVEL 9////////////////////////////

int mi_read(const char *camino, void *buf, unsigned int offset, unsigned int nbytes)
{
    unsigned int p_inodo = 0, p_inodo_dir = 0, p_entrada = 0, encontrado = 0;

   for (int i = 0; i < CACHE_ULT-1; i++){
       if (strcmp(camino, UltimaEntradaEscritura[i].camino) == -1)
    {
        p_inodo = UltimaEntradaEscritura[i].p_inodo;
        encontrado = 1;
    }
    
   }
    
    if(!encontrado){
        
        int error = buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, 0, 4);
        if (error < 0)
        {
            mostrar_error_buscar_entrada(error);
            return -1;
        }
        
        if(max_cache > 0){
            strcpy(UltimaEntradaEscritura[CACHE_ULT-max_cache].camino, camino);
            UltimaEntradaEscritura[CACHE_ULT-max_cache].p_inodo = p_inodo;
            max_cache--;
            #if DEBUG9
            fprintf(stderr, "[mi_read() -> Actualizamos la caché de lectura]\n");
            #endif
        }
        else{
            for (int i = 0; i < CACHE_ULT-1; i++){
                strcpy(UltimaEntradaEscritura[i].camino, UltimaEntradaEscritura[i+1].camino);
                UltimaEntradaEscritura[i].p_inodo = UltimaEntradaEscritura[i+1].p_inodo;
            }
            strcpy(UltimaEntradaEscritura[CACHE_ULT-1].camino, camino);
            UltimaEntradaEscritura[CACHE_ULT-1].p_inodo = p_inodo;
            #if DEBUG9
            fprintf(stderr, "[mi_read() -> Actualizamos la caché de lectura]\n");
            #endif
        }
    }
          
    int bytesLeidos = mi_read_f(p_inodo, buf, offset, nbytes);
    if (bytesLeidos == -1)
    {
        mostrar_error_buscar_entrada(ERROR_PERMISO_LECTURA);
        return -1;
    }
    return bytesLeidos;
}



int mi_write(const char *camino, const void *buf, unsigned int offset, unsigned int nbytes)
{
   unsigned int p_inodo_dir = 0, p_inodo = 0, p_entrada = 0;
   int encontrado = 0, error, resultado;
   //comprobar caché
   for(int i = 0; i<max_cache-1; i++)
   {
      if (strcmp(camino, UltimaEntradaEscritura[i].camino) == 0)
      {
         encontrado = 1;
         p_inodo = UltimaEntradaEscritura[i].p_inodo;
         break;
      }
      
   }
   if (!encontrado)
   {
      if ((error = buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, 0, 0)) != EXIT_SUCCESS)
      {
         mostrar_error_buscar_entrada(error);
         return -1;
      }
      
      if (max_cache > 0)
      {
         //actualizar caché
         strcpy(UltimaEntradaEscritura[CACHE_ULT - max_cache].camino, camino);
         UltimaEntradaEscritura[CACHE_ULT - max_cache].p_inodo = p_inodo;
         
         max_cache--;
         #if DEBUG9 
         fprintf(stderr, "[mi_write()-> Actualizamos la caché de escritura\n");
            #endif
      }
      else{
          for (int i = 0; i < CACHE_ULT-1; i++){
                strcpy(UltimaEntradaEscritura[i].camino, UltimaEntradaEscritura[i+1].camino);
                UltimaEntradaEscritura[i].p_inodo = UltimaEntradaEscritura[i+1].p_inodo;
          }
            strcpy(UltimaEntradaEscritura[CACHE_ULT-1].camino, camino);
            UltimaEntradaEscritura[CACHE_ULT-1].p_inodo = p_inodo;
      }
    
   }
   resultado = mi_write_f(p_inodo, buf, offset, nbytes);
   if (resultado == -1){
       resultado = 0;
   }
   return resultado;
}

//////////////////NIVEL 10////////////////////////////

int mi_link(const char *camino1, const char *camino2)
{

    unsigned int p_inodo_dir1 = 0;
    unsigned int p_inodo1 = 0;
    unsigned int p_entrada1 = 0;	
    unsigned int p_inodo_dir2 = 0;
    unsigned int p_inodo2 = 0;
    unsigned int p_entrada2 = 0;

    // Comprobamos primera entrada
    int error = buscar_entrada(camino1, &p_inodo_dir1, &p_inodo1, &p_entrada1, 0, 6);
    if (error < 0)
    {
        mostrar_error_buscar_entrada(error);

        return -1;
    }

    
    error = buscar_entrada(camino2, &p_inodo_dir2, &p_inodo2, &p_entrada2, 1, 6);
    if (error < 0)
    {
        mostrar_error_buscar_entrada(error);   
        

        return -1;
    }

    int ninodo1 = p_inodo1;
    struct inodo inodo;
    if (leer_inodo(ninodo1, &inodo) == -1)
    {
        printf("Error (mi_link) . No se pudo leer el inodo\n");


        return -1;
    }

    
    if (inodo.tipo != 'f')
    {
        printf("Error (mi_link) no es un fichero\n");

        return -1;
    }

    struct entrada entrada;
    
    if (mi_read_f (p_inodo_dir2, &entrada, p_entrada2 * sizeof(entrada), sizeof(entrada)) == -1)
    {
		printf("Error (mi_link) ejecutando mi_read_f()\n");

        return -1;
	}

    
	liberar_inodo (p_inodo2);

    
    entrada.ninodo = p_inodo1;
    
    
	if (mi_write_f (p_inodo_dir2, &entrada, p_entrada2 * sizeof(entrada), sizeof(entrada)) == -1)
    {
		printf("Error (mi_link) ejecutando mi_write_f()\n");


        return -1;
	}

    if (leer_inodo(ninodo1, &inodo) == -1)
    {
        printf("Error (mi_link) . No se pudo leer el inodo\n");

        return -1;
    }
    
    
    inodo.nlinks++;
    
    inodo.ctime = time(NULL);

    
    if (escribir_inodo(ninodo1, inodo) == -1)
    {
        printf("Error (mi_link) en escribir el inodo\n");

        return -1;
    }
    

    return 0;
}



int mi_unlink(const char *camino)
{ 
    
    unsigned int p_inodo_dir = 0;
    unsigned int p_inodo = 0;
    unsigned int p_entrada = 0;
    
    
    int error = buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, 0, 6);

    if (error == 0)
    {
       
	    struct inodo inodo_2;
        if (leer_inodo(p_inodo, &inodo_2) == -1)
        {
            printf("Error (mi_unlink) . No se pudo leer el inodo\n");

            return -1;
        }
	    
    	struct inodo inodo_1;
        if (leer_inodo(p_inodo_dir, &inodo_1) == -1)
        {
            printf("Error (mi_unlink) . No se pudo leer el inodo\n");

            return -1;
        }

        
	    int nentradas = inodo_1.tamEnBytesLog / sizeof (struct entrada);

        if (inodo_2.tamEnBytesLog != 0 && inodo_2.tipo == 'd')
        {
		    printf("Error: El directorio %s no está vacío\n",camino);


		    return -1;
	    }

        
	    if (p_entrada != nentradas - 1){
		    struct entrada entrada;
		    
		    if (mi_read_f (p_inodo_dir, &entrada, inodo_1.tamEnBytesLog - sizeof(struct entrada), sizeof(struct entrada)) == -1)
            {
			    printf("Error (mi_unlink) ejecutando mi_read_f()\n");


                return -1;
		    }

		    if (mi_write_f (p_inodo_dir, &entrada, p_entrada * sizeof(struct entrada), sizeof(struct entrada)) == -1)
            {
			    printf("Error (mi_unlink) ejecutando mi_write_f()\n");


                return -1;
		    }
        }

	    
	    mi_truncar_f (p_inodo_dir, inodo_1.tamEnBytesLog - sizeof (struct entrada));
	    
	    if (leer_inodo(p_inodo, &inodo_1) == -1)
        {
            printf("Error (mi_unlink) . No se pudo leer el inodo\n");


            return -1;
        }
	    
	    inodo_1.nlinks--;         
    	
	    if (inodo_1.nlinks == 0)
        {
		    liberar_inodo (p_inodo);
	    }
        else
        {
		    
		    inodo_1.ctime = time (NULL);
		    if (escribir_inodo (p_inodo, inodo_1) == -1)
            {
                printf("Error (mi_unlink) . No se pudo escribir en el inodo\n");


                return -1;
            }
	    }
	}  
    else
    {
        mostrar_error_buscar_entrada(error);

        
        return -1;
    }
    

    return 0;
}