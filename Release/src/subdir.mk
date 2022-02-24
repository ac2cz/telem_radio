################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/TelemEncoding.c \
../src/audio_processor.c \
../src/audio_tools.c \
../src/fir_filter.c \
../src/iir_filter.c \
../src/main.c \
../src/oscillator.c \
../src/telem_processor.c 

C_DEPS += \
./src/TelemEncoding.d \
./src/audio_processor.d \
./src/audio_tools.d \
./src/fir_filter.d \
./src/iir_filter.d \
./src/main.d \
./src/oscillator.d \
./src/telem_processor.d 

OBJS += \
./src/TelemEncoding.o \
./src/audio_processor.o \
./src/audio_tools.o \
./src/fir_filter.o \
./src/iir_filter.o \
./src/main.o \
./src/oscillator.o \
./src/telem_processor.o 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-src

clean-src:
	-$(RM) ./src/TelemEncoding.d ./src/TelemEncoding.o ./src/audio_processor.d ./src/audio_processor.o ./src/audio_tools.d ./src/audio_tools.o ./src/fir_filter.d ./src/fir_filter.o ./src/iir_filter.d ./src/iir_filter.o ./src/main.d ./src/main.o ./src/oscillator.d ./src/oscillator.o ./src/telem_processor.d ./src/telem_processor.o

.PHONY: clean-src

