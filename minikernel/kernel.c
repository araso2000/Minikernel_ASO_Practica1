//Fichero que contiene la funcionalidad del sistema operativo

#include "kernel.h"	/* Contiene defs. usadas por este modulo */

/*
 *
 * Funciones relacionadas con la tabla de procesos:
 *	iniciar_tabla_proc buscar_BCP_libre
 *
 */

/*
 * Funci�n que inicia la tabla de procesos
 */
static void iniciar_tabla_proc(){
	int i;

	for (i=0; i<MAX_PROC; i++)
		tabla_procs[i].estado=NO_USADA;
}

/*
 * Funcion que busca una entrada libre en la tabla de procesos
 */
static int buscar_BCP_libre(){
	int i;

	for (i=0; i<MAX_PROC; i++)
		if (tabla_procs[i].estado==NO_USADA)
			return i;
	return -1;
}

/*
 *
 * Funciones que facilitan el manejo de las listas de BCPs
 *	insertar_ultimo eliminar_primero eliminar_elem
 *
 * NOTA: PRIMERO SE DEBE LLAMAR A eliminar Y LUEGO A insertar
 */

/*
 * Inserta un BCP al final de la lista.
 */
static void insertar_ultimo(lista_BCPs *lista, BCP * proc){
	if (lista->primero==NULL)
		lista->primero= proc;
	else
		lista->ultimo->siguiente=proc;
	lista->ultimo= proc;
	proc->siguiente=NULL;
}

/*
 * Elimina el primer BCP de la lista.
 */
static void eliminar_primero(lista_BCPs *lista){

	if (lista->ultimo==lista->primero)
		lista->ultimo=NULL;
	lista->primero=lista->primero->siguiente;
}

/*
 * Elimina un determinado BCP de la lista.
 */
static void eliminar_elem(lista_BCPs *lista, BCP * proc){
	BCP *paux=lista->primero;

	if (paux==proc)
		eliminar_primero(lista);
	else {
		for ( ; ((paux) && (paux->siguiente!=proc));
			paux=paux->siguiente);
		if (paux) {
			if (lista->ultimo==paux->siguiente)
				lista->ultimo=paux;
			paux->siguiente=paux->siguiente->siguiente;
		}
	}
}

/*
 *
 * Funciones relacionadas con la planificacion
 *	espera_int planificador
 */

/*
 * Espera a que se produzca una interrupcion
 */
static void espera_int(){
	int nivel;

	/* Baja al m�nimo el nivel de interrupci�n mientras espera */
	nivel=fijar_nivel_int(NIVEL_1);
	halt();
	fijar_nivel_int(nivel);
}

/*
 * Funcion de planificacion que implementa un algoritmo FIFO.
 */
static BCP * planificador(){
	//NUEVO_INI
	//Variable local que guarda el valor de la constante
	ticksPorRodaja = TICKS_POR_RODAJA;
	//Inicialización de la variable
	procesoAExpulsar = NULL;
	//NUEVO_FIN

	while (listaProcesosListos.primero==NULL)
		espera_int();		/* No hay nada que hacer */
	return listaProcesosListos.primero;
}

/*
 *
 * Funcion auxiliar que termina proceso actual liberando sus recursos.
 * Usada por llamada terminar_proceso y por rutinas que tratan excepciones
 *
 */
static void liberar_proceso(){
	BCP * p_proc_anterior;
	liberar_imagen(p_proc_actual->info_mem); /* liberar mapa */

	p_proc_actual->estado=TERMINADO;
	eliminar_primero(&listaProcesosListos); /* proc. fuera de listos */

	/* Realizar cambio de contexto */
	p_proc_anterior=p_proc_actual;
	p_proc_actual=planificador();

	printk("-> C.CONTEXTO POR FIN: de %d a %d\n",
			p_proc_anterior->id, p_proc_actual->id);

	liberar_pila(p_proc_anterior->pila);
	cambio_contexto(NULL, &(p_proc_actual->contexto_regs));
        return; /* no deberia llegar aqui */
}

