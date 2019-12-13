# kvm-utils
There are two parts of this project: debug & microbench.

## debug
Base on kprobe, it could be installed/removed easily.

### kvmexitreason
Report kvm exit reasons detail every second.

### kvmwrmsr
Report kvm exit wrmsr detail every second.

## microbenchmark
Benchmark key performence for virtual machine & bare metal.

### ipi-bench
Benchmark single/broadcast IPI within/across NUMA node(s).

### msr-bench
Benchmark wrmsr MSR_IA32_POWER_CTL/MSR_IA32_TSCDEADLINE.

### pio-mmio-bench
Benchmark PIO pic0(handled by kernel), keyboard(handled by QEMU),
empty PIO(handled by QEMU).
Benchmark MMIO vram, virtio-pci-modern.

### tlb-shootdown-bench
Benchmark TLB shootdown by madvise(*addr, length, MADV_DONTNEED).
