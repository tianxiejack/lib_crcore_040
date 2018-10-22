################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/nvMedia/NvBuffer.cpp \
../src/nvMedia/NvElement.cpp \
../src/nvMedia/NvElementProfiler.cpp \
../src/nvMedia/NvLogging.cpp \
../src/nvMedia/NvV4l2Element.cpp \
../src/nvMedia/NvV4l2ElementPlane.cpp \
../src/nvMedia/NvVideoEncoder.cpp 

OBJS += \
./src/nvMedia/NvBuffer.o \
./src/nvMedia/NvElement.o \
./src/nvMedia/NvElementProfiler.o \
./src/nvMedia/NvLogging.o \
./src/nvMedia/NvV4l2Element.o \
./src/nvMedia/NvV4l2ElementPlane.o \
./src/nvMedia/NvVideoEncoder.o 

CPP_DEPS += \
./src/nvMedia/NvBuffer.d \
./src/nvMedia/NvElement.d \
./src/nvMedia/NvElementProfiler.d \
./src/nvMedia/NvLogging.d \
./src/nvMedia/NvV4l2Element.d \
./src/nvMedia/NvV4l2ElementPlane.d \
./src/nvMedia/NvVideoEncoder.d 


# Each subdirectory must supply rules for building sources it contributes
src/nvMedia/%.o: ../src/nvMedia/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: NVCC Compiler'
	/usr/local/cuda-8.0/bin/nvcc -I../include -I../src/nvMedia -I../src/cr_osa/inc -I../src/encTrans -I/usr/include/opencv -I/usr/include/GL -I../src/capture -I../src/core -O3 -Xcompiler -fPIC -ccbin aarch64-linux-gnu-g++ -gencode arch=compute_50,code=sm_50 -m64 -odir "src/nvMedia" -M -o "$(@:%.o=%.d)" "$<"
	/usr/local/cuda-8.0/bin/nvcc -I../include -I../src/nvMedia -I../src/cr_osa/inc -I../src/encTrans -I/usr/include/opencv -I/usr/include/GL -I../src/capture -I../src/core -O3 -Xcompiler -fPIC --compile -m64 -ccbin aarch64-linux-gnu-g++  -x c++ -o  "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


