################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

CFLAGS += -O3 -Wall -Wformat=0
SWIGCFLAGS += -O -Wall -Wformat=0

# All Target
Release release all: $(EXECUTABLE) $(SWIGLIB)

# Tool invocations
$(EXECUTABLE): $(OBJS)
	@echo 'swiglib: $(SWIGLIB)'
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
	$(CROSSPREFIX)g++ $(SWIGCFLAGS) -c $< -I"..\include" $(JNIINCLUDEPATHS)
	@echo 'Finished building: $@'
	@echo ' '

$(SWIGCPP): $(INCLUDES) $(SWIGINCLUDES)
	@echo 'Building file $@'
	@echo 'Invoking: SWIG Compiler'
	swig -java -c++ -o $@ -I../include ../include/dqr.i
	@echo 'Finished building: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) *.d $(OBJS) $(EXECUTABLE) $(SWIGCPP) $(SWIGOBJ) $(SWIGLIB) *.java *.class
	-@echo ' '

.PHONY: all clean dependents
