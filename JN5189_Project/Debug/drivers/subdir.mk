################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../drivers/fsl_adc.c \
../drivers/fsl_clock.c \
../drivers/fsl_common.c \
../drivers/fsl_ctimer.c \
../drivers/fsl_flash.c \
../drivers/fsl_flexcomm.c \
../drivers/fsl_gpio.c \
../drivers/fsl_inputmux.c \
../drivers/fsl_power.c \
../drivers/fsl_reset.c \
../drivers/fsl_usart.c 

C_DEPS += \
./drivers/fsl_adc.d \
./drivers/fsl_clock.d \
./drivers/fsl_common.d \
./drivers/fsl_ctimer.d \
./drivers/fsl_flash.d \
./drivers/fsl_flexcomm.d \
./drivers/fsl_gpio.d \
./drivers/fsl_inputmux.d \
./drivers/fsl_power.d \
./drivers/fsl_reset.d \
./drivers/fsl_usart.d 

OBJS += \
./drivers/fsl_adc.o \
./drivers/fsl_clock.o \
./drivers/fsl_common.o \
./drivers/fsl_ctimer.o \
./drivers/fsl_flash.o \
./drivers/fsl_flexcomm.o \
./drivers/fsl_gpio.o \
./drivers/fsl_inputmux.o \
./drivers/fsl_power.o \
./drivers/fsl_reset.o \
./drivers/fsl_usart.o 


# Each subdirectory must supply rules for building sources it contributes
drivers/%.o: ../drivers/%.c drivers/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: MCU C Compiler'
	arm-none-eabi-gcc -D__REDLIB__ -DCPU_JN5189HN -DCPU_JN5189HN_cm4 -DFSL_RTOS_BM -DSDK_OS_BAREMETAL -DSDK_DEBUGCONSOLE=1 -DCR_INTEGER_PRINTF -DPRINTF_FLOAT_ENABLE=0 -D__MCUXPRESSO -D__USE_CMSIS -DDEBUG -I"C:\NXP\mg_workspace\Czujnik-ruchu-HNM01-do-sterowania-sieci-o-wietlenia\JN5189_Project\drivers" -I"C:\NXP\mg_workspace\Czujnik-ruchu-HNM01-do-sterowania-sieci-o-wietlenia\JN5189_Project\component\lists" -I"C:\NXP\mg_workspace\Czujnik-ruchu-HNM01-do-sterowania-sieci-o-wietlenia\JN5189_Project\component\serial_manager" -I"C:\NXP\mg_workspace\Czujnik-ruchu-HNM01-do-sterowania-sieci-o-wietlenia\JN5189_Project\CMSIS" -I"C:\NXP\mg_workspace\Czujnik-ruchu-HNM01-do-sterowania-sieci-o-wietlenia\JN5189_Project\component\uart" -I"C:\NXP\mg_workspace\Czujnik-ruchu-HNM01-do-sterowania-sieci-o-wietlenia\JN5189_Project\utilities" -I"C:\NXP\mg_workspace\Czujnik-ruchu-HNM01-do-sterowania-sieci-o-wietlenia\JN5189_Project\device" -I"C:\NXP\mg_workspace\Czujnik-ruchu-HNM01-do-sterowania-sieci-o-wietlenia\JN5189_Project\board" -I"C:\NXP\mg_workspace\Czujnik-ruchu-HNM01-do-sterowania-sieci-o-wietlenia\JN5189_Project\source" -I"C:\NXP\mg_workspace\Czujnik-ruchu-HNM01-do-sterowania-sieci-o-wietlenia\JN5189_Project" -O0 -fno-common -g3 -gdwarf-4 -Wall -c -ffunction-sections -fdata-sections -fno-builtin -fmerge-constants -fmacro-prefix-map="$(<D)/"= -mcpu=cortex-m4 -mthumb -D__REDLIB__ -fstack-usage -specs=redlib.specs -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-drivers

clean-drivers:
	-$(RM) ./drivers/fsl_adc.d ./drivers/fsl_adc.o ./drivers/fsl_clock.d ./drivers/fsl_clock.o ./drivers/fsl_common.d ./drivers/fsl_common.o ./drivers/fsl_ctimer.d ./drivers/fsl_ctimer.o ./drivers/fsl_flash.d ./drivers/fsl_flash.o ./drivers/fsl_flexcomm.d ./drivers/fsl_flexcomm.o ./drivers/fsl_gpio.d ./drivers/fsl_gpio.o ./drivers/fsl_inputmux.d ./drivers/fsl_inputmux.o ./drivers/fsl_power.d ./drivers/fsl_power.o ./drivers/fsl_reset.d ./drivers/fsl_reset.o ./drivers/fsl_usart.d ./drivers/fsl_usart.o

.PHONY: clean-drivers

