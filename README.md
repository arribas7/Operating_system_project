# TP Grupo 4 2024-1C K3050

### [Enunciado](https://docs.google.com/document/d/1-AqFTroovEMcA1BfC2rriB5jsLE6SUa4mbcAox1rPec/edit)


## Dependencias

Para poder compilar y ejecutar el proyecto, es necesario tener instalada la
biblioteca [so-commons-library] de la cátedra:

```bash
git clone https://github.com/sisoputnfrba/so-commons-library
cd so-commons-library
make debug
make install
```

## Compilación

Cada módulo del proyecto se compila de forma independiente a través de un
archivo `makefile`. Para compilar un módulo, es necesario ejecutar el comando
`make` desde la carpeta correspondiente.

El ejecutable resultante se guardará en la carpeta `bin` del módulo.

## Importar desde Visual Studio Code

Para importar el workspace, debemos abrir el archivo `tp.code-workspace` desde
la interfaz o ejecutando el siguiente comando desde la carpeta raíz del
repositorio:

```bash
code tp.code-workspace
```

## Checkpoint

Para cada checkpoint de control obligatorio, se debe crear un tag en el
repositorio con el siguiente formato:

```
checkpoint-{número}
```

Donde `{número}` es el número del checkpoint.

Para crear un tag y subirlo al repositorio, podemos utilizar los siguientes
comandos:

```bash
git tag -a checkpoint-{número} -m "Checkpoint {número}"
git push origin checkpoint-{número}
```

Asegúrense de que el código compila y cumple con los requisitos del checkpoint
antes de subir el tag.

## Entrega

Para desplegar el proyecto en una máquina Ubuntu Server, podemos utilizar el
script [so-deploy] de la cátedra:

```bash
git clone https://github.com/sisoputnfrba/so-deploy.git
cd so-deploy
./deploy.sh -r=release -p=utils -p=kernel -p=cpu -p=memoria -p=entradasalida "tp-{año}-{cuatri}-{grupo}"
```

El mismo se encargará de instalar las Commons, clonar el repositorio del grupo
y compilar el proyecto en la máquina remota.

Ante cualquier duda, podés consultar la documentación en el repositorio de
[so-deploy], o utilizar el comando `./deploy.sh -h`.

[so-commons-library]: https://github.com/sisoputnfrba/so-commons-library
[so-deploy]: https://github.com/sisoputnfrba/so-deploy


## Soporte ayudantes 25/05

- Script de pruebas: https://github.com/sisoputnfrba/c-comenta-pruebas/tree/main/preliminares
- Checkpoints: No estan haciendo revisión de checkpoints. Solo hay que hacerlos para control, y sabiendo que fuimos avanzando. Solo se hacen los tags, no es necesario el deploy.


#### KERNEL
- Asumimos que tenemos un CPU De un núcleo, entonces tenemos un solo proceso en running. esta bien manejarlo de esta manera? Si, no dice nada de multinúcleo.

- PCB debe conocer su estado (BLOCKED, EXIT, READY)? -> Libres de hacerlos como queramos. Si lo necesitamos lo metemos.

- Necesitamos alguna estrategia para manejar los deadlocks? -> No hay que manejar ninguna.

- Un desalojo del kernel->CPU (como el timeout de quantum) y un interrupt son lo mismo? Se envía todo por medio de un interrupt. El CPU solo tiene un puerto para interrupt. Si necesitamos, se envía el motivo de desalojo, pero eso quizá no le interesa al CPU.

- Variables globales, debemos usar mutex? -> Debemos proteger todas las variables globales. Por cada variable global tiene que haber un mutex.


#### MEMORIA
- Usar un dictionary con key=pid, value=list de instrucciones
- El value puede tener también los valores que necesita de la memoria.

### RUN tests

`sudo apt-get update`
`sudo apt-get install gnome-terminal xdotool`

Luego, se pueden levantar las terminales con las configuraciones para la prueba que necesiten ejecutar:
`./run_tests.sh prueba_io`
`./run_tests.sh prueba_plani`
`./run_tests.sh prueba_memoria_tlb`
`./run_tests.sh prueba_deadlock`
`./run_tests.sh prueba_fs`
`./run_tests.sh prueba_salvations_edge`

Esto levanta los modulos en terminales aparte, por lo que no se puede debuggear con VS code. Si necesitan levanta un modulo para debuggear en vscode, pueden:

Ej: cpu
1. Run del modulo cpu desde vscode
2. Agregarle el param `-no_cpu` a la llamada al script
`./run_tests.sh prueba_salvations_edge -no_cpu`

### Valgrind
Dos comandos para correr el valgrind en la consola. Estan customizados para la prueba plani de kernel: 

valgrind --tool=helgrind ./bin/kernel config/kernel_plani.config

valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./bin/kernel  kernel_plani.config