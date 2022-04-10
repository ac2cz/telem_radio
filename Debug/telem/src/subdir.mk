################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../telem/src/TelemEncoding.c \
../telem/src/telem_processor.c \
../telem/src/telem_thread.c 

C_DEPS += \
./telem/src/TelemEncoding.d \
./telem/src/telem_processor.d \
./telem/src/telem_thread.d 

OBJS += \
./telem/src/TelemEncoding.o \
./telem/src/telem_processor.o \
./telem/src/telem_thread.o 


# Each subdirectory must supply rules for building sources it contributes
telem/src/%.o: ../telem/src/%.c telem/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I../inc -I../dsp/inc -I../audio/inc -I../telem/inc -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-telem-2f-src

clean-telem-2f-src:
	-$(RM) ./telem/src/TelemEncoding.d ./telem/src/TelemEncoding.o ./telem/src/telem_processor.d ./telem/src/telem_processor.o ./telem/src/telem_thread.d ./telem/src/telem_thread.o

.PHONY: clean-telem-2f-src

