package main

import (
	"bytes"
	"encoding/binary"
	"fmt"
	"log"
	"os"
	"os/signal"
	"syscall"

	"github.com/cilium/ebpf"
	"github.com/cilium/ebpf/link"
	"github.com/cilium/ebpf/ringbuf"
	"github.com/cilium/ebpf/rlimit"
)

type Event struct {
	Msg [32]byte
}
func must(err error) {
	if err != nil {
		log.Fatal(err)
	}
}

func main() {
	   // 🔑 PERMITIR MEMLOCK PARA eBPF
    if err := rlimit.RemoveMemlock(); err != nil {
        panic(err)
    }
	// 1️⃣ Cargar el objeto eBPF (.o)
	spec, err := ebpf.LoadCollectionSpec("../../hello.bpf.o")
	must(err)

	coll, err := ebpf.NewCollection(spec)
	must(err)
	defer coll.Close()

	// 2️⃣ Obtener el programa
	prog := coll.Programs["hello"]
	if prog == nil {
		log.Fatal("program 'hello' not found")
	}

	// 3️⃣ Attach kprobe
	kp, err := link.Kprobe("__x64_sys_execve", prog, nil)
	must(err)
	defer kp.Close()

	// 4️⃣ Obtener el ring buffer
	events := coll.Maps["events"]
	if events == nil {
		log.Fatal("map 'events' not found")
	}

	rd, err := ringbuf.NewReader(events)
	must(err)
	defer rd.Close()

	fmt.Println("Listening for events... (Ctrl+C to exit)")

	// 5️⃣ Manejar señales
	sig := make(chan os.Signal, 1)
	signal.Notify(sig, syscall.SIGINT, syscall.SIGTERM)

	// 6️⃣ Loop de lectura
	go func() {
		for {
			record, err := rd.Read()
			if err != nil {
				return
			}

			var e Event
			err = binary.Read(bytes.NewBuffer(record.RawSample), binary.LittleEndian, &e)
			if err != nil {
				continue
			}

			fmt.Println(string(bytes.TrimRight(e.Msg[:], "\x00")))
		}
	}()

	<-sig
	fmt.Println("\nCleaning up!")
}
