// #include <bpf/xsk.h>
// #include <bpf/bpf.h>
// #include <errno.h>
// #include <net/if.h>
// #include <unistd.h>
// #include <poll.h>
// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>

// #define FRAME_SIZE 2048
// #define NUM_FRAMES 4096
// #define RX_RING_SIZE 1024

// struct xsk_umem_info {
//     struct xsk_umem *umem;
//     void *buffer;
// };

// struct xsk_socket_info {
//     struct xsk_socket *xsk;
//     struct xsk_ring_cons rx;
// };

// int main(int argc, char **argv)
// {
//     const char *ifname = "eth0";
//     int ifindex = if_nametoindex(ifname);
//     __u32 qid = 0;

//     struct xsk_umem_info umem = {};
//     struct xsk_socket_info xsk = {};
//     struct xsk_umem_config umem_cfg = {
//         .fill_size = NUM_FRAMES,
//         .comp_size = NUM_FRAMES,
//         .frame_size = FRAME_SIZE,
//         .frame_headroom = 0,
//     };

//     umem.buffer = aligned_alloc(getpagesize(), NUM_FRAMES * FRAME_SIZE);
//     if (!umem.buffer) {
//         perror("aligned_alloc");
//         return 1;
//     }

//     if (xsk_umem__create(&umem.umem, umem.buffer,
//                          NUM_FRAMES * FRAME_SIZE,
//                          NULL, NULL, &umem_cfg)) {
//         fprintf(stderr, "UMEM create failed\n");
//         return 1;
//     }

//     struct xsk_socket_config xsk_cfg = {
//         .rx_size = RX_RING_SIZE,
//         .tx_size = 0,
//         .libbpf_flags = 0,
//         .xdp_flags = 0,
//         .bind_flags = XDP_COPY,
//     };

//     if (xsk_socket__create(&xsk.xsk, ifname, qid,
//                            umem.umem,
//                            &xsk.rx, NULL,
//                            &xsk_cfg)) {
//         fprintf(stderr, "xsk_socket create failed\n");
//         return 1;
//     }


// // REGISTRAR SOCKET EN XSKMAP


// /* FD del socket AF_XDP */
// int xsk_fd = xsk_socket__fd(xsk.xsk);
// if (xsk_fd < 0) {
//     perror("xsk_socket__fd");
//     return 1;
// }

// /* Abrir el mapa xsks_map (creado por el programa XDP) */
// int xsks_map_fd = bpf_obj_get("/sys/fs/bpf/xsks_map");
// if (xsks_map_fd < 0) {
//     perror("bpf_obj_get xsks_map");
//     return 1;
// }

// /* Asociar RX queue → socket */
// if (bpf_map_update_elem(xsks_map_fd, &qid, &xsk_fd, 0) < 0) {
//     perror("bpf_map_update_elem xsks_map");
//     return 1;
// }

// printf("Socket AF_XDP insertado en xsks_map (qid=%u, fd=%d)\n",
//        qid, xsk_fd);


//     printf("AF_XDP socket listo. Escuchando...\n");

//     while (1) {
//         __u32 idx;
//         unsigned int rcvd = xsk_ring_cons__peek(&xsk.rx, 1, &idx);
//         if (!rcvd)
//             continue;

//         const struct xdp_desc *desc = xsk_ring_cons__rx_desc(&xsk.rx, idx);
//         void *pkt = umem.buffer + desc->addr;

//         printf("RX packet len=%u\n", desc->len);

//         xsk_ring_cons__release(&xsk.rx, 1);
//     }

//     return 0;
// }

// #include <bpf/xsk.h>
// #include <bpf/bpf.h>
// #include <errno.h>
// #include <net/if.h>
// #include <unistd.h>
// #include <poll.h>
// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include <sys/mman.h>

// #define FRAME_SIZE     2048
// #define NUM_FRAMES     4096
// #define RX_RING_SIZE   1024
// #define IFNAME         "eth0"
// #define QUEUE_ID       0

// /* =========================
//  * UMEM + SOCKET STRUCTS
//  * ========================= */

// struct xsk_umem_info {
//     struct xsk_umem *umem;
//     void *buffer;
//     struct xsk_ring_prod fill;
//     struct xsk_ring_cons comp;
// };

// struct xsk_socket_info {
//     struct xsk_socket *xsk;
//     struct xsk_ring_cons rx;
// };

// /* =========================
//  * MAIN
//  * ========================= */

// int main(void)
// {
//     int ifindex = if_nametoindex(IFNAME);
//     if (!ifindex) {
//         perror("if_nametoindex");
//         return 1;
//     }

//     struct xsk_umem_info umem = {};
//     struct xsk_socket_info xsk = {};

