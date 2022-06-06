Miembros del grupo:
    + Andreu Marquès Valerià
    + Álvaro Pimentel Lorente

Sintaxis específica de los programas empleados:
    + mi_mkfs.c: mi_mkfs <nombre_dispositivo> <numero_de_bloques>
    + escribir.c: escribir <nombre_dispositivo> <\"$(cat fichero)\"> <diferentes_diferentes_inodos>
    + leer.c: leer <nombre_dispositivo> <nº_de_inodo>
    + leer_sf.c: leer_sf <nombre_dispositivo>
    + permitir.c: permitir <nombre_dispositivo> <nº_de_inodo> <permisos>
    + truncar.c: truncar <nombre_dispositivo> <nº_de_inodo> <nº_de_bytes>
    + mi_cat.c: mi_cat <nombre_dispositivo> </ruta>
    + mi_stat.c: mi_stat <nombre_dispositivo> </ruta>
    + mi_chmod.c: mi_chmod <nombre_dispositivo> <permisos> </ruta>
    + mi_escribir.c mi_escribir <nombre_dispositivo> </ruta_fichero> <texto> <offset>
    + mi_link.c: mi_link <nombre_dispositivo> </ruta_fichero_original> </ruta_enlace>
    + mi_ls.c: mi_ls <nombre_dispositivo> </ruta_directorio>
    + mi_mkdir.c: mi_mkdir <nombre_dispositivo> <permisos> </ruta>
    + mi_rm.c: mi_rm <nombre_dispositivo> </ruta>
    + mi_rmdir.c : mi_rmdir <nombre_dispositivo> </ruta>
    + mi_stat.c : mi_stat <nombre_dispositivo> </ruta>
    + mi_touch.c : mi_touch <nombre_dispositivo> <permisos> </ruta>
    + simulacion.c: simulacion <nombre_dispositivo>
    + verificacion.c: verificacion <nombre_dispositivo> <directorio_simulacion>

Observaciones adicionales:
    + Mejoras:
        - Durante la inicialización del mapa de bits el la funcion InitMB() en el fichero "ficheros_basico.c" se emplea la función
        escribir_bit() para inicializar a 1 los bits correspondientes a los metadatos.
        - Durante la verificacion en verificacion.c se leer las 100 de golpe con una sola llamada a mi_read().
