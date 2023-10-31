################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../telem_capture/src/device_ads1015.c \
../telem_capture/src/device_ds3231.c \
../telem_capture/src/device_lps25hb.c 

C_DEPS += \
./telem_capture/src/device_ads1015.d \
./telem_capture/src/device_ds3231.d \
./telem_capture/src/device_lps25hb.d 

OBJS += \
./telem_capture/src/device_ads1015.o \
./telem_capture/src/device_ds3231.o \
./telem_capture/src/device_lps25hb.o 


# Each subdirectory must supply rules for building sources it contributes
telem_capture/src/%.o: ../telem_capture/src/%.c telem_capture/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I../inc -I../dsp/inc -I../audio/inc -I../telem_send/inc -I../telem_capture/inc -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-telem_capture-2f-src

clean-telem_capture-2f-src:
	-$(RM) ./telem_capture/src/device_ads1015.d ./telem_capture/src/device_ads1015.o ./telem_capture/src/device_ds3231.d ./telem_capture/src/device_ds3231.o ./telem_capture/src/device_lps25hb.d ./telem_capture/src/device_lps25hb.o

.PHONY: clean-telem_capture-2f-src

