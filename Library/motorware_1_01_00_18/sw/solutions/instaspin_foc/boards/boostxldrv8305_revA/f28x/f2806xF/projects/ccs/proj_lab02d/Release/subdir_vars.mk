################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CMD_SRCS += \
/Users/paiva/Documents/ti/motorware_1_01_00_18/sw/ide/ccs/cmd/f2806x/f28069F_ram_lnk.cmd 

LIB_SRCS += \
/Users/paiva/Documents/ti/motorware_1_01_00_18/sw/modules/fast/lib/32b/f28x/f2806x/2806xRevB_FastSpinROMSymbols_fpu32.lib \
/Users/paiva/Documents/ti/motorware_1_01_00_18/sw/modules/iqmath/lib/f28x/float/IQmath_fpu32.lib 

ASM_SRCS += \
/Users/paiva/Documents/ti/motorware_1_01_00_18/sw/drivers/cpu/src/32b/f28x/f2806x/CodeStartBranch.asm \
/Users/paiva/Documents/ti/motorware_1_01_00_18/sw/modules/usDelay/src/32b/f28x/usDelay.asm 

C_SRCS += \
/Users/paiva/Documents/ti/motorware_1_01_00_18/sw/drivers/adc/src/32b/f28x/f2806x/adc.c \
/Users/paiva/Documents/ti/motorware_1_01_00_18/sw/modules/clarke/src/32b/clarke.c \
/Users/paiva/Documents/ti/motorware_1_01_00_18/sw/drivers/clk/src/32b/f28x/f2806x/clk.c \
/Users/paiva/Documents/ti/motorware_1_01_00_18/sw/drivers/cpu/src/32b/f28x/f2806x/cpu.c \
/Users/paiva/Documents/ti/motorware_1_01_00_18/sw/modules/ctrl/src/32b/ctrl.c \
/Users/paiva/Documents/ti/motorware_1_01_00_18/sw/drivers/drvic/drv8305/src/32b/f28x/f2806x/drv8305.c \
/Users/paiva/Documents/ti/motorware_1_01_00_18/sw/modules/filter/src/32b/filter_fo.c \
/Users/paiva/Documents/ti/motorware_1_01_00_18/sw/drivers/flash/src/32b/f28x/f2806x/flash.c \
/Users/paiva/Documents/ti/motorware_1_01_00_18/sw/drivers/gpio/src/32b/f28x/f2806x/gpio.c \
/Users/paiva/Documents/ti/motorware_1_01_00_18/sw/modules/hal/boards/boostxldrv8305_revA/f28x/f2806x/src/hal.c \
/Users/paiva/Documents/ti/motorware_1_01_00_18/sw/modules/ipark/src/32b/ipark.c \
/Users/paiva/Documents/ti/motorware_1_01_00_18/sw/modules/offset/src/32b/offset.c \
/Users/paiva/Documents/ti/motorware_1_01_00_18/sw/drivers/osc/src/32b/f28x/f2806x/osc.c \
/Users/paiva/Documents/ti/motorware_1_01_00_18/sw/modules/park/src/32b/park.c \
/Users/paiva/Documents/ti/motorware_1_01_00_18/sw/modules/pid/src/32b/pid.c \
/Users/paiva/Documents/ti/motorware_1_01_00_18/sw/drivers/pie/src/32b/f28x/f2806x/pie.c \
/Users/paiva/Documents/ti/motorware_1_01_00_18/sw/drivers/pll/src/32b/f28x/f2806x/pll.c \
/Users/paiva/Documents/ti/motorware_1_01_00_18/sw/solutions/instaspin_foc/src/proj_lab02d.c \
/Users/paiva/Documents/ti/motorware_1_01_00_18/sw/drivers/pwm/src/32b/f28x/f2806x/pwm.c \
/Users/paiva/Documents/ti/motorware_1_01_00_18/sw/drivers/pwr/src/32b/f28x/f2806x/pwr.c \
/Users/paiva/Documents/ti/motorware_1_01_00_18/sw/drivers/spi/src/32b/f28x/f2806x/spi.c \
/Users/paiva/Documents/ti/motorware_1_01_00_18/sw/modules/svgen/src/32b/svgen.c \
/Users/paiva/Documents/ti/motorware_1_01_00_18/sw/drivers/timer/src/32b/f28x/f2806x/timer.c \
/Users/paiva/Documents/ti/motorware_1_01_00_18/sw/modules/traj/src/32b/traj.c \
/Users/paiva/Documents/ti/motorware_1_01_00_18/sw/modules/user/src/32b/user.c \
/Users/paiva/Documents/ti/motorware_1_01_00_18/sw/drivers/wdog/src/32b/f28x/f2806x/wdog.c 

C_DEPS += \
./adc.d \
./clarke.d \
./clk.d \
./cpu.d \
./ctrl.d \
./drv8305.d \
./filter_fo.d \
./flash.d \
./gpio.d \
./hal.d \
./ipark.d \
./offset.d \
./osc.d \
./park.d \
./pid.d \
./pie.d \
./pll.d \
./proj_lab02d.d \
./pwm.d \
./pwr.d \
./spi.d \
./svgen.d \
./timer.d \
./traj.d \
./user.d \
./wdog.d 

OBJS += \
./CodeStartBranch.obj \
./adc.obj \
./clarke.obj \
./clk.obj \
./cpu.obj \
./ctrl.obj \
./drv8305.obj \
./filter_fo.obj \
./flash.obj \
./gpio.obj \
./hal.obj \
./ipark.obj \
./offset.obj \
./osc.obj \
./park.obj \
./pid.obj \
./pie.obj \
./pll.obj \
./proj_lab02d.obj \
./pwm.obj \
./pwr.obj \
./spi.obj \
./svgen.obj \
./timer.obj \
./traj.obj \
./usDelay.obj \
./user.obj \
./wdog.obj 

