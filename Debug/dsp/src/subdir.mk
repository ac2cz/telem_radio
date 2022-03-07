################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../dsp/src/cheby_iir_filter.c \
../dsp/src/fir_filter.c \
../dsp/src/iir_filter.c \
../dsp/src/oscillator.c 

C_DEPS += \
./dsp/src/cheby_iir_filter.d \
./dsp/src/fir_filter.d \
./dsp/src/iir_filter.d \
./dsp/src/oscillator.d 

OBJS += \
./dsp/src/cheby_iir_filter.o \
./dsp/src/fir_filter.o \
./dsp/src/iir_filter.o \
./dsp/src/oscillator.o 


# Each subdirectory must supply rules for building sources it contributes
dsp/src/%.o: ../dsp/src/%.c dsp/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I../inc -I../dsp/inc -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-dsp-2f-src

clean-dsp-2f-src:
	-$(RM) ./dsp/src/cheby_iir_filter.d ./dsp/src/cheby_iir_filter.o ./dsp/src/fir_filter.d ./dsp/src/fir_filter.o ./dsp/src/iir_filter.d ./dsp/src/iir_filter.o ./dsp/src/oscillator.d ./dsp/src/oscillator.o

.PHONY: clean-dsp-2f-src

