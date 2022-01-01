// Este archivo es el fichero fuente que al compilarse produce el ejecutable PADRE.

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <wait.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/msg.h>
#define MAX 256


int *pids;
int numRonda;

int main(int argc, char *argv[]) {

	// Variables
	key_t llave;		// Llave
	int mensajes;		// Identificador de la cola de mensajes
	int lista; 			// Identificador de la región de memoria compartida
	int N;				// Número de procesos iniciales
	int K; 				// Variable que controla en número de procesos vivos 
	int sem;			// Identificador del semáforo
	int barrera[2];		// Tubería sin nombre, barrera
	int i;				// Variable para bucle for
	int pidHijo;		// Número PID de los hijos que se crean
	int eliminados;		// Número de hijos eliminados en cada ronda


	// Se comprueba si el número de argumentos de entrada es correcto
	if(argc != 2) {
		perror("Error en los parámetros de entrada");
		exit(-1);
	}


	// 1. Se crea la llave asociada al propio fichero ejecutable
	llave = ftok(argv[0], 'Y');
	if (llave == -1) { 
		perror("Error en en ftok"); 
		exit(1);
	}


	// 2. Se crea la cola de mensajes, mensajes
	mensajes = msgget(llave, IPC_CREAT | 0600);
	if (mensajes == -1) {
		perror("Error en msgget");
		exit(1);
	}


	// Estructura que contiene el tipo del mensaje, el PID y el estado
	struct {
		int tipo;
		int pid;
		char estado[MAX];
	} mensaje;


	// 3. Se crea la región de memoria compartida, lista
	N = atoi(argv[1]);		// Se recupera el número de hijos
	K = N;					// De inicio, el número de procesos vivos son todos los procesos
	
	lista = shmget(llave, N*sizeof(int), IPC_CREAT | 0600);
	if (lista == -1) { 
		perror("Error en shmget"); 
		exit(1);
	}

	pids = shmat(lista, 0, 0);	// Unión de la región de memoria compartida con lista
	

	// 4. Se crea el semáforo, sem
	sem = semget(llave, 1, IPC_CREAT | 0600);
	if (sem == -1) { 
		perror("Error en semget"); 
		exit(1);
	}
	
	semctl(sem, 0, SETVAL, 0);		// Se inicializa el valor del semáforo
 
	struct sembuf operP[1];			/* Operacion que emula P */
	operP[0].sem_num = 0;	
	operP[0].sem_op = -1; 	
	operP[0].sem_flg = 0;

	struct sembuf operV[1];			/* Operacion que emula V */
	operV[0].sem_num = 0;	
	operV[0].sem_op = 1; 	
	operV[0].sem_flg = 0;
	
	semop(sem, operV, 1); 			// Operación V para que tenga el valor 1


	// 5. Se crea la tubería sin nombre, barrera
	if(pipe(barrera) == -1) {
		perror("Error al crear la tubería");
		exit(-1);
	}
	
	
	// 6. Se crean N procesos hijos
	printf("\nINICIO: Creación de 10 procesos\n");
	printf("-------------------------------\n");
	for(i = 0; i < N; i++) {
		pidHijo = fork();
		if(pidHijo == -1) {
			printf("Error al crear el proceso");
			exit(-1);
		} else if (pidHijo != 0) {
			// Se ejecuta el padre
			printf("Hijo %d: %d\n", (i + 1), pidHijo);
			semop(sem, operP, 1);			// Operación P para acceder a la región de memoria compartida
			pids[i] = pidHijo;				// Se guarda el PID del hijo en el array
			semop(sem, operV, 1);			// Operación V al salir de la región de memoria compartida			

		} else {
			// Se ejecuta el hijo
			char *argHijo[] = {"./hijo", argv[1], NULL};	// Se pasa como argumento el número total de hijos	
			execvp("./hijo", argHijo);						// Hijo realiza exec sobre el ejecutable hijo
		}
	}

	numRonda = 1;	// Se inicializa la variable ronda


	// 7. Inicio de ronda
	do{
		printf("\nRonda número %d \n", numRonda);
		printf("--------------\n");		
		char mensajeInicio[] = "Iniciando ronda de ataques"; 

		// Por cada hijo que quede vivo, se escribe en barrera
		for(i = 0; i < K; i++) {
			if ((write(barrera[1], mensajeInicio, strlen(mensajeInicio))) < 0) {
				printf("Error en tubería");
				exit(1);
			}	
		}

		int tam = sizeof(mensaje) - sizeof(mensaje.tipo);
		eliminados = 0;
		for(i = 0; i < K; i++) {
			msgrcv(mensajes, &mensaje, tam, 1, 0);
			printf("PROCESO: %d - ESTADO %s \n", mensaje.pid, mensaje.estado);

			// Si el mensaje es KO, se envia señal SIGTERM
			if(strcmp(mensaje.estado, "KO") == 0) {

				if(kill(mensaje.pid, SIGTERM) == -1) {
					printf("Error en SIGTERM");
					exit(1);
				}
				eliminados = eliminados + 1;	// Se decrementa el número de procesos hijos vivos
				wait(&pidHijo);  				// Se espera a que finalice
				semctl(sem, 0, GETVAL, 0);		// Se obtiene el valor del semáforo
	
				while (semctl(sem, 0, GETVAL, 0) <= 0) {	// Se queda a la espera hasta que se puede acceder a la región de memoria compartida
				} 
				semop(sem, operP, 0); 
				// Se accede a memoria compartida 
				for(int j = 0; j < N; j++) {
					if(pids[j] == mensaje.pid) {
		
						pids[j] = 0;
						break;
					}
				}
				semop(sem, operV, 0);
			}
		}
		K = K - eliminados;
		numRonda++;
		printf("\nResumen ronda: \n");
		printf("\tProcesos eliminados: %d \n", eliminados);
		printf("\tProcesos vivos: %d\n", K);
		
		if(K == 0) {
			break;
		}
	} while(K > 1);

	printf("\n\n---------------------\n");
	printf("-- Fin del combate --\n");
	printf("---------------------\n");

	if(K == 0) {
		char mensajeEmpate[] = ("Empate\n\n");
		write(1, mensajeEmpate, sizeof(mensajeEmpate));
	} else {
		// Se matan los procesos pendientes
		for(i = 0; i < N; i++) {
			if(pids[i] != 0) {
				// Se envía señal de terminación
				if(kill(pids[i], SIGTERM) == -1) {
					printf("Error al enviar señal SIGTERM");
					exit(-1);
				}
				char mensajeGanador[25];
				sprintf(mensajeGanador, "El hijo %d ha ganado\n\n", pids[i]);
				write(1, mensajeGanador, sizeof(mensajeGanador));
				wait(&pidHijo);
				break;
			}
		}
	}

	// Se cierra la tubería y se eliminan la región de memoria compartida, la cola y el semáforo
	close(barrera[0]);
	close(barrera[1]);
	shmdt(pids);
	shmctl(lista, IPC_RMID, 0);
	msgctl(mensajes, IPC_RMID, 0);
	msgctl(sem, IPC_RMID, 0);
	exit(0);
}