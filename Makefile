.PHONY: help

help::
	@echo  " Makefile Usage:"
	@echo  ""
	@echo  "  make build TARGET=<sw_emu/hw_emu/hw> "
	@echo  "  Command to generate the design for specified target"
	@echo  ""
	@echo  "  make run TARGET=<sw_emu/hw_emu/hw> "
	@echo  "  Command to generate and run the design for specified target"
	@echo  ""
	@echo  "  make clean TARGET=<sw_emu/hw_emu/hw> "
	@echo  "  Command to remove the generated non-hardware files for specified target"
	@echo  ""

################ Make File Describing host and kernel compile options
#include ./common.mk should be included
TARGET ?= hw_emu
PLATFORM ?= xilinx_u25_gen3x8_xdma_1_202010_1
ENABLE_STALL_TRACE ?= yes
TRACE_DDR=DDR[1]

############## setting up kernel variables
EXECUTABLE = ./host_kvs
CMD_ARGS = -x $(BUILD_DIR)/kvs.$(TARGET).xclbin

############## Kernel Configuration File
KERNEL_CONFIG_FILE :=config.cfg

############## Different log  dirs
VPP_TEMP_DIRS :=vpp_temp_dir
VPP_LOG_DIRS :=vpp_log_dir

XO_NAME := kvs.$(TARGET)
XCLBIN := kvs.$(TARGET).xclbin

################ Source Folder 
SRC_REPO := ./src

################ Build directory and number of images to process
BUILD_DIR ?= ./build/$(TARGET)

############## Host Application CPP dependencies
##HOST_SRC_CPP := $(SRC_REPO)/kvs.cpp 
HOST_SRC_CPP += $(SRC_REPO)/xcl2.cpp
HOST_SRC_CPP += $(SRC_REPO)/cmdlineparser.cpp
HOST_SRC_CPP += $(SRC_REPO)/logger.cpp
HOST_SRC_CPP += $(SRC_REPO)/socket.cpp

############## Kernel Source Files  Dependencies
KERNEL_SRC_CPP := $(SRC_REPO)/copy_kernel.cpp
KERNEL_INCLUDES := -Iinclude

############## Set "HOST" Compiler Paths and Flags
CXXFLAGS += -I$(XILINX_XRT)/include/
CXXFLAGS += -I$(XILINX_VIVADO)/include/
CXXFLAGS += -Iinclude
CXXFLAGS += -O3 -Wall -fmessage-length=0 -std=c++14 -pthread

############## Set "HOST" Set Linker Paths and Flags
CXXLDFLAGS := -L$(XILINX_XRT)/lib/
CXXLDFLAGS += -lOpenCL -lpthread -lrt -luuid -lstdc++ -lxilinxopencl -fopenmp -lxrt_coreutil

############## Kernel Compiler and Linker Flags
VPPFLAGS := -t $(TARGET)
VPPFLAGS += --platform $(PLATFORM) -R1 --save-temps
VPPFLAGS += --temp_dir $(BUILD_DIR)/$(VPP_TEMP_DIRS)
VPPFLAGS += --log_dir $(BUILD_DIR)/$(VPP_LOG_DIRS)
VPPFLAGS += --profile.data all:all:all:all
VPPFLAGS += --profile.trace_memory $(TRACE_DDR)
#ifeq ($(ENABLE_STALL_TRACE),yes)
#	VPPFLAGS += --profile.stall all:all:all
#endif
#VPPFLAGS += --config $(KERNEL_CONFIG_FILE)

create_dirs: 
	mkdir -p $(BUILD_DIR)
	mkdir -p $(BUILD_DIR)/$(VPP_TEMP_DIRS)
	mkdir -p $(BUILD_DIR)/$(VPP_LOG_DIRS)

############## host file generation
.PHONY: host
host: create_dirs $(EXECUTABLE)
$(EXECUTABLE): src/host_kvs.cpp $(HOST_SRC_CPP)
	$(CXX) -o $@ $^ $(CXXFLAGS) $(CXXLDFLAGS)

host_vadd: src/host_vadd.cpp $(HOST_SRC_CPP)
	$(CXX) -o $@ $^ $(CXXFLAGS) $(CXXLDFLAGS)

host_sw: src/host_sw.cpp $(HOST_SRC_CPP)
	$(CXX) -o $@ $^ $(CXXFLAGS) $(CXXLDFLAGS)

############## client file generation
.PHONY: client
client: src/gen_pkt.cpp src/socket.cpp
	$(CXX) -o $@ $^ 

############## Kernel XO and Xclbin File Generation
.PHONY: build
build: create_dirs $(BUILD_DIR)/$(XCLBIN)

#Compile Kernel 
$(BUILD_DIR)/copy_kernel.xo: src/copy_kernel.cpp
	v++ $(VPPFLAGS) --config kvs.cfg -c -k copy_kernel $(KERNEL_INCLUDES) -o $@ $<

$(BUILD_DIR)/get_value.xo: src/get_value.cpp
	v++ $(VPPFLAGS) -c -k get_value $(KERNEL_INCLUDES) -o $@ $<

$(BUILD_DIR)/vadd.xo: src/vadd.cpp
	v++ $(VPPFLAGS) -c -k vadd $(KERNEL_INCLUDES) -o $@ $<

# Link Kernel
$(BUILD_DIR)/$(XCLBIN): $(BUILD_DIR)/copy_kernel.xo
	v++ $(VPPFLAGS) -l -o $@ $(BUILD_DIR)/copy_kernel.xo

#$(BUILD_DIR)/vadd.xclbin: $(BUILD_DIR)/vadd.xo
vadd: $(BUILD_DIR)/vadd.xo
	v++ $(VPPFLAGS) -l -o $(BUILD_DIR)/vadd.xclbin $<

############## Emulation Files Generation
EMCONFIG_FILE = emconfig.json

$(BUILD_DIR)/$(EMCONFIG_FILE):
	emconfigutil --nd 1  --platform $(PLATFORM) --od $(BUILD_DIR)

run: create_dirs $(BUILD_DIR)/$(XCLBIN) $(BUILD_DIR)/$(EMCONFIG_FILE) $(EXECUTABLE)
ifeq ($(TARGET),$(filter $(TARGET),sw_emu hw_emu))
	cp $(BUILD_DIR)/emconfig.json .
	XCL_EMULATION_MODE=$(TARGET) $(EXECUTABLE) $(CMD_ARGS)
else
	$(EXECUTABLE) $(CMD_ARGS)
endif

clean:
	rm -f profile_* TempConfig system_estimate.xtxt *.rpt *.csv
	rm -f src/*.ll *v++* emconfig.json dltmp* xmltmp* *.log *.jou *.wcfg *.wdb
