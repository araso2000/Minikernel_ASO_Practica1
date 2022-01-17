/*
 *  minikernel/kernel/include/llamsis.h
 *
 *  Minikernel. Versi�n 1.0
 *
 *  Fernando P�rez Costoya
 *
 */

/*
 *
 * Fichero de cabecera que contiene el numero asociado a cada llamada
 *
 * 	SE DEBE MODIFICAR PARA INCLUIR NUEVAS LLAMADAS
 *
 */

#ifndef _LLAMSIS_H
#define _LLAMSIS_H

/* Numero de llamadas disponibles */
//NUEVO_INI
//Modificamos la constante que guarda el numero de servicios ofrecidos por el kernel
#define NSERVICIOS 11
//NUEVO_FIN

#define CREAR_PROCESO 0
#define TERMINAR_PROCESO 1
#define ESCRIBIR 2
#define OBTENER_ID 3

//NUEVO_INI
//Definimos constantes para hacer el codigo mas legible
#define DORMIR 4
#define CREAR_MUTEX 5
#define ABRIR_MUTEX 6
#define LOCK 7
#define UNLOCK 8 
#define CERRAR_MUTEX 9
//NUEVO_FIN

//#define LEER_CARACTER 10

#endif /* _LLAMSIS_H */

