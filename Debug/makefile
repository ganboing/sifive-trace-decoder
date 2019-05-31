################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

CFLAGS += -O0 -g3 -Wall -Wformat=0

# All Target
all: dqr.exe
debug: dqr.exe

# Tool invocations
dqr.exe: $(OBJS) 
	@echo 'Building target: $@'
	@echo 'Invoking: GCC C++ Linker'
	g++ -L$(LIBPATH) -static -o "dqr.exe" $(OBJS) $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) *.d $(OBJS) dqr.exe
	-@echo ' '

.PHONY: all clean dependents
