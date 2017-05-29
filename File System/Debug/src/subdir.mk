################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/File\ System.c 

OBJS += \
./src/File\ System.o 

C_DEPS += \
./src/File\ System.d 


# Each subdirectory must supply rules for building sources it contributes
src/File\ System.o: ../src/File\ System.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross GCC Compiler'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"src/File System.d" -MT"src/File\ System.d" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