//     /* =========================
//      * CREATE UMEM BUFFER
//      * ========================= */

//     size_t umem_size = NUM_FRAMES * FRAME_SIZE;

//     umem.buffer = mmap(NULL, umem_size,
//                        PROT_READ | PROT_WRITE,
//                        MAP_PRIVATE | MAP_ANONYMOUS | MAP_POPULATE,
//                        -1, 0);
//     if (umem.buffer == MAP_FAILED) {
//         perror("mmap");
//         return 1;
//     }

//     struct xsk_umem_config umem_cfg = {
//         .fill_size = NUM_FRAMES,
//         .comp_size = NUM_FRAMES,
//         .frame_size = FRAME_SIZE,
//         .frame_headroom = 0,
//         .flags = 0,
//     };

//     if (xsk_umem__create(&umem.umem,
//                          umem.buffer,
//                          umem_size,
//                          &umem.fill,
//                          &umem.comp,
//                          &umem_cfg)) {
//         fprintf(stderr, "xsk_umem__create failed\n");
//         return 1;
//     }

//     /* =========================
//      * CREATE AF_XDP SOCKET
//      * ========================= */

//     struct xsk_socket_config xsk_cfg = {
//         .rx_size = RX_RING_SIZE,
//         .tx_size = 0,
//         .libbpf_flags = 0,
//         .xdp_flags = 0,
//         .bind_flags = XDP_COPY, /* usar XDP_ZEROCOPY si la NIC lo soporta */
//     };

//     if (xsk_socket__create(&xsk.xsk,
//                            IFNAME,
//                            QUEUE_ID,
//                            umem.umem,
//                            &xsk.rx,
//                            NULL,
//                            &xsk_cfg)) {
//         fprintf(stderr, "xsk_socket__create failed\n");
//         return 1;
//     }

//     /* =========================
//      * FILL RING INITIALIZATION
//      * ========================= */

//     for (int i = 0; i < NUM_FRAMES; i++) {
//         __u32 idx;
//         if (xsk_ring_prod__reserve(&umem.fill, 1, &idx) == 1) {
//             *xsk_ring_prod__fill_addr(&umem.fill, idx) =
//                 i * FRAME_SIZE;
//             xsk_ring_prod__submit(&umem.fill, 1);
//         }
//     }

//     /* =========================
//      * INSERT SOCKET FD INTO XSKMAP
//      * ========================= */

//     int xsk_fd = xsk_socket__fd(xsk.xsk);
//     if (xsk_fd < 0) {
//         perror("xsk_socket__fd");
//         return 1;
//     }

//     int xsks_map_fd = bpf_obj_get("/sys/fs/bpf/xsks_map");
//     if (xsks_map_fd < 0) {
//         perror("bpf_obj_get xsks_map");
//         return 1;
//     }

//     __u32 qid = QUEUE_ID;
//     if (bpf_map_update_elem(xsks_map_fd, &qid, &xsk_fd, 0) < 0) {
//         perror("bpf_map_update_elem xsks_map");
//         return 1;
//     }

//     printf("AF_XDP socket registrado (if=%s, qid=%u, fd=%d)\n",
//            IFNAME, qid, xsk_fd);

//     /* =========================
//      * RX LOOP
//      * ========================= */

//     printf("Escuchando paquetes...\n");

//     while (1) {
//         __u32 idx_rx;

//         unsigned int rcvd =
//             xsk_ring_cons__peek(&xsk.rx, 1, &idx_rx);

//         if (!rcvd)
//             continue;

//         const struct xdp_desc *desc =
//             xsk_ring_cons__rx_desc(&xsk.rx, idx_rx);

//         void *pkt =
//             umem.buffer + desc->addr;

//         printf("RX packet len=%u\n", desc->len);

//         /* liberar RX */
//         xsk_ring_cons__release(&xsk.rx, 1);

//         /* devolver frame al fill ring */
//         __u32 idx_f;
//         if (xsk_ring_prod__reserve(&umem.fill, 1, &idx_f) == 1) {
//             *xsk_ring_prod__fill_addr(&umem.fill, idx_f) =
//                 desc->addr;
//             xsk_ring_prod__submit(&umem.fill, 1);
//         }
//     }

//     return 0;
// }


#include <bpf/xsk.h>
#include <bpf/bpf.h>
#include <errno.h>
#include <net/if.h>
#include <unistd.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

/*CONFIG*/
#define IFNAME       "eth0"
#define QUEUE_ID     0

#define FRAME_SIZE   2048
#define NUM_FRAMES   4096
#define RX_RING_SIZE 1024

