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
    

Observaciones adicionales:
    + Niveles de DEBUG habilitados: 5, 6, 10
    + Durante la inicialización del mapa de bits el la funcion InitMB() en el fichero "ficheros_basico.c" se emplea la función
        reservar_bloque() para inicializar a 1 los bits correspondientes a los metadatos.
