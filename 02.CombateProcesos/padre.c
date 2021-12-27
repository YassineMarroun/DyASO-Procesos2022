// Este archivo es el fichero fuente que al compilarse produce el ejecutable PADRE.

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <unistd.h>
#include <signal.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <wait.h>
#define MAX 256



int *pids;
int numRonda=0;

int main(int argc, char *argv[])
{
	// Variables
	//===========
	key_t llave;		// llave
	int mensajes;		// identificador de la cola de mensajes
	int lista; 		// identificador de la zona de memoria compartida
	int N;			// Nº de procesos iniciales
	int K; 			// Variable que controla en nº de procesos vivos 
	int sem;		// identificador del semaforo
	int barrera[2];		// Tubería sin nombre, barrera
	int i;			// Variable para bucle for
	int pidHijo;		// Nº  pid de los hijos que se crean
	int eliminados;		// Nº de hijos eliminados en cada ronda

	// Comprobamos si el nº de argumentos de entrada es correcto
	if(argc!=2) {
		perror("Error en los parámetros de entrada \n");
		exit(-1);
	}


	// 1º Creamos llave asociada al propio fichero ejecutable
	//=======================================================
	llave = ftok(argv[0],'X');
	if (llave ==-1)	 { 
		perror("Error en en ftok"); 
		exit(1);
	}

	// 2º Creamos cola de mensajes, mensajes
	//=====================================
	mensajes = msgget(llave,IPC_CREAT | 0600);
	if (mensajes==-1) {
		perror("Error en msgget");
		exit(1);
	}

	// Estructura que contiene el tupo del mensaje, el pid y el estado
	struct {
		int tipo;
		int pid;
		char estado[MAX];
	}mensaje;


	//3º Creamos region de memoria compartida, lista
	//==============================================

	N = atoi(argv[1]);		// Recuperamos el 1º argumento nº de hijos
	K = N;				// Inicialmentel nº prcesos vivos seran todos los procesos
	
	lista=shmget(llave,N*sizeof(int),IPC_CREAT | 0600);
	if (lista==-1) { 
		perror("Error en shmget"); 
		exit(1);
	}

	pids = shmat(lista, 0, 0);	// Union de la zona de memoria compartida con lista
	

	// 4º Creamos semáforo, sem
	//=========================
	sem= semget(llave, 1, IPC_CREAT| 0600);
	if (sem ==-1) { 
		perror("Error en semget\n"); 
		exit(1);
	}
	
	semctl(sem, 0, SETVAL, 0);		// Incializamos valor del semanaforo
 
	struct sembuf operP[1];			/* Operacion que emula P */
	operP[0].sem_num=0;	
	operP[0].sem_op=-1; 	
	operP[0].sem_flg=0;

	struct sembuf operV[1];			/* Operacion que emula V */
	operV[0].sem_num=0;	
	operV[0].sem_op=1; 	
	operV[0].sem_flg=0;
	
	semop(sem, operV, 1); 			// Operación V para que tenga el valor 1


	// 5º Creamos tuberia sin nombre, barrera
	//=========================
	if(pipe(barrera)==-1) {
		perror("Error al crear la tubería \n");
		exit(-1);
	}
	
	

	// 6º Se crean N procesos hijos
	//=============================
	printf("\nINICIO: se crean los  siguientes procesos\n");
	printf("=========================================\n");
	for(i=0;i<N;i++)
	{
		pidHijo = fork();
		if(pidHijo==-1) {
			printf("Error al crear el proceso \n");
			exit(-1);
		} else if (pidHijo !=0){
			// Se ejecuta el padre
			printf("Hijo %d: %d\n",(i+1),pidHijo);
			semop(sem, operP, 1);				// OperacionP para acceder a zona de memoria compartida
			pids[i]=pidHijo;				// Guardamos PID del hijo en el array
			semop(sem, operV, 1);				// OperacionV al salir de la zona de memoria compartida			

		} else {
			// Se ejecuta el hijo
			char *argHijo[]={"./hijo",argv[1],NULL};	// Como argumento nº total de hijos	
			execvp("./hijo",argHijo);			// Hijo realiza exec del ejecutable hijo
		}
	}


	numRonda =1;	// Inicializamos variable ronda

	// 7º Inicio de ronda
	//===================
	do{
		printf("\nRONDA Nº %d \n",numRonda);
		printf("==========\n");		
		char mensajeInicio[] = "Iniciando ronda de ataques"; 

		// Por cada hijo que quede vivo, escribimos en barrera
		for(i=0;i<K;i++)
		{
			if ( (write(barrera[1],mensajeInicio,strlen(mensajeInicio))) <0) {
				printf("Error en tubería \n");
				exit(1);
			}	
		}


		int tam = sizeof(mensaje)- sizeof(mensaje.tipo);
		//int tam
		eliminados=0;
		for(i=0;i<K;i++)
		{
			msgrcv(mensajes,&mensaje,tam,1,0);
			printf("PROCESO: %d - ESTADO %s \n",mensaje.pid,mensaje.estado);

			// Si el mensaje es KO, se envia señal SIGTERM
			if(strcmp(mensaje.estado,"KO")==0)
			{
		
				if( kill(mensaje.pid,SIGTERM) ==-1) {
					printf("Error enSIGTERM \n");
					exit(1);
				}
				eliminados = eliminados + 1;	// Decrementamos nº de procesos hijos vivos
				wait(&pidHijo);  // Esperamos a que finalice
	
				// Accedemos a la zona de memoria compartida y ponemos su pid a 0

				semctl(sem, 0, GETVAL, 0);	// obtenemos valor del semaforo
	
				while (semctl(sem, 0, GETVAL, 0) <= 0)	// Quedamos a la espera hasta que se puede acceder a la zona de memoria compartida
				{
				} 
				semop(sem, operP, 0); 
				// Accedemos a memoria compartida 
				int x;
	
				for(x=0;x<N;x++)
				{
					if(pids[x]==mensaje.pid)
					{
						
						pids[x]=0;
						break;
					}
				}
				semop(sem, operV, 0);

				}

		}
		K = K - eliminados;
		numRonda++;
		printf("\n-> Resumen ronda: \n");
		printf("\tProcesos eliminados: %d \n",eliminados);

		printf("\tProcesos vivos: %d\n",K);
		
		if(K==0)
		{
			break;
		}

	}while(K>1);

	printf("\n\n********************\n");
	printf("* Fin del combate  *\n");
	printf("********************\n");

	if(K==0)
	{
		printf("Empate\n");
	} else {
		// Matar el procesos pendientes
		for(i=0;i<N;i++)
		{
			if(pids[i]!=0)
			{
				//printf("Hijo %d ha terminado \n",i+1);
				// Enviamos señal de terminacion
				if( kill(pids[i],SIGTERM) ==-1)
				{
					printf("Error al enviar señal SIGTERM \n");
					exit(-1);
				}
				printf("El hijo %d ha ganado\n\n",pids[i]);
				wait(&pidHijo);
				break;
			}
		}
	}

	// Cerramos tuberias y borramos zona de memoria compartida, cola y semaforo
	close(barrera[0]);
	close(barrera[1]);
	shmdt(pids);
	shmctl(lista, IPC_RMID,0);
	msgctl(mensajes,IPC_RMID,0);
	msgctl(sem,IPC_RMID,0);

	exit(0);
}