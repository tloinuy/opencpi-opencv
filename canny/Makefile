
WORDSIZE=64

# ---- Executable targets ------------------------------------------------- #

PROGRAMS := canny

LIBRARY_PATH += -L./ -L$(OCPI_BASE_DIR)/lib/$(OCPI_OUT_DIR)

IMPORTS += \
  -lrpl_container \
  -lcp289_container \
  -linterfaces \
  -ltransport \
  -lrdma_driver_interface \
  -lwci_api \
  -lrdma_drivers \
  -lutil \
  -llogger \
  -locpios \
  -lpthread \
  -lrt \
  -ldl

# OpenCV libraries
LIBRARY_PATH += -L/usr/local/lib

IMPORTS += \
	-lcxcore \
	-lcv \
	-lcvaux \
	-lhighgui \
	-lml

OBJS_canny := \
	canny.o \
	DemoWorkerFacade.o \
	$(NULL)

# ---- Common include paths ----------------------------------------------- #

INCLUDE_PATH += \
        -I$(OCPI_BASE_DIR)/core/local/util/vfs/include \
        -I$(OCPI_BASE_DIR)/core/local/util/misc/include \
        -I$(OCPI_BASE_DIR)/core/local/util/list/include \
        -I$(OCPI_BASE_DIR)/core/local/util/misc/include \
        -I$(OCPI_BASE_DIR)/core/local/util/ezxml/include \
        -I$(OCPI_BASE_DIR)/core/local/util/thread/include \
        -I$(OCPI_BASE_DIR)/core/local/util/driver/include \
        -I$(OCPI_BASE_DIR)/core/local/util/filefs/include \
        -I$(OCPI_BASE_DIR)/core/local/util/timeEmit/include \
        -I$(OCPI_BASE_DIR)/core/control/wci_api/api/include \
        -I$(OCPI_BASE_DIR)/core/local/util/property/include \
        -I$(OCPI_BASE_DIR)/adapt/os/ocpios/interfaces/include \
        -I$(OCPI_BASE_DIR)/core/local/util/parentChild/include \
        -I$(OCPI_BASE_DIR)/core/container/interfaces/api/include \
        -I$(OCPI_BASE_DIR)/core/dataplane/transport/impl/include \
        -I$(OCPI_BASE_DIR)/core/control/wci_api/interface/include \
        -I$(OCPI_BASE_DIR)/core/container/cp289_container/cp289/include \
        -I$(OCPI_BASE_DIR)/core/dataplane/transport/client_server/include \
        -I$(OCPI_BASE_DIR)/core/dataplane/rdma_driver_interface/interfaces/include

# OpenCV inclues
INCLUDE_PATH += \
		-I/usr/local/include/opencv

# ---- Makefile fragment with the build rules ----------------------------- #

include ../scripts/build/scripts/tools.mk
