#include "kern.bpf.h"

/* XSKMAP para AF_XDP */
struct {
    __uint(type, BPF_MAP_TYPE_XSKMAP);
    __uint(max_entries, 64);   // igual o mayor a RX queues
} xsks_map SEC(".maps");



SEC("xdp")
int xdp_redirect_tcp(struct xdp_md *ctx)
{
    void *data_end = (void *)(long)ctx->data_end;
    void *data     = (void *)(long)ctx->data;

    /* Ethernet: dropea Paquetes más chicos que un header Ethernet por qué el verificador exige que pruebes 
        que la memoria existe antes de leerla. Si no está → paquete corrupto o truncado. No es una decisión lógica, 
        es una obligación de seguridad
    */
    struct ethhdr *eth = data;
    if ((void *)(eth + 1) > data_end)
        return XDP_DROP;

    /* Solo IPv4: “solo TCP/IP” Todo lo demás es ruido. dropea ARP, IPv6, VLAN, LLDP. Cualquier EtherType ≠ IPv4
    */
    if (eth->h_proto != __constant_htons(ETH_P_IP))
        return XDP_DROP;

    /* IP: dropea paquetes IPv4 incompletos por el mismo motivo: el verificador no puede leer ip->protocol sin validar
    */
    struct iphdr *ip = (void *)(eth + 1);
    if ((void *)(ip + 1) > data_end)
        return XDP_DROP;

    /* Solo TCP: dropea UDP, ICMP, GRE, ESP. cualquier IP ≠ TCP */
    if (ip->protocol != IPPROTO_TCP)
        return XDP_DROP;

    /* Permitir SSH */
    if (tcp->dest == __constant_htons(22))
        return XDP_PASS;

    /* RX queue actual */
    __u32 qid = ctx->rx_queue_index;

    /* Si existe socket AF_XDP asociado → redirect 
    */
    if (bpf_map_lookup_elem(&xsks_map, &qid))
        return bpf_redirect_map(&xsks_map, qid, 0);


    /* Si no hay socket asociado → drop: dropea TCP si no hay socket asociado a esa RX queue. Evitás que el paquete:
    llegue al kernel, se copie, se pierda silenciosamente. Es un fail-closed, no fail-open
    */
    return XDP_DROP;
}

char LICENSE[] SEC("license") = "GPL";