/*
 *
 * Funciones relacionadas con el tratamiento de interrupciones
 *	excepciones: exc_arit exc_mem
 *	interrupciones de reloj: int_reloj
 *	interrupciones del terminal: int_terminal
 *	llamadas al sistemas: llam_sis
 *	interrupciones SW: int_sw
 *
 */

/*
 * Tratamiento de excepciones aritmeticas
 */
static void exc_arit(){

	if (!viene_de_modo_usuario())
		panico("excepcion aritmetica cuando estaba dentro del kernel");


	printk("-> EXCEPCION ARITMETICA EN PROC %d\n", p_proc_actual->id);
	liberar_proceso();

        return; /* no deber�a llegar aqui */
}

/*
 * Tratamiento de excepciones en el acceso a memoria
 */
static void exc_mem(){

	if (!viene_de_modo_usuario())
		panico("excepcion de memoria cuando estaba dentro del kernel");


	printk("-> EXCEPCION DE MEMORIA EN PROC %d\n", p_proc_actual->id);
	liberar_proceso();

    return; /* no deber�a llegar aqui */
}

/*
 * Tratamiento de interrupciones de terminal
 */
static void int_terminal(){
	int n_interrupcion = fijar_nivel_int(NIVEL_2);
	char car;
	car = leer_puerto(DIR_TERMINAL);

	if(char_escritos < TAM_BUF_TERM){
		printk("-> TRATANDO INT. DE TERMINAL %c\n", car);
		buffer_char[ind_escribir] = car;
		ind_escribir = (ind_escribir +1 ) % TAM_BUF_TERM;
		char_escritos++;
		if(lista_espera_char.primero != NULL){
		//	printk("Habia un proceso esperando a la escritura");
			//si hay procesos esperando a que se escriba en el terminal
			BCPptr proceso = lista_espera_char.primero;
			//elimina al proceso de la lista de procesos en espera de escritura
			eliminar_primero(&lista_espera_char);
			proceso->estado=LISTO;
			//añade el proceso a  la lista de procesos listos
			insertar_ultimo(&listaProcesosListos, proceso);
		}
	} else {
		printk("El buffer esta lleno\n");
	}
  //modificar para la lectura de caracteres
	fijar_nivel_int(n_interrupcion);
    return;
}

//NUEVO_INI
//Funcion que gestiona el algoritmo de gestión de procesos
static void roundRobin(){
	//Reducimos los ticks en una unidad
	ticksPorRodaja --;
	//Si los ticks son 0 o menos
	if(ticksPorRodaja <= 0){
		//El proceso actual se ha quedado sin tiempo de ejecucion con lo que 
		//se expulsa de la cola de procesos
		procesoAExpulsar = p_proc_actual;
		//Activa la interrupción de software para que el proceso actual se 
		//expulse y se elija el siguiente proceso a ejecutar
		activar_int_SW();
	}
}
//NUEVO_FIN

/*
 * Tratamiento de interrupciones de reloj
 */
static void int_reloj(){
	//NUEVO_INI
	//Ante una interrupcion de reloj
	//Se llama a reducirDormir, que gestiona los segundos que deben dormir los procesos
	reducirDormir();
	//Se llama a roundRobin, que gestiona el algoritmo que ejecuta procesos
	roundRobin();
	//NUEVO_FIN
  return;
}

/*
 * Tratamiento de llamadas al sistema
 */
static void tratar_llamsis(){
	int nserv, res;

	nserv=leer_registro(0);
	if (nserv<NSERVICIOS)
		res=(tabla_servicios[nserv].fservicio)();
	else
		res=-1;		/* servicio no existente */
	escribir_registro(0,res);
	return;
}

/*
 * Tratamiento de interrupciones software
 */
