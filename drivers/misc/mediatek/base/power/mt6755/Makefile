ccflags-y += -I$(srctree)/drivers/misc/mediatek/base/power/include/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/base/power/include/spm_v2/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/base/power/cpuidle_v1/

ccflags-y += -I$(srctree)/drivers/misc/mediatek/irq/$(MTK_PLATFORM)/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/dramc/$(MTK_PLATFORM)/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/thermal/fakeHeader/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/video/$(MTK_PLATFORM)/videox/

obj-y += mt_pm_init.o
obj-y += mt_clkbuf_ctl.o
obj-y += mt_dcm.o
#obj-y += mt_spm.o
#obj-y += mt_spm_fs.o
#obj-y += mt_spm_internal.o
obj-y += mt_spm_mtcmos.o
#obj-y += mt_pbm.o
obj-$(CONFIG_DEBUG_FS) += mt_clkdbg.o
obj-y += mt_picachu.o
obj-y += mt_static_power.o
obj-y += mt_cpufreq.o
obj-y += mt_cpufreq_hybrid.o
obj-y += mt_gpufreq.o
obj-y += mt_ptp.o

obj-y += mt_vcorefs_governor.o
obj-y += mt_vcorefs_manager.o

ifneq ($(CONFIG_ARM64), y)

#obj-y += mt-smp.o
#obj-y += hotplug.o
#obj-y += mt-headsmp.o
#obj-y += mt_hotplug.o

else

obj-$(CONFIG_MTK_PSCI)  += mt_psci.o

endif

obj-y += mt_hotplug_strategy_main.o
obj-y += mt_hotplug_strategy_core.o
obj-y += mt_hotplug_strategy_procfs.o
obj-y += mt_hotplug_strategy_cpu.o
obj-y += mt_hotplug_strategy_api.o
obj-y += mt_hotplug_strategy_algo.o
