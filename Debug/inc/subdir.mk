################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../inc/telem_processor.c 

C_DEPS += \
./inc/telem_processor.d 

OBJS += \
./inc/telem_processor.o 


# Each subdirectory must supply rules for building sources it contributes
inc/%.o: ../inc/%.c inc/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-inc

clean-inc:
	-$(RM) ./inc/telem_processor.d ./inc/telem_processor.o

.PHONY: clean-inc

