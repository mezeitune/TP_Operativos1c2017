################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/CPU.c \
../src/dummy_ansisop.c \
../src/funcionesComunes.c 

OBJS += \
./src/CPU.o \
./src/dummy_ansisop.o \
./src/funcionesComunes.o 

C_DEPS += \
./src/CPU.d \
./src/dummy_ansisop.d \
./src/funcionesComunes.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross GCC Compiler'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


