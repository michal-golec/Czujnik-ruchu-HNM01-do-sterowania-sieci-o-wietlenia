################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include utilities/subdir.mk
-include startup/subdir.mk
-include source/subdir.mk
-include drivers/subdir.mk
-include device/subdir.mk
-include component/uart/subdir.mk
-include component/serial_manager/subdir.mk
-include component/lists/subdir.mk
-include board/subdir.mk
ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

-include ../makefile.defs

OPTIONAL_TOOL_DEPS := \
$(wildcard ../makefile.defs) \
$(wildcard ../makefile.init) \
$(wildcard ../makefile.targets) \


BUILD_ARTIFACT_NAME := jn5189dk6_lpc_adc_basic
BUILD_ARTIFACT_EXTENSION := axf
BUILD_ARTIFACT_PREFIX :=
BUILD_ARTIFACT := $(BUILD_ARTIFACT_PREFIX)$(BUILD_ARTIFACT_NAME)$(if $(BUILD_ARTIFACT_EXTENSION),.$(BUILD_ARTIFACT_EXTENSION),)

# Add inputs and outputs from these tool invocations to the build variables 

# All Target
all:
	+@$(MAKE) --no-print-directory main-build && $(MAKE) --no-print-directory post-build

# Main-build Target
main-build: jn5189dk6_lpc_adc_basic.axf

# Tool invocations
jn5189dk6_lpc_adc_basic.axf: $(OBJS) $(USER_OBJS) makefile $(OPTIONAL_TOOL_DEPS)
	@echo 'Building target: $@'
	@echo 'Invoking: MCU Linker'
	arm-none-eabi-gcc -nostdlib -Xlinker -Map="jn5189dk6_lpc_adc_basic.map" -Xlinker --gc-sections -Xlinker -print-memory-usage -Xlinker --sort-section=alignment -Xlinker --cref -mcpu=cortex-m4 -mthumb -T jn5189dk6_lpc_adc_basic_Debug.ld -o "jn5189dk6_lpc_adc_basic.axf" $(OBJS) $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) jn5189dk6_lpc_adc_basic.axf
	-@echo ' '

post-build:
	-@echo 'Performing post-build steps'
	-arm-none-eabi-size "jn5189dk6_lpc_adc_basic.axf"; echo "win32" | grep -q win* && py -3 "../scripts/dk6_image_tool.py" -s 294912  jn5189dk6_lpc_adc_basic.axf || python3 "../scripts/dk6_image_tool.py" -s 294912  jn5189dk6_lpc_adc_basic.axf; arm-none-eabi-objcopy -v -O binary "jn5189dk6_lpc_adc_basic.axf" "jn5189dk6_lpc_adc_basic.bin"
	-@echo ' '

.PHONY: all clean dependents main-build post-build

-include ../makefile.targets
