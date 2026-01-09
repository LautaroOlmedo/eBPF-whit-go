package main

// import (
// 	"C"

// 	bpf "github.com/aquasecurity/tracee/libbpfgo"
// )

// func main(){
// 	b, err := bpf.NewModuleFromFile("../hello.bpf.o") // abre el archivo
// 	must(err)

// 	defer b.Close()

// 	must(b.BPFLoadObject()) // CARGAR EL ARCHIVO EN EL KERNEL

// 	p, err := b.GetProgram("hello")// obtener la funcion/programa hello
// 	must(err)

// 	_, err = p.AttachKprobe("__x64_sys_execve") // ADJUNTAR EL PROGRAMA A KPROBE
// 	must(err)

// 	// El codigo c esta escribiendo infrmacion de tracing cada vez que algo llama a esa syscall y se necesita
// 	// algo desde el espacio de usuario para imprimirlo
// 	rb, _ := b.InitRingBuf("events", callback)
//     rb.Start()
// 	println("Cleaning up!")

// }

// func must(err error) {
// 	if err != nil {
// 		panic(err)
// 	}
//

import (
	"log"

	xdp "github.com/mdlayher/socket/xdp"
)

func main(){

umem, err := xdp.NewUMEM(&xdp.UMEMConfig{
    FrameSize:  2048,
    FrameCount: 4096,
})

xsk, err := xdp.NewSocket(
    umem,
    &xdp.SocketConfig{
        NumRXDescriptors: 1024,
        NumTXDescriptors: 0,
    },
)
if err != nil {
    log.Fatalf("new socket: %v", err)
}
defer xsk.Close()


}

