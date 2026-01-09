Proyecto Arquant eBPF-QUIC.

En este repositorio se abordan los conceptos basicos para comenzar a trabajar con las tecnologías eBPF, Go (librerías cilium/ebpf y libbpfgo) y QUIC, con la finalidad de generar:
   * XDP Firewall
   * AF_XDP Socket. Para rutear mensajes TCP/IP directamente desde la NIC. Sin pasar por el stack de red del kernel
   * Ruteo en Go como ingesta de datos ("API Gateway" / "Load Balancer") para una red que trabaja 100% en QUIC

Este documento escapa de nociones básicas acerca de eBPF o Go por lo que se asume que se cuenta con una base de conocimiento y se centra en los objetivos prácticos de ruteo y parseo a QUIC.



<filename>.bpf.c  ──clang──▶  <filename>.bpf.o  ──loader──▶  kernel
                                                    ▲
                                                    │
                                        cilium/ebpf | libbpfgo 




*** Conceptios fundamentales ***

Las librerías que te permiten trabajar con eBPF y Go (cilium/ebpf, libbpfgo) 
**sirven para:**
Cargar programas eBPF (XDP, kprobe, tc, tracepoint)
Crear / pinnear / actualizar mapas eBPF
Insertar FDs en mapas (XSKMAP, ARRAY, etc.)
**NO sirve para:**
Crear sockets
Crear UMEM
Leer paquetes

AF_XDP (UMEM, RX rings, TX rings) NO es eBPF. Es un socket del kernel (AF_XDP), como TCP o UDP.

**Se necesita una librería de userspace que:**
haga socket(AF_XDP)
haga mmap de UMEM
maneje RX/TX rings
haga poll()


Al día de la fecha no he encontrado una librería de utilidada escrita en Go que abstraiga la implementación de los componentes necesarios en userspace por lo que en este repositorio se desarrollarán en C.















En AF_XDP:
   * El socket AF_XDP se crea en userspace  
   * El kernel NO crea sockets
   * El programa XDP solo redirige paquetes a un mapa (XSKMAP)
   * El socket AF_XDP está asociado a una interfaz, a una RX queue y a un UMEM compartido

                              NIC
                               │
                               ▼
                              XDP program
                               │
                               ▼
                              xsks_map[qid] ───► AF_XDP socket ───► UMEM ───► Go


Qué NO hace AF_XDP:
   * parsea TCP
   * reensambla streams
   * maneja retransmisiones
   * entiende sockets TCP
   * Es raw packets.


*** Go y C en userspace ***: 
Go:
 ├─ crea UMEM
 ├─ crea socket AF_XDP
 ├─ bind iface + queue
 ├─ inserta fd en xsks_map
 ├─ llena Fill Queue
 └─ loop Receive()

Kernel:
 └─ XDP redirect → xsks_map → socket










# ANTES DE COMPILAR

1. generar vmlinux.h. En el root del proyecto ejecutar: sudo bpftool btf dump file /sys/kernel/btf/vmlinux format c > vmlinux.h
2. Verificar: ls -l vmlinux.h
3. Si vmlinux.h no está en el mismo directorio, el editor se queja.

# COMPILACION

clang \
 -O2 -g \
 -target bpf \
 -D\_\_TARGET_ARCH_x86 \
 -I. \
 -c <filename>.bpf.c \
 -o <filename>.bpf.o


clang \
 -O2 -g \
 -target bpf \
 -D__TARGET_ARCH_x86 \
 -c xdp.bpf.c \
 -o xdp_bpf.o

2. Ejecutar: readelf -a xdp.bpf.o 
# Esto genera el archivo .o (BPF bytecode). El codigo que muestra este comando es lo que ayuda al codigo GO a saber como insertar los bytecodes en el kernel


3. verificar la existencia de /sys/fs/bpf/xdp_bpf:
mount | grep bpf
Si no existe: 
sudo mount -t bpf bpf /sys/fs/bpf

