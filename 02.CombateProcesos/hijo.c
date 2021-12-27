// Este archivo es el fichero fuente que al compilarse produce el ejecutable HIJO.

#include <signal.h>
#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/stat.h> 
#include <fcntl.h> 
#include <signal.h> 
#include <errno.h> 
#include <sys/msg.h> 
#include <libgen.h> 
#include <sys/ipc.h> 
#include <time.h> 
#include <sys/types.h> 
#include <sys/ipc.h> 
#include <sys/shm.h> 
#include <sys/sem.h> 
#define MAX 256

char estado[MAX];
int N;
int *lista;


void defensa()
{
	printf("\tEl hijo %d ha repelido un ataque \n", getpid());
	usleep(200000);			// Dormimos el proceso 0.2 segundos
	strcpy(estado,"OK");		// Establecemos estado a OK
}

void indefenso()
{
	// Variables
	//==========
	key_t llave;		// llave
	int memoriaC;		// identificador de la zona de memoria compartida
	int ale1;		// Variable para almacenar el nº aleatorio para seleccionar el PID
	int sem;		// Identificador del semaforo
	int pidAleatorio ;	// PID del proceso seleccionado aleatoriamente
	int pidProceso;		// PID del proceso 

	printf("\tEl hijo %d ha sido emboscado mientras realizaba un ataque \n",getpid());
	usleep(100000); 	// Dormimos el proceso 0.1 segundos
	strcpy(estado,"KO");	// Establecemos estado a KO

	// LLAVE
	llave = ftok("padre",'X');
	if (llave ==-1)	 { 
		perror("Error en en ftok"); 
		exit(1);
	}

	// Inicializmos variables de los PID de los procesos
	pidAleatorio = 0;	
	pidProceso = getpid();

	
	// ZONA DE MEMORIA COMPARTIDA
	memoriaC=shmget(llave,N*sizeof(int),IPC_CREAT | 0600);
	if (memoriaC==-1) { 
		perror("Error en shmget"); 
		exit(1);
	}

	lista = shmat(memoriaC, 0, 0);

	// SEMAFORO
	sem = semget(llave, 1, 0600); 

	if (sem ==-1) { 
		perror("Errore en semget"); 
		exit(1);
	}

	struct sembuf operP[1];		/* Operacion que emula P */
	operP[0].sem_num=0;	
	operP[0].sem_op=-1; 	
	operP[0].sem_flg=0;

	struct sembuf operV[1];		/* Operacion que emula V */
	operV[0].sem_num=0;	
	operV[0].sem_op=1; 	
	operV[0].sem_flg=0;

	// De los procesos que están vivos seleccionamos uno de ellos aleatoriamente
	do{
		ale1 = rand() % N;
		int valor = semctl(sem, 0, GETVAL, 0);

		while (semctl(sem, 0, GETVAL, 0) <= 0)
		{
		} 
		semop(sem, operP, 0); 
		// Accedemos a memoria compartida 
		pidAleatorio = lista[ale1];

		semop(sem, operV, 0); 

	if(pidAleatorio !=0 && pidAleatorio !=pidProceso)
	{
		printf("\tAtacando al proceso Hi - %d\n",lista[ale1]);
		//kill(lista[ale1], SIGUSR1);
	}
	
	} while(pidAleatorio ==0 ||pidAleatorio ==pidProceso);

	usleep(100000);		// Dormimos el proceso 0.1 segundos
}


int main(int argc, char *argv[])
{
	// Variables
	//==========
	int aleatorio;			// Aletario para ver si ataca o defiende
	int mensajes;			// identificador de la cola de mensajes
	key_t llave;			// llave
	int size;			// Tamaño del mensaje a enviar
	char mensaje[26];		// Mensaje a enviar

	N = atoi(argv[1]);		// Recuperamos numero de procesos vivos
	srand(getpid());		// Generamos semilla con el número del PID
	
	// Bucle mientras el proceso este vivo
	while(1){ 
		read(3,mensaje,26);	// leemos mensaje de la tuberia
		aleatorio = rand() %2;	// Generamos aleorio [1-2]

		if(aleatorio==0){
			indefenso();
		} else{
			defensa();
		}

		llave = ftok("padre",'X');	 // Creamos llave asociada al propio fichero ejecutable
		if (llave ==-1)	 { 
			perror("Error en en ftok"); 
			exit(1);
		}

		struct {
			int tipo;
			int pid;
			char estado[MAX];
		}mensajeCola;
	
		size = sizeof(mensajeCola)- sizeof(mensajeCola.tipo);
		mensajeCola.tipo=1;
		mensajeCola.pid= getpid();
		strcpy(mensajeCola.estado,estado);
		
		mensajes = msgget(llave,IPC_CREAT | 0600);

		if (mensajes==-1) {
			perror("Error en msgget");
			exit(1);
		}

		// Enviar mensaje por cola 
		if(msgsnd(mensajes,&mensajeCola,size,0)==-1)	// Enviamos mensaje a la cola
		{
			printf("Error en msgsnd");
			exit(1);
		}
	}
	exit(0);
}