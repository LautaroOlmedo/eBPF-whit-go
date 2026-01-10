#include "kern.bpf.h"

// struct {
//     __uint(type, BPF_MAP_TYPE_RINGBUF);
//     __uint(max_entries, 256 * 1024);
// } events SEC(".maps");

// struct event {
//     char msg[32];
// };

// SEC("kprobe/sys_execve")
// int hello(void *ctx) {
//     struct event *e;

//     e = bpf_ringbuf_reserve(&events, sizeof(*e), 0);
//     if (!e)
//         return 0;

//     __builtin_memcpy(e->msg, "Hello Lautaro!", 14);
//     bpf_ringbuf_submit(e, 0);
//     return 0;
// }

struct {
    __uint(type, BPF_MAP_TYPE_RINGBUF);
    __uint(max_entries, 256 * 1024);
} events SEC(".maps");

struct event {
    char msg[32];
};

SEC("kprobe/sys_execve")
int hello(void *ctx) {
    struct event *e;

    e = bpf_ringbuf_reserve(&events, sizeof(*e), 0);
    if (!e)
        return 0;

    __builtin_memcpy(e->msg, "Hello Lautaro!", 14);
    bpf_ringbuf_submit(e, 0);
    return 0;
}


// #include "hello.bpf.h"

// /* Ring buffer para kprobe */
// struct {
//     __uint(type, BPF_MAP_TYPE_RINGBUF);
//     __uint(max_entries, 256 * 1024);
// } events SEC(".maps");



// //* EVENT STRUCT
// struct event {
//     char msg[32];
// };


// SEC("kprobe/sys_execve")
// int hello(void *ctx)
// {
//     struct event *e;

//     e = bpf_ringbuf_reserve(&events, sizeof(*e), 0);
//     if (!e)
//         return 0;

//     __builtin_memcpy(e->msg, "Hello Lautaro!", 14);
//     bpf_ringbuf_submit(e, 0);

//     return 0;
// }

