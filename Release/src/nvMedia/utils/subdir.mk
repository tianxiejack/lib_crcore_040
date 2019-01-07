################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/nvMedia/utils/Thread.cpp 

OBJS += \
./src/nvMedia/utils/Thread.o 

CPP_DEPS += \
./src/nvMedia/utils/Thread.d 


# Each subdirectory must supply rules for building sources it contributes
src/nvMedia/utils/%.o: ../src/nvMedia/utils/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: NVCC Compiler'
	/usr/local/cuda-8.0/bin/nvcc -I../include -I../src/nvMedia -I../src/encTrans -I/usr/include/opencv -I/usr/include/GL -I../src/core -I../include/osa -I../src/osd -I../include/GL -I../include/osd -I../src/core/UtilNPP -I../src/nvMedia/libjpeg-8b -O3 -Xcompiler -fPIC -Xcompiler -fopenmp -ccbin aarch64-linux-gnu-g++ -gencode arch=compute_50,code=sm_50 -m64 -odir "src/nvMedia/utils" -M -o "$(@:%.o=%.d)" "$<"
	/usr/local/cuda-8.0/bin/nvcc -I../include -I../src/nvMedia -I../src/encTrans -I/usr/include/opencv -I/usr/include/GL -I../src/core -I../include/osa -I../src/osd -I../include/GL -I../include/osd -I../src/core/UtilNPP -I../src/nvMedia/libjpeg-8b -O3 -Xcompiler -fPIC -Xcompiler -fopenmp --compile -m64 -ccbin aarch64-linux-gnu-g++  -x c++ -o  "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


