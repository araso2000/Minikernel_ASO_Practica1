/*
 *  minikernel/include/kernel.h
 *
 *  Minikernel. Versi�n 1.0
 *
 *  Fernando P�rez Costoya
 *
 */

/*
 *
 * Fichero de cabecera que contiene definiciones usadas por kernel.c
 *
 *      SE DEBE MODIFICAR PARA INCLUIR NUEVA FUNCIONALIDAD
 *
 */

#ifndef _KERNEL_H
#define _KERNEL_H

#include "const.h"
#include "HAL.h"
#include "llamsis.h"

//NUEVO_INI
//Libreria que implementa Strings en C
#include "string.h"
//Definición de constantes para hacer el código mas legible
#define NO_RECURSIVO 0 
#define RECURSIVO 1
#define LIBRE 1
#define OCUPADO 0
//NUEVO_FIN

/*
 *
 * Definicion del tipo que corresponde con el BCP.
 * Se va a modificar al incluir la funcionalidad pedida.
 *
 */
typedef struct BCP_t *BCPptr;
int char_escritos;
int ind_escribir;
int ind_leer;
char buffer_char[TAM_BUF_TERM];

typedef struct BCP_t {
    int id;				/* ident. del proceso */
    int estado;				/* TERMINADO|LISTO|EJECUCION|BLOQUEADO*/
    contexto_t contexto_regs;		/* copia de regs. de UCP */
    void * pila;				/* dir. inicial de la pila */
	BCPptr siguiente;			/* puntero a otro BCP */
	void *info_mem;			/* descriptor del mapa de memoria */
	
	//NUEVO_INI
	int n_descriptores; //Numero de descriptores en uso
	unsigned int segundos_dormir; //Guardamos los segundos que debe dormir un proceso si es necesario
	int descriptores[NUM_MUT_PROC];	//Array de los descriptores de MUTEX en uso
	//NUEVO_FIN
} BCP;

/*
 *
 * Definicion del tipo que corresponde con la cabecera de una lista
 * de BCPs. Este tipo se puede usar para diversas listas (procesos listos,
 * procesos bloqueados en sem�foro, etc.).
 *
 */

typedef struct{
	BCP *primero;
	BCP *ultimo;
} lista_BCPs;


//NUEVO_INI
//Struct que guarda los datos necesarios para realizar un MUTEX					  
typedef struct{
	int bloqueos; //Numero total de bloqueos activo
	int ocupado; //Guardamos si el MUTEX está libre u ocupado
	int recursivo; //Guardamos si el MUTEX es recursivo o no
	BCPptr procesoEnUso;	//Guardamos el proceso que está en uso actual
	lista_BCPs procesosEnEspera; //Lista de procesos en espera
	char nombre[MAX_NOM_MUT]; //Guardamos el nombre como un array de char
	int contadorProcesosEnEspera; // numero de procesos esperando asociado a procesosEnEspera
} mutex;
//NUEVO_FIN

/*
 * Variable global que identifica el proceso actual
 */

BCP * p_proc_actual=NULL;

/*
 * Variable global que representa la tabla de procesos
 */

BCP tabla_procs[MAX_PROC];

/*
 * Variable global que representa la cola de procesos listos
 */

//NUEVO_INI
//Lista que guarda los procesos listos
lista_BCPs listaProcesosListos= {NULL, NULL};
//Lista que guarda los procesos bloqueados
lista_BCPs listaProcesosBloqueados={NULL, NULL};
//Lista que guarda los procesos en espera de un MUTEX libre
lista_BCPs listaProcesosEsperandoMutex={NULL, NULL};
//Lista de los MUTEX en uso
mutex listaMutex[NUM_MUT];
//Numero total de MUTEX en uso
int mutexTotales = 0;
//NUEVO_FIN

lista_BCPs lista_espera_char={NULL,NULL};

/*
 *
 * Definici�n del tipo que corresponde con una entrada en la tabla de
 * llamadas al sistema.
 *
 */
typedef struct{
	int (*fservicio)();
} servicio;

//NUEVO_INI
//Funcion que permite desbloquear los procesos bloqueados una vez haya acabado su tiempo de espera
void reducirDormir();
//Funcion que permite buscar un MUTEX por su nombre
int buscarMutexPorNombre();
//Funcion que busca un MUTEX libre
int buscarHuecoLibre();
//Funcion que libera TODOS los procesos bloqueados por un MUTEX
int liberarTodosLosProcesosBloqueadosMutex(mutex* m);
//Variable que lleva la cuenta de los ticks de reloj para el algoritmo RoundRobin
int ticksPorRodaja;
//Variable que guarda el proceso que ha consumido su parte delimitada de tiempo de proceso
BCPptr procesoAExpulsar;
//NUEVO_FIN

/*
 * Prototipos de las rutinas que realizan cada llamada al sistema
 */
int sis_crear_proceso();
int sis_terminar_proceso();
int sis_escribir();
int obtener_id_pr();

//NUEVO_INI
//Funcion que bloquea un proceso y lo mantiene en espera
int dormir(unsigned int segundos);
//Asigna y activa un MUTEX a un proceso que lo requiera
int crear_mutex(char* nombre, int tipo);
//Funcion que asigna un MUTEX al proceso actual
int abrir_mutex(char*nombre);
//Funcion que bloquea el proceso hasta que cumpla la condición de ejecución
int lock(unsigned int mutexid);
//Funcion que ejecuta un proceso si cumple la condicion de carrera
int unlock(unsigned int mutexid);
//Cierra el MUTEX y libera el proceso
int cerrar_mutex(unsigned int mutexid);
//NUEVO_FIN

//int leer_caracter();

/*
 * Variable global que contiene las rutinas que realizan cada llamada
 */


servicio tabla_servicios[NSERVICIOS]={	{sis_crear_proceso},
					{sis_terminar_proceso},
					{sis_escribir},
//NUEVO_INI
//Añadimos los servicios que vamos a programar a la tabla de servicios del kernel
					{obtener_id_pr},
					{dormir},
					{crear_mutex},
					{abrir_mutex},
					{lock},
					{unlock},
					{cerrar_mutex},
//NUEVO_FIN
					/*{leer_caracter}*/};

#endif /* _KERNEL_H */
