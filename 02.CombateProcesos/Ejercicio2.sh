####################################################################
# Trabajo Práctico de Diseño y Administración de Sistemas Operativos
# TRABAJO 2: Combate de procesos
####################################################################

#!/bin/bash
# Este archivo es un scrip que:
# 1. Compila los archivos fuente padre.c e hijo.c con gcc
# 2. Crea el fichero fifo "resultado"
# 3. Lanza un cat en segundo plano para leer "resultado"  
# 4. Lanza el proceso padre
# 5. Al acabar limpia todos los ficheros que ha creado

cd Trabajo2

gcc padre.c -o padre
gcc hijo.c -o hijo

mknod resultado p
cat resultado &

./padre 10

rm resultado
rm padre
rm hijo
