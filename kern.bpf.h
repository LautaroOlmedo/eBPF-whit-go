#ifndef __HELLO_BPF_H__
#define __HELLO_BPF_H__

#include "vmlinux.h"

#include <bpf/bpf_helpers.h>
#include <bpf/bpf_tracing.h>

/* Headers de red (XDP) */
#include <linux/if_ether.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/udp.h>

/* Tipos explícitos */
typedef __u8  u8;
typedef __u16 u16;
typedef __u32 u32;
typedef __u64 u64;

/* Licencia obligatoria */
char LICENSE[] SEC("license") = "GPL";

#endif /* __HELLO_BPF_H__ */
