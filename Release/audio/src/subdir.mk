################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../audio/src/audio_processor.c \
../audio/src/audio_tools.c \
../audio/src/jack_audio.c 

C_DEPS += \
./audio/src/audio_processor.d \
./audio/src/audio_tools.d \
./audio/src/jack_audio.d 

OBJS += \
./audio/src/audio_processor.o \
./audio/src/audio_tools.o \
./audio/src/jack_audio.o 


# Each subdirectory must supply rules for building sources it contributes
audio/src/%.o: ../audio/src/%.c audio/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -DRASPBERRY_PI -I../inc -I../dsp/inc -I../audio/inc -I../telem/inc -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-audio-2f-src

clean-audio-2f-src:
	-$(RM) ./audio/src/audio_processor.d ./audio/src/audio_processor.o ./audio/src/audio_tools.d ./audio/src/audio_tools.o ./audio/src/jack_audio.d ./audio/src/jack_audio.o

.PHONY: clean-audio-2f-src

