################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/encTrans/gpio_rdwr.cpp \
../src/encTrans/spidev_trans.cpp \
../src/encTrans/sync422_trans.cpp \
../src/encTrans/vidScheduler.cpp 

OBJS += \
./src/encTrans/gpio_rdwr.o \
./src/encTrans/spidev_trans.o \
./src/encTrans/sync422_trans.o \
./src/encTrans/vidScheduler.o 

CPP_DEPS += \
./src/encTrans/gpio_rdwr.d \
./src/encTrans/spidev_trans.d \
./src/encTrans/sync422_trans.d \
./src/encTrans/vidScheduler.d 


# Each subdirectory must supply rules for building sources it contributes
src/encTrans/%.o: ../src/encTrans/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: NVCC Compiler'
	/usr/local/cuda-8.0/bin/nvcc -I../include -I../src/nvMedia -I../src/cr_osa/inc -I../src/encTrans -I/usr/include/opencv -I/usr/include/GL -I../src/capture -I../src/core -O3 -Xcompiler -fPIC -ccbin aarch64-linux-gnu-g++ -gencode arch=compute_50,code=sm_50 -m64 -odir "src/encTrans" -M -o "$(@:%.o=%.d)" "$<"
	/usr/local/cuda-8.0/bin/nvcc -I../include -I../src/nvMedia -I../src/cr_osa/inc -I../src/encTrans -I/usr/include/opencv -I/usr/include/GL -I../src/capture -I../src/core -O3 -Xcompiler -fPIC --compile -m64 -ccbin aarch64-linux-gnu-g++  -x c++ -o  "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