static void int_sw(){
	//NUEVO_INI
	//Fijamos el nivel del vector de interrupciones a 1 (interrupcion de software)
	int n_interrupcion = fijar_nivel_int(NIVEL_1);
	//printk("-> TRATANDO INT. SW\n");

	//Si el proceso actual es el proceso a expulsar (ya que se ha quedado sin tiempo de ejecucion 
	//en el algoritmo de orden de procesos Round Robin)
	if (p_proc_actual == procesoAExpulsar)
	{
		//Marcamos el proceso actual
		BCPptr actual = p_proc_actual;
		//Fijamos el nivel de interrupcion en 3 (interrupcion de reloj)
		int n_interrupcion = fijar_nivel_int(NIVEL_3);
		//Eliminamos el primer proceso de la lista de procesos listos (era el actual ejecutandose)
		eliminar_primero(&listaProcesosListos);
		//Le añadimos al final de la lista
		insertar_ultimo(&listaProcesosListos, actual);
		//Pedimos al planificador un nuevo proceso a marca como el actual
		p_proc_actual = planificador();
		//Fijamos el nivel de la interrupcion
		fijar_nivel_int(n_interrupcion);
		//Cambiamos de contexto (de modo KERNEL a modo USUARIO)
		cambio_contexto(&(actual->contexto_regs), &(p_proc_actual->contexto_regs));
	}
	//Fijamos el nivel de la interrupcion
	fijar_nivel_int(n_interrupcion);
	//NUEVO_FIN
	return;
}

/*
 *
 * Funcion auxiliar que crea un proceso reservando sus recursos.
 * Usada por llamada crear_proceso.
 *
 */
static int crear_tarea(char *prog){
	void * imagen, *pc_inicial;
	int error=0;
	int proc;
	BCP *p_proc;

	proc=buscar_BCP_libre();
	if (proc==-1)
		return -1;	/* no hay entrada libre */

	/* A rellenar el BCP ... */
	p_proc=&(tabla_procs[proc]);

	/* crea la imagen de memoria leyendo ejecutable */
	imagen=crear_imagen(prog, &pc_inicial);
	if (imagen)
	{
		p_proc->info_mem=imagen;
		p_proc->pila=crear_pila(TAM_PILA);
		fijar_contexto_ini(p_proc->info_mem, p_proc->pila, TAM_PILA,
			pc_inicial,
			&(p_proc->contexto_regs));
		p_proc->id=proc;
		p_proc->estado=LISTO;

		// modificado
		for(int i = 0; i< NUM_MUT_PROC; i++){
			p_proc->descriptores[i] = -1;
		}
		p_proc->n_descriptores = 0;

		/* lo inserta al final de cola de listos */
		insertar_ultimo(&listaProcesosListos, p_proc);
		error= 0;
	}
	else
		error= -1; /* fallo al crear imagen */

	return error;
}

/*
 *
 * Rutinas que llevan a cabo las llamadas al sistema
 *	sis_crear_proceso sis_escribir
 *
 */

/*
 * Tratamiento de llamada al sistema crear_proceso. Llama a la
 * funcion auxiliar crear_tarea sis_terminar_proceso
 */
int sis_crear_proceso(){
	char *prog;
	int res;

	printk("-> PROC %d: CREAR PROCESO\n", p_proc_actual->id);
	prog=(char *)leer_registro(1);
	res=crear_tarea(prog);
	return res;
}

/*
 * Tratamiento de llamada al sistema escribir. Llama simplemente a la
 * funcion de apoyo escribir_ker
 */
int sis_escribir()
{
	char *texto;
	unsigned int longi;

	texto=(char *)leer_registro(1);
	longi=(unsigned int)leer_registro(2);

	escribir_ker(texto, longi);
	return 0;
}

/*
 * Tratamiento de llamada al sistema terminar_proceso. Llama a la
 * funcion auxiliar liberar_proceso
 */
int sis_terminar_proceso(){

	printk("-> FIN PROCESO %d\n", p_proc_actual->id);

	liberar_proceso();

        return 0; /* no deber�a llegar aqui */
}

int obtener_id_pr(){
	return p_proc_actual->id;
}

