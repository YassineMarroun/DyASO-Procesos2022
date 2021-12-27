// Este archivo es el fichero fuente que al compilarse produce el ejecutable HIJO.

#include <signal.h>
#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <fcntl.h> 
#include <errno.h> 
#include <libgen.h> 
#include <time.h> 
#include <sys/types.h>
#include <sys/stat.h> 
#include <sys/ipc.h>  
#include <sys/msg.h>
#include <sys/shm.h> 
#include <sys/sem.h> 
#define MAX 256


char estado[MAX];
int N;
int *lista;


void defensa() {
	printf("\tEl hijo %d ha repelido un ataque \n", getpid());
	usleep(200000);			// Se duerme el proceso 0.2 segundos
	strcpy(estado, "OK");	// Se establece estado al valor OK
}


void indefenso() {
	
	// Variables
	key_t llave;		// Llave
	int memCompartida;	// Identificador de la región de memoria compartida
	int sem;			// Identificador del semaforo
	int aleatorio1;		// Variable para almacenar el número aleatorio para seleccionar el PID
	int pidAleatorio ;	// PID del proceso seleccionado aleatoriamente
	int pidProceso;		// PID del proceso 

	printf("\tEl hijo %d ha sido emboscado mientras realizaba un ataque \n", getpid());
	usleep(100000); 		// Se duerme el proceso 0.1 segundos
	strcpy(estado, "KO");	// Se establece estado al valor KO

	// Se crea la llave
	llave = ftok("padre", 'Y');
	if (llave == -1) { 
		perror("Error en ftok"); 
		exit(1);
	}

	// Se inicializan las variables de los PID de procesos
	pidAleatorio = 0;	
	pidProceso = getpid();

	
	// Se crea la región de memoria compartida
	memCompartida = shmget(llave, N*sizeof(int), IPC_CREAT | 0600);
	if (memCompartida == -1) { 
		perror("Error en shmget"); 
		exit(1);
	}
	lista = shmat(memCompartida, 0, 0);

	// Se crea el semáforo
	sem = semget(llave, 1, 0600); 
	if (sem == -1) { 
		perror("Error en semget"); 
		exit(1);
	}

	struct sembuf operP[1];		/* Operacion que emula P */
	operP[0].sem_num = 0;	
	operP[0].sem_op = -1; 	
	operP[0].sem_flg = 0;

	struct sembuf operV[1];		/* Operacion que emula V */
	operV[0].sem_num = 0;	
	operV[0].sem_op = 1; 	
	operV[0].sem_flg = 0;

	// Se selecciona un proceso aleatorio entre aquellos que están vivos
	do {
		aleatorio1 = rand() % N;
		int valor = semctl(sem, 0, GETVAL, 0);

		while (semctl(sem, 0, GETVAL, 0) <= 0) {
		} 
		semop(sem, operP, 0);

		// Se accede a la memoria compartida 
		pidAleatorio = lista[aleatorio1];

		semop(sem, operV, 0); 

		if(pidAleatorio != 0 && pidAleatorio != pidProceso) {
			printf("\tAtacando al proceso Hi - %d\n",lista[aleatorio1]);
		}
	
	} while(pidAleatorio == 0 || pidAleatorio == pidProceso);

	usleep(100000);		// Se duerme el proceso 0.1 segundos
}


int main(int argc, char *argv[]) {
	
	// Variables
	key_t llave;			// Llave
	int mensajes;			// Identificador de la cola de mensajes
	int size;				// Tamaño del mensaje a enviar
	char mensaje[26];		// Mensaje a enviar
	int aleatorio;			// Aletario para ver si ataca o defiende
	

	N = atoi(argv[1]);		// Se recupera el número de procesos vivos
	srand(getpid());		// Se gerenra semilla con el número de PID
	
	// Bucle mientras el proceso este vivo
	while(1) { 
		read(3, mensaje, 26);	// Se lee el mensaje de la tubería
		aleatorio = rand() % 2;	// Se genera un aleorio [0-1]

		if(aleatorio == 0) {
			indefenso();
		} else {
			defensa();
		}

		llave = ftok("padre", 'Y');	 // Se crea una llave asociada al propio fichero ejecutable
		if (llave == -1) { 
			perror("Error en en ftok"); 
			exit(1);
		}

		struct {
			int tipo;
			int pid;
			char estado[MAX];
		} mensajeCola;
	
		size = sizeof(mensajeCola) - sizeof(mensajeCola.tipo);
		mensajeCola.tipo = 1;
		mensajeCola.pid = getpid();
		strcpy(mensajeCola.estado, estado);
		
		mensajes = msgget(llave,IPC_CREAT | 0600);

		if (mensajes == -1) {
			perror("Error en msgget");
			exit(1);
		}

		// Se envia el mensaje a la cola 
		if(msgsnd(mensajes, &mensajeCola, size, 0) == -1) {
			printf("Error en msgsnd");
			exit(1);
		}
	}
	exit(0);
}