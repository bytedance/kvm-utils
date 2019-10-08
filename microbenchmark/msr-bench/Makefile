obj-m := msr_bench.o
KERNELDIR := /lib/modules/$(shell uname -r)/build
#KERNELDIR := /root/source/linux-image-bm/
PWD := $(shell pwd)

all:
	make -C $(KERNELDIR) M=$(PWD) clean
	make -C $(KERNELDIR) M=$(PWD) modules

clean:
	make -C $(KERNELDIR) M=$(PWD) clean
