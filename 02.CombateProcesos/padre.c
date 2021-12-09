// Este archivo es el fichero fuente que al compilarse produce el ejecutable PADRE.

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/msg.h>

int main(int argc, char *argv[]){

    // Variables
    key_t llave;
    int msqid, shmid,semid;
    int n = atoi(argv[1]);
    int *lista;
    int barrera[2];
    int resultadoSem;
    int pid;

    printf("Parametro: %s\n", argv[0]);


    // Se crea la llave.
    if ((llave=ftok("padre",'Y')) == -1) {
        perror("Error en ftok");
        exit(1);
    }
    printf("Llave creada: %d\n", llave);
    

    // Se crea una cola de mensajes.
    msqid = msgget(llave, IPC_CREAT | 0600);
    if (msqid == -1) {
        perror("Error semid");
        exit(2);
    }
    printf("Cola creada: %d\n", msqid);


    // Se crea una región de memoria compartida lista que enlazará con un array con capacidad para N PIDs.
    shmid = shmget(llave, n*sizeof(int), IPC_CREAT | 0600);
    if (shmid == -1) {
        perror("Error shmid");
        exit(3);
    }
    printf("Memoria compartida creada: %d\n", shmid);
    lista=shmat(shmid, 0, 0);


    // Se crea un semáforo sem que se usará para proteger el acceso a dicha variable compartida. 
    semid = semget(llave, 1, IPC_CREAT | 0600);

    if (semid == -1) {
        perror("Error semget"); 
        exit(3);
    }
    printf("Semãforo creado: %d\n", semid);

    resultadoSem = semctl(semid, 0, SETVAL, 0);
    
    if (resultadoSem == -1) {
        perror("Error semctl"); 
        exit(3);
    }

    struct sembuf operaciones[2];

    operaciones[0].sem_num = 0; /*Semáforo número 0*/
    operaciones[0].sem_op = -1; /*Operación que emula P*/
    operaciones[0].sem_flg = 0;

    operaciones[1].sem_num = 0; /*Semáforo número 0*/
    operaciones[1].sem_op = 1;  /*Operación que emula a V*/
    operaciones[1].sem_flg=0;

    //semop(semid, operaciones, 1);


    // Se crea una tubería sin nombre barrera.
    if (pipe(barrera) == -1) { 
        perror("Error en pipe"); 
        exit(4); 
    }
    printf("Se crean procesos\n");


    // A continuación creará N procesos hijos (H1…HN), donde N es un parámetro que 
    // se pasada como entrada al invocarlo desde Ejecicio2.sh. Cada hijo realizará un 
    // exec() del ejecutable HIJO.
    for(int i=1; i <= n; i++){
        pid = fork();
        if (pid == -1) {
            printf("Error en la ejecución del fork");
            exit(5);
        } else if (pid == 0) {
            char *arE[1];
            printf("pid del proceso hijo = %d\n", getpid());
            execvp("./hijo", arE);

        } else {
	    /* Este código solo lo ejecuta el proceso padre */
            printf("pid del proceso padre=%d\n", getpid());
        }
    }


    // Se cierra todo.
    close(barrera[0]);
    close(barrera[1]);
    shmdt(lista);
    shmctl(shmid, IPC_RMID, 0);
    msgctl(msqid, IPC_RMID, 0);
    // Revisar si se cierran los semáforos.
}
