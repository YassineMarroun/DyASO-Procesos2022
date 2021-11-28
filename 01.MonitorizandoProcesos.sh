####################################################################
# Yassine Marroun Nettah - ymarroun1@alumno.uned.es
# Trabajo Práctico de Diseño y Administraciõn de Sistemas Operativos
# TRABAJO 1: Monitorizando procesos
####################################################################

#!/bin/bash

# Se resetea el archivo de salida donde se almacenan los datos obtenidos.
rm -f archivoSalida.txt

# Variable donde se guarda el número total de procesos.
totalPIDS=`ls /proc | grep [0-9] | wc -l`

# Vector donde se guardan en orden ascendente los procesos.
PIDS=($(ls / proc | grep [0-9] | sort -n))

# Cálculo de memoria RAM total y memoria RAM libre.
totalRAM=$(awk '/MemTotal/ {print $2}' < /proc/meminfo)
freeRAM=$(awk '/MemFree/ {print $2}' < /proc/meminfo)

# Cãlculo de memoria RAM en uso.
usedRAM=$((totalRAM - ($freeRAM)))

# Obtenemos el tamaño de página con PAGESIZE y lo pasamos de bytes a kilobytes.
pageSIZE=$(($(getconf PAGESIZE)/1024))

# Uso de CPU.
totalCPU=0
hertz=$(getconf CLK_TCK)


# Primera ejecución del bucle sobre el vector PIDS para obtener la información de uso del procesador de cada proceso.

# Se inicializa un contador.
counter=0

# Se recorren todos los procesos y se calcula el tiempo de ejecución en modo usuario y núcleo de cada proceso.
while [ $counter -le $(($totalPIDS-1)) ];
do
	# Se almacena el tiempo de ejecución del modo usuario (utime - 14) y núcleo (stime - 15).
	utime=$(cat "/proc/${PIDS[$counter]}/stat" 2> /dev/null | awk '{print $14 }')
	stime=$(cat "/proc/${PIDS[$counter]}/stat" 2> /dev/null | awk '{print $15 }')

	# Se comprueba si alguna variable tiene valor nulo.
	if [ -z $utime ] || [ -z $stime ]; then
		echo > /dev/null
	else
		startExecution[$counter]=$(($utime+$stime))
	fi

	# Se almacena la hora de lectura en décimas de segundo.
	startTime[$counter]=$(date +%s%1N)

	# Se suma uno al contador.
	let "counter++"
done


# Se espera un segundo.
sleep 1


# Segunda ejecución del bucle sobre el vector PIDS para obtener la información de uso del procesador de cada proceso.

# Se inicializa de nuevo el contador.
counter=0

# De nuevo, se recorren todos los procesos y se calcula el tiempo de ejecución en modo usuario y núcleo de cada proceso.
while [ $counter -le $(($totalPIDS-1)) ];
do
	# PID	
	# Se obtiene el PID del proceso a analizar.
	PID=${PIDS[$counter]}

	# USER
	# Se obtiene el usuario que invoca el proceso.
	userID=`awk '/Uid/ {print $2}' 2> /dev/null < /proc/$PID/status`
	# Se comprueba que el valor de userID no tiene valor nulo.
	if [ -z $userID ]; then
		echo > /dev/null
	else
		# Se busca el nombre del usuario en /etc/passwd
		USER=`awk -F : -v var="$userID" '{ if ($3 == var) print $1}' /etc/passwd`
	fi

	# PR
	# Se obtiene el valor de prioridad del proceso en cuestión.
	PR=$(awk '{print $18}' 2> /dev/null < /proc/$PID/stat)

	# VIRT
	# Se obtiene el tamaño de la memoria virtual del proceso.
	VIRT=$(awk '{print $23 / 1024}' 2> /dev/null < /proc/$PID/stat)

	# S
	# Se obtiene el estado del proceso.
	S=$(awk '{print $3}' 2> /dev/null < /proc/$PID/stat)

done

		














# Número de procesos, uso total de la CPU, memoria total, memoria utilizada y memoria libre. 
cat /proc/meminfo

echo 'Final de ejecución'