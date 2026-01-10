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
 -c <filename>.bpf.c \
 -o <filename>_bpf.o

2. Ejecutar: readelf -a <filename>.bpf.o 
# Esto genera el archivo .o (BPF bytecode). El codigo que muestra este comando es lo que ayuda al codigo GO a saber como insertar los bytecodes en el kernel


Ejemplo de salida de readelf:

ELF Header:
  Magic:   7f 45 4c 46 02 01 01 00 00 00 00 00 00 00 00 00 
  Class:                             ELF64
  Data:                              2's complement, little endian
  Version:                           1 (current)
  OS/ABI:                            UNIX - System V
  ABI Version:                       0
  Type:                              REL (Relocatable file)
  Machine:                           Linux BPF
  Version:                           0x1
  Entry point address:               0x0
  Start of program headers:          0 (bytes into file)
  Start of section headers:          3824 (bytes into file)
  Flags:                             0x0
  Size of this header:               64 (bytes)
  Size of program headers:           0 (bytes)
  Number of program headers:         0
  Size of section headers:           64 (bytes)
  Number of section headers:         28
  Section header string table index: 1

Section Headers:
  [Nr] Name              Type             Address           Offset
       Size              EntSize          Flags  Link  Info  Align
  [ 0]                   NULL             0000000000000000  00000000
       0000000000000000  0000000000000000           0     0     0
  [ 1] .strtab           STRTAB           0000000000000000  00000dc3
       0000000000000128  0000000000000000           0     0     1
  [ 2] .text             PROGBITS         0000000000000000  00000040
       0000000000000000  0000000000000000  AX       0     0     4
  [ 3] kprobe/sys_execve PROGBITS         0000000000000000  00000040
       0000000000000120  0000000000000000  AX       0     0     8
  [ 4] .relkprobe/s[...] REL              0000000000000000  00000ad0
       0000000000000010  0000000000000010   I      27     3     8
  [ 5] license           PROGBITS         0000000000000000  00000160
       0000000000000004  0000000000000000  WA       0     0     1
  [ 6] .maps             PROGBITS         0000000000000000  00000168
       0000000000000010  0000000000000000  WA       0     0     8
  [ 7] .rodata.str1.1    PROGBITS         0000000000000000  00000178
       000000000000000f  0000000000000001 AMS       0     0     1
  [ 8] .debug_loclists   PROGBITS         0000000000000000  00000187
       0000000000000017  0000000000000000           0     0     1
  [ 9] .debug_abbrev     PROGBITS         0000000000000000  0000019e
       00000000000000f4  0000000000000000           0     0     1
  [10] .debug_info       PROGBITS         0000000000000000  00000292
       000000000000011d  0000000000000000           0     0     1
  [11] .rel.debug_info   REL              0000000000000000  00000ae0
       0000000000000050  0000000000000010   I      27    10     8
  [12] .debug_str_o[...] PROGBITS         0000000000000000  000003af
       0000000000000054  0000000000000000           0     0     1
  [13] .rel.debug_s[...] REL              0000000000000000  00000b30
       0000000000000130  0000000000000010   I      27    12     8
  [14] .debug_str        PROGBITS         0000000000000000  00000403
       00000000000000e1  0000000000000001  MS       0     0     1
  [15] .debug_addr       PROGBITS         0000000000000000  000004e4
       0000000000000020  0000000000000000           0     0     1
  [16] .rel.debug_addr   REL              0000000000000000  00000c60
       0000000000000030  0000000000000010   I      27    15     8
  [17] .BTF              PROGBITS         0000000000000000  00000504
       0000000000000283  0000000000000000           0     0     4
  [18] .rel.BTF          REL              0000000000000000  00000c90
       0000000000000020  0000000000000010   I      27    17     8
  [19] .BTF.ext          PROGBITS         0000000000000000  00000788
       00000000000000a0  0000000000000000           0     0     4
  [20] .rel.BTF.ext      REL              0000000000000000  00000cb0
       0000000000000070  0000000000000010   I      27    19     8
  [21] .debug_frame      PROGBITS         0000000000000000  00000828
       0000000000000028  0000000000000000           0     0     8
  [22] .rel.debug_frame  REL              0000000000000000  00000d20
       0000000000000020  0000000000000010   I      27    21     8
  [23] .debug_line       PROGBITS         0000000000000000  00000850
       00000000000000b3  0000000000000000           0     0     1
  [24] .rel.debug_line   REL              0000000000000000  00000d40
       0000000000000080  0000000000000010   I      27    23     8
  [25] .debug_line_str   PROGBITS         0000000000000000  00000903
       0000000000000061  0000000000000001  MS       0     0     1
  [26] .llvm_addrsig     LOOS+0xfff4c03   0000000000000000  00000dc0
       0000000000000003  0000000000000000   E      27     0     1
  [27] .symtab           SYMTAB           0000000000000000  00000968
       0000000000000168  0000000000000018           1    12     8
