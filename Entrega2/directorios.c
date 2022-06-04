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

int buscar_entrada(const char *camino_parcial, unsigned int *p_inodo_dir, unsigned int *p_inodo, unsigned int *p_entrada, char reservar, unsigned char permisos)
{
    // Si (es el directorio raíz) entonces
    if (!strcmp(camino_parcial, "/")) // Camino_parcial es “/”
    {
        // *p_inodo:=SB.posInodoRaiz
        *p_inodo = 0; // Nuestra raiz siempre estará asociada al inodo 0
        *p_entrada = 0; // *p_entrada:=0
        fprintf(stderr, "Inodo raiz: %d\n", *p_inodo);
        return 0; // devolver 0
    }

    // inicial[sizeof(entrada.nombre)]: car
    // final[sizeof(strlen(camino_parcial))]: car
    char inicial[strlen(camino_parcial)], final[strlen(camino_parcial)], tipo;
    // extraer_camino (camino_parcial, inicial, final, &tipo)
    int error = extraer_camino(camino_parcial, inicial, final, &tipo);

    // Si error al extraer camino entonces devolver ERROR_EXTRAER_CAMINO  fsi
    if (error == -1)
    { 
        return ERROR_CAMINO_INCORRECTO;
    }
    //fprintf(stderr,"[buscar_entrada() → inicial: %s, final: %s, reservar: %d]\n",inicial,final,reservar);

    // Buscamos la entrada cuyo nombre se encuentra en inicial
    struct inodo inodo_dir;
    // leer_inodo( *p_inodo_dir, &inodo_dir)
    leer_inodo(*p_inodo_dir, &inodo_dir);
    // si inodo_dir no tiene permisos de lectura entonces
    if ((inodo_dir.permisos & 4) != 4)
    {
        return ERROR_PERMISO_LECTURA; // devolver ERROR_PERMISO_LECTURA
    }

    // El buffer de lectura puede ser un struct tipo entrada 
    // o bien un array de las entradas que caben en un bloque, para optimizar la lectura en RAM
    struct entrada entrada;
    // Calcular cant_entradas_inodo = tamañoFicheroIndicandoElMasAlejado/tamañoEntarda
    int cant_entradas_inodo = inodo_dir.tamEnBytesLog / sizeof(struct entrada);  // Cantidad de entradas que contiene el inodo
    //  num_entrada_inodo := 0
    int num_entrada_inodo = 0;  // nº de entrada inicial
    unsigned int offset = 0;
    // Si cant_entradas_inodo > 0 entonces
    if (cant_entradas_inodo > 0)
    {
        // Leer entrada
        mi_read_f(*p_inodo_dir, &entrada, offset, sizeof(struct entrada));
        // Mientras ((num_entrada_inodo < cant_entradas_inodo) y (inicial ≠ entrada.nombre)) hacer 
        while ((num_entrada_inodo < cant_entradas_inodo) && (strcmp(inicial, entrada.nombre)))
        {
            num_entrada_inodo++;
            offset += sizeof(struct entrada);
            memset(entrada.nombre, 0, sizeof(struct entrada)); // Previamente inicializar el buffer de lectura con 0s
            // Leer siguiente entrada 
            mi_read_f(*p_inodo_dir, &entrada, offset, sizeof(struct entrada));
        }
    }

    // Si (num_entrada_inodo = cant_entradas_inodo) y (inicial ≠ entrada.nombre) entonces
    if (num_entrada_inodo == cant_entradas_inodo)
    { // La entrada no existe
        switch (reservar)
        {

            case 0: // Modo consulta. Como no existe retornamos error
                return ERROR_NO_EXISTE_ENTRADA_CONSULTA;
            case 1:  // Modo escritura. 
                // Creamos la entrada en el directorio referenciado por *p_inodo_dir
                // Si es fichero no permitir escritura 
                if (inodo_dir.tipo == 'f') // si inodo_dir.tipo = ‘f’ entonces
                {
                    return ERROR_NO_SE_PUEDE_CREAR_ENTRADA_EN_UN_FICHERO; // devolver ERROR_NO_SE_PUEDE_CREAR_ENTRADA_EN_UN_FICHERO
                }
                // Si es directorio comprobar que tiene permiso de escritura
                if ((inodo_dir.permisos & 2) != 2) // si inodo_dir no tiene permisos de escritura entonces
                {
                    return ERROR_PERMISO_ESCRITURA; // devolver ERROR_PERMISO_ESCRITURA
                }
                else
                {
                    strcpy(entrada.nombre, inicial); // copiar inicial en el nombre de la entrada
                    if (tipo == 'd') // si tipo = 'd' entonces
                    {
                        if (!strcmp(final, "/")) // si final es igual a "/" entonces
                        {
                            // reservar un inodo como directorio y asignarlo a la entrada
                            int ninodo = reservar_inodo('d', permisos);
                            entrada.ninodo = ninodo;
                            //fprintf(stderr,"[buscar_entrada() → reservado inodo %d tipo %c con permisos %d para %s]\n",ninodo,tipo,permisos,inicial);                      
                        }
                        else // Cuelgan más diretorios o ficheros
                        {
                            return ERROR_NO_EXISTE_DIRECTORIO_INTERMEDIO; // devolver ERROR_NO_EXISTE_DIRECTORIO_INTERMEDIO
                        }
                    }
                    else  // Es un fichero
                    {
                        // reservar un inodo como fichero y asignarlo a la entrada
                        int ninodo = reservar_inodo('f', 6);
                        entrada.ninodo = ninodo;
                        //fprintf(stderr,"[buscar_entrada() → reservado inodo %d tipo %c con permisos %d para %s]\n",ninodo,tipo,permisos,inicial);
                    }
                    // escribir la entrada
                    error = mi_write_f(*p_inodo_dir, &entrada, inodo_dir.tamEnBytesLog, sizeof(struct entrada));
                    if (error == -1) // si error de escritura entonces
                    {
                        if (entrada.ninodo != -1) // si se había reservado un inodo para la entrada entonces 
                        {
                            liberar_inodo(entrada.ninodo); // liberar el inodo
                        }
                        return -1; // devolver EXIT_FAILURE
                    }
                }
        }
    }
    if ((!strcmp(final, "/")) || !(strcmp(final, "\0"))) // si hemos llegado al final del camino  entonces
    {
        if ((num_entrada_inodo < cant_entradas_inodo) && (reservar == 1)) // si (num_entrada_inodo < cant_entradas_inodo) && (reservar=1) entonces
        {
            // Modo escritura y la entrada ya existe
            return ERROR_ENTRADA_YA_EXISTENTE; // devolver ERROR_ENTRADA_YA_EXISTENTE
        }
        // Cortamos la recursividad
        *p_inodo = entrada.ninodo; //  asignar a *p_inodo el numero de inodo del directorio/fichero creado/leido
        *p_entrada = num_entrada_inodo; // asignar a *p_entrada el número de su entrada dentro del último directorio que lo contiene
        //fprintf(stderr,"[buscar_entrada() → creada entrada: %s, %d]\n",inicial,entrada.ninodo);
        return 0;
    }
    else
    {
        *p_inodo_dir = entrada.ninodo; // asignamos a *p_inodo_dir el puntero al inodo que se indica en la entrada;
        // Llamada recursiva
        return buscar_entrada(final, p_inodo_dir, p_inodo, p_entrada, reservar, permisos);
    }
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
    if (error != EXIT_SUCCESS)
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

    return p_inodo;
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
    
//mi_waitSem();
   unsigned int p_inodo_dir, p_inodo, p_entrada;
   int error, numentradas;
   struct inodo inodo;
   struct entrada entrada;
   int tamEntrada = sizeof(struct entrada);
   p_inodo_dir = 0;
   if ((error = buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, 0, 0)) != EXIT_SUCCESS)
   {
      mostrar_error_buscar_entrada(error);
      //mi_signalSem();
      return -1;
   }
   if (leer_inodo(p_inodo, &inodo) == -1)
   {
      //mi_signalSem();
      return -1;
   }
   if (inodo.tipo == 'd' && inodo.tamEnBytesLog > 0)
   {
      fprintf(stderr, "Error: El directorio %s no está vacío\n", camino);
      //mi_signalSem();
      return -1;
   }
   if (leer_inodo(p_inodo_dir, &inodo) == -1)
   {
      //mi_signalSem();
      return -1;
   }
   //Eliminar la entrada en el directorio
   numentradas = inodo.tamEnBytesLog / tamEntrada;
   if (p_entrada != (numentradas - 1))
   {
      if (mi_read_f(p_inodo_dir, &entrada, (numentradas - 1) * tamEntrada, tamEntrada) == -1)
      {
         //mi_signalSem();
         return -1;
      }
      if (mi_write_f(p_inodo_dir, &entrada, p_entrada * tamEntrada, tamEntrada) == -1)
      {
         //mi_signalSem();
         return -1;
      }
   }
   if (mi_truncar_f(p_inodo_dir, inodo.tamEnBytesLog - tamEntrada) == -1)
   {
      //mi_signalSem();
      return -1;
   }
   if (leer_inodo(p_inodo, &inodo) == -1)
   {
      //mi_signalSem();
      return -1;
   }
   inodo.nlinks--;
   if (inodo.nlinks == 0)
   {
      //Eliminar el inodo
      if (liberar_inodo(p_inodo) == -1)
      {
         //mi_signalSem();
         return -1;
      }
   }
   else
   {
      inodo.ctime = time(NULL);
      if (escribir_inodo(p_inodo, inodo) == -1)
      {
         //mi_signalSem();
         return -1;
      }
   }
   //mi_signalSem();
   return EXIT_SUCCESS;
}