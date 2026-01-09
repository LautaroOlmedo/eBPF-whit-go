# C

# ANTES DE COMPILAR

1. generar vmlinux.h. En el root del proyecto ejecutar: sudo bpftool btf dump file /sys/kernel/btf/vmlinux format c > vmlinux.h
2. Verificar: ls -l vmlinux.h

# COMPILACION

clang \
 -O2 -g \
 -target bpf \
 -D\_\_TARGET_ARCH_x86 \
 -I. \
 -c hello.bpf.c \
 -o hello.bpf.o

# Esto genera el archivo .o (BPF bytecode)?. El codigo que muestra este comando es lo que ayuda al codigo GO a saber como insertar los bytecodes en el kernel

3. Ejecutar: readelf -a hello.bpf.o

# GO

sudo /usr/local/go/bin/go run .
