################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/Project_Chat.cpp \
../src/udpServer.cpp 

OBJS += \
./src/Project_Chat.o \
./src/udpServer.o 

CPP_DEPS += \
./src/Project_Chat.d \
./src/udpServer.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -I"/home/anand/course/CIS505/Project3/CIS505Project/Project_Chat/inc" -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


