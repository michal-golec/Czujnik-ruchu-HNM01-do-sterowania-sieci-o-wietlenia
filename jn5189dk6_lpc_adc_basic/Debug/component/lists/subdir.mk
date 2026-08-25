################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../component/lists/generic_list.c 

C_DEPS += \
./component/lists/generic_list.d 

OBJS += \
./component/lists/generic_list.o 


# Each subdirectory must supply rules for building sources it contributes
component/lists/%.o: ../component/lists/%.c component/lists/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: MCU C Compiler'
	arm-none-eabi-gcc -std=gnu99 -D__REDLIB__ -DCPU_JN5189HN -DCPU_JN5189HN_cm4 -DCPU_JN5189 -DJN5189DK6 -DDK6 -DCPU_JN518X -DSDK_DEBUGCONSOLE=1 -DCR_INTEGER_PRINTF -DPRINTF_FLOAT_ENABLE=0 -D__MCUXPRESSO -D__USE_CMSIS -DDEBUG -I"C:\NXP\mg_workspace\jn5189dk6_lpc_adc_basic\source" -I"C:\NXP\mg_workspace\jn5189dk6_lpc_adc_basic" -I"C:\NXP\mg_workspace\jn5189dk6_lpc_adc_basic\drivers" -I"C:\NXP\mg_workspace\jn5189dk6_lpc_adc_basic\device" -I"C:\NXP\mg_workspace\jn5189dk6_lpc_adc_basic\utilities" -I"C:\NXP\mg_workspace\jn5189dk6_lpc_adc_basic\component\serial_manager" -I"C:\NXP\mg_workspace\jn5189dk6_lpc_adc_basic\component\lists" -I"C:\NXP\mg_workspace\jn5189dk6_lpc_adc_basic\component\uart" -I"C:\NXP\mg_workspace\jn5189dk6_lpc_adc_basic\CMSIS" -I"C:\NXP\mg_workspace\jn5189dk6_lpc_adc_basic\board" -O0 -fno-common -g3 -gdwarf-4 -Wall -c  -ffunction-sections  -fdata-sections  -ffreestanding  -fno-builtin -fmerge-constants -fmacro-prefix-map="$(<D)/"= -mcpu=cortex-m4 -mthumb -D__REDLIB__ -fstack-usage -specs=redlib.specs -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-component-2f-lists

clean-component-2f-lists:
	-$(RM) ./component/lists/generic_list.d ./component/lists/generic_list.o

.PHONY: clean-component-2f-lists

