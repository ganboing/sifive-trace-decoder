################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

CFLAGS += -O0 -g3 -Wall -Wformat=0
SWIGCFLAGS += -O0 -g3 -Wall -Wformat=0

# All Target
all: $(EXECUTABLE) $(SWIGLIB)
debug: $(EXECUTABLE) $(SWIGLIB)
Debug: $(EXECUTABLE) $(SWIGLIB)

# Tool invocations

$(EXECUTABLE): $(OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: GCC C++ Linker'
	$(CROSSPREFIX)g++ -L$(LIBPATH) $(LNFLAGS) -o $(EXECUTABLE) $(OBJS) $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '

$(SWIGLIB): $(SWIGLIBOBJS)
	@echo 'Building swig library: $@'
	@echo 'Invoking: GCC C++ Linker'
	$(CROSSPREFIX)g++ -shared -L$(LIBPATH) $(SWIGLIBOBJS) $(LIBS) -o $(SWIGLIB)
	@echo 'Finished building swig library: $@'
	@echo ' '

$(SWIGOBJ): $(SWIGCPP)
	@echo 'Building file $@'
	@echo 'Invoking: GCC C++ Compiler'
	$(CROSSPREFIX)g++ $(CFLAGS) $(SWIGCFLAGS) -c $< -I"../include" $(JNIINCLUDEPATHS)
	@echo 'Finished building: $@'
	@echo ' '

$(SWIGCPP): $(INCLUDES) $(SWIGINCLUDES)
	@echo 'Building file $@'
	@echo 'Invoking: SWIG Compiler'
	swig -package com.sifive.trace -outdir com/sifive/trace -java -c++ -o dqr_wrap.cpp -I../include ../include/dqr.i
	javac com/sifive/trace/*.java
	jar cf TraceDecoder.jar com/sifive/trace/*
	@echo 'Finished building: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) *.d $(OBJS) $(EXECUTABLE) $(SWIGCPP) $(SWIGOBJ) $(SWIGLIB) com/sifive/trace/*.java com/sifive/trace/*.class TraceDecoder.jar
	-@echo ' '

install:
ifndef FS_DEV_TRACE_PLUGIN_FOLDER
	$(error FS_DEV_TRACE_PLUGIN_FOLDER is undefined)
endif
	cp TraceDecoder.jar $(FS_DEV_TRACE_PLUGIN_FOLDER)
	cp $(SWIGLIB) $(FS_DEV_TRACE_PLUGIN_FOLDER)/os/win32/x86_64

.PHONY: all clean dependents
