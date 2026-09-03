################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../startup/startup_jn5189.c 

C_DEPS += \
./startup/startup_jn5189.d 

OBJS += \
./startup/startup_jn5189.o 


# Each subdirectory must supply rules for building sources it contributes
startup/%.o: ../startup/%.c startup/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: MCU C Compiler'
	arm-none-eabi-gcc -D__REDLIB__ -DCPU_JN5189HN -DCPU_JN5189HN_cm4 -DFSL_RTOS_BM -DSDK_OS_BAREMETAL -DSDK_DEBUGCONSOLE=1 -DCR_INTEGER_PRINTF -DPRINTF_FLOAT_ENABLE=0 -D__MCUXPRESSO -D__USE_CMSIS -DDEBUG -I"C:\NXP\mg_workspace\Czujnik-ruchu-HNM01-do-sterowania-sieci-o-wietlenia\JN5189_Project4\drivers" -I"C:\NXP\mg_workspace\Czujnik-ruchu-HNM01-do-sterowania-sieci-o-wietlenia\JN5189_Project4\component\lists" -I"C:\NXP\mg_workspace\Czujnik-ruchu-HNM01-do-sterowania-sieci-o-wietlenia\JN5189_Project4\component\serial_manager" -I"C:\NXP\mg_workspace\Czujnik-ruchu-HNM01-do-sterowania-sieci-o-wietlenia\JN5189_Project4\CMSIS" -I"C:\NXP\mg_workspace\Czujnik-ruchu-HNM01-do-sterowania-sieci-o-wietlenia\JN5189_Project4\component\uart" -I"C:\NXP\mg_workspace\Czujnik-ruchu-HNM01-do-sterowania-sieci-o-wietlenia\JN5189_Project4\utilities" -I"C:\NXP\mg_workspace\Czujnik-ruchu-HNM01-do-sterowania-sieci-o-wietlenia\JN5189_Project4\device" -I"C:\NXP\mg_workspace\Czujnik-ruchu-HNM01-do-sterowania-sieci-o-wietlenia\JN5189_Project4\board" -I"C:\NXP\mg_workspace\Czujnik-ruchu-HNM01-do-sterowania-sieci-o-wietlenia\JN5189_Project4\source" -I"C:\NXP\mg_workspace\Czujnik-ruchu-HNM01-do-sterowania-sieci-o-wietlenia\JN5189_Project4" -O0 -fno-common -g3 -gdwarf-4 -Wall -c -ffunction-sections -fdata-sections -fno-builtin -fmerge-constants -fmacro-prefix-map="$(<D)/"= -mcpu=cortex-m4 -mthumb -D__REDLIB__ -fstack-usage -specs=redlib.specs -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-startup

clean-startup:
	-$(RM) ./startup/startup_jn5189.d ./startup/startup_jn5189.o

.PHONY: clean-startup

