################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL = cmd.exe

# Each subdirectory must supply rules for building sources it contributes
build-1649527345:
	@$(MAKE) --no-print-directory -Onone -f subdir_rules.mk build-1649527345-inproc

build-1649527345-inproc: ../event.cfg
	@echo 'Building file: "$<"'
	@echo 'Invoking: XDCtools'
	"C:/ti/ccs1011/xdctools_3_61_02_27_core/xs" --xdcpath="C:/ti/simplelink_msp432p4_sdk_3_40_01_02/source;C:/ti/simplelink_msp432p4_sdk_3_40_01_02/kernel/tirtos/packages;" xdc.tools.configuro -o configPkg -t ti.targets.arm.elf.M4F -p ti.platforms.msp432:MSP432P401R -r release -c "C:/ti/ccs1011/ccs/tools/compiler/ti-cgt-arm_20.2.1.LTS" --compileOptions "-mv7M4 --code_state=16 --float_support=FPv4SPD16 -me --include_path=\"C:/advembed_labs/MSOE_LIB\" --include_path=\"C:/ti/ccs1011/ccs/ccs_base/arm/include\" --include_path=\"C:/advembed_labs/bartmanLab6\" --include_path=\"C:/advembed_labs/bartmanLab6/Debug\" --include_path=\"C:/ti/simplelink_msp432p4_sdk_3_40_01_02/source\" --include_path=\"C:/ti/simplelink_msp432p4_sdk_3_40_01_02/source/third_party/CMSIS/Include\" --include_path=\"C:/ti/simplelink_msp432p4_sdk_3_40_01_02/source/ti/posix/ccs\" --include_path=\"C:/ti/ccs1011/ccs/tools/compiler/ti-cgt-arm_20.2.1.LTS/include\" --advice:power=none -g --diag_warning=225 --diag_warning=255 --diag_wrap=off --display_error_number --gen_func_subsections=on  " "$<"
	@echo 'Finished building: "$<"'
	@echo ' '

configPkg/linker.cmd: build-1649527345 ../event.cfg
configPkg/compiler.opt: build-1649527345
configPkg/: build-1649527345

build-1002708449: ../event.syscfg
	@echo 'Building file: "$<"'
	@echo 'Invoking: SysConfig'
	"C:/ti/ccs1011/ccs/utils/sysconfig_1.6.0/sysconfig_cli.bat" -s "C:/ti/simplelink_msp432p4_sdk_3_40_01_02/.metadata/product.json" -o "syscfg" "$<"
	@echo 'Finished building: "$<"'
	@echo ' '

syscfg/ti_drivers_config.c: build-1002708449 ../event.syscfg
syscfg/ti_drivers_config.h: build-1002708449
syscfg/syscfg_c.rov.xs: build-1002708449
syscfg/: build-1002708449

syscfg/%.obj: ./syscfg/%.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: ARM Compiler'
	"C:/ti/ccs1011/ccs/tools/compiler/ti-cgt-arm_20.2.1.LTS/bin/armcl" -mv7M4 --code_state=16 --float_support=FPv4SPD16 -me --include_path="C:/advembed_labs/MSOE_LIB" --include_path="C:/ti/ccs1011/ccs/ccs_base/arm/include" --include_path="C:/advembed_labs/bartmanLab6" --include_path="C:/advembed_labs/bartmanLab6/Debug" --include_path="C:/ti/simplelink_msp432p4_sdk_3_40_01_02/source" --include_path="C:/ti/simplelink_msp432p4_sdk_3_40_01_02/source/third_party/CMSIS/Include" --include_path="C:/ti/simplelink_msp432p4_sdk_3_40_01_02/source/ti/posix/ccs" --include_path="C:/ti/ccs1011/ccs/tools/compiler/ti-cgt-arm_20.2.1.LTS/include" --advice:power=none -g --diag_warning=225 --diag_warning=255 --diag_wrap=off --display_error_number --gen_func_subsections=on --preproc_with_compile --preproc_dependency="syscfg/$(basename $(<F)).d_raw" --include_path="C:/advembed_labs/bartmanLab6/Debug/syscfg" --obj_directory="syscfg" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: "$<"'
	@echo ' '

%.obj: ../%.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: ARM Compiler'
	"C:/ti/ccs1011/ccs/tools/compiler/ti-cgt-arm_20.2.1.LTS/bin/armcl" -mv7M4 --code_state=16 --float_support=FPv4SPD16 -me --include_path="C:/advembed_labs/MSOE_LIB" --include_path="C:/ti/ccs1011/ccs/ccs_base/arm/include" --include_path="C:/advembed_labs/bartmanLab6" --include_path="C:/advembed_labs/bartmanLab6/Debug" --include_path="C:/ti/simplelink_msp432p4_sdk_3_40_01_02/source" --include_path="C:/ti/simplelink_msp432p4_sdk_3_40_01_02/source/third_party/CMSIS/Include" --include_path="C:/ti/simplelink_msp432p4_sdk_3_40_01_02/source/ti/posix/ccs" --include_path="C:/ti/ccs1011/ccs/tools/compiler/ti-cgt-arm_20.2.1.LTS/include" --advice:power=none -g --diag_warning=225 --diag_warning=255 --diag_wrap=off --display_error_number --gen_func_subsections=on --preproc_with_compile --preproc_dependency="$(basename $(<F)).d_raw" --include_path="C:/advembed_labs/bartmanLab6/Debug/syscfg" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: "$<"'
	@echo ' '


