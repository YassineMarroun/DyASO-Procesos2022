#include <sys/types.h>
#include <sys/ipc.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <string.h>
#include <sys/msg.h>

int main(int argc, char *argv[]){

    // Variables
    key_t llave;
    int msqid, shmid, semid;
    int n = atoi(argv[1]));
    int *lista;
    int barrera[2];



    argv[0];

    // Se crea la llave
    if((llave=ftok("argv[0]", 'Y'))==-1)
    {
        perror("Error en ftok");
        exit(1);
    }

    // Se crea una cola de mensajes
    msqid = mssget(llave, IPC_CREAT | 0600);
    if (msqid == -1)
    {
        perror("Error semid");
        exit(2);
    }

    // Se crea una región de memoria compartida lista (que enlazará con un array con capacidad para N PIDs)
    shmid = shmget(llave, n*sizeof(int), IPC_REPEAT | 0600);
    if (shmid == -1)
    {
        perror("Error")
    }


    resultado = semctl(semid, )
    struct sembuf operaciones[2];
    {
        /* data */
    };

    // Tubería sin nombre barrera.
    if(pipe(barrera) == -1)
    {
        perror("Error en pipe");
        exit(4);
    }


    for(int i=1; i <= n; i++) {
        pid = fork();
        if (pid == -1)
        {
            printf("Error en la ejecución")
        }
    }


    // Se cierra todo
    close(barrera[0]);
    close(barrera[1]);
    shmdt(lista);
    shmctl(shmid, IPC_RMID, 0);
    msgctl(msqid, IPC_RMID, 0);

    // Revisar si se cierran los sem´foros
    
}