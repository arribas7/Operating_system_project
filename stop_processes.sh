
#!/bin/bash

# Patrones de los nombres de los procesos que deseas detener
process_patterns=("bin/cpu" "bin/memoria" "bin/kernel" "bin/entradasalida")

for pattern in "${process_patterns[@]}"; do
	# Obtiene el PID del proceso, excluyo el proceso del mismo grep
	pids=$(ps aux | grep "$pattern" | grep -v grep | awk '{print $2}')
	if [ -z "$pids" ]; then
    	echo "No se encontraron procesos con el patrón $pattern"
	else
    	# Deten el proceso con kill -9
    	for pid in $pids; do
        	kill -9 $pid
        	echo "Deteniendo proceso con patrón $pattern y PID $pid"
    	done
	fi
done

# Limpieza de procesos zombis
zombie_info=$(ps -ef | grep '<defunct>' | grep -v grep)
if [ -n "$zombie_info" ]; then
	echo "Procesos zombis encontrados:"
	echo "$zombie_info"
	parent_pids=$(echo "$zombie_info" | awk '{print $3}')
	for parent_pid in $parent_pids; do
    	echo "Terminando proceso padre $parent_pid de procesos zombis"
    	kill -9 $parent_pid
	done
fi

# Verificación final para asegurar que no queden procesos zombis
echo "Verificación final para procesos zombis:"
ps -ef | grep '<defunct>' | grep -v grep


