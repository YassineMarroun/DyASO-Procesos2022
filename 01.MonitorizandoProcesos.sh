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

# Cálculo de memoria RAM en uso.
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

    # %CPU
    # Se obtiene el porcentaje de uso del procesado.

    # Primero, volvemos a obtener el tiempo de ejecución del modo usuario (utime - 14) y núcleo (stime - 15).
    utime=$(cat "/proc/$PID/stat" 2> /dev/null | awk '{print $14 }')
    stime=$(cat "/proc/$PID/stat" 2> /dev/null | awk '{print $15 }')

    # Se comprueba si alguna variable tiene valor nulo.
	if [ -z $utime ] || [ -z $stime ]; then
        CPU=0
    else



  
	# Medidos en TICs
	ejecucionFin[$contador]=$(($utime+$stime))
	ejecucionTotal=$((${ejecucionFin[$contador]} - ${ejecucionInicio[$contador]} ))
  	#Almacenamos la hora de la lectur, en décimas de segundo para que sea más precisoa
   	timeFin[$contador]=$(date +%s%1N)
	# Cálculo tiempo transcurrido entre las dos lecturas
  	tTrans=$(( ${timeFin[$contador]} - ${timeInicio[$contador]} ))
	# %Consumo CPU = consumo proceso *100 / total CPU de todo el sistemas entre las dos lecturas
	# Multiplicmos *10 ya que tTrans esta expresado en décimas de segundo
	CPU=$(echo "scale=2; ($ejecucionTotal*100*10) / ($tTrans*$hertz)" | bc)
	totalCPU=$(echo "scale=2; $totalCPU + $CPU" | bc)
  fi

  # %MEM: Porcentaje de uso de la memoria
  #######################################
  # %MEM = (nº paginas del proceso * tamaño página * 100) / Tamaño de la memoria
  # RSS (Resident Set Size) número de páginas del proceso
  rss=$(awk '{print $24}' 2> /dev/null < /proc/$PID/stat)
  # Comprobamos que rss no sea nula ni 0 (evitar errores /0)
  if [ -z $rss ] || [ $rss -eq 0 ]; then
	MEM=0
  else
	# scale -> nos permitirá especificar la cantidad de dígitos decimales que deseamos tener en el resultado de nuestros cálculos.
	MEM=$(echo "scale=2; ($rss*$pageSize*100)/$totalRAM" | bc)
  fi

  # TIME: tiempo de ejecución del proceso
  #######################################
  # Comprobamos si ejecucionFin es nula
  if [ -z ${ejecucionFin[$contador]} ];  then
	TIME="0 s"
  else
	# Hay que transformar los tics en segundos -> dividir variable CLK_TCK (ticks por segundo)
  	segundos=$((${ejecucionFin[$contador]}/ $hertz))
        # Mostramos TIME en formato m:s
	TIME=$(printf '%dm:%ds\n' $(($segundos/60)) $(($segundos%60)))
  fi

  # COMMAND: Nombre del programa invocado
  #######################################
  # Con sed Eliminamos 1º "(" y último caracter ")"
  COMMAND=$(awk '{print $2}' 2> /dev/null < /proc/$PID/stat | sed 's/^.\|.$//g')

  # Guardamos el resultado de cada proceso en un archivo
  printf "%-10s %-10s %-5s %-10s %-5s %-10s %-10s %-10s %-20s\n" "$PID" "$USER" "$PR" "$VIRT" "$S" "$CPU" "$MEM" "$TIME" "$COMMAND" >> resultado.txt
  # Aumentamos contador
  let "contador++"
done

# CABECERA
##########
echo -e "\e[1;31m"
echo '*******************************' 
echo '*      Programa mitop         *'
echo '* Realizado por Victor Colomo *'
echo '*******************************' 
echo -e "\e[0m"
# MOSTRAMOS INFORMACIÓN
echo "Número de procesos: $totalPID"
echo "Uso total CPU: $totalCPU%"
echo "Memoria total: $totalRAM kb"
echo "Memoria utilizada: $utilizadaRAM kb"
echo "Memoria libre: $libreRAM kb"
echo
echo -e "\e[1;30mTOP 10 PROCESOS CON MAYOR UTILIZACIÓN DEL PROCESADOR\e[0m"
# Mostramos el fichero resultado, ordenado y sólo los 10 procesos con más % de CPU
echo -e "\e[40m"
printf "%-10s %-10s %-5s %-10s %-5s %-10s %-10s %-10s %-20s" PID USER PR VIRT S %CPU %MEM TIME COMMAND
echo -e "\e[0m"
cat resultado.txt | sort -k6 -nr | head -10
echo

		














# Número de procesos, uso total de la CPU, memoria total, memoria utilizada y memoria libre. 
cat /proc/meminfo

echo 'Final de ejecución'