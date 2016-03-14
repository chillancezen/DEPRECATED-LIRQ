NAME=lirq_test
obj-m := $(NAME).o
KBUILD_EXTRA_SYMBOLS:=/mnt/projects/LIRQ/Module.symvers
KVERS=$(shell uname -r)
CURDIR=$(shell pwd)

build: kernel_modules

kernel_modules:
	make -C /lib/modules/$(KVERS)/build M=$(CURDIR) modules
install:
	insmod ./$(NAME).ko
uninstall:
	-rmmod $(NAME)
clean:
	make  -C /lib/modules/$(KVERS)/build M=$(CURDIR) clean
test:
	@echo $($(NAME)-objs)
reset:
	make uninstall -C .
	make install -C .
