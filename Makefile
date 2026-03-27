##################################################################
#
#	Makefile for GeoAnalysis using gcc on Linux. 
#       Add analyzed data to generated NTuple from PiDA tool
#
#
#	Modified	by	Reason
# 	--------	--	------
#	20-Mar-26       CBL     Original
#
#
######################################################################
# Machine specific stuff
#
#
TARGET = GeoAnalysis
#
# Compile time resolution.
#
INCLUDE = -I$(ROOT_INC) -I$(DRIVE)/common/utility \
	-I$(DRIVE)/common/libNavBasic -I/usr/include/hdf5/serial
LIBS = $(ROOT_LIBS) -lutility -lNavBasic -lproj -lhdf5_cpp -lhdf5
LIBS += -L$(HDF5LIB) -lconfig++


# Rules to make the object files depend on the sources.
SRC     = 
SRCCPP  = main.cpp PiDA.cpp UserSignals.cpp
SRCS    = $(SRC) $(SRCCPP)

HEADERS = PiDA.hh UserSignals.hh Version.hh

# When we build all, what do we build?
all:      $(TARGET)

include $(DRIVE)/common/makefiles/makefile.inc

