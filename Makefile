INCLUDE=-I./include -I./src/common
LIBS=./lib/libmpi.a ./lib/libive.a ./lib/libhdmi.a ./lib/lib_hiae.a ./lib/libisp.a ./lib/lib_hidehaze.a ./lib/lib_hidrc.a ./lib/lib_hildci.a ./lib/lib_hiawb.a ./lib/libsns_imx327.a ./lib/libsns_imx327_2l.a ./lib/libsns_imx307.a ./lib/libsns_imx335.a ./lib/libsns_imx458.a ./lib/libsns_mn34220.a ./lib/libsns_os05a.a ./lib/libVoiceEngine.a ./lib/libupvqe.a ./lib/libdnvqe.a ./lib/libsecurec.a ./lib/libnnie.a  -lpthread -lm -ldl

SRCS := $(wildcard *.c)
SRCS += $(wildcard ./src/common/*.c)
CenterSrcs = ${SRCS} ./src/main.c

benchsrcs = ${SRCS} ./src/benchmark.c

all: centerface benchmark

centerface:
	arm-himix200-linux-gcc -o centerface -w -DISP_V2 -DHI_RELEASE -DVER_X=1 -DVER_Y=0 -DVER_Z=0  -DVER_P=0 -DVER_B=10 -DUSER_BIT_32 -DKERNEL_BIT_32 -Wno-date-time -DSENSOR0_TYPE=SONY_IMX327_MIPI_2M_30FPS_12BIT -DSENSOR1_TYPE=SONY_IMX327_MIPI_2M_30FPS_12BIT -mcpu=cortex-a7 -mfloat-abi=softfp -mfpu=neon-vfpv4 -fno-aggressive-loop-optimizations -ldl -ffunction-sections -fdata-sections -O2 -fstack-protector-strong -fPIC ${CenterSrcs} -Wl,--start-group $(INCLUDE) $(LIBS) -Wl,--end-group

benchmark:
	arm-himix200-linux-gcc -o benchmark -w -DISP_V2 -DHI_RELEASE -DVER_X=1 -DVER_Y=0 -DVER_Z=0  -DVER_P=0 -DVER_B=10 -DUSER_BIT_32 -DKERNEL_BIT_32 -Wno-date-time -DSENSOR0_TYPE=SONY_IMX327_MIPI_2M_30FPS_12BIT -DSENSOR1_TYPE=SONY_IMX327_MIPI_2M_30FPS_12BIT -mcpu=cortex-a7 -mfloat-abi=softfp -mfpu=neon-vfpv4 -fno-aggressive-loop-optimizations -ldl -ffunction-sections -fdata-sections -O2 -fstack-protector-strong -fPIC ${benchsrcs} -Wl,--start-group $(INCLUDE) $(LIBS) -Wl,--end-group

clean:
	rm -rfv centerface benchmark