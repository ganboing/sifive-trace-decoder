################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

#OBJS += ./dqr.o ./main.o 

CFLAGS := -O3 -Wall -Wformat=0

# All Target
release all: dqr.exe

# Tool invocations
dqr.exe: $(OBJS) 
	@echo 'Building target: $@'
	@echo 'Invoking: GCC C++ Linker'
	g++ -L"../lib" -static -o "dqr.exe" $(OBJS) $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) *.d $(OBJS) dqr.exe
	-@echo ' '

.PHONY: all clean dependents
