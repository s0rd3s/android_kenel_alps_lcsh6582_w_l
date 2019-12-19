include $(srctree)/drivers/misc/mediatek/Makefile.custom

# Linux driver folder
ccflags-y += -I$(srctree)/arch/arm/mach-$(MTK_PLATFORM)/$(ARCH_MTK_PROJECT)/touchpanel/MSG2138/
ccflags-y += -I$(srctree)/drivers/input/touchscreen/mediatek/MSG2138/
ccflags-y += -I$(srctree)/drivers/input/touchscreen/mediatek/

obj-y	+=  msg2133_driver.o
obj-y	+=  msg2133_test.o

