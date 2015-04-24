################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/DChat.cpp \
../src/chatMembers.cpp \
../src/udpServer.cpp \
../src/utils.cpp 

OBJS += \
./src/DChat.o \
./src/chatMembers.o \
./src/udpServer.o \
./src/utils.o 

CPP_DEPS += \
./src/DChat.d \
./src/chatMembers.d \
./src/udpServer.d \
./src/utils.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


