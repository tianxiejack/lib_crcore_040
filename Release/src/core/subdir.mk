################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CU_SRCS += \
../src/core/cuda.cu \
../src/core/cuda_convert.cu 

CPP_SRCS += \
../src/core/BlobDetector.cpp \
../src/core/Displayer.cpp \
../src/core/GeneralProcess.cpp \
../src/core/SVMDetectProcess.cpp \
../src/core/SceneProcess.cpp \
../src/core/backgroundProcess.cpp \
../src/core/blobDetectProcess.cpp \
../src/core/crCore.cpp \
../src/core/encTrans.cpp \
../src/core/gluVideoWindow.cpp \
../src/core/gluVideoWindowSecondScreen.cpp \
../src/core/gluWindow.cpp \
../src/core/glvideo.cpp \
../src/core/glvideoRender.cpp \
../src/core/mmtdProcess.cpp \
../src/core/motionDetectProcess.cpp \
../src/core/thread.cpp \
../src/core/trackerProcess.cpp 

OBJS += \
./src/core/BlobDetector.o \
./src/core/Displayer.o \
./src/core/GeneralProcess.o \
./src/core/SVMDetectProcess.o \
./src/core/SceneProcess.o \
./src/core/backgroundProcess.o \
./src/core/blobDetectProcess.o \
./src/core/crCore.o \
./src/core/cuda.o \
./src/core/cuda_convert.o \
./src/core/encTrans.o \
./src/core/gluVideoWindow.o \
./src/core/gluVideoWindowSecondScreen.o \
./src/core/gluWindow.o \
./src/core/glvideo.o \
./src/core/glvideoRender.o \
./src/core/mmtdProcess.o \
./src/core/motionDetectProcess.o \
./src/core/thread.o \
./src/core/trackerProcess.o 

CU_DEPS += \
./src/core/cuda.d \
./src/core/cuda_convert.d 

CPP_DEPS += \
./src/core/BlobDetector.d \
./src/core/Displayer.d \
./src/core/GeneralProcess.d \
./src/core/SVMDetectProcess.d \
./src/core/SceneProcess.d \
./src/core/backgroundProcess.d \
./src/core/blobDetectProcess.d \
./src/core/crCore.d \
./src/core/encTrans.d \
./src/core/gluVideoWindow.d \
./src/core/gluVideoWindowSecondScreen.d \
./src/core/gluWindow.d \
./src/core/glvideo.d \
./src/core/glvideoRender.d \
./src/core/mmtdProcess.d \
./src/core/motionDetectProcess.d \
./src/core/thread.d \
./src/core/trackerProcess.d 


# Each subdirectory must supply rules for building sources it contributes
src/core/%.o: ../src/core/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: NVCC Compiler'
	/usr/local/cuda-8.0/bin/nvcc -I../include -I../src/nvMedia -I../src/encTrans -I/usr/include/opencv -I/usr/include/GL -I../src/core -I../include/osa -I../src/osd -I../include/GL -I../include/osd -I../src/core/UtilNPP -I../src/nvMedia/libjpeg-8b -O3 -Xcompiler -fPIC -Xcompiler -fopenmp -ccbin aarch64-linux-gnu-g++ -gencode arch=compute_50,code=sm_50 -m64 -odir "src/core" -M -o "$(@:%.o=%.d)" "$<"
	/usr/local/cuda-8.0/bin/nvcc -I../include -I../src/nvMedia -I../src/encTrans -I/usr/include/opencv -I/usr/include/GL -I../src/core -I../include/osa -I../src/osd -I../include/GL -I../include/osd -I../src/core/UtilNPP -I../src/nvMedia/libjpeg-8b -O3 -Xcompiler -fPIC -Xcompiler -fopenmp --compile -m64 -ccbin aarch64-linux-gnu-g++  -x c++ -o  "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/core/%.o: ../src/core/%.cu
	@echo 'Building file: $<'
	@echo 'Invoking: NVCC Compiler'
	/usr/local/cuda-8.0/bin/nvcc -I../include -I../src/nvMedia -I../src/encTrans -I/usr/include/opencv -I/usr/include/GL -I../src/core -I../include/osa -I../src/osd -I../include/GL -I../include/osd -I../src/core/UtilNPP -I../src/nvMedia/libjpeg-8b -O3 -Xcompiler -fPIC -Xcompiler -fopenmp -ccbin aarch64-linux-gnu-g++ -gencode arch=compute_50,code=sm_50 -m64 -odir "src/core" -M -o "$(@:%.o=%.d)" "$<"
	/usr/local/cuda-8.0/bin/nvcc -I../include -I../src/nvMedia -I../src/encTrans -I/usr/include/opencv -I/usr/include/GL -I../src/core -I../include/osa -I../src/osd -I../include/GL -I../include/osd -I../src/core/UtilNPP -I../src/nvMedia/libjpeg-8b -O3 -Xcompiler -fPIC -Xcompiler -fopenmp --compile --relocatable-device-code=false -gencode arch=compute_50,code=compute_50 -gencode arch=compute_50,code=sm_50 -m64 -ccbin aarch64-linux-gnu-g++  -x cu -o  "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