//NUEVO_INI
int dormir(unsigned int segundos){
	//Leemos el parametro pasado a la funcion usando el lector de registros del procesador
	unsigned int seg_registro = (unsigned int)leer_registro(1);

	//Fijamos el nivel de interrupcion
	int n_interrupcion = fijar_nivel_int(NIVEL_3);
	//Marcamos como actual el proeso actual
	BCPptr actual = p_proc_actual;

	//Marcamos el proceso actual como BLOQUEADO
	actual->estado = BLOQUEADO;
	//Mandamos el proceso a dormir tantos segundos recibidos por parametro por ticks fijados en la rodaja del Round Robin
	actual->segundos_dormir = seg_registro * TICK;


	//Eliminamos el proceso actual de la lista de listos
	eliminar_elem(&listaProcesosListos, actual);
	//Añadimos al final de la lista de bloqueados el proceso actual
	insertar_ultimo(&listaProcesosBloqueados, actual);

	//Pedimos al planificador el nuevo proceso a ejecutar
	p_proc_actual = planificador();

	//Restauramos el nivel de interrupcion
	fijar_nivel_int(n_interrupcion);
	//Cambiamos de contexto (de modo KERNEL a modo USUARIO)
	cambio_contexto(&(actual->contexto_regs), &(p_proc_actual->contexto_regs));

	printk("El proceso %d se despierta\n", p_proc_actual->id);
	return 0;
}
//NUEVO_FIN

//NUEVO_INI
void reducirDormir(){
	//Accedemos al primer elemento de la lista de bloqueados
	BCPptr aux = listaProcesosBloqueados.primero;
	//Mientras aux no sea NULL
	while(aux != NULL){
		//Cogemos el siguiente al actual
		BCPptr siguiente = aux->siguiente;
		//Reducimos en una unidad los segundo que el proceso aux debe dormir
		aux->segundos_dormir--;
		//Si los segundos son 0 o menos
		if(aux->segundos_dormir <=0){
			//Lo marcamos como LISTO
			aux->estado = LISTO;
			//Lo eliminamos de la lista de procesos bloqueados
			eliminar_elem(&listaProcesosBloqueados, aux);
			//Lo añadimos a la lista de procesos listos
			insertar_ultimo(&listaProcesosListos, aux);
		}
		//Marcamos aux como el siguiente para seguir recorriendo la lista de procesos
		aux = siguiente;
	}
	return;
}
//NUEVO_FIN

//NUEVO_INI
int buscarMutexPorNombre(char* nombre){
	//Buscamos el nombre recibido por parametro en la lista de mutex
	for(int i = 0;i < NUM_MUT;i++){
		if(strcmp(listaMutex[i].nombre, nombre) == 0){
			return i; //Si se encuentra, se devuelve su indice
		}
	}
	return -1;
}
//NUEVO_FIN

//NUEVO_INI
//Funcion que busca un mutex libre
int buscarHuecoLibre(){
	int i = 0;
	for(i = 0;i < NUM_MUT;i++){
		//Si el mutex está marcado como LIBRE se devuelve su indice
		if(listaMutex[i].ocupado == LIBRE) return i;
	}
	return -1;
}
//NUEVO_FIN

