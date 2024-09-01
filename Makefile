obj-m += burnin.o

PWD := $(CURDIR)
BUILD_DIR := $(CURDIR)/build

all:
	mkdir -p $(BUILD_DIR)
	make -C /lib/modules/$(shell uname -r)/build M=$(BUILD_DIR) src=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(BUILD_DIR) clean
	rm -rf $(BUILD_DIR)