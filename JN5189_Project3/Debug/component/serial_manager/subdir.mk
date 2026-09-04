################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../component/serial_manager/serial_manager.c \
../component/serial_manager/serial_port_uart.c 

C_DEPS += \
./component/serial_manager/serial_manager.d \
./component/serial_manager/serial_port_uart.d 

OBJS += \
./component/serial_manager/serial_manager.o \
./component/serial_manager/serial_port_uart.o 


# Each subdirectory must supply rules for building sources it contributes
component/serial_manager/%.o: ../component/serial_manager/%.c component/serial_manager/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: MCU C Compiler'
	arm-none-eabi-gcc -D__REDLIB__ -DCPU_JN5189HN -DCPU_JN5189HN_cm4 -DFSL_RTOS_BM -DSDK_OS_BAREMETAL -DSDK_DEBUGCONSOLE=1 -DCR_INTEGER_PRINTF -DPRINTF_FLOAT_ENABLE=0 -D__MCUXPRESSO -D__USE_CMSIS -DDEBUG -I"C:\NXP\mg_workspace\Czujnik-ruchu-HNM01-do-sterowania-sieci-o-wietlenia\JN5189_Project3\drivers" -I"C:\NXP\mg_workspace\Czujnik-ruchu-HNM01-do-sterowania-sieci-o-wietlenia\JN5189_Project3\device" -I"C:\NXP\mg_workspace\Czujnik-ruchu-HNM01-do-sterowania-sieci-o-wietlenia\JN5189_Project3\component\uart" -I"C:\NXP\mg_workspace\Czujnik-ruchu-HNM01-do-sterowania-sieci-o-wietlenia\JN5189_Project3\board" -I"C:\NXP\mg_workspace\Czujnik-ruchu-HNM01-do-sterowania-sieci-o-wietlenia\JN5189_Project3\CMSIS" -I"C:\NXP\mg_workspace\Czujnik-ruchu-HNM01-do-sterowania-sieci-o-wietlenia\JN5189_Project3\utilities" -I"C:\NXP\mg_workspace\Czujnik-ruchu-HNM01-do-sterowania-sieci-o-wietlenia\JN5189_Project3\component\serial_manager" -I"C:\NXP\mg_workspace\Czujnik-ruchu-HNM01-do-sterowania-sieci-o-wietlenia\JN5189_Project3\component\lists" -I"C:\NXP\mg_workspace\Czujnik-ruchu-HNM01-do-sterowania-sieci-o-wietlenia\JN5189_Project3\source" -I"C:\NXP\mg_workspace\Czujnik-ruchu-HNM01-do-sterowania-sieci-o-wietlenia\JN5189_Project3" -O0 -fno-common -g3 -gdwarf-4 -Wall -c -ffunction-sections -fdata-sections -fno-builtin -fmerge-constants -fmacro-prefix-map="$(<D)/"= -mcpu=cortex-m4 -mthumb -D__REDLIB__ -fstack-usage -specs=redlib.specs -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-component-2f-serial_manager

clean-component-2f-serial_manager:
	-$(RM) ./component/serial_manager/serial_manager.d ./component/serial_manager/serial_manager.o ./component/serial_manager/serial_port_uart.d ./component/serial_manager/serial_port_uart.o

.PHONY: clean-component-2f-serial_manager

