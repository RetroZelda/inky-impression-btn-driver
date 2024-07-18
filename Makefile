obj-m += inky-impression-btn-driver.o

KERNEL_DIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)
MODULE_NAME := inky-impression-btn-driver
MODULE_VERSION := 1.0
MODULE_FILE := inky-impression-btn-driver.ko
MODULE_INSTALL_DIR := /lib/modules/$(shell uname -r)/extra
MODULE_LOAD_CONF := /etc/modules-load.d/$(MODULE_NAME).conf
MODULE_OPTIONS_CONF := /etc/modprobe.d/$(MODULE_NAME).conf
DKMS_DIR := /usr/src/$(MODULE_NAME)-$(MODULE_VERSION)

all:
	make -C $(KERNEL_DIR) M=$(PWD) modules

clean:
	make -C $(KERNEL_DIR) M=$(PWD) clean

install: all create_dkms_conf
	@echo "Copying module to $(MODULE_INSTALL_DIR)..."
	sudo cp $(MODULE_FILE) $(MODULE_INSTALL_DIR)
	@echo "Updating module dependencies..."
	sudo depmod
	@echo "Creating module load configuration..."
	echo "$(MODULE_NAME)" | sudo tee $(MODULE_LOAD_CONF)
	@echo "Creating module options configuration (if needed)..."
	# Uncomment and modify the following line if your module has parameters
	# echo "options $(MODULE_NAME) param=value" | sudo tee $(MODULE_OPTIONS_CONF)
	@echo "Setting up DKMS..."
	sudo mkdir -p $(DKMS_DIR)
	sudo cp -r * $(DKMS_DIR)
	sudo dkms add -m $(MODULE_NAME) -v $(MODULE_VERSION)
	sudo dkms build -m $(MODULE_NAME) -v $(MODULE_VERSION)
	sudo dkms install -m $(MODULE_NAME) -v $(MODULE_VERSION)
	@echo "Module installed successfully. To load it immediately, use: sudo modprobe $(MODULE_NAME)"

uninstall:
	@echo "Removing the kernel module..."
	sudo rmmod $(MODULE_NAME)
	@echo "Deleting the module file from $(MODULE_INSTALL_DIR)..."
	sudo rm -f $(MODULE_INSTALL_DIR)/$(MODULE_FILE)
	@echo "Removing module load configuration..."
	sudo rm -f $(MODULE_LOAD_CONF)
	@echo "Removing module options configuration..."
	sudo rm -f $(MODULE_OPTIONS_CONF)
	@echo "Updating module dependencies..."
	sudo depmod
	@echo "Removing DKMS setup..."
	sudo dkms remove -m $(MODULE_NAME) -v $(MODULE_VERSION) --all
	sudo rm -rf $(DKMS_DIR)
	@echo "Module uninstalled successfully."

create_dkms_conf:
	@echo "Creating dkms.conf..."
	@sudo mkdir -p $(DKMS_DIR)
	@echo 'PACKAGE_NAME="$(MODULE_NAME)"' | sudo tee $(DKMS_DIR)/dkms.conf
	@echo 'PACKAGE_VERSION="$(MODULE_VERSION)"' | sudo tee -a $(DKMS_DIR)/dkms.conf
	@echo 'BUILT_MODULE_NAME[0]="inky-impression-btn-driver"' | sudo tee -a $(DKMS_DIR)/dkms.conf
	@echo 'DEST_MODULE_LOCATION[0]="/extra"' | sudo tee -a $(DKMS_DIR)/dkms.conf
	@echo 'AUTOINSTALL="yes"' | sudo tee -a $(DKMS_DIR)/dkms.conf
