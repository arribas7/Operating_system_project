## Soporte ayudantes 25/05

- Script de pruebas: https://github.com/sisoputnfrba/c-comenta-pruebas/tree/main/preliminares
- Checkpoints: No estan haciendo revisión de checkpoints. Solo hay que hacerlos para control, y sabiendo que fuimos avanzando. Solo se hacen los tags, no es necesario el deploy.


#### KERNEL
- Asumimos que tenemos un CPU De un núcleo, entonces tenemos un solo proceso en running. esta bien manejarlo de esta manera? Si, no dice nada de multinúcleo.

- PCB debe conocer su estado (BLOCKED, EXIT, READY)? -> Libres de hacerlos como queramos. Si lo necesitamos lo metemos.

- Necesitamos alguna estrategia para manejar los deadlocks? -> No hay que manejar ninguna.

- Un desalojo del kernel->CPU (como el timeout de quantum) y un interrupt son lo mismo? Se envía todo por medio de un interrupt. El CPU solo tiene un puerto para interrupt. Si necesitamos, se envía el motivo de desalojo, pero eso quizá no le interesa al CPU.

- Variables globales, debemos usar mutex? -> Debemos proteger todas las variables globales. Por cada variable global tiene que haber un mutex.

==========================================================================


#### MEMORIA
- Usar un dictionary con key=pid, value=list de instrucciones
- El value puede tener también los valores que necesita de la memoria.