4. Cargar el programa XDP en el kernel:
sudo bpftool prog load xdp_bpf.o /sys/fs/bpf/xdp_bpf

5. Pinnear el mapa xsks_map:

sudo bpftool map pin \
  name xsks_map \
  /sys/fs/bpf/xsks_map
sudo bpftool map pin name xsks_map /sys/fs/bpf/xsks_map

6. Atachar el programa XDP a la NIC:
sudo ip link set dev eth0 xdp pinned /sys/fs/bpf/xdp_bpf

7. Chequeo rápido:
ip link show dev eth0

8. Verificar que todo esté bien cargado:
bpftool prog show
bpftool map show
ls -l /sys/fs/bpf/









*** UTILS COMMANDS ***
Ver el map: bpftool map show
Ver contenido: bpftool map dump pinned /sys/fs/bpf/myapp/xsks_map





# Archivo .h
el .h existe para compartir contratos y definiciones entre el mundo eBPF (kernel) y el userspace, y para aislar dependencias del kernel.

debe contener:
   * tipos
   * structs
   * macros
   * helpers
   * mapas


# Archivos .c
El .c debe contener:
   * programas
   * mapas
   * LICENSE







*** FILE DESCRIPTOR (FD) ***
¿Qué es un FD? FD = File Descriptor
En Linux, todo es un archivo:
   * sockets
   * pipes
   * epoll
   * eBPF maps
   * AF_XDP sockets
Un FD es un número entero que identifica un objeto del kernel abierto por un proceso.

¿Por qué AF_XDP usa FD? Porque XDP corre en el kernel, y no puede usar punteros a userspace. No puede llamar funciones de userspace, solo puede trabajar con objetos del kernel. El FD es la referencia segura al objeto kernel.






***PINNING***

Pinning es el acto de anclar (persistir) un objeto eBPF en el filesystem especial de BPF (bpffs), normalmente en: /sys/fs/bpf/

Cuando un objeto está pinneado:
   * vive aunque el proceso que lo creó termine
   * puede ser reutilizado por otros procesos
   * tiene un path estable (como un archivo)

¿Qué objetos se pueden pinnear?
   * Maps 
   * Programs
   * Links (depende kernel/libbpf)

Ejemplos:
BPF_MAP_TYPE_XSKMAP
BPF_MAP_TYPE_ARRAY
BPF_PROG_TYPE_XDP

¿Cómo funciona internamente? eBPF usa un filesystem virtual llamado bpffs. Cuando hacés pinning:
   * el kernel incrementa el refcount
   * asocia el objeto eBPF a un inode
   * ese inode vive en /sys/fs/bpf

Mientras exista el archivo:
   * el objeto no se libera

Sin pinning: “esto es una variable en memoria del proceso”
Con pinning: “esto es un archivo compartido del kernel”

Si un mapa conecta kernel ↔ userspace a largo plazo → se pinnea
Si vive y muere con el proceso → no hace falta

El pinning NO ocurre en el programa XDP, ocurre en userspace, cuando lo cargás.

¿Cómo se “despinnea”? Borrando el archivo /sys/fs/bpf/myapp/xsks_map (o el que fuese). Cuando el último FD se cierra:
   * el kernel libera el objeto



Patrón CORRECTO para crear archivos en producción:

📁 Estructura típica
/sys/fs/bpf/
└── mycompany/
    └── xdp/
        ├── xsks_map
        ├── stats_map
        └── config_map


**Seguridad:**
En prod se combina con:
   * mount bpffs read-only excepto path propio
   * permisos estrictos
   * SELinux / AppArmor
   * systemd sandboxing

Ejemplo:
mount -o remount,ro /sys/fs/bpf
mount -o remount,rw /sys/fs/bpf/mycompany


Documentation: https://ants-gitlab.inf.um.es/jorgegm/xdp-tutorial/-/tree/344191124593c32497505606075524ed8e5b24df/basic04-pinning-maps




*** skeleton (libbpf) ***