//NUEVO_INI
//Funcion que crea un MUTEX
int crear_mutex(char* nombre, int tipo){
	printk("Creando mutex\n");

	//Leemos el primer parametro usando el lector de registros
	char* nom = (char*)leer_registro(1);
	//Leemos el segundo parametro usando el lector de registros
	int t = (int) leer_registro(2);

	//Fijamos el nivel de interrupcion en 1 (interrupcion de software)
	int n_interrupcion = fijar_nivel_int(NIVEL_1);

	printk("Comprobando si el nombre del mutex es mas largo de 8 caracteres\n");
	//Si el nombre del mutex es mas largo de 8 caracteres
	if(strlen(nom) > (MAX_NOM_MUT-1)){
		//Se elimina el ultimo caracter
		nom[MAX_NOM_MUT] = '\0';
	}

	//Creamos una variable mutex
	mutex m;

	//Si buscar un mutex devuelve error, creamos uno nuevo
	if(buscarMutexPorNombre(nom) == -1){
		printk("No existe un mutex con ese nombre. Creando nuevo mutex\n");
		int pos = buscarHuecoLibre();
		//Si hay un hueco libre, creamos un mutex nuevo
		if(pos >= 0){
			printk("Creando mutex\n");
			//Inicializamos el mutex
			//Copiamos el nombre recibido al nombre del mutex
			strcpy(m.nombre, nom);
			//Ponemos el mutex como ocupado
			m.ocupado = OCUPADO;
			//Marcamos si es recursivo o no
			m.recursivo = t;
			//Inicializamos diversas variables del mutex
			m.contadorProcesosEnEspera = 0;
			m.procesoEnUso = NULL;
			m.bloqueos = 0;

			//Guardamos el mutex en la lista de mutex
			listaMutex[pos] = m;
			mutexTotales++;

			//Recuperamos el indice del mutex en la lista de mutex segun su nombre
			int desc = abrir_mutex(nom);
			//Restauramos el nivel de interrupcion
			fijar_nivel_int(n_interrupcion);
			return desc;
		} else {
			//Si no quedan mutex el proceso se bloquea
			printk("No hay mutex libres, proceso bloqueado\n");

			//Se fija como actual el proceso ejecutandose actualmente
			BCPptr aux = p_proc_actual;

			//Cambiamos su estado a BLOQUEADO
			aux->estado = BLOQUEADO;
			//Eliminamos el proceso actual de la lista de procesos listos
			eliminar_elem(&listaProcesosListos, aux);
			//Insetamos el proceso actual de la lista de procesos esperando para mutex
			insertar_ultimo(&listaProcesosEsperandoMutex, aux);

			//Pedimos al planificador un nuevo proceso a ejecutar
			p_proc_actual = planificador();

			//Restauramos el nivel de interrupcion
			fijar_nivel_int(n_interrupcion);

			//Cambiamos de contexto (de modo KERNEL a modo USUARIO)
			cambio_contexto(&(aux->contexto_regs), &(p_proc_actual->contexto_regs));

			return -1;
		}
	} else {
		//Si el nombre del mutex a crear ya existe se devuelve un error
		printk("Nombre no disponible, ya está en uso\n");
		fijar_nivel_int(n_interrupcion);
		return -1;
	}
	return 0;
}
//NUEVO_FIN

//NUEVO_INI
//Funcion que abre un mutex ya creado
int abrir_mutex(char* nombre){
	printk("Abriendo mutex\n");
	//Guardamos el parametro recibido usando la funcion leer_registro
	char* nom = (char*) leer_registro(1);

	//Fijamos el nivel de interrupcion en 1 (interrupcion de software)
	int n_interrupcion = fijar_nivel_int(NIVEL_1);

	//Si el numero de descriptores usados por el proceso actual es menor que el numero de descriptores maximo por proceso
	if(p_proc_actual->n_descriptores < NUM_MUT_PROC){
		//Obtenemos el indice del mutex en la lista del mutex segun el nombre
		int desc = buscarMutexPorNombre(nom);
		//Si el indice es mayor o igual de 0
		if(desc >= 0){
			printk("Mutex encontrado\n");
			//Guardamos al final de la lista de descriptores del proceso el indice del mutex libre
			p_proc_actual->descriptores[p_proc_actual->n_descriptores] = desc;
			//Sumamos en uno la variable que almacena cuantos descriptores tenemos
			p_proc_actual->n_descriptores++;
			//Restauramos el nivel de interrupcion
			fijar_nivel_int(n_interrupcion);
			//Devolvemos el indice del mutex
			return desc;
		} else {
			//Si el mutex no existe, error
			printk("No existe el mutex\n");
			fijar_nivel_int(n_interrupcion);
			return -1;
		}
	} else {
		//Si el proceso ya tiene el maximo de descriptores por proceso, error
		printk("No quedan descriptores disponibles, el proceso ha llegado al maximo de descriptores por proceso permitido\n");
		fijar_nivel_int(n_interrupcion);
		return -1;
	}
}
//NUEVO_FIN

