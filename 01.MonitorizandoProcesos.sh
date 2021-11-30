####################################################################
# Yassine Marroun Nettah - ymarroun1@alumno.uned.es
# Trabajo Práctico de Diseño y Administración de Sistemas Operativos
# TRABAJO 1: Monitorizando procesos
####################################################################


#!/bin/bash

# Se resetea el archivo de salida donde se almacenan los datos obtenidos.
rm -f outputFile.txt

# Variable donde se guarda el número total de procesos.
totalPIDS=`ls /proc | grep [0-9] | wc -l`

# Vector donde se guardan en orden ascendente los procesos.
PIDS=($(ls /proc | grep [0-9]| sort -n))

# Cálculo de memoria RAM total y memoria RAM libre.
totalRAM=$(awk '/MemTotal/ {print $2}' < /proc/meminfo)
freeRAM=$(awk '/MemFree/ {print $2}' < /proc/meminfo)

# Cálculo de memoria RAM
usedRAM=$(($totalRAM-($freeRAM)))

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

	# Se suma uno al contador para proceder con el siguiente elemento del vector PIDS.
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
		# Se busca el nombre del usuario en /etc/passwd.
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

	# %CPU
	# Se obtiene el porcentaje de uso del procesador.

	# Primero, volvemos a obtener el tiempo de ejecución del modo usuario (utime - 14) y núcleo (stime - 15).
	utime=$(cat "/proc/$PID/stat" 2> /dev/null | awk '{print $14 }')
	stime=$(cat "/proc/$PID/stat" 2> /dev/null | awk '{print $15 }')

	# Se comprueba si alguna variable tiene valor nulo.
	if [ -z $utime ] || [ -z $stime ]; then
		CPU=0
	else
		# Se obtiene los datos de ejecución en TIC.
		endExecution[$counter]=$(($utime+$stime))
		totalExecution=$((${endExecution[$counter]} - ${startExecution[$counter]} ))

	  	# Se almacena la hora de lectura en décimas de segundo.
   		finalTime[$counter]=$(date +%s%1N)
		
		# Se calcula el tiempo transcurrido entre las dos lecturas.
  		elapsedTime=$(( ${finalTime[$counter]} - ${startTime[$counter]} ))
	
		# %CPU = Consumo de proceso * 100 / Total CPU del sistema entre las dos lecturas.
        	# Se debe multiplicar por 10 ya que el dato final de elapsedTime se ha obtenido en décimas de segundo.
		CPU=$(echo "scale=2; ($totalExecution*100*10) / ($elapsedTime*$hertz)" | bc)
		totalCPU=$(echo "scale=2; $totalCPU + $CPU" | bc)
	fi

	# %MEM
	# Se obtiene el porcentaje de uso de la memoria.
    
    	# %MEM = (Número de páginas del proceso * tamaño de página * 100) / Tamaño de la memoria.
    	# RSS (Resident Set Size) Número de páginas del proceso.
  	RSS=$(awk '{print $24}' 2> /dev/null < /proc/$PID/stat)
	
	# Se comprueba que RSS no tiene valor nulo o valor 0.
	if [ -z $RSS ] || [ $RSS -eq 0 ]; then
		MEM=0
	else
		# scale permite especificar el número de dígitos decimales que se quiere tener en el resultado obtenido de los cálculos.
		MEM=$(echo "scale=2; ($RSS*$pageSIZE*100)/$totalRAM" | bc)
  	fi

	# TIME
	# Se obtiene el tiempo de ejecución del proceso.
    
	# Se comprueba si endExecution tiene valor nulo.
	if [ -z ${endExecution[$counter]} ];  then
		TIME="0 s"
	else
		# Se pasa de TIC a segundos.
  		seconds=$((${endExecution[$counter]}/ $hertz))
        	# Se da formato al valor de TIME para que aparezca como minutos:segundos.
		TIME=$(printf '%dm:%ds\n' $(($seconds/60)) $(($seconds%60)))
	fi

	# COMMAND
	# Se obtiene el nombre del programa invocado.

	# Con la instrucción sed se omiten los paréntesis de inicio y final.
  	COMMAND=$(awk '{print $2}' 2> /dev/null < /proc/$PID/stat | sed 's/^.\|.$//g')

  	# Se guardan los resultado obtenidos para cada proceso en un archivo de salida.
  	printf "%-10s %-10s %-5s %-10s %-5s %-10s %-10s %-10s %-20s\n" "$PID" "$USER" "$PR" "$VIRT" "$S" "$CPU" "$MEM" "$TIME" "$COMMAND" >> outputFile.txt
  
	# Se suma uno al contador para proceder con el siguiente elemento del vector PIDS.
	let "counter++"
done

# Información de salida.
echo
echo '******************************************************' 
echo '*  Monitorización en la ejecución de procesos,       *'
echo '*  emulando la funcionalidad básica del comando top  *'
echo '******************************************************' 
echo
echo "Número de procesos: $totalPIDS"
echo "Uso total CPU: $totalCPU%"
echo "Memoria total: $totalRAM kb"
echo "Memoria utilizada: $usedRAM kb"
echo "Memoria libre: $freeRAM kb"
echo
echo '10 procesos con mayor uso de procesador: '
echo '*************************************************************************************'
# Se muestra una cabecera con el nombre de los datos que se muestran listados a continuación.
printf "%-10s %-10s %-5s %-10s %-5s %-10s %-10s %-10s %-20s" PID USER PR VIRT S %CPU %MEM TIME COMMAND
echo
echo '*************************************************************************************'
# Se recupera el archivo de salida, se ordena y se muestra únicamente los 10 procesos con mayor porcentaje de CPU.
cat outputFile.txt | sort -k6 -nr | head -10
echo
echo '__________________'
echo 'Final de ejecución'
echo

