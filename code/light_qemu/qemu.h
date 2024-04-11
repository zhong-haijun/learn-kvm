#ifndef QEMU_H
#define QEMU_H

typedef struct vm_t{
    int vm_fd;
    int vcpu_fd;
    unsigned char *ram;
    struct kvm_run *run;
} vm_t;

typedef struct QemuClass QemuClass;
typedef int (*KVMOpFunc)(QemuClass *obj, ...);

struct QemuClass{
    int kvm_fd;
    vm_t vm;
    KVMOpFunc kvm_check;
    KVMOpFunc create_vm;
    int (*init_vm_mem)(QemuClass *obj, ...);
    KVMOpFunc create_mem;
    KVMOpFunc create_vcpu;
    KVMOpFunc set_sregs;
    int (*run_vm)(QemuClass *obj, ...);
};

#endif