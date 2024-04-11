#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/kvm.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdbool.h>
#include <stdarg.h>

#include "qemu.h"

#define VM_OS "vm_os.bin"

int kvm_check(QemuClass *obj, ...)
{
    int ret;
    // Get KVM_GET_API_VERSION
    ret = ioctl(obj->kvm_fd, KVM_GET_API_VERSION, NULL);
    if (ret != -1) {
        printf("KVM api Version:%d\n", ret);
    } else {
        return false;
    }

    // Check KVM_CAP_MAX_VCPUS
    ret = ioctl(obj->kvm_fd, KVM_CHECK_EXTENSION, KVM_CAP_MAX_VCPUS);
    if (ret != -1) {
        printf("KVM supports max vcpus per quest(vm):%d\n", ret);
    } else {
        return false;
    }

    ret = ioctl(obj->kvm_fd, KVM_CHECK_EXTENSION, KVM_CAP_IOMMU);
    if (ret != -1) {
        printf("KVM supports IOMMU:%d\n", ret);
    } else {
        printf("KVM unsupports IOMMU:%d\n", ret);
        return false;
    }

    printf("KVM check finished\n");
    return true;
}

int kvm_create_vm(QemuClass *obj, ...)
{
    obj->vm.vm_fd = ioctl(obj->kvm_fd, KVM_CREATE_VM, 0);
    if (obj->vm.vm_fd == -1) {
        return false;
    }
    printf("KVM create vm finished\n");
    return true;
}

int kvm_create_vcpu(QemuClass *obj, ...)
{
    int ret;
    int vcpufd = ioctl(obj->vm.vm_fd, KVM_CREATE_VCPU, 0);
    if (vcpufd == -1)
        return false;

    int mmap_size = ioctl(obj->kvm_fd, KVM_GET_VCPU_MMAP_SIZE, NULL);
    if (mmap_size == -1)
        return false;
    
    obj->vm.run = mmap(NULL, mmap_size, 
                       PROT_READ | PROT_WRITE, MAP_SHARED, 
                       vcpufd, 0);
    obj->vm.vcpu_fd = vcpufd;

    printf("KVM create vcpu finished\n");

    return true;
}

int init_vm_mem(QemuClass *obj, ...)
{
    int fd;
    unsigned char *ram = mmap(NULL, 0x1000, 
                              PROT_READ | PROT_WRITE, 
                              MAP_SHARED | MAP_ANONYMOUS,
                              -1, 0);
    fd = open(VM_OS, O_RDONLY);
    read(fd, ram, 4096);
    close(fd);
    obj->vm.ram = ram;
    return true;
}

int kvm_create_mem(QemuClass *obj, ...)
{
    int ret;

    obj->init_vm_mem(obj);

    struct kvm_userspace_memory_region mem = {
        .slot = 0,
        .guest_phys_addr = 0,
        .memory_size = 0x1000,
        .userspace_addr = (unsigned long)(obj->vm.ram),
    };
    
    ret = ioctl(obj->vm.vm_fd, KVM_SET_USER_MEMORY_REGION, &mem);
    if (ret != -1) {
        printf("KVM create mem finished\n");
    } else {
        return false;
    }

    return true;
}

int kvm_set_sregs(QemuClass *obj, ...)
{
    int ret;
    struct kvm_sregs sregs;
    struct kvm_regs regs;
    
    ret = ioctl(obj->vm.vcpu_fd, KVM_GET_SREGS, &sregs);
    if (ret == -1)
        return false;

    sregs.cs.base = 0;
    sregs.cs.selector = 0;
    ret = ioctl(obj->vm.vcpu_fd, KVM_SET_SREGS, &sregs);
    if (ret == -1)
        return false;

    regs.rip = 0;
    ret = ioctl(obj->vm.vcpu_fd, KVM_SET_REGS, &regs);
    if (ret == -1)
        return false;
    
    printf("KVM set sregs finished\n");
    
    return true;
}

int run_vm(QemuClass *obj, ...)
{
    int ret;
    obj->set_sregs(obj);
    while (1) {
        ret = ioctl(obj->vm.vcpu_fd, KVM_RUN, NULL);
        if (ret == -1) {
            printf("exit unknown\n");
            return false;
        }
        switch (obj->vm.run->exit_reason) {
            case KVM_EXIT_HLT:
                puts("KVM_EXIT_HLT");
                return true;
            case KVM_EXIT_IO:
                putchar(*(((char*)obj->vm.run) + obj->vm.run->io.data_offset));
                break;
            case KVM_EXIT_FAIL_ENTRY:
                puts("entry error");
                return false;
            default:
                puts("other error");
                printf("exit_reason: %d\n", obj->vm.run->exit_reason);
                return false;
        }    
    } 
}

QemuClass * new_qemu()
{
    QemuClass *qemu = (QemuClass *)malloc(sizeof(QemuClass));
    qemu->kvm_fd = open("/dev/kvm", O_RDWR);
    if (!(qemu->kvm_fd > 0)) {
        printf("Open /dev/kvm error\n");
        return NULL;
    }
    qemu->kvm_check = kvm_check;
    qemu->create_vm = kvm_create_vm;
    qemu->init_vm_mem = init_vm_mem;
    qemu->create_mem = kvm_create_mem;
    qemu->create_vcpu = kvm_create_vcpu;
    qemu->set_sregs = kvm_set_sregs;
    qemu->run_vm = run_vm;
    return qemu;
}

int main()
{
    int ret;
    QemuClass *qemu = new_qemu();
    if (qemu == NULL) {
        printf("Init qemu failed\n");
        return false;
    }

    // Check kvm 
    if (!qemu->kvm_check(qemu)) {
        printf("kvm check failed\n");
        return false;
    }
    // Create vm
    ret = qemu->create_vm(qemu);
    if (!ret) {
        printf("create vm failed\n");
        return false;
    }
    // Create vm's memory
    ret = qemu->create_mem(qemu);
    if (!ret) {
        printf("create mem failed\n");
        return false;
    }
    // Create vm's vcpu
    ret = qemu->create_vcpu(qemu);
    if (!ret) {
        printf("create vcpu failed\n");
        return false;
    }
    // Run vm
    ret = qemu->run_vm(qemu);
    if (!ret) {
        printf("run vm failed\n");
        return false;
    }
}