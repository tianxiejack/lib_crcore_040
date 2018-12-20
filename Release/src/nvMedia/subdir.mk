################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/nvMedia/NvApplicationProfiler.cpp \
../src/nvMedia/NvBuffer.cpp \
../src/nvMedia/NvCudaProc.cpp \
../src/nvMedia/NvEglRenderer.cpp \
../src/nvMedia/NvElement.cpp \
../src/nvMedia/NvElementProfiler.cpp \
../src/nvMedia/NvJpegDecoder.cpp \
../src/nvMedia/NvJpegEncoder.cpp \
../src/nvMedia/NvLogging.cpp \
../src/nvMedia/NvUtils.cpp \
../src/nvMedia/NvV4l2Element.cpp \
../src/nvMedia/NvV4l2ElementPlane.cpp \
../src/nvMedia/NvVideoConverter.cpp \
../src/nvMedia/NvVideoDecoder.cpp \
../src/nvMedia/NvVideoEncoder.cpp \
../src/nvMedia/crEglRenderer.cpp 

CU_SRCS += \
../src/nvMedia/NvAnalysis.cu 

CU_DEPS += \
./src/nvMedia/NvAnalysis.d 

OBJS += \
./src/nvMedia/NvAnalysis.o \
./src/nvMedia/NvApplicationProfiler.o \
./src/nvMedia/NvBuffer.o \
./src/nvMedia/NvCudaProc.o \
./src/nvMedia/NvEglRenderer.o \
./src/nvMedia/NvElement.o \
./src/nvMedia/NvElementProfiler.o \
./src/nvMedia/NvJpegDecoder.o \
./src/nvMedia/NvJpegEncoder.o \
./src/nvMedia/NvLogging.o \
./src/nvMedia/NvUtils.o \
./src/nvMedia/NvV4l2Element.o \
./src/nvMedia/NvV4l2ElementPlane.o \
./src/nvMedia/NvVideoConverter.o \
./src/nvMedia/NvVideoDecoder.o \
./src/nvMedia/NvVideoEncoder.o \
./src/nvMedia/crEglRenderer.o 

CPP_DEPS += \
./src/nvMedia/NvApplicationProfiler.d \
./src/nvMedia/NvBuffer.d \
./src/nvMedia/NvCudaProc.d \
./src/nvMedia/NvEglRenderer.d \
./src/nvMedia/NvElement.d \
./src/nvMedia/NvElementProfiler.d \
./src/nvMedia/NvJpegDecoder.d \
./src/nvMedia/NvJpegEncoder.d \
./src/nvMedia/NvLogging.d \
./src/nvMedia/NvUtils.d \
./src/nvMedia/NvV4l2Element.d \
./src/nvMedia/NvV4l2ElementPlane.d \
./src/nvMedia/NvVideoConverter.d \
./src/nvMedia/NvVideoDecoder.d \
./src/nvMedia/NvVideoEncoder.d \
./src/nvMedia/crEglRenderer.d 


# Each subdirectory must supply rules for building sources it contributes
src/nvMedia/%.o: ../src/nvMedia/%.cu
	@echo 'Building file: $<'
	@echo 'Invoking: NVCC Compiler'
	/usr/local/cuda-8.0/bin/nvcc -I../include -I../src/nvMedia -I../src/encTrans -I/usr/include/opencv -I/usr/include/GL -I../src/core -I../include/osa -I../src/osd -I../include/GL -I../include/osd -I../src/core/UtilNPP -I../src/nvMedia/libjpeg-8b -O3 -Xcompiler -fPIC -Xcompiler -fopenmp -ccbin aarch64-linux-gnu-g++ -gencode arch=compute_50,code=sm_50 -m64 -odir "src/nvMedia" -M -o "$(@:%.o=%.d)" "$<"
	/usr/local/cuda-8.0/bin/nvcc -I../include -I../src/nvMedia -I../src/encTrans -I/usr/include/opencv -I/usr/include/GL -I../src/core -I../include/osa -I../src/osd -I../include/GL -I../include/osd -I../src/core/UtilNPP -I../src/nvMedia/libjpeg-8b -O3 -Xcompiler -fPIC -Xcompiler -fopenmp --compile --relocatable-device-code=false -gencode arch=compute_50,code=compute_50 -gencode arch=compute_50,code=sm_50 -m64 -ccbin aarch64-linux-gnu-g++  -x cu -o  "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/nvMedia/%.o: ../src/nvMedia/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: NVCC Compiler'
	/usr/local/cuda-8.0/bin/nvcc -I../include -I../src/nvMedia -I../src/encTrans -I/usr/include/opencv -I/usr/include/GL -I../src/core -I../include/osa -I../src/osd -I../include/GL -I../include/osd -I../src/core/UtilNPP -I../src/nvMedia/libjpeg-8b -O3 -Xcompiler -fPIC -Xcompiler -fopenmp -ccbin aarch64-linux-gnu-g++ -gencode arch=compute_50,code=sm_50 -m64 -odir "src/nvMedia" -M -o "$(@:%.o=%.d)" "$<"
	/usr/local/cuda-8.0/bin/nvcc -I../include -I../src/nvMedia -I../src/encTrans -I/usr/include/opencv -I/usr/include/GL -I../src/core -I../include/osa -I../src/osd -I../include/GL -I../include/osd -I../src/core/UtilNPP -I../src/nvMedia/libjpeg-8b -O3 -Xcompiler -fPIC -Xcompiler -fopenmp --compile -m64 -ccbin aarch64-linux-gnu-g++  -x c++ -o  "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


