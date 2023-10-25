################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../telem_send/src/TelemEncoding.c \
../telem_send/src/telem_processor.c \
../telem_send/src/telem_thread.c 

C_DEPS += \
./telem_send/src/TelemEncoding.d \
./telem_send/src/telem_processor.d \
./telem_send/src/telem_thread.d 

OBJS += \
./telem_send/src/TelemEncoding.o \
./telem_send/src/telem_processor.o \
./telem_send/src/telem_thread.o 


# Each subdirectory must supply rules for building sources it contributes
telem_send/src/%.o: ../telem_send/src/%.c telem_send/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I../inc -I../dsp/inc -I../audio/inc -I../telem_send/inc -I../telem_capture/inc -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-telem_send-2f-src

clean-telem_send-2f-src:
	-$(RM) ./telem_send/src/TelemEncoding.d ./telem_send/src/TelemEncoding.o ./telem_send/src/telem_processor.d ./telem_send/src/telem_processor.o ./telem_send/src/telem_thread.d ./telem_send/src/telem_thread.o

.PHONY: clean-telem_send-2f-src

