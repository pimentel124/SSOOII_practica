/****************************************** FUNCIONES AUXILIARES **************************************************/

/*	-Función: datos_inodo.
        -Descripción: Imprime los valores de un inodo.
        -Parámetros: Inodo que se va a mostrar
        -Return:
*/
void datos_inodo(struct inodo inodo) {
    struct tm *ts;
    char atime[80];
    char mtime[80];
    char ctime[80];
    ts = localtime(&inodo.atime);
    strftime(atime, sizeof(atime), "%a %Y-%m-%d %H:%M:%S", ts);
    ts = localtime(&inodo.mtime);
    strftime(mtime, sizeof(mtime), "%a %Y-%m-%d %H:%M:%S", ts);
    ts = localtime(&inodo.ctime);
    strftime(ctime, sizeof(ctime), "%a %Y-%m-%d %H:%M:%S", ts);
    printf("tipo: %c\n", inodo.tipo);
    printf("permisos: %d\n", inodo.permisos);
    printf("atime: %s\n", atime);
    printf("ctime: %s\n", ctime);
    printf("mtime: %s\n", mtime);
    printf("nlinks: %u\n", inodo.nlinks);
    printf("tamEnBytesLog: %u\n", inodo.tamEnBytesLog);
    printf("numBloquesOcupados: %u\n", inodo.numBloquesOcupados);
}

/*	-Función: datos_STAT.
        -Descripción: Imprime los valores de un inodo.
        -Parámetros: STAT del inodo que se va a mostrar.
        -Return:
*/
void datos_STAT(struct STAT STAT) {
    struct tm *ts;
    char atime[80];
    char mtime[80];
    char ctime[80];
    ts = localtime(&STAT.atime);
    strftime(atime, sizeof(atime), "%a %Y-%m-%d %H:%M:%S", ts);
    ts = localtime(&STAT.mtime);
    strftime(mtime, sizeof(mtime), "%a %Y-%m-%d %H:%M:%S", ts);
    ts = localtime(&STAT.ctime);
    strftime(ctime, sizeof(ctime), "%a %Y-%m-%d %H:%M:%S", ts);
    fprintf(stderr, "tipo=%c\n", STAT.tipo);
    fprintf(stderr, "permisos=%d\n", STAT.permisos);
    fprintf(stderr, "atime: %s\n", atime);
    fprintf(stderr, "ctime: %s\n", ctime);
    fprintf(stderr, "mtime: %s\n", mtime);
    fprintf(stderr, "nlinks=%u\n", STAT.nlinks);
    fprintf(stderr, "tamEnBytesLog=%u\n", STAT.tamEnBytesLog);
    fprintf(stderr, "numBloquesOcupados=%u\n\n", STAT.numBloquesOcupados);
}