Key to Flags:
  W (write), A (alloc), X (execute), M (merge), S (strings), I (info),
  L (link order), O (extra OS processing required), G (group), T (TLS),
  C (compressed), x (unknown), o (OS specific), E (exclude),
  D (mbind), p (processor specific)

There are no section groups in this file.

There are no program headers in this file.

There is no dynamic section in this file.

Relocation section '.relkprobe/sys_execve' at offset 0xad0 contains 1 entry:
  Offset          Info           Type           Sym. Value    Sym. Name
000000000008  000d00000001 R_BPF_INSN_64     0000000000000000 events

Relocation section '.rel.debug_info' at offset 0xae0 contains 5 entries:
  Offset          Info           Type           Sym. Value    Sym. Name
000000000008  000500000003 R_BPF_INSN_16     0000000000000000 .debug_abbrev
000000000011  000600000003 R_BPF_INSN_16     0000000000000000 .debug_str_offsets
000000000015  000a00000003 R_BPF_INSN_16     0000000000000000 .debug_line
00000000001f  000800000003 R_BPF_INSN_16     0000000000000000 .debug_addr
000000000023  000400000003 R_BPF_INSN_16     0000000000000000 .debug_loclists

Relocation section '.rel.debug_str_offsets' at offset 0xb30 contains 19 entries:
  Offset          Info           Type           Sym. Value    Sym. Name
000000000008  000700000003 R_BPF_INSN_16     0000000000000000 .debug_str
00000000000c  000700000003 R_BPF_INSN_16     0000000000000000 .debug_str
000000000010  000700000003 R_BPF_INSN_16     0000000000000000 .debug_str
000000000014  000700000003 R_BPF_INSN_16     0000000000000000 .debug_str
000000000018  000700000003 R_BPF_INSN_16     0000000000000000 .debug_str
00000000001c  000700000003 R_BPF_INSN_16     0000000000000000 .debug_str
000000000020  000700000003 R_BPF_INSN_16     0000000000000000 .debug_str
000000000024  000700000003 R_BPF_INSN_16     0000000000000000 .debug_str
000000000028  000700000003 R_BPF_INSN_16     0000000000000000 .debug_str
00000000002c  000700000003 R_BPF_INSN_16     0000000000000000 .debug_str
000000000030  000700000003 R_BPF_INSN_16     0000000000000000 .debug_str
000000000034  000700000003 R_BPF_INSN_16     0000000000000000 .debug_str
000000000038  000700000003 R_BPF_INSN_16     0000000000000000 .debug_str
00000000003c  000700000003 R_BPF_INSN_16     0000000000000000 .debug_str
000000000040  000700000003 R_BPF_INSN_16     0000000000000000 .debug_str
000000000044  000700000003 R_BPF_INSN_16     0000000000000000 .debug_str
000000000048  000700000003 R_BPF_INSN_16     0000000000000000 .debug_str
00000000004c  000700000003 R_BPF_INSN_16     0000000000000000 .debug_str
000000000050  000700000003 R_BPF_INSN_16     0000000000000000 .debug_str

Relocation section '.rel.debug_addr' at offset 0xc60 contains 3 entries:
  Offset          Info           Type           Sym. Value    Sym. Name
000000000008  000e00000002 R_BPF_INSN_32     0000000000000000 LICENSE
000000000010  000d00000002 R_BPF_INSN_32     0000000000000000 events
000000000018  000200000002 R_BPF_INSN_32     0000000000000000 kprobe/sys_execve

Relocation section '.rel.BTF' at offset 0xc90 contains 2 entries:
  Offset          Info           Type           Sym. Value    Sym. Name
000000000128  000d00000004 R_BPF_INSN_DISP16 0000000000000000 events
000000000140  000e00000004 R_BPF_INSN_DISP16 0000000000000000 LICENSE

Relocation section '.rel.BTF.ext' at offset 0xcb0 contains 7 entries:
  Offset          Info           Type           Sym. Value    Sym. Name