//NUEVO_INI
//Funcion que bloquea el mutex de un proceso
int lock(unsigned int mutexid){
	printk("LOCK init\n");
	//Leemos el parametro usando la funcion leer_registro
	unsigned int id = (unsigned int) leer_registro(1);
	//Fijamos el nivel de interrupcion en 1 (interrupcion de software)
	int n_interrupcion = fijar_nivel_int(NIVEL_1);

	int encontrado = -1;
	int i = 0;
	for(i = 0;i < NUM_MUT_PROC;i++){
		//Si se encuentra el id del mutex pasado por parametro
		if(p_proc_actual->descriptores[i] == id) encontrado = 0;
	}
	//Si no se encuentra el id del mutex, eror
	if(encontrado == -1){
		printk("El mutex no existe\n");
		fijar_nivel_int(n_interrupcion);
		return -1;
	}

	//Mientras el proceso que está usando el mutex no sea nulo ni sea el proceso actual
	while(listaMutex[id].procesoEnUso != NULL && listaMutex[id].procesoEnUso != p_proc_actual){
		//Bloqueamos el proceso
		p_proc_actual->estado = BLOQUEADO;
		//Marcamos la interrupcion de nivel 3 (interrupcion de reloj)
		int n_interrupcion2 = fijar_nivel_int(NIVEL_3);
		//Eliminamos el proceso de la lista de listos
		eliminar_elem(&listaProcesosListos, p_proc_actual);
		//Añadimos el proceso a la lista de procesos esperando mutex
		insertar_ultimo(&listaMutex[id].procesosEnEspera,p_proc_actual);
		//Aumentamos el contador de procesos esperando mutex
		listaMutex[id].contadorProcesosEnEspera++;
		//Restauramos el nivel de interrupcion
		fijar_nivel_int(n_interrupcion2);

		//Marcamos aux como el proceso actual
		BCPptr aux = p_proc_actual;
		//Pedimos al planificador un nuevo proceso que marcar como actual
		p_proc_actual = planificador();
		//Cambiamos de contexto (de modo KERNEL a modo USUARIO)
		cambio_contexto(&(aux->contexto_regs), &(p_proc_actual->contexto_regs));
	}
	//Marcamos el proceso bloqueado por el mutex como el actual
	listaMutex[id].procesoEnUso = p_proc_actual;

	//Si el mutex solo tiene un bloqueo y es NO RECURSIVO, error
	if(listaMutex[id].bloqueos == 1 && listaMutex[id].recursivo == NO_RECURSIVO){
		printk("El mutex no es recursivo\n");
		return -2;
	}
	//Aumentamos el contador de bloqueos en una unidad
	listaMutex[id].bloqueos++;

	printk("LOCK finish\n");
	//Restauramos el nivel de interrupcion
	fijar_nivel_int(n_interrupcion);
	return 0;
}
//NUEVO_FIN