ASM_DEPS += \
./CodeStartBranch.d \
./usDelay.d 

OBJS__QUOTED += \
"CodeStartBranch.obj" \
"adc.obj" \
"clarke.obj" \
"clk.obj" \
"cpu.obj" \
"ctrl.obj" \
"drv8305.obj" \
"filter_fo.obj" \
"flash.obj" \
"gpio.obj" \
"hal.obj" \
"ipark.obj" \
"offset.obj" \
"osc.obj" \
"park.obj" \
"pid.obj" \
"pie.obj" \
"pll.obj" \
"proj_lab02d.obj" \
"pwm.obj" \
"pwr.obj" \
"spi.obj" \
"svgen.obj" \
"timer.obj" \
"traj.obj" \
"usDelay.obj" \
"user.obj" \
"wdog.obj" 

C_DEPS__QUOTED += \
"adc.d" \
"clarke.d" \
"clk.d" \
"cpu.d" \
"ctrl.d" \
"drv8305.d" \
"filter_fo.d" \
"flash.d" \
"gpio.d" \
"hal.d" \
"ipark.d" \
"offset.d" \
"osc.d" \
"park.d" \
"pid.d" \
"pie.d" \
"pll.d" \
"proj_lab02d.d" \
"pwm.d" \
"pwr.d" \
"spi.d" \
"svgen.d" \
"timer.d" \
"traj.d" \
"user.d" \
"wdog.d" 

ASM_DEPS__QUOTED += \
"CodeStartBranch.d" \
"usDelay.d" 

ASM_SRCS__QUOTED += \
"/Users/paiva/Documents/ti/motorware_1_01_00_18/sw/drivers/cpu/src/32b/f28x/f2806x/CodeStartBranch.asm" \
"/Users/paiva/Documents/ti/motorware_1_01_00_18/sw/modules/usDelay/src/32b/f28x/usDelay.asm" 

C_SRCS__QUOTED += \
"/Users/paiva/Documents/ti/motorware_1_01_00_18/sw/drivers/adc/src/32b/f28x/f2806x/adc.c" \
"/Users/paiva/Documents/ti/motorware_1_01_00_18/sw/modules/clarke/src/32b/clarke.c" \
"/Users/paiva/Documents/ti/motorware_1_01_00_18/sw/drivers/clk/src/32b/f28x/f2806x/clk.c" \
"/Users/paiva/Documents/ti/motorware_1_01_00_18/sw/drivers/cpu/src/32b/f28x/f2806x/cpu.c" \
"/Users/paiva/Documents/ti/motorware_1_01_00_18/sw/modules/ctrl/src/32b/ctrl.c" \
"/Users/paiva/Documents/ti/motorware_1_01_00_18/sw/drivers/drvic/drv8305/src/32b/f28x/f2806x/drv8305.c" \
"/Users/paiva/Documents/ti/motorware_1_01_00_18/sw/modules/filter/src/32b/filter_fo.c" \
"/Users/paiva/Documents/ti/motorware_1_01_00_18/sw/drivers/flash/src/32b/f28x/f2806x/flash.c" \
"/Users/paiva/Documents/ti/motorware_1_01_00_18/sw/drivers/gpio/src/32b/f28x/f2806x/gpio.c" \
"/Users/paiva/Documents/ti/motorware_1_01_00_18/sw/modules/hal/boards/boostxldrv8305_revA/f28x/f2806x/src/hal.c" \
"/Users/paiva/Documents/ti/motorware_1_01_00_18/sw/modules/ipark/src/32b/ipark.c" \
"/Users/paiva/Documents/ti/motorware_1_01_00_18/sw/modules/offset/src/32b/offset.c" \
"/Users/paiva/Documents/ti/motorware_1_01_00_18/sw/drivers/osc/src/32b/f28x/f2806x/osc.c" \
"/Users/paiva/Documents/ti/motorware_1_01_00_18/sw/modules/park/src/32b/park.c" \
"/Users/paiva/Documents/ti/motorware_1_01_00_18/sw/modules/pid/src/32b/pid.c" \
"/Users/paiva/Documents/ti/motorware_1_01_00_18/sw/drivers/pie/src/32b/f28x/f2806x/pie.c" \
"/Users/paiva/Documents/ti/motorware_1_01_00_18/sw/drivers/pll/src/32b/f28x/f2806x/pll.c" \
"/Users/paiva/Documents/ti/motorware_1_01_00_18/sw/solutions/instaspin_foc/src/proj_lab02d.c" \
"/Users/paiva/Documents/ti/motorware_1_01_00_18/sw/drivers/pwm/src/32b/f28x/f2806x/pwm.c" \
"/Users/paiva/Documents/ti/motorware_1_01_00_18/sw/drivers/pwr/src/32b/f28x/f2806x/pwr.c" \
"/Users/paiva/Documents/ti/motorware_1_01_00_18/sw/drivers/spi/src/32b/f28x/f2806x/spi.c" \
"/Users/paiva/Documents/ti/motorware_1_01_00_18/sw/modules/svgen/src/32b/svgen.c" \
"/Users/paiva/Documents/ti/motorware_1_01_00_18/sw/drivers/timer/src/32b/f28x/f2806x/timer.c" \
"/Users/paiva/Documents/ti/motorware_1_01_00_18/sw/modules/traj/src/32b/traj.c" \
"/Users/paiva/Documents/ti/motorware_1_01_00_18/sw/modules/user/src/32b/user.c" \
"/Users/paiva/Documents/ti/motorware_1_01_00_18/sw/drivers/wdog/src/32b/f28x/f2806x/wdog.c" 