00000000002c  000200000004 R_BPF_INSN_DISP16 0000000000000000 kprobe/sys_execve
000000000040  000200000004 R_BPF_INSN_DISP16 0000000000000000 kprobe/sys_execve
000000000050  000200000004 R_BPF_INSN_DISP16 0000000000000000 kprobe/sys_execve
000000000060  000200000004 R_BPF_INSN_DISP16 0000000000000000 kprobe/sys_execve
000000000070  000200000004 R_BPF_INSN_DISP16 0000000000000000 kprobe/sys_execve
000000000080  000200000004 R_BPF_INSN_DISP16 0000000000000000 kprobe/sys_execve
000000000090  000200000004 R_BPF_INSN_DISP16 0000000000000000 kprobe/sys_execve

Relocation section '.rel.debug_frame' at offset 0xd20 contains 2 entries:
  Offset          Info           Type           Sym. Value    Sym. Name
000000000014  000900000003 R_BPF_INSN_16     0000000000000000 .debug_frame
000000000018  000200000002 R_BPF_INSN_32     0000000000000000 kprobe/sys_execve

Relocation section '.rel.debug_line' at offset 0xd40 contains 8 entries:
  Offset          Info           Type           Sym. Value    Sym. Name
000000000022  000b00000003 R_BPF_INSN_16     0000000000000000 .debug_line_str
000000000026  000b00000003 R_BPF_INSN_16     0000000000000000 .debug_line_str
00000000002a  000b00000003 R_BPF_INSN_16     0000000000000000 .debug_line_str
000000000036  000b00000003 R_BPF_INSN_16     0000000000000000 .debug_line_str
00000000004b  000b00000003 R_BPF_INSN_16     0000000000000000 .debug_line_str
000000000060  000b00000003 R_BPF_INSN_16     0000000000000000 .debug_line_str
000000000075  000b00000003 R_BPF_INSN_16     0000000000000000 .debug_line_str
00000000008f  000200000002 R_BPF_INSN_32     0000000000000000 kprobe/sys_execve

The decoding of unwind sections for machine type Linux BPF is not currently supported.

Symbol table '.symtab' contains 15 entries:
   Num:    Value          Size Type    Bind   Vis      Ndx Name
     0: 0000000000000000     0 NOTYPE  LOCAL  DEFAULT  UND 
     1: 0000000000000000     0 FILE    LOCAL  DEFAULT  ABS kprobe.bpf.c
     2: 0000000000000000     0 SECTION LOCAL  DEFAULT    3 kprobe/sys_execve
     3: 0000000000000110     0 NOTYPE  LOCAL  DEFAULT    3 LBB0_2
     4: 0000000000000000     0 SECTION LOCAL  DEFAULT    8 .debug_loclists
     5: 0000000000000000     0 SECTION LOCAL  DEFAULT    9 .debug_abbrev
     6: 0000000000000000     0 SECTION LOCAL  DEFAULT   12 .debug_str_offsets
     7: 0000000000000000     0 SECTION LOCAL  DEFAULT   14 .debug_str
     8: 0000000000000000     0 SECTION LOCAL  DEFAULT   15 .debug_addr
     9: 0000000000000000     0 SECTION LOCAL  DEFAULT   21 .debug_frame
    10: 0000000000000000     0 SECTION LOCAL  DEFAULT   23 .debug_line
    11: 0000000000000000     0 SECTION LOCAL  DEFAULT   25 .debug_line_str
    12: 0000000000000000   288 FUNC    GLOBAL DEFAULT    3 hello
    13: 0000000000000000    16 OBJECT  GLOBAL DEFAULT    6 events
    14: 0000000000000000     4 OBJECT  GLOBAL DEFAULT    5 LICENSE

No version information found in this file.
readelf: Warning: unable to apply unsupported reloc type 3 to section .debug_info
readelf: Warning: Unrecognized form: 0x22


3. verificar la existencia de /sys/fs/bpf/xdp_bpf: mount | grep bpf
Si no existe: 
sudo mount -t bpf bpf /sys/fs/bpf

**¿Qué es el BPF filesystem (/sys/fs/bpf)?** El BPF filesystem (bpffs) es un filesystem especial del kernel, no es un disco real.

Sirve para:

*  Pinear programas eBPF

*  Pinear mapas eBPF

*  Compartir eBPF entre procesos

*  Persistencia mientras el sistema esté vivo

Sin bpffs:

*  no podés pinnear

libbpf pierde referencias

bpftool funciona a medias

**¿Qué significa cada parte?**
*  bpf on /sys/fs/bpf

*  bpf → tipo de filesystem

*  /sys/fs/bpf → punto de montaje

Es el lugar estándar donde viven los objetos eBPF


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