/*STRUCTS*/
struct xsk_umem_info {
    struct xsk_umem *umem;
    void *buffer;
    struct xsk_ring_prod fill;
    struct xsk_ring_cons comp;
};

struct xsk_socket_info {
    struct xsk_socket *xsk;
    struct xsk_ring_cons rx;
};

/*MAIN*/
int main(void)
{
    int ifindex = if_nametoindex(IFNAME);
    if (!ifindex) {
        perror("if_nametoindex");
        return 1;
    }

    struct xsk_umem_info umem = {};
    struct xsk_socket_info xsk = {};

    /*UMEM ALLOCATION*/

    size_t umem_size = NUM_FRAMES * FRAME_SIZE;

    umem.buffer = mmap(NULL, umem_size,
                       PROT_READ | PROT_WRITE,
                       MAP_PRIVATE | MAP_ANONYMOUS | MAP_POPULATE,
                       -1, 0);
    if (umem.buffer == MAP_FAILED) {
        perror("mmap");
        return 1;
    }

    struct xsk_umem_config umem_cfg = {
        .fill_size = NUM_FRAMES,
        .comp_size = NUM_FRAMES,
        .frame_size = FRAME_SIZE,
        .frame_headroom = 0,
        .flags = 0,
    };

    if (xsk_umem__create(&umem.umem,
                         umem.buffer,
                         umem_size,
                         &umem.fill,
                         &umem.comp,
                         &umem_cfg)) {
        fprintf(stderr, "xsk_umem__create failed\n");
        return 1;
    }

    /*SOCKET AF_XDP */
    struct xsk_socket_config xsk_cfg = {
        .rx_size = RX_RING_SIZE,
        .tx_size = 0,
        .libbpf_flags = 0,
        .xdp_flags = 0,
        .bind_flags = XDP_COPY, /* usar XDP_ZEROCOPY si la NIC soporta */
    };

    if (xsk_socket__create(&xsk.xsk,
                           IFNAME,
                           QUEUE_ID,
                           umem.umem,
                           &xsk.rx,
                           NULL,
                           &xsk_cfg)) {
        fprintf(stderr, "xsk_socket__create failed\n");
        return 1;
    }

    /*FILL RING INIT*/

    for (int i = 0; i < NUM_FRAMES; i++) {
        __u32 idx;
        if (xsk_ring_prod__reserve(&umem.fill, 1, &idx) == 1) {
            *xsk_ring_prod__fill_addr(&umem.fill, idx) =
                i * FRAME_SIZE;
            xsk_ring_prod__submit(&umem.fill, 1);
        }
    }

    /*INSERT SOCKET FD IN XSKMAP*/
    int xsk_fd = xsk_socket__fd(xsk.xsk);
    if (xsk_fd < 0) {
        perror("xsk_socket__fd");
        return 1;
    }

    int xsks_map_fd = bpf_obj_get("/sys/fs/bpf/xsks_map");
    if (xsks_map_fd < 0) {
        perror("bpf_obj_get xsks_map");
        return 1;
    }

    __u32 qid = QUEUE_ID;
    if (bpf_map_update_elem(xsks_map_fd, &qid, &xsk_fd, 0) < 0) {
        perror("bpf_map_update_elem xsks_map");
        return 1;
    }

    printf("AF_XDP listo: if=%s qid=%u fd=%d\n",
           IFNAME, qid, xsk_fd);


    /*RX LOOP (poll) */
    struct pollfd fds[1];
    fds[0].fd = xsk_fd;
    fds[0].events = POLLIN;

    printf("Escuchando paquetes...\n");

    while (1) {
        int ret = poll(fds, 1, -1);
        if (ret < 0) {
            perror("poll");
            break;
        }

        if (!(fds[0].revents & POLLIN))
            continue;

        __u32 idx_rx;
        unsigned int rcvd =
            xsk_ring_cons__peek(&xsk.rx, 1, &idx_rx);

        if (!rcvd)
            continue;

        const struct xdp_desc *desc =
            xsk_ring_cons__rx_desc(&xsk.rx, idx_rx);

        void *pkt = umem.buffer + desc->addr;

        printf("RX packet len=%u\n", desc->len);

        xsk_ring_cons__release(&xsk.rx, 1);

        
        /* devolver frame al fill ring */
        __u32 idx_f;
        if (xsk_ring_prod__reserve(&umem.fill, 1, &idx_f) == 1) {
            *xsk_ring_prod__fill_addr(&umem.fill, idx_f) =
                desc->addr;
            xsk_ring_prod__submit(&umem.fill, 1);
        }
    }

    return 0;
}


// COMPILACION
// gcc af_xdp_user.c -o af_xdp_user \
//     -lbpf -lelf -lz
