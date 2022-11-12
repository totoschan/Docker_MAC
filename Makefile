obj-m := para.o
# obj-m := update_list.o



current_path = $(shell pwd)
kernel_path = /usr/src/kernels/$(shell uname -r)
high_version = /usr/src/kernels/linux-4.12.3
ubuntu_path = /usr/src/linux-headers-4.15.0-70-generic


all:
	make -C $(kernel_path) M=$(current_path) modules

clean:
	make -C $(kernel_path) M=$(current_path) clean 