################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

CFLAGS += -O3 -Wall -Wformat=0

# All Target
Release release all: $(EXECUTABLE)

# Tool invocations
$(EXECUTABLE): $(OBJS) 
	@echo 'Building target: $@'
	@echo 'Invoking: GCC C++ Linker'
	$(CROSSPREFIX)g++ -L$(LIBPATH) $(LNFLAGS) -o $(EXECUTABLE) $(OBJS) $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) *.d $(OBJS) $(EXECUTABLE)
	-@echo ' '

.PHONY: all clean dependents