//NUEVO_INI
//Funcion que desbloquea un mutex
int unlock(unsigned int mutexid){
	printk("UNLOCK init\n");
	//Obtenemos el parametro por la funcion leer_registro
	unsigned int id = leer_registro(1);
	//Fijamos el nivel de interrupcion en 1 (interrupcion de software)
	int n_interrupcion = fijar_nivel_int(NIVEL_1);

	int i = 0;
	int encontrado = -1;
	for(i = 0;i < NUM_MUT_PROC;i++){
		//Si se encuentra el id del mutex pasado por parametro
		if(p_proc_actual->descriptores[i] == id){
			encontrado = 0;
		}
	}

	//Si no se encuentra el id del mutex, error
	if(encontrado == -1){ // el mutex no existe
	 	printk("El mutex no se encuentra en la lista de descriptores\n");
	 	fijar_nivel_int(n_interrupcion);
	 	return -1;
	}

	//Si el proceso que usa el mutex del indice no es el proceso actual, error
	if(listaMutex[id].procesoEnUso != p_proc_actual) {
		printk("El mutex no corresponde con el proceso actual\n");
	 	fijar_nivel_int(n_interrupcion);
		return -2;
	}

	//Reducimos en una unidad el numero de bloqueos
	listaMutex[id].bloqueos--;
	//Si el numero de bloqueos es distinto a 0
	if(listaMutex[id].bloqueos != 0){
		printk("El mutex es recursivo\nUnlock\n");
		//Restauramos el nivel de interrupcion
	 	fijar_nivel_int(n_interrupcion);
		return 0;
	}
	//Marcamos el proceso del mutex como NULO
	listaMutex[id].procesoEnUso = NULL;

	//Si el primer proceso de la lista de procesos esperando mutex es nulo no hay procesos queriendo obtener mutex
	if(listaMutex[id].procesosEnEspera.primero == NULL){
		printk("Mutex libre\n");
	 	fijar_nivel_int(n_interrupcion);
		return -3;
	}

	//Marcamos el primer proceso en la lista de procesos esperando como LISTO
	printk("El proceso actual está el primero en la cola de espera\n");
	BCPptr aux = listaMutex[id].procesosEnEspera.primero;
	aux->estado = LISTO;

	//Fijamos el nivel de interrupcion como 3 (interrupcion de reloj)
	int n_interrupcion2 = fijar_nivel_int(NIVEL_3);
	//Eliminamos el proceso de la lista de procesos esperando mutex
	eliminar_primero(&listaMutex[id].procesosEnEspera);
	//Añadimos el proceso a la lista de listos
	insertar_ultimo(&listaProcesosListos, aux);
	//Restauramos el nivel de interrupcion
	fijar_nivel_int(n_interrupcion2);

	//Marcamos al proceso aux como el proceso usando el mutex actual
	listaMutex[id].procesoEnUso = aux;

	printk("UNLOCK finish\n");
	//Restauramos el nivel de interrupcion
	fijar_nivel_int(n_interrupcion);
	return 0;
}
//NUEVO_FIN

//NUEVO_INI
//Funcion que libera todos los procesos bloqueados desbloqueandolos
int liberarMutexRecursivo(mutex* m){
	printk("Liberando procesos bloqueados en el mutex %s\n", m->nombre);
	//Obtenemos el primer proceso bloqueado
	BCPptr actual = m->procesosEnEspera.primero;
	BCPptr siguiente = NULL;
	//Mientras el primer proceso no sea NULO
	while(m->procesosEnEspera.primero != NULL){
		siguiente = actual->siguiente;
		//Fijamos nivel de interrupcion en 3 (interrupcion de reloj)
		int n_interrupcion = fijar_nivel_int(NIVEL_3);
		//Eliminamos el proceso de la lista de procesos esperando mutex
		eliminar_primero(&m->procesosEnEspera);
		//Metemos el proceso en la lista de listos
		insertar_ultimo(&listaProcesosListos, siguiente);
		//Le marcamos como LISTO
		actual->estado = LISTO;
		//Restauramos el nivel de interrupcion
		fijar_nivel_int(n_interrupcion);
		//Recorremos la lista
		actual = siguiente;
	}
	return m->procesosEnEspera.primero == NULL ?  1 :  -1;
}
//NUEVO_FIN

