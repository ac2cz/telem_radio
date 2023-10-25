################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/cmd_console.c \
../src/config.c \
../src/gpio_interface.c \
../src/main.c \
../src/serial.c 

C_DEPS += \
./src/cmd_console.d \
./src/config.d \
./src/gpio_interface.d \
./src/main.d \
./src/serial.d 

OBJS += \
./src/cmd_console.o \
./src/config.o \
./src/gpio_interface.o \
./src/main.o \
./src/serial.o 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -DRASPBERRY_PI -I../inc -I../dsp/inc -I../audio/inc -I../telem_send/inc -I../telem_capture/inc -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-src

clean-src:
	-$(RM) ./src/cmd_console.d ./src/cmd_console.o ./src/config.d ./src/config.o ./src/gpio_interface.d ./src/gpio_interface.o ./src/main.d ./src/main.o ./src/serial.d ./src/serial.o

.PHONY: clean-src

