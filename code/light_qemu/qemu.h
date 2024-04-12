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
typedef int (*QemuOpFunc)(QemuClass *obj, ...);

struct QemuClass{
    int kvm_fd;
    vm_t vm;
    KVMOpFunc kvm_check;
    KVMOpFunc create_vm;
    QemuOpFunc init_vm_mem;
    KVMOpFunc create_mem;
    KVMOpFunc create_vcpu;
    KVMOpFunc set_sregs;
    QemuOpFunc run_vm;
};

#endif