//NUEVO_INI
//Funcion que sirve para cerrar un mutex
int cerrar_mutex(unsigned int mutexid){
	printk("Cerrando mutex\n");
	//Obtenemos el parametro por la funcion leer_registro
	unsigned int id = (unsigned int) leer_registro(1);
	//Fijamos nivel de interrupcion en 1 (interrupcion de software)
	int n_interrupcion = fijar_nivel_int(NIVEL_1);
	int i = 0;
	int encontrado = -1;
	//Si encontramos el id del descriptor del proceso actual
	for (i = 0; i < NUM_MUT_PROC; i++){
		if(p_proc_actual->descriptores[i] == id) encontrado = 1;
	}
	//Si no encontramos el id del descriptor del proceso actual, error
	if(encontrado == -1){
		printk("El descriptor no existe en el proceso actual\n");
		fijar_nivel_int(n_interrupcion);
		return -1;
	}

	//Mutex temporal que guarda el mutex relacionado con su id
	mutex* m = &listaMutex[id];

	//Si el proceso del mutex con el mismo id tiene el proceso actual
	if(m->procesoEnUso == p_proc_actual){
		//Marcamos el proceso del mutex como NULO
		m->procesoEnUso = NULL;
		//Dejamos en 0 los procesos esperando
		m->contadorProcesosEnEspera = 0;
	}
	//Reducimos en una unidad los descriptores en uso del proceso actual
	p_proc_actual->n_descriptores--;
	//Le marcamos con -1
	p_proc_actual->descriptores[i] = -1;
	//Marcamos el mutex como LIBRE
	m->ocupado = LIBRE;
	//Reducimos en una unidad el contador de mutex en uso
	mutexTotales--;

	//Si el mutex es recursivo, libero todos los bloqueos
	if(m->recursivo == RECURSIVO){
		if(liberarMutexRecursivo(m) == -1){
			printk("Error al liberar todos los procesos bloqueados por el mutex\n");
			return -2;
		}
	}

	m->bloqueos = 0;
	//Si el primer proceso de la lista de procesos esperando mutex no es NULO
	if(listaProcesosEsperandoMutex.primero != NULL){
		printk("Hay procesos en espera para crear un mutex\n");
		BCPptr proceso = listaProcesosEsperandoMutex.primero;
		//Fijamos nivel de interrupcion en 3 (interrupcion de reloj)
		int n_interrupcion2 = fijar_nivel_int(NIVEL_3);
		//Eliminamos el proceso actual de la lista de procesos esperando mutex
		eliminar_primero(&listaProcesosEsperandoMutex);
		//Le metemos en la lista de procesos listos
		insertar_ultimo(&listaProcesosListos, proceso);
		//Le marcamos como LISTO
		proceso->estado = LISTO;
		//Restauramos el nivel de interrupcion
		fijar_nivel_int(n_interrupcion2);
	}
	printk("Mutex cerrado\n");
	//Restauramos el nivel de interrupcion
	fijar_nivel_int(n_interrupcion);
	return 0;
}
//NUEVO_FIN
/*
 *
 * Rutina de inicializaci�n invocada en arranque
 *
 */
int main(){
	/* se llega con las interrupciones prohibidas */

	instal_man_int(EXC_ARITM, exc_arit);
	instal_man_int(EXC_MEM, exc_mem);
	instal_man_int(INT_RELOJ, int_reloj);
	//instal_man_int(INT_TERMINAL, int_terminal);
	instal_man_int(LLAM_SIS, tratar_llamsis);
	instal_man_int(INT_SW, int_sw);

	iniciar_cont_int();		/* inicia cont. interr. */
	iniciar_cont_reloj(TICK);	/* fija frecuencia del reloj */
	iniciar_cont_teclado();		/* inici cont. teclado */

	iniciar_tabla_proc();		/* inicia BCPs de tabla de procesos */

	//NUEVO_INI
	//Bucle que marca todos los mutex como LIBRE
	for(int i = 0;i < NUM_MUT;i++) listaMutex[i].ocupado = LIBRE;
	//NUEVO_FIN
	/* crea proceso inicial */
	if (crear_tarea((void *)"init")<0)
		panico("no encontrado el proceso inicial");

	/* activa proceso inicial */
	p_proc_actual=planificador();
	cambio_contexto(NULL, &(p_proc_actual->contexto_regs));
	panico("S.O. reactivado inesperadamente");
	return 0;
}
