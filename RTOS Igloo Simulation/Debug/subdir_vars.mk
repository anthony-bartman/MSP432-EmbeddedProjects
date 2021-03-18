################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL = cmd.exe

# Add inputs and outputs from these tool invocations to the build variables 
CFG_SRCS += \
../event.cfg 

CMD_SRCS += \
../MSP_EXP432P401R_TIRTOS.cmd 

SYSCFG_SRCS += \
../event.syscfg 

C_SRCS += \
./syscfg/ti_drivers_config.c \
../main.c 

GEN_CMDS += \
./configPkg/linker.cmd 

GEN_FILES += \
./configPkg/linker.cmd \
./configPkg/compiler.opt \
./syscfg/ti_drivers_config.c 

GEN_MISC_DIRS += \
./configPkg/ \
./syscfg/ 

C_DEPS += \
./syscfg/ti_drivers_config.d \
./main.d 

GEN_OPTS += \
./configPkg/compiler.opt 

OBJS += \
./syscfg/ti_drivers_config.obj \
./main.obj 

GEN_MISC_FILES += \
./syscfg/ti_drivers_config.h \
./syscfg/syscfg_c.rov.xs 

GEN_MISC_DIRS__QUOTED += \
"configPkg\" \
"syscfg\" 

OBJS__QUOTED += \
"syscfg\ti_drivers_config.obj" \
"main.obj" 

GEN_MISC_FILES__QUOTED += \
"syscfg\ti_drivers_config.h" \
"syscfg\syscfg_c.rov.xs" 

C_DEPS__QUOTED += \
"syscfg\ti_drivers_config.d" \
"main.d" 

GEN_FILES__QUOTED += \
"configPkg\linker.cmd" \
"configPkg\compiler.opt" \
"syscfg\ti_drivers_config.c" 

SYSCFG_SRCS__QUOTED += \
"../event.syscfg" 

C_SRCS__QUOTED += \
"./syscfg/ti_drivers_config.c" \
"../main.c" 


