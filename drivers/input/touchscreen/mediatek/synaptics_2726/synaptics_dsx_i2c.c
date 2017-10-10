/* Synaptics DSX touchscreen driver
 *
 * Copyright (C) 2012 Synaptics Incorporated
 *
 * Copyright (C) 2012 Alexandra Chin <alexandra.chin@tw.synaptics.com>
 * Copyright (C) 2012 Scott Lin <scott.lin@tw.synaptics.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/interrupt.h>
#include <linux/input.h>
#include <linux/gpio.h>
#include <linux/dma-mapping.h> 
#include <linux/string.h>

#include <linux/regulator/consumer.h>
#include <linux/suspend.h>
#include <linux/delay.h>
#include <linux/rtpm_prio.h>
#include <linux/kthread.h>

#include "mt_boot_common.h"

#include "tpd.h"

#include "synaptics_dsx_i2c.h"
#include "synaptics_dsx.h"
#include "tpd_custom_synaptics.h"
#ifdef KERNEL_ABOVE_2_6_38
#include <linux/input/mt.h>
#endif

#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#include <linux/of_irq.h>

#define DRIVER_NAME "synaptics_dsx_i2c"
#define INPUT_PHYS_NAME "synaptics_dsx_i2c/input0"

#ifdef KERNEL_ABOVE_2_6_38
/*[Lavender][poting_chang] Support to 10 fingers which is fixed by discarding TYPE_B protocol  20150227 begin*/
#if 0
#define TYPE_B_PROTOCOL
#endif
/*[Lavender][poting_chang] 20150227 end*/
#endif

/*[Lavender][bozhi_lin] synaptics touch detect glove or finger in glove mode 20150720 begin*/
/*[Lavender][poting_chang] GLOVE mode interface  20150303 begin*/
//#define LAVENDER_GLOVE
//static bool glovemode=0;
//static bool glovemode_is_finger=0;
//static bool glovemode_support_detection=0;
//for glove
//static struct synaptics_rmi4_data * rmi4_glove;
/*[Lavender][poting_chang]  20150302 end*/
/*[Lavender][bozhi_lin] 20150720 end*/


#define NO_0D_WHILE_2D
/*
#define REPORT_2D_Z
*/
#define REPORT_2D_W

#define F12_DATA_15_WORKAROUND

/*
#define IGNORE_FN_INIT_FAILURE
*/

#define RPT_TYPE (1 << 0)
#define RPT_X_LSB (1 << 1)
#define RPT_X_MSB (1 << 2)
#define RPT_Y_LSB (1 << 3)
#define RPT_Y_MSB (1 << 4)
#define RPT_Z (1 << 5)
#define RPT_WX (1 << 6)
#define RPT_WY (1 << 7)
#define RPT_DEFAULT (RPT_TYPE | RPT_X_LSB | RPT_X_MSB | RPT_Y_LSB | RPT_Y_MSB)

#define EXP_FN_WORK_DELAY_MS 1000 /* ms */
#define SYN_I2C_RETRY_TIMES 10
#define MAX_F11_TOUCH_WIDTH 15

#define CHECK_STATUS_TIMEOUT_MS 100
#define DELAY_S7300_BOOT_READY  160
#define DELAY_S7300_RESET       20
#define DELAY_S7300_RESET_READY 90
#define I2C_DMA_LIMIT 252

#define F01_STD_QUERY_LEN 21
#define F01_BUID_ID_OFFSET 18
#define F11_STD_QUERY_LEN 9
#define F11_STD_CTRL_LEN 10
#define F11_STD_DATA_LEN 12

#define STATUS_NO_ERROR 0x00
#define STATUS_RESET_OCCURRED 0x01
#define STATUS_INVALID_CONFIG 0x02
#define STATUS_DEVICE_FAILURE 0x03
#define STATUS_CONFIG_CRC_FAILURE 0x04
#define STATUS_FIRMWARE_CRC_FAILURE 0x05
#define STATUS_CRC_IN_PROGRESS 0x06

#define NORMAL_OPERATION (0 << 0)
#define SENSOR_SLEEP (1 << 0)
#define NO_SLEEP_OFF (0 << 2)
#define NO_SLEEP_ON (1 << 2)
#define CONFIGURED (1 << 7)

/*[SM20][zihweishen] config id register change to 0x0009 for new config of firmware  20160422 begin*/
/*[Lavender][bozhi_lin] config id register change to 0x000A with 6 inch touch  20150304 begin*/
/*[Lavender][poting_chang] add Synaptics_S2726 config id for firmware check 20150211 begin*/
#define REG_CONFIG_ID	0x0009
/*[Lavender][poting_chang] 20150211 end*/
/*[Lavender][bozhi_lin] 20150304 end*/
/*[SM20][zihweishen] 20160422 end*/

/*[Lavender][bozhi_lin] add Synaptics_S2726 product id check 20150305 begin*/
#define REG_PRODUCT_ID	0x0032
/*[Lavender][bozhi_lin] 20150305 end*/

#define SYNAPTICS_MT6752
#define CUST_EINT_TOUCH_PANEL_SENSITIVE 1

//[SM20][zihweishen] Modify power enable begin 2015/11/11
#define   SYN_ENABLE         1
#define   SYN_DISABLE        0
static void syn_power_enable(int enable)
{
#if !defined(BUILD_LK)
    int ret;
#endif

    if( SYN_ENABLE == enable )
    {
    #if defined( BUILD_LK )
        //mt6351_set_register_value( flagname, 1 );
    #else
        printk("Device Tree get regulator! \n");
        tpd->reg = regulator_get(tpd->tpd_dev, "vtouch");
        ret = regulator_set_voltage(tpd->reg, 2800000, 2800000);	/*set 2.8v*/
	if (ret) {
            printk("[elan]regulator_set_voltage(%d) failed!\n", ret);
            return;
        }

        ret = regulator_enable(tpd->reg);	/*enable regulator*/
        if (ret)
        printk("[elan]regulator_enable() failed!\n");
    #endif
    }
    else
    {
    #if defined( BUILD_LK )
        //mt6351_set_register_value( flagname, 0 );
    #else
        ret = regulator_disable(tpd->reg);	/*disable regulator*/
        if (ret)
            printk("[elna]regulator_disable() failed!\n");
    #endif
    }
}
//[SM20][zihweishen] Modify power enable end 2015/11/11


// for MTK
static struct task_struct *thread = NULL;
static DECLARE_WAIT_QUEUE_HEAD(waiter);
static int tpd_halt = 0;
static int tpd_flag = 0;
#ifdef TPD_HAVE_BUTTON 
static int tpd_keys_local[TPD_KEY_COUNT] = TPD_KEYS;
static int tpd_keys_dim_local[TPD_KEY_COUNT][4] = TPD_KEYS_DIM;
#endif
static u8 boot_mode;

//[SM20][zihweishen] Modify DMAbuf begin 2015/11/11
// for DMA accessing
static u8 *gpwDMABuf_va = NULL;
//static u32 gpwDMABuf_pa = NULL;
static dma_addr_t gpwDMABuf_pa=0;
static u8 *gprDMABuf_va = NULL;
//static u32 gprDMABuf_pa = NULL;
static dma_addr_t gprDMABuf_pa=0;
//[SM20][zihweishen] Modify DMAbuf end 2015/11/11


struct i2c_msg *read_msg;
static struct device *g_dev = NULL;

// for 0D button
static unsigned char cap_button_codes[] = TPD_0D_BUTTON;
static struct synaptics_dsx_cap_button_map cap_button_map = {
	.nbuttons = ARRAY_SIZE(cap_button_codes),
	.map = cap_button_codes,
};
unsigned int syn_touch_irq = 0;
//u8 int_type = 0;


/*[Lavender][bozhi_lin] store touch vendor and firmware version to tpd_show_vendor_firmware 20150213 begin*/
#if defined(TPD_REPORT_VENDOR_FW)
extern char *tpd_show_vendor_firmware;
//<2015/10/15-stevenchen, store lcm vendor information in /sys/module/tpd_misc/parameters/lcm_vendor
extern char *lcm_vendor;
//>2015/10/15-stevenchen
#endif
/*[Lavender][bozhi_lin] 20150213 end*/

//static void tpd_eint_handler(void);
//[SM20][zihweishen] Modify touch screen randomly stops working begin 2016/8/23
static int mt_get_gpio_value(int gpio_num)
{
	int value = 0;
	//if (mt_nfc_get_gpio_dir(gpio_num) != MTK_NFC_GPIO_DIR_INVALID) {

		value = __gpio_get_value(gpio_num);

	//}
	return value;
}
//[SM20][zihweishen] end 2016/8/23

static int touch_event_handler(void *data);

static int synaptics_rmi4_i2c_read(struct synaptics_rmi4_data *rmi4_data,
		unsigned short addr, unsigned char *data,
		unsigned short length);

static int synaptics_rmi4_i2c_write(struct synaptics_rmi4_data *rmi4_data,
		unsigned short addr, unsigned char *data,
		unsigned short length);

static int synaptics_rmi4_f12_set_enables(struct synaptics_rmi4_data *rmi4_data,
		unsigned short ctrl28);

static int synaptics_rmi4_free_fingers(struct synaptics_rmi4_data *rmi4_data);
static int synaptics_rmi4_reinit_device(struct synaptics_rmi4_data *rmi4_data);
static int synaptics_rmi4_reset_device(struct synaptics_rmi4_data *rmi4_data);

#ifdef CONFIG_HAS_EARLYSUSPEND
static ssize_t synaptics_rmi4_full_pm_cycle_show(struct device *dev,
		struct device_attribute *attr, char *buf);

static ssize_t synaptics_rmi4_full_pm_cycle_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count);

static void synaptics_rmi4_early_suspend(struct early_suspend *h);

static void synaptics_rmi4_late_resume(struct early_suspend *h);
#endif

//[SM20][zihweishen] Modify  for build error begin 2015/11/11
static int synaptics_rmi4_dev_suspend(struct device *dev);

static int synaptics_rmi4_dev_resume(struct device *dev);
//[SM20][zihweishen] Modify  for build error end 2015/11/11

static void synaptics_rmi4_suspend(struct device *h);

static void synaptics_rmi4_resume(struct device *h);

static ssize_t synaptics_rmi4_f01_reset_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count);

static ssize_t synaptics_rmi4_f01_productinfo_show(struct device *dev,
		struct device_attribute *attr, char *buf);

static ssize_t synaptics_rmi4_f01_buildid_show(struct device *dev,
		struct device_attribute *attr, char *buf);

/*[Lavender][poting_chang] add Synaptics_S2726 config id for firmware check 20150211 begin*/
static ssize_t synaptics_rmi4_f34_configid_show(struct device *dev,
		struct device_attribute *attr, char *buf);
/*[Lavender][poting_chang] 20150211 end*/

/*[Lavender][bozhi_lin] add Synaptics_S2726 product id check 20150305 begin*/
static ssize_t synaptics_rmi4_f01_productid_show(struct device *dev,
		struct device_attribute *attr, char *buf);
/*[Lavender][bozhi_lin] 20150305 end*/

static ssize_t synaptics_rmi4_f01_flashprog_show(struct device *dev,
		struct device_attribute *attr, char *buf);

static ssize_t synaptics_rmi4_0dbutton_show(struct device *dev,
		struct device_attribute *attr, char *buf);

static ssize_t synaptics_rmi4_0dbutton_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count);

static ssize_t synaptics_rmi4_suspend_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count);

struct synaptics_rmi4_f01_device_status {
	union {
		struct {
			unsigned char status_code:4;
			unsigned char reserved:2;
			unsigned char flash_prog:1;
			unsigned char unconfigured:1;
		} __packed;
		unsigned char data[1];
	};
};

//[SM20][zihweishen] Add Touch ESD_CHECK begin 2016/02/5
  //#define ESD_CHECK
#if defined( ESD_CHECK )
	static int	  have_interrupts = 0;
	static struct workqueue_struct *esd_wq = NULL;
	static struct delayed_work		esd_work;
	static unsigned long  delay = 2*HZ;

//declare function
  static void synaptic_touch_esd_func(struct work_struct *work);
#endif
//[SM20][zihweishen] end 2016/02/5

struct synaptics_rmi4_f12_query_5 {
	union {
		struct {
			unsigned char size_of_query6;
			struct {
				unsigned char ctrl0_is_present:1;
				unsigned char ctrl1_is_present:1;
				unsigned char ctrl2_is_present:1;
				unsigned char ctrl3_is_present:1;
				unsigned char ctrl4_is_present:1;
				unsigned char ctrl5_is_present:1;
				unsigned char ctrl6_is_present:1;
				unsigned char ctrl7_is_present:1;
			} __packed;
			struct {
				unsigned char ctrl8_is_present:1;
				unsigned char ctrl9_is_present:1;
				unsigned char ctrl10_is_present:1;
				unsigned char ctrl11_is_present:1;
				unsigned char ctrl12_is_present:1;
				unsigned char ctrl13_is_present:1;
				unsigned char ctrl14_is_present:1;
				unsigned char ctrl15_is_present:1;
			} __packed;
			struct {
				unsigned char ctrl16_is_present:1;
				unsigned char ctrl17_is_present:1;
				unsigned char ctrl18_is_present:1;
				unsigned char ctrl19_is_present:1;
				unsigned char ctrl20_is_present:1;
				unsigned char ctrl21_is_present:1;
				unsigned char ctrl22_is_present:1;
				unsigned char ctrl23_is_present:1;
			} __packed;
			struct {
				unsigned char ctrl24_is_present:1;
				unsigned char ctrl25_is_present:1;
				unsigned char ctrl26_is_present:1;
				unsigned char ctrl27_is_present:1;
				unsigned char ctrl28_is_present:1;
				unsigned char ctrl29_is_present:1;
				unsigned char ctrl30_is_present:1;
				unsigned char ctrl31_is_present:1;
			} __packed;
		};
		unsigned char data[5];
	};
};

struct synaptics_rmi4_f12_query_8 {
	union {
		struct {
			unsigned char size_of_query9;
			struct {
				unsigned char data0_is_present:1;
				unsigned char data1_is_present:1;
				unsigned char data2_is_present:1;
				unsigned char data3_is_present:1;
				unsigned char data4_is_present:1;
				unsigned char data5_is_present:1;
				unsigned char data6_is_present:1;
				unsigned char data7_is_present:1;
			} __packed;
			struct {
				unsigned char data8_is_present:1;
				unsigned char data9_is_present:1;
				unsigned char data10_is_present:1;
				unsigned char data11_is_present:1;
				unsigned char data12_is_present:1;
				unsigned char data13_is_present:1;
				unsigned char data14_is_present:1;
				unsigned char data15_is_present:1;
			} __packed;
		};
		unsigned char data[3];
	};
};

struct synaptics_rmi4_f12_ctrl_8 {
	union {
		struct {
			unsigned char max_x_coord_lsb;
			unsigned char max_x_coord_msb;
			unsigned char max_y_coord_lsb;
			unsigned char max_y_coord_msb;
			unsigned char rx_pitch_lsb;
			unsigned char rx_pitch_msb;
			unsigned char tx_pitch_lsb;
			unsigned char tx_pitch_msb;
			unsigned char low_rx_clip;
			unsigned char high_rx_clip;
			unsigned char low_tx_clip;
			unsigned char high_tx_clip;
			unsigned char num_of_rx;
			unsigned char num_of_tx;
		};
		unsigned char data[14];
	};
};

struct synaptics_rmi4_f12_ctrl_23 {
	union {
		struct {
			unsigned char obj_type_enable;
			unsigned char max_reported_objects;
		};
		unsigned char data[2];
	};
};

struct synaptics_rmi4_f12_finger_data {
	unsigned char object_type_and_status;
	unsigned char x_lsb;
	unsigned char x_msb;
	unsigned char y_lsb;
	unsigned char y_msb;
#ifdef REPORT_2D_Z
	unsigned char z;
#endif
#ifdef REPORT_2D_W
	unsigned char wx;
	unsigned char wy;
#endif
};

struct synaptics_rmi4_f1a_query {
	union {
		struct {
			unsigned char max_button_count:3;
			unsigned char reserved:5;
			unsigned char has_general_control:1;
			unsigned char has_interrupt_enable:1;
			unsigned char has_multibutton_select:1;
			unsigned char has_tx_rx_map:1;
			unsigned char has_perbutton_threshold:1;
			unsigned char has_release_threshold:1;
			unsigned char has_strongestbtn_hysteresis:1;
			unsigned char has_filter_strength:1;
		} __packed;
		unsigned char data[2];
	};
};

struct synaptics_rmi4_f1a_control_0 {
	union {
		struct {
			unsigned char multibutton_report:2;
			unsigned char filter_mode:2;
			unsigned char reserved:4;
		} __packed;
		unsigned char data[1];
	};
};

struct synaptics_rmi4_f1a_control {
	struct synaptics_rmi4_f1a_control_0 general_control;
	unsigned char button_int_enable;
	unsigned char multi_button;
	unsigned char *txrx_map;
	unsigned char *button_threshold;
	unsigned char button_release_threshold;
	unsigned char strongest_button_hysteresis;
	unsigned char filter_strength;
};

struct synaptics_rmi4_f1a_handle {
	int button_bitmask_size;
	unsigned char max_count;
	unsigned char valid_button_count;
	unsigned char *button_data_buffer;
	unsigned char *button_map;
	struct synaptics_rmi4_f1a_query button_query;
	struct synaptics_rmi4_f1a_control button_control;
};

struct synaptics_rmi4_exp_fhandler {
	struct synaptics_rmi4_exp_fn *exp_fn;
	bool insert;
	bool remove;
	struct list_head link;
};

struct synaptics_rmi4_exp_fn_data {
	bool initialized;
	bool queue_work;
	struct mutex mutex;
	struct list_head list;
	struct delayed_work work;
	struct workqueue_struct *workqueue;
	struct synaptics_rmi4_data *rmi4_data;
};

static struct synaptics_rmi4_exp_fn_data exp_data;

static struct device_attribute attrs[] = {
#ifdef CONFIG_HAS_EARLYSUSPEND
	__ATTR(full_pm_cycle, (S_IRUGO | S_IWUGO),
			synaptics_rmi4_full_pm_cycle_show,
			synaptics_rmi4_full_pm_cycle_store),
#endif
	__ATTR(reset, 0640,synaptics_rmi4_show_error,synaptics_rmi4_f01_reset_store),
	__ATTR(productinfo, 0640,synaptics_rmi4_f01_productinfo_show,synaptics_rmi4_store_error),
	__ATTR(buildid, 0640,synaptics_rmi4_f01_buildid_show,synaptics_rmi4_store_error),
/*[Lavender][poting_chang] add Synaptics_S2726 config id for firmware check 20150211 begin*/
	__ATTR(configid, 0640,synaptics_rmi4_f34_configid_show,synaptics_rmi4_store_error),
/*[Lavender][poting_chang] 20150211 end*/
/*[Lavender][bozhi_lin] add Synaptics_S2726 product id check 20150305 begin*/
	__ATTR(productid, 0640,synaptics_rmi4_f01_productid_show,synaptics_rmi4_store_error),
/*[Lavender][bozhi_lin] 20150305 end*/
	__ATTR(flashprog, 0640,synaptics_rmi4_f01_flashprog_show,synaptics_rmi4_store_error),
	__ATTR(0dbutton, 0640,synaptics_rmi4_0dbutton_show,synaptics_rmi4_0dbutton_store),
	__ATTR(suspend, 0640,synaptics_rmi4_show_error,synaptics_rmi4_suspend_store),
};
//[SM20][zihweishen] Modify build error end 2015/11/11

//[SM20][zihweishen] Modify to replace mt_eint_mask and mt_eint_registration begin 2015/11/11
/*1 enable,0 disable,touch_panel_eint default status, need to confirm after register eint*/
int syn_irq_flag = 1;
static spinlock_t syn_irq_flag_lock;

void synaptic_irq_enable(void)
{
	unsigned long flags;

	spin_lock_irqsave(&syn_irq_flag_lock, flags);

	if (syn_irq_flag == 0) {
		syn_irq_flag = 1;
		spin_unlock_irqrestore(&syn_irq_flag_lock, flags);
		enable_irq(syn_touch_irq);
	} else if (syn_irq_flag == 1) {
		spin_unlock_irqrestore(&syn_irq_flag_lock, flags);
		printk("Touch Eint already enabled!");
	} else {
		spin_unlock_irqrestore(&syn_irq_flag_lock, flags);
		printk("Invalid syn_irq_flag %d!", syn_irq_flag);
	}
	/*GTP_INFO("Enable irq_flag=%d",irq_flag);*/
}

void synaptic_irq_disable(void)
{
	unsigned long flags;

	spin_lock_irqsave(&syn_irq_flag_lock, flags);

	if (syn_irq_flag == 1) {
		syn_irq_flag = 0;
		spin_unlock_irqrestore(&syn_irq_flag_lock, flags);
		disable_irq(syn_touch_irq);
	} else if (syn_irq_flag == 0) {
		spin_unlock_irqrestore(&syn_irq_flag_lock, flags);
		printk("Touch Eint already disabled!");
	} else {
		spin_unlock_irqrestore(&syn_irq_flag_lock, flags);
		printk("Invalid syn_irq_flag %d!", syn_irq_flag);
	}
	/*GTP_INFO("Disable irq_flag=%d",irq_flag);*/
}

//[SM20][zihweishen] Modify touch screen randomly stops working begin 2016/8/23
 static irqreturn_t tpd_eint_handler(unsigned irq, struct irq_desc *desc)
{
  unsigned long flags;

  spin_lock_irqsave(&syn_irq_flag_lock, flags);
  
//[SM20][zihweishen] Add Touch ESD_CHECK begin 2016/02/5
#if defined( ESD_CHECK )
	have_interrupts = 1;
#endif
	tpd_flag = 1;
//	wake_up_interruptible( &waiter );
//[SM20][zihweishen] end 2016/02/5

  if (syn_irq_flag == 0) 
  {
      spin_unlock_irqrestore(&syn_irq_flag_lock, flags);
      return IRQ_HANDLED;
  }
  /* enter EINT handler disable INT, make sure INT is disable when handle touch event including top/bottom half */
  /* use _nosync to avoid deadlock */
  syn_irq_flag = 0;
  spin_unlock_irqrestore(&syn_irq_flag_lock, flags);
  disable_irq_nosync(syn_touch_irq);
	wake_up_interruptible( &waiter );
  return IRQ_HANDLED;
}
//[SM20][zihweishen] end 2016/8/23
/*static void tpd_eint_handler(void)
{
	TPD_DEBUG_PRINT_INT;
	tpd_flag=1;
	wake_up_interruptible(&waiter);
}*/
	
//[SM20][zihweishen] Modify  to replace mt_eint_mask and mt_eint_registration end 2015/11/11


#ifdef CONFIG_HAS_EARLYSUSPEND
static ssize_t synaptics_rmi4_full_pm_cycle_show(struct device *dev,
		struct device_attribute *attr, const char *buf)
{
	struct synaptics_rmi4_data *rmi4_data = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "%u\n",
			rmi4_data->full_pm_cycle);
}

static ssize_t synaptics_rmi4_full_pm_cycle_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned int input;
	struct synaptics_rmi4_data *rmi4_data = dev_get_drvdata(dev);

	if (sscanf(buf, "%u", &input) != 1)
		return -EINVAL;

	rmi4_data->full_pm_cycle = input > 0 ? 1 : 0;

	return count;
}
#endif




static ssize_t synaptics_rmi4_f01_reset_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int retval;
	unsigned int reset;
	struct synaptics_rmi4_data *rmi4_data = dev_get_drvdata(dev);

	if (sscanf(buf, "%u", &reset) != 1)
		return -EINVAL;

	if (reset != 1)
		return -EINVAL;

	retval = synaptics_rmi4_reset_device(rmi4_data);
	if (retval < 0) {
		dev_err(dev,
				"%s: Failed to issue reset command, error = %d\n",
				__func__, retval);
		return retval;
	}

	return count;
}

static ssize_t synaptics_rmi4_f01_productinfo_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct synaptics_rmi4_data *rmi4_data = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "0x%02x 0x%02x\n",
			(rmi4_data->rmi4_mod_info.product_info[0]),
			(rmi4_data->rmi4_mod_info.product_info[1]));
}

static ssize_t synaptics_rmi4_f01_buildid_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct synaptics_rmi4_data *rmi4_data = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "%u\n",
			rmi4_data->firmware_id);
}

/*[Lavender][poting_chang] add Synaptics_S2726 config id for firmware check 20150211 begin*/
static ssize_t synaptics_rmi4_f34_configid_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct synaptics_rmi4_data *rmi4_data = dev_get_drvdata(dev);
	int retval;
	unsigned char config_id[4]={0};
	
/*[SM20][zihweishen] Modify read config ID and product ID addr for INNX and Truly 20160428 begin*/
	retval = synaptics_rmi4_i2c_read(rmi4_data,
			rmi4_data->f34_ctrl_base,
			config_id,
			sizeof(config_id));
/*[SM20][zihweishen] 20160428 end*/
	if (retval < 0)
		return retval;
	
	return snprintf(buf, PAGE_SIZE, "%02d %02d %02d %02d \n",
			config_id[0], config_id[1], config_id[2], config_id[3]);
}
/*[Lavender][poting_chang] 20150211 end*/

/*[Lavender][bozhi_lin] add Synaptics_S2726 product id check 20150305 begin*/
static ssize_t synaptics_rmi4_f01_productid_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct synaptics_rmi4_data *rmi4_data = dev_get_drvdata(dev);
	int retval;
	unsigned char product_id[10]={0};

/*[SM20][zihweishen] Modify read config ID and product ID addr for INNX and Truly 20160428 begin*/	
	retval = synaptics_rmi4_i2c_read(rmi4_data,
			rmi4_data->f01_query_base_addr + 11,
			product_id,
			sizeof(product_id));
/*[SM20][zihweishen] 20160428 end*/
	if (retval < 0)
		return retval;
	
	return snprintf(buf, PAGE_SIZE, "%s", product_id);
}
/*[Lavender][bozhi_lin] 20150305 end*/

static ssize_t synaptics_rmi4_f01_flashprog_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int retval;
	struct synaptics_rmi4_f01_device_status device_status;
	struct synaptics_rmi4_data *rmi4_data = dev_get_drvdata(dev);

	retval = synaptics_rmi4_i2c_read(rmi4_data,
			rmi4_data->f01_data_base_addr,
			device_status.data,
			sizeof(device_status.data));
	if (retval < 0) {
		dev_err(dev,
				"%s: Failed to read device status, error = %d\n",
				__func__, retval);
		return retval;
	}

	return snprintf(buf, PAGE_SIZE, "%u\n",
			device_status.flash_prog);
}

static ssize_t synaptics_rmi4_0dbutton_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct synaptics_rmi4_data *rmi4_data = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "%u\n",
			rmi4_data->button_0d_enabled);
}

static ssize_t synaptics_rmi4_0dbutton_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int retval;
	unsigned int input;
	unsigned char ii;
	unsigned char intr_enable;
	struct synaptics_rmi4_fn *fhandler;
	struct synaptics_rmi4_data *rmi4_data = dev_get_drvdata(dev);
	struct synaptics_rmi4_device_info *rmi;

	rmi = &(rmi4_data->rmi4_mod_info);

	if (sscanf(buf, "%u", &input) != 1)
		return -EINVAL;

	input = input > 0 ? 1 : 0;

	if (rmi4_data->button_0d_enabled == input)
		return count;

	if (list_empty(&rmi->support_fn_list))
		return -ENODEV;

	list_for_each_entry(fhandler, &rmi->support_fn_list, link) {
		if (fhandler->fn_number == SYNAPTICS_RMI4_F1A) {
			ii = fhandler->intr_reg_num;

			retval = synaptics_rmi4_i2c_read(rmi4_data,
					rmi4_data->f01_ctrl_base_addr + 1 + ii,
					&intr_enable,
					sizeof(intr_enable));
			if (retval < 0)
				return retval;

			if (input == 1)
				intr_enable |= fhandler->intr_mask;
			else
				intr_enable &= ~fhandler->intr_mask;

			retval = synaptics_rmi4_i2c_write(rmi4_data,
					rmi4_data->f01_ctrl_base_addr + 1 + ii,
					&intr_enable,
					sizeof(intr_enable));
			if (retval < 0)
				return retval;
		}
	}

	rmi4_data->button_0d_enabled = input;

	return count;
}

static ssize_t synaptics_rmi4_suspend_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned int input;

	if (sscanf(buf, "%u", &input) != 1)
		return -EINVAL;

	if (input == 1)
		synaptics_rmi4_suspend(dev);
	else if (input == 0)
		synaptics_rmi4_resume(dev);
	else
		return -EINVAL;

	return count;
}

#if defined( LAVENDER_GLOVE )
/*[Lavender][poting_chang] GLOVE mode interface  20150303 begin*/
static ssize_t elan_ktf2k_glove_show(struct device *dev,
    struct device_attribute *attr, char *buf)
{
	ssize_t   ret = 0;
	struct synaptics_rmi4_data *rmi4_data = rmi4_glove;
	uint8_t buf_glove_enable_status[3];
	uint8_t buf_glove_detect_status[1];
	uint8_t buf_glove_thick_status[20];	

	ret = synaptics_rmi4_i2c_read(rmi4_data,
		0x001C,
		buf_glove_enable_status,
		sizeof(buf_glove_enable_status));

	ret = synaptics_rmi4_i2c_read(rmi4_data,
		0x001e,
		buf_glove_detect_status,
		sizeof(buf_glove_detect_status));
	
	ret = synaptics_rmi4_i2c_read(rmi4_data,
		0x0015,
		buf_glove_thick_status,
		sizeof(buf_glove_thick_status));	
	
    if( (buf_glove_enable_status[0] == 0x05) && (buf_glove_detect_status[0]==0x00) && (buf_glove_thick_status[19]==0x00) )
    	{
    			glovemode=0;
				sprintf( buf, "glovemode=%d\n", glovemode );    		
    	}
	else
		{
				glovemode=1;
				sprintf( buf, "glovemode=%d\n", glovemode );    		
		}

	ret = strlen( buf) + 1;
	return ret;
}

//the fuction is used to swich glove mode or normal mode
static void elan_glove_enable(bool glove)
{
	//for 5.5,remember6 inch must to change
	////Object Report Enable,Max Number of Reported Objects,Report as Finger	
	uint8_t buff_glove_en[1]={0x25};
	uint8_t buff_glove_dis[1]={0x05};
	uint8_t buff_glove_detection[1]={0x01};	
	uint8_t buff_glove_dis_detection[1]={0x00};		
	uint8_t buff_glove_thick[20]={0x00}; 
	uint8_t buff_glove_dis_thick[20]={0x00}; 	
		
	uint8_t ret;
	struct synaptics_rmi4_data *rmi4_data = rmi4_glove;
/*[Lavender][bozhi_lin] add check touch ap2 product id to only ap2 can enable glove mode 20150518 begin*/
	struct synaptics_rmi4_device_info *rmi;

	rmi = &(rmi4_data->rmi4_mod_info);

#if 1
	if (!strncmp(rmi->product_id_string, "I53LV3", 6)) {
		printk("[B]%s(%d): rmi->product_id_string=%s, is AP2 run, can enable glove mode\n", __func__, __LINE__, rmi->product_id_string);
	}
	else {
		printk("[B]%s(%d): rmi->product_id_string=%s, is DP run, can't enable glove mode\n", __func__, __LINE__, rmi->product_id_string);
		printk("[B]%s(%d): rmi->product_id_string=%s, is not AP2 run, can't enable glove mode\n", __func__, __LINE__, rmi->product_id_string);
		return;
	}
#else
	if ( (!strncmp(rmi->product_id_string, "s2726", 5)) || (!strncmp(rmi->product_id_string, "S2726", 5))) {
		printk("[B]%s(%d): rmi->product_id_string=%s, is DP1 or DP2 run, can't enable glove mode\n", __func__, __LINE__, rmi->product_id_string);
		return;
	}
	else if (!strncmp(rmi->product_id_string, "I53LV2", 6)) {
		printk("[B]%s(%d): rmi->product_id_string=%s, is SP or AP1 run, can't enable glove mode\n", __func__, __LINE__, rmi->product_id_string);
		return;
	}
	else {
		printk("[B]%s(%d): rmi->product_id_string=%s, is AP2 run, can enable glove mode\n", __func__, __LINE__, rmi->product_id_string);
	}
#endif
/*[Lavender][bozhi_lin] 20150518 end*/
	
	if(glove){
		ret = synaptics_rmi4_i2c_write(rmi4_data,
		 0x001C,
		 buff_glove_en,
		sizeof(buff_glove_en));

		ret = synaptics_rmi4_i2c_write(rmi4_data,
		 0x001e,
		 buff_glove_detection,
		sizeof(buff_glove_detection));

		//0x0015 is speshial
		ret = synaptics_rmi4_i2c_read(rmi4_data,
		0x0015,
		buff_glove_thick,
		sizeof(buff_glove_thick));	
		buff_glove_thick[19]=0x01;
		
		ret = synaptics_rmi4_i2c_write(rmi4_data,
		 0x0015,
		 buff_glove_thick,
		sizeof(buff_glove_thick));		
		/*[Lavender][bozhi_lin] synaptics touch detect glove or finger in glove mode 20150727 begin*/
		glovemode = 1;
		/*[Lavender][bozhi_lin] 20150727 end*/
	}
	else{
		ret = synaptics_rmi4_i2c_write(rmi4_data,
				0x001C,
				buff_glove_dis,
				sizeof(buff_glove_dis));
		ret = synaptics_rmi4_i2c_write(rmi4_data,
				0x001e,
				buff_glove_dis_detection,
				sizeof(buff_glove_dis_detection));
		//0x0015
		ret = synaptics_rmi4_i2c_read(rmi4_data,
				0x0015,
				buff_glove_dis_thick,
				sizeof(buff_glove_dis_thick));
		
		buff_glove_dis_thick[19]=0x00;
		
		ret = synaptics_rmi4_i2c_write(rmi4_data,
				0x0015,
				buff_glove_dis_thick,
				sizeof(buff_glove_dis_thick));				
		/*[Lavender][bozhi_lin] synaptics touch detect glove or finger in glove mode 20150727 begin*/
		glovemode = 0;
		/*[Lavender][bozhi_lin] 20150727 end*/
	}

}

static ssize_t  elan_ktf2k_glove_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	bool GloveSitch;  
	GloveSitch=buf[0]-'0';
	if( GloveSitch == 1)
		elan_glove_enable( true );
	else if(GloveSitch == 0)
		elan_glove_enable( false );
	else
	printk("you send neither 1 nor 0 buf[0]=%d\n",buf[0]);
	printk("elan_ktf2k_vendor_store enter......\n");
	printk("buf[store]=%d %d %d %d\n",buf[0],buf[1],buf[2],buf[3]);
	//	ret = strlen(buf) + 1;
	return count;
	
	
}

static DEVICE_ATTR( glove, S_IWUSR|S_IRUSR|S_IRGRP, elan_ktf2k_glove_show, elan_ktf2k_glove_store );
/*[Lavender][poting_chang]  20150302 end*/

/*[Lavender][bozhi_lin] synaptics touch detect glove or finger in glove mode 20150720 begin*/
static ssize_t synaptics_glovemode_is_finger_show(struct device *dev,
    struct device_attribute *attr, char *buf)
{
	//ssize_t   ret = 0;
	struct synaptics_rmi4_data *rmi4_data = rmi4_glove;
	struct synaptics_rmi4_device_info *rmi;

	rmi = &(rmi4_data->rmi4_mod_info);

	if (!strncmp(rmi->product_id_string, "I53LV3", 6)) {
		printk("[B]%s(%d): rmi->product_id_string=%s, is AP2 run, can support glove mode, glovemode_is_finger=%d\n", __func__, __LINE__, rmi->product_id_string, glovemode_is_finger);
		return snprintf(buf, PAGE_SIZE, "%d", glovemode_is_finger);
	}
	else {
		printk("[B]%s(%d): rmi->product_id_string=%s, is not AP2 run, can not support glove mode, glovemode_is_finger=%d\n", __func__, __LINE__, rmi->product_id_string, glovemode_is_finger);
		glovemode_is_finger = 0;
		return snprintf(buf, PAGE_SIZE, "%d", glovemode_is_finger);
	}
}

static DEVICE_ATTR( glovemode_is_finger, 0640, synaptics_glovemode_is_finger_show, NULL );
/*[Lavender][bozhi_lin] 20150720 end*/
#endif

/*[Lavender][poting_chang] GLOVE mode interface  20150303 begin*/
#if defined( LAVENDER_GLOVE )
static struct kobject * android_touch_kobj = NULL;

static int Syna_touch_sysfs_init(void)
{
	int ret = 0;

	android_touch_kobj = kobject_create_and_add( "android_touch", NULL ) ;
	if( android_touch_kobj == NULL )
	{
		printk( KERN_ERR "[elan]%s: subsystem_register failed\n", __func__ );
		ret = -ENOMEM;
		return ret;
	}

	ret = sysfs_create_file( android_touch_kobj, &dev_attr_glove.attr );
	if( ret )
	{
	printk( KERN_ERR "[elan]%s: sysfs_create_group (dev_attr_glove) failed\n", __func__ );
	return ret;
	}

	/*[Lavender][bozhi_lin] synaptics touch detect glove or finger in glove mode 20150720 begin*/
	ret = sysfs_create_file( android_touch_kobj, &dev_attr_glovemode_is_finger.attr );
	if( ret )
	{
		printk( KERN_ERR "[elan]%s: sysfs_create_group (dev_attr_glovemode_is_finger) failed\n", __func__ );
		return ret;
	}
	/*[Lavender][bozhi_lin] 20150720 end*/

	return 0 ;
}
#endif
/*[Lavender][poting_chang]  20150302 end*/







 /**
 * synaptics_rmi4_set_page()
 *
 * Called by synaptics_rmi4_i2c_read() and synaptics_rmi4_i2c_write().
 *
 * This function writes to the page select register to switch to the
 * assigned page.
 */
static int synaptics_rmi4_set_page(struct synaptics_rmi4_data *rmi4_data,
		unsigned int address)
{
	int retval = 0;
	unsigned char retry;
	unsigned char buf[PAGE_SELECT_LEN];
	unsigned char page;
	struct i2c_client *i2c = rmi4_data->i2c_client;

	page = ((address >> 8) & MASK_8BIT);
	if (page != rmi4_data->current_page) {
		buf[0] = MASK_8BIT;
		buf[1] = page;
		for (retry = 0; retry < SYN_I2C_RETRY_TIMES; retry++) {
			retval = i2c_master_send(i2c, buf, PAGE_SELECT_LEN);
//[SM20][zihweishen]  Dynamic detect elan and Synaptics touch driver begin 2015/11/11
			if (retval != PAGE_SELECT_LEN)
				return -EIO;
//[SM20][zihweishen]  Dynamic detect elan and Synaptics touch driver end 2015/11/11
			if (retval != PAGE_SELECT_LEN) {
				dev_err(&i2c->dev,
						"%s: zihI2C retry %d\n",
						__func__, retry + 1);
				msleep(20);
			} else {
				rmi4_data->current_page = page;
				break;
			}
		}
	} else {
		retval = PAGE_SELECT_LEN;
	}

	return (retval == PAGE_SELECT_LEN) ? retval : -EIO;
}

 //[SM20][zihweishen] Modify  to replace mt_eint_mask and mt_eint_registration begin 2015/11/11
 static int tpd_irq_registration(void)
 {
	 struct device_node *node = NULL;
	 int ret = 0;
	 u32 ints[2] = { 0, 0 };
 
	 printk("Device Tree Tpd_irq_registration!");
 
	 node = of_find_matching_node(node, touch_of_match);
 
	 if (node) 
	 {
		 of_property_read_u32_array(node, "debounce", ints, ARRAY_SIZE(ints));
		 gpio_set_debounce(ints[0], ints[1]);
 
		 syn_touch_irq = irq_of_parse_and_map(node, 0);
 
		 ret = request_irq(syn_touch_irq, (irq_handler_t) tpd_eint_handler, IRQF_TRIGGER_FALLING,"TOUCH_PANEL-eint", NULL);
 
		 if (ret > 0) 
		 {
			 ret = -1;
			 printk("tpd request_irq IRQ LINE NOT AVAILABLE!.");
		 }
		 
	 } 
	 else 
	 {
		 printk("tpd request_irq can not find touch eint device node!.");
		 ret = -1;
	 }
	 
	 printk("[%s]irq:%d, debounce:%d-%d:", __func__, syn_touch_irq, ints[0], ints[1]);
	 
	 return ret;
 }
 //[SM20][zihweishen] Modify  to replace mt_eint_mask and mt_eint_registration end 2015/11/11



 /**
 * synaptics_rmi4_i2c_read()
 *
 * Called by various functions in this driver, and also exported to
 * other expansion Function modules such as rmi_dev.
 *
 * This function reads data of an arbitrary length from the sensor,
 * starting from an assigned register address of the sensor, via I2C
 * with a retry mechanism.
 */
static int synaptics_rmi4_i2c_read(struct synaptics_rmi4_data *rmi4_data,
		unsigned short addr, unsigned char *data, unsigned short length)
{
	int retval;
	unsigned char retry;
	unsigned char buf;
	unsigned char *buf_va = NULL;
	int full = length / I2C_DMA_LIMIT;
	int partial = length % I2C_DMA_LIMIT;
	int total;
	int last;
	int  ii;
	static int msg_length;

	mutex_lock(&(rmi4_data->rmi4_io_ctrl_mutex));

	if(!gprDMABuf_va){
#if defined(SYNAPTICS_MT6752)
	  gprDMABuf_va = (u8 *)dma_alloc_coherent(&(rmi4_data->input_dev->dev), 4096, &gprDMABuf_pa, GFP_KERNEL);
#else	
	  gprDMABuf_va = (u8 *)dma_alloc_coherent(NULL, 4096, &gprDMABuf_pa, GFP_KERNEL);
#endif
	  if(!gprDMABuf_va){
		printk("[Error] Allocate DMA I2C Buffer failed!\n");
	  }
	}

	buf_va = gprDMABuf_va;

	if ((full + 2) > msg_length) {
		kfree(read_msg);
		msg_length = full + 2;
		read_msg = kcalloc(msg_length, sizeof(struct i2c_msg), GFP_KERNEL);
	}

	read_msg[0].addr = (rmi4_data->i2c_client->addr & I2C_MASK_FLAG);
	read_msg[0].flags = 0;
	read_msg[0].len = 1;
	read_msg[0].buf = &buf;
	read_msg[0].timing = 400;

	if (partial) {
		total = full + 1;
		last = partial;
	} else {
		total = full;
		last = I2C_DMA_LIMIT;
	}

	for (ii = 1; ii <= total; ii++) {
		read_msg[ii].addr = (rmi4_data->i2c_client->addr & I2C_MASK_FLAG);
		read_msg[ii].flags = I2C_M_RD;
		read_msg[ii].len = (ii == total) ? last : I2C_DMA_LIMIT;
		read_msg[ii].buf = (u8 *)(uintptr_t)(gprDMABuf_pa+ I2C_DMA_LIMIT * (ii - 1));
		read_msg[ii].ext_flag = (rmi4_data->i2c_client->ext_flag | I2C_ENEXT_FLAG | I2C_DMA_FLAG);
		read_msg[ii].timing = 400;
	}

	buf = addr & MASK_8BIT;

	retval = synaptics_rmi4_set_page(rmi4_data, addr);
	if (retval != PAGE_SELECT_LEN)
		goto exit;

	for (retry = 0; retry < SYN_I2C_RETRY_TIMES; retry++) {
		if (i2c_transfer(rmi4_data->i2c_client->adapter, read_msg, (total + 1)) == (total + 1)) {

			retval = length;
			break;
		}
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: zzzI2C retry %d\n",
				__func__, retry + 1);
		msleep(20);
	}

	if (retry == SYN_I2C_RETRY_TIMES) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: I2C read over retry limit\n",
				__func__);
		retval = -EIO;
	}

	memcpy(data, buf_va, length);

exit:
/*[Lavender][bozhi_lin] fix touch driver cause memory leakage, not free allocated memmory 20150424 begin*/
#if 0
        if(gprDMABuf_va){
                dma_free_coherent(NULL, 4096, gprDMABuf_va, gprDMABuf_pa);
                gprDMABuf_va = NULL;
                gprDMABuf_pa = NULL;
        }
#endif
/*[Lavender][bozhi_lin] 20150424 end*/
	mutex_unlock(&(rmi4_data->rmi4_io_ctrl_mutex));

	return retval;
}

 /**
 * synaptics_rmi4_i2c_write()
 *
 * Called by various functions in this driver, and also exported to
 * other expansion Function modules such as rmi_dev.
 *
 * This function writes data of an arbitrary length to the sensor,
 * starting from an assigned register address of the sensor, via I2C with
 * a retry mechanism.
 */

  //[SM20][zihweishen] Modify for build error begin 2015/11/11
	 static int synaptics_rmi4_i2c_write(struct synaptics_rmi4_data *rmi4_data,
			 unsigned short addr, unsigned char *data, unsigned short length)
	 {
		 int retval;
		 unsigned char retry;
		 //unsigned char buf[length + 1];
		 unsigned char *buf_va = NULL;

		 struct i2c_msg msg;
			
				/* .addr = (rmi4_data->i2c_client->addr & I2C_MASK_FLAG),
				 .flags = 0,
				 .len = length + 1,
				 .buf = (u8 *)(uintptr_t)gpwDMABuf_pa,
				 .ext_flag=(rmi4_data->i2c_client->ext_flag|I2C_ENEXT_FLAG|I2C_DMA_FLAG),
				 .timing = 400,*/

		 
		 mutex_lock(&(rmi4_data->rmi4_io_ctrl_mutex));
	 
		 if(!gpwDMABuf_va){
#if defined(SYNAPTICS_MT6752)
		   gpwDMABuf_va = (u8 *)dma_alloc_coherent(&(rmi4_data->input_dev->dev), 1024, &gpwDMABuf_pa, GFP_KERNEL);
#else	
		   gpwDMABuf_va = (u8 *)dma_alloc_coherent(NULL, 1024, &gpwDMABuf_pa, GFP_KERNEL);
#endif
		   if(!gpwDMABuf_va){
			 printk("[Error] Allocate DMA I2C Buffer failed!\n");
		   }
		 }
		 buf_va = gpwDMABuf_va;
	 
	 				 msg.addr = (rmi4_data->i2c_client->addr & I2C_MASK_FLAG);
				 msg.flags = 0;
				 msg.len = length + 1;
				 msg.buf = (u8 *)(uintptr_t)gpwDMABuf_pa;
				 msg.ext_flag=(rmi4_data->i2c_client->ext_flag|I2C_ENEXT_FLAG|I2C_DMA_FLAG);
				 msg.timing = 400;
		 retval = synaptics_rmi4_set_page(rmi4_data, addr);
		 if (retval != PAGE_SELECT_LEN)
			 goto exit;
	 
		 buf_va[0] = addr & MASK_8BIT;
	 
		 memcpy(&buf_va[1],&data[0] , length);

		 for (retry = 0; retry < SYN_I2C_RETRY_TIMES; retry++) {
			 if (i2c_transfer(rmi4_data->i2c_client->adapter, &msg, 1) == 1) {
				 retval = length;
				 break;
			 }
			 dev_err(&rmi4_data->i2c_client->dev,
					 "%s: I2C retry %d\n",
					 __func__, retry + 1);
			 msleep(20);
		 }
	 
		 if (retry == SYN_I2C_RETRY_TIMES) {
			 dev_err(&rmi4_data->i2c_client->dev,
					 "%s: I2C write over retry limit\n",
					 __func__);
			 retval = -EIO;
		 }
	 
	 exit:


/*[Lavender][bozhi_lin] fix touch driver cause memory leakage, not free allocated memmory 20150424 begin*/
#if 0
	if(gpwDMABuf_va){
                dma_free_coherent(NULL, 1024, gpwDMABuf_va, gpwDMABuf_pa);
                gpwDMABuf_va = NULL;
                gpwDMABuf_pa = NULL;
        }
#endif
/*[Lavender][bozhi_lin] 20150424 end*/
	mutex_unlock(&(rmi4_data->rmi4_io_ctrl_mutex));

	return retval;
}
 //[SM20][zihweishen] Modify for build error end 2015/11/11

 /**
 * synaptics_rmi4_f11_abs_report()
 *
 * Called by synaptics_rmi4_report_touch() when valid Function $11
 * finger data has been detected.
 *
 * This function reads the Function $11 data registers, determines the
 * status of each finger supported by the Function, processes any
 * necessary coordinate manipulation, reports the finger data to
 * the input subsystem, and returns the number of fingers detected.
 */
static int synaptics_rmi4_f11_abs_report(struct synaptics_rmi4_data *rmi4_data,
		struct synaptics_rmi4_fn *fhandler)
{
	int retval;
	unsigned char touch_count = 0; /* number of touch points */
	unsigned char reg_index;
	unsigned char finger;
	unsigned char fingers_supported;
	unsigned char num_of_finger_status_regs;
	unsigned char finger_shift;
	unsigned char finger_status;
	unsigned char data_reg_blk_size;
	unsigned char finger_status_reg[3];
	unsigned char data[F11_STD_DATA_LEN];
	unsigned short data_addr;
	unsigned short data_offset;
	int x;
	int y;
	int wx;
	int wy;
	//int temp;

	/*
	 * The number of finger status registers is determined by the
	 * maximum number of fingers supported - 2 bits per finger. So
	 * the number of finger status registers to read is:
	 * register_count = ceil(max_num_of_fingers / 4)
	 */
	fingers_supported = fhandler->num_of_data_points;
	num_of_finger_status_regs = (fingers_supported + 3) / 4;
	data_addr = fhandler->full_addr.data_base;
	data_reg_blk_size = fhandler->size_of_data_register_block;

	retval = synaptics_rmi4_i2c_read(rmi4_data,
			data_addr,
			finger_status_reg,
			num_of_finger_status_regs);
	if (retval < 0)
		return 0;

	for (finger = 0; finger < fingers_supported; finger++) {
		reg_index = finger / 4;
		finger_shift = (finger % 4) * 2;
		finger_status = (finger_status_reg[reg_index] >> finger_shift)
				& MASK_2BIT;

		/*
		 * Each 2-bit finger status field represents the following:
		 * 00 = finger not present
		 * 01 = finger present and data accurate
		 * 10 = finger present but data may be inaccurate
		 * 11 = reserved
		 */
#ifdef TYPE_B_PROTOCOL
		input_mt_slot(rmi4_data->input_dev, finger);
		input_mt_report_slot_state(rmi4_data->input_dev,
				MT_TOOL_FINGER, finger_status);
#endif

		if (finger_status) {
			data_offset = data_addr +
					num_of_finger_status_regs +
					(finger * data_reg_blk_size);
			retval = synaptics_rmi4_i2c_read(rmi4_data,
					data_offset,
					data,
					data_reg_blk_size);
			if (retval < 0)
				return 0;

			x = (data[0] << 4) | (data[2] & MASK_4BIT);
			y = (data[1] << 4) | ((data[2] >> 4) & MASK_4BIT);
			wx = (data[3] & MASK_4BIT);
			wy = (data[3] >> 4) & MASK_4BIT;
			
			x = (rmi4_data->sensor_max_x) - x;
			y = (rmi4_data->sensor_max_y) - y;

			input_report_key(rmi4_data->input_dev,
					BTN_TOUCH, 1);
			input_report_key(rmi4_data->input_dev,
					BTN_TOOL_FINGER, 1);
			input_report_abs(rmi4_data->input_dev,
					ABS_MT_POSITION_X, x);
			input_report_abs(rmi4_data->input_dev,
					ABS_MT_POSITION_Y, y);
#ifdef REPORT_2D_W
			input_report_abs(rmi4_data->input_dev,
					ABS_MT_TOUCH_MAJOR, max(wx, wy));
			input_report_abs(rmi4_data->input_dev,
					ABS_MT_TOUCH_MINOR, min(wx, wy));
#endif
#ifndef TYPE_B_PROTOCOL
			input_mt_sync(rmi4_data->input_dev);
#endif

#ifdef TPD_HAVE_BUTTON
			if (NORMAL_BOOT != boot_mode)
			{   
				tpd_button(x, y, 1);  
			}	
#endif
			dev_dbg(&rmi4_data->i2c_client->dev,
					"%s: Finger %d:\n"
					"status = 0x%02x\n"
					"synaptics_rmi4_f12_abs_report x = %d\n"
					"synaptics_rmi4_f12_abs_report y = %d\n"
					"wx = %d\n"
					"wy = %d\n",
					__func__, finger,
					finger_status,
					x, y, wx, wy);

			touch_count++;
		}
	}

	if (touch_count == 0) {
		input_report_key(rmi4_data->input_dev,
				BTN_TOUCH, 0);
		input_report_key(rmi4_data->input_dev,
				BTN_TOOL_FINGER, 0);
#ifndef TYPE_B_PROTOCOL
		input_mt_sync(rmi4_data->input_dev);
#endif
#ifdef TPD_HAVE_BUTTON
		if (NORMAL_BOOT != boot_mode)
		{   
			tpd_button(x, y, 0); 
		}   
#endif
	}

	input_sync(rmi4_data->input_dev);

	return touch_count;
}

 /**
 * synaptics_rmi4_f12_abs_report()
 *
 * Called by synaptics_rmi4_report_touch() when valid Function $12
 * finger data has been detected.
 *
 * This function reads the Function $12 data registers, determines the
 * status of each finger supported by the Function, processes any
 * necessary coordinate manipulation, reports the finger data to
 * the input subsystem, and returns the number of fingers detected.
 */
static int synaptics_rmi4_f12_abs_report(struct synaptics_rmi4_data *rmi4_data,
		struct synaptics_rmi4_fn *fhandler)
{
	int retval;
	unsigned char touch_count = 0; /* number of touch points */
	unsigned char finger;
	unsigned char fingers_to_process;
	unsigned char finger_status;
	unsigned char size_of_2d_data;
	unsigned short data_addr;
	int x;
	int y;
	int wx;
	int wy;
	int temp;
	struct synaptics_rmi4_f12_extra_data *extra_data;
	struct synaptics_rmi4_f12_finger_data *data;
	struct synaptics_rmi4_f12_finger_data *finger_data;
#ifdef F12_DATA_15_WORKAROUND
	static unsigned char fingers_already_present;
#endif

	fingers_to_process = fhandler->num_of_data_points;
	data_addr = fhandler->full_addr.data_base;
	extra_data = (struct synaptics_rmi4_f12_extra_data *)fhandler->extra;
	size_of_2d_data = sizeof(struct synaptics_rmi4_f12_finger_data);


	/* Determine the total number of fingers to process */
	if (extra_data->data15_size) {
		retval = synaptics_rmi4_i2c_read(rmi4_data,
				data_addr + extra_data->data15_offset,
				extra_data->data15_data,
				extra_data->data15_size);
		if (retval < 0)
			return 0;

		/* Start checking from the highest bit */
		temp = extra_data->data15_size - 1; /* Highest byte */
		finger = (fingers_to_process - 1) % 8; /* Highest bit */
		do {
			if (extra_data->data15_data[temp] & (1 << finger))
				break;

			if (finger) {
				finger--;
			} else {
				temp--; /* Move to the next lower byte */
				finger = 7;
			}

			fingers_to_process--;
		} while (fingers_to_process);

		dev_dbg(&rmi4_data->i2c_client->dev,
			"%s: Number of fingers to process = %d\n",
			__func__, fingers_to_process);
	}

#ifdef F12_DATA_15_WORKAROUND
	fingers_to_process = max(fingers_to_process, fingers_already_present);
#endif

	if (!fingers_to_process) {
		synaptics_rmi4_free_fingers(rmi4_data);
		return 0;
	}

	retval = synaptics_rmi4_i2c_read(rmi4_data,
			data_addr + extra_data->data1_offset,
			(unsigned char *)fhandler->data,
			fingers_to_process * size_of_2d_data);
	if (retval < 0)
		return 0;

	data = (struct synaptics_rmi4_f12_finger_data *)fhandler->data;

	for (finger = 0; finger < fingers_to_process; finger++) {
		finger_data = data + finger;
		finger_status = finger_data->object_type_and_status & MASK_1BIT;

/*[Lavender][bozhi_lin] synaptics touch detect glove or finger in glove mode 20150720 begin*/
#if defined(LAVENDER_GLOVE)
		if (glovemode_support_detection) {
			if (glovemode) {
				if (finger_status) {
					glovemode_is_finger = 1;
				}
				else {
					if (finger_data->object_type_and_status == 0x06) { 
						finger_status = 1;
					}
					glovemode_is_finger = 0;
				}
			}
			else {
				glovemode_is_finger = 1;
			}
		} else {
			glovemode_is_finger = 0;		
		}
#endif
/*[Lavender][bozhi_lin] 20150720 end*/

#ifdef TYPE_B_PROTOCOL
		input_mt_slot(rmi4_data->input_dev, finger);
		input_mt_report_slot_state(rmi4_data->input_dev,
				MT_TOOL_FINGER, finger_status);
#endif

		if (finger_status) {
#ifdef F12_DATA_15_WORKAROUND
			fingers_already_present = finger + 1;
#endif

			x = (finger_data->x_msb << 8) | (finger_data->x_lsb);
			y = (finger_data->y_msb << 8) | (finger_data->y_lsb);
#ifdef REPORT_2D_W
			wx = finger_data->wx;
			wy = finger_data->wy;
#endif
			//x = (rmi4_data->sensor_max_x) - x;
			//y = (rmi4_data->sensor_max_y) - y;

			input_report_key(rmi4_data->input_dev,
					BTN_TOUCH, 1);
			input_report_key(rmi4_data->input_dev,
					BTN_TOOL_FINGER, 1);
//[SM20][zihweishen] Modify muti touch problem of random report ghost node begin 2016/4/12
			 input_report_abs( rmi4_data->input_dev, ABS_MT_TOUCH_MAJOR, 8 );
			 input_report_abs( rmi4_data->input_dev, ABS_MT_TRACKING_ID, finger );
//[SM20][zihweishen] end 2016/4/12
			input_report_abs(rmi4_data->input_dev,
					ABS_MT_POSITION_X, x);
			input_report_abs(rmi4_data->input_dev,
					ABS_MT_POSITION_Y, y);
#ifdef REPORT_2D_W
			input_report_abs(rmi4_data->input_dev,
					ABS_MT_TOUCH_MAJOR, max(wx, wy));
			input_report_abs(rmi4_data->input_dev,
					ABS_MT_TOUCH_MINOR, min(wx, wy));
#endif
#ifndef TYPE_B_PROTOCOL
			input_mt_sync(rmi4_data->input_dev);
#endif

			dev_dbg(&rmi4_data->i2c_client->dev,
					"%s: Finger %d:\n"
					"status = 0x%02x\n"
					"synaptics_rmi4_f12_abs_report zihwei x = %d\n"
					"synaptics_rmi4_f12_abs_report zihwei y = %d\n"
					"wx = %d\n"
					"wy = %d\n",
					__func__, finger,
					finger_status,
					x, y, wx, wy);

			touch_count++;
		}
	}

	if (touch_count == 0) {
		input_report_key(rmi4_data->input_dev,
				BTN_TOUCH, 0);
		input_report_key(rmi4_data->input_dev,
				BTN_TOOL_FINGER, 0);
#ifndef TYPE_B_PROTOCOL
		input_mt_sync(rmi4_data->input_dev);
#endif
	}

	input_sync(rmi4_data->input_dev);

	return touch_count;
}

static void synaptics_rmi4_f1a_report(struct synaptics_rmi4_data *rmi4_data,
		struct synaptics_rmi4_fn *fhandler)
{
	int retval;
	unsigned char touch_count = 0;
	unsigned char button;
	unsigned char index;
	unsigned char shift;
	unsigned char status;
	unsigned char *data;
	unsigned short data_addr = fhandler->full_addr.data_base;
	struct synaptics_rmi4_f1a_handle *f1a = fhandler->data;
	static unsigned char do_once = 1;
	static bool current_status[MAX_NUMBER_OF_BUTTONS];
#ifdef NO_0D_WHILE_2D
	static bool before_2d_status[MAX_NUMBER_OF_BUTTONS];
	static bool while_2d_status[MAX_NUMBER_OF_BUTTONS];
#endif

	if (do_once) {
		memset(current_status, 0, sizeof(current_status));
#ifdef NO_0D_WHILE_2D
		memset(before_2d_status, 0, sizeof(before_2d_status));
		memset(while_2d_status, 0, sizeof(while_2d_status));
#endif
		do_once = 0;
	}

	retval = synaptics_rmi4_i2c_read(rmi4_data,
			data_addr,
			f1a->button_data_buffer,
			f1a->button_bitmask_size);
	if (retval < 0) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Failed to read button data registers\n",
				__func__);
		return;
	}

	data = f1a->button_data_buffer;

	for (button = 0; button < f1a->valid_button_count; button++) {
		index = button / 8;
		shift = button % 8;
		status = ((data[index] >> shift) & MASK_1BIT);

		if (current_status[button] == status)
			continue;
		else
			current_status[button] = status;

		dev_dbg(&rmi4_data->i2c_client->dev,
				"%s: Button %d (code %d) ->%d\n",
				__func__, button,
				f1a->button_map[button],
				status);
#ifdef NO_0D_WHILE_2D
		if (rmi4_data->fingers_on_2d == false) {
			if (status == 1) {
				before_2d_status[button] = 1;
			} else {
				if (while_2d_status[button] == 1) {
					while_2d_status[button] = 0;
					continue;
				} else {
					before_2d_status[button] = 0;
				}
			}
			touch_count++;
			input_report_key(rmi4_data->input_dev,
					f1a->button_map[button],
					status);
		} else {
			if (before_2d_status[button] == 1) {
				before_2d_status[button] = 0;
				touch_count++;
				input_report_key(rmi4_data->input_dev,
						f1a->button_map[button],
						status);
			} else {
				if (status == 1)
					while_2d_status[button] = 1;
				else
					while_2d_status[button] = 0;
			}
		}
#else
		touch_count++;
		input_report_key(rmi4_data->input_dev,
				f1a->button_map[button],
				status);
#endif
	}

	if (touch_count)
		input_sync(rmi4_data->input_dev);

	return;
}

 /**
 * synaptics_rmi4_report_touch()
 *
 * Called by synaptics_rmi4_sensor_report().
 *
 * This function calls the appropriate finger data reporting function
 * based on the function handler it receives and returns the number of
 * fingers detected.
 */
static void synaptics_rmi4_report_touch(struct synaptics_rmi4_data *rmi4_data,
		struct synaptics_rmi4_fn *fhandler)
{
	unsigned char touch_count_2d;

	dev_dbg(&rmi4_data->i2c_client->dev,
			"%s: Function %02x reporting\n",
			__func__, fhandler->fn_number);

	switch (fhandler->fn_number) {
	case SYNAPTICS_RMI4_F11:
		touch_count_2d = synaptics_rmi4_f11_abs_report(rmi4_data,
				fhandler);

		if (touch_count_2d)
			rmi4_data->fingers_on_2d = true;
		else
			rmi4_data->fingers_on_2d = false;
		break;
	case SYNAPTICS_RMI4_F12:
		touch_count_2d = synaptics_rmi4_f12_abs_report(rmi4_data,
				fhandler);

		if (touch_count_2d)
			rmi4_data->fingers_on_2d = true;
		else
			rmi4_data->fingers_on_2d = false;
		break;
	case SYNAPTICS_RMI4_F1A:
		synaptics_rmi4_f1a_report(rmi4_data, fhandler);
		break;
	default:
		break;
	}

	return;
}

 /**
 * synaptics_rmi4_sensor_report()
 *
 * Called by synaptics_rmi4_irq().
 *
 * This function determines the interrupt source(s) from the sensor
 * and calls synaptics_rmi4_report_touch() with the appropriate
 * function handler for each function with valid data inputs.
 */
static void synaptics_rmi4_sensor_report(struct synaptics_rmi4_data *rmi4_data)
{
	int retval;
	unsigned char data[MAX_INTR_REGISTERS + 1];
	unsigned char *intr = &data[1];
	struct synaptics_rmi4_f01_device_status status;
	struct synaptics_rmi4_fn *fhandler;
	struct synaptics_rmi4_exp_fhandler *exp_fhandler;
	struct synaptics_rmi4_device_info *rmi;

	rmi = &(rmi4_data->rmi4_mod_info);

	/*
	 * Get interrupt status information from F01 Data1 register to
	 * determine the source(s) that are flagging the interrupt.
	 */
	retval = synaptics_rmi4_i2c_read(rmi4_data,
			rmi4_data->f01_data_base_addr,
			data,
			rmi4_data->num_of_intr_regs + 1);
	if (retval < 0) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Failed to read interrupt status\n",
				__func__);
		return;
	}

	status.data[0] = data[0];
	if (status.unconfigured && !status.flash_prog) {
		pr_notice("%s: spontaneous reset detected\n", __func__);
		retval = synaptics_rmi4_reinit_device(rmi4_data);
		if (retval < 0) {
			dev_err(&rmi4_data->i2c_client->dev,
					"%s: Failed to reinit device\n",
					__func__);
		}
		return;
	}

	/*
	 * Traverse the function handler list and service the source(s)
	 * of the interrupt accordingly.
	 */
	if (!list_empty(&rmi->support_fn_list)) {
		list_for_each_entry(fhandler, &rmi->support_fn_list, link) {
			if (fhandler->num_of_data_sources) {
				if (fhandler->intr_mask &
						intr[fhandler->intr_reg_num]) {
					synaptics_rmi4_report_touch(rmi4_data,
							fhandler);
				}
			}
		}
	}

	mutex_lock(&exp_data.mutex);
	if (!list_empty(&exp_data.list)) {
		list_for_each_entry(exp_fhandler, &exp_data.list, link) {
			if (!exp_fhandler->insert &&
					!exp_fhandler->remove &&
					(exp_fhandler->exp_fn->attn != NULL))
				exp_fhandler->exp_fn->attn(rmi4_data, intr[0]);
		}
	}
	mutex_unlock(&exp_data.mutex);

	return;
}

 /**
 * synaptics_rmi4_irq()
 *
 * Called by the kernel when an interrupt occurs (when the sensor
 * asserts the attention irq).
 *
 * This function is the ISR thread and handles the acquisition
 * and the reporting of finger data when the presence of fingers
 * is detected.
 */

static int touch_event_handler(void *data)
{
	struct synaptics_rmi4_data *rmi4_data = data;
	struct sched_param param = { .sched_priority = RTPM_PRIO_TPD };

	sched_setscheduler(current, SCHED_RR, &param);
	do{
		set_current_state(TASK_INTERRUPTIBLE);

		while (tpd_halt) {
			tpd_flag = 0;
			msleep(20);
		}

		wait_event_interruptible(waiter, tpd_flag != 0);
		tpd_flag = 0;
		TPD_DEBUG_SET_TIME;
		set_current_state(TASK_RUNNING);

		if (!rmi4_data->touch_stopped)
			synaptics_rmi4_sensor_report(rmi4_data);
#if defined(SYNAPTICS_MT6752)
		synaptic_irq_enable();
#else
		mt65xx_eint_unmask(CUST_EINT_TOUCH_PANEL_NUM);
#endif

	}while(1);

	return 0;
}

 /**
 * synaptics_rmi4_irq_enable()
 *
 * Called by synaptics_rmi4_probe() and the power management functions
 * in this driver and also exported to other expansion Function modules
 * such as rmi_dev.
 *
 * This function handles the enabling and disabling of the attention
 * irq including the setting up of the ISR thread.
 */
static int synaptics_rmi4_irq_enable(struct synaptics_rmi4_data *rmi4_data,
		bool enable)
{
	int retval = 0;
	unsigned char intr_status[MAX_INTR_REGISTERS];

	if (enable) {

		/* Clear interrupts first */
		retval = synaptics_rmi4_i2c_read(rmi4_data,
				rmi4_data->f01_data_base_addr + 1,
				intr_status,
				rmi4_data->num_of_intr_regs);
		if (retval < 0)
			return retval;

		// set up irq
		if (!rmi4_data->irq_enabled) {
			
 //[SM20][zihweishen] Modify for android M of GPIO setting begin 2015/11/11
 
			/*mt_set_gpio_mode(GPIO_CTP_EINT_PIN, GPIO_CTP_EINT_PIN_M_EINT);
			mt_set_gpio_dir(GPIO_CTP_EINT_PIN, GPIO_DIR_IN);
			mt_set_gpio_pull_enable(GPIO_CTP_EINT_PIN, GPIO_PULL_ENABLE);
			mt_set_gpio_pull_select(GPIO_CTP_EINT_PIN, GPIO_PULL_UP);*/
				GTP_GPIO_AS_INT(GTP_INT_PORT);
				
				//mdelay(50);

			tpd_irq_registration();
			
//[SM20][zihweishen] Modify for android M of GPIO setting end 2015/11/11

			}
#if defined(SYNAPTICS_MT6752)
		synaptic_irq_enable();
#else
		mt65xx_eint_unmask(CUST_EINT_TOUCH_PANEL_NUM);
#endif
		rmi4_data->irq_enabled = true;
	} else {
		if (rmi4_data->irq_enabled) {
#if defined(SYNAPTICS_MT6752)
			synaptic_irq_disable();
#else
			mt65xx_eint_mask(CUST_EINT_TOUCH_PANEL_NUM);
#endif

//[SM20][zihweishen] Modify touch do not response when phone wake up begin 2016/4/7
//[SM20][zihweishen] Modify for android M of GPIO setting begin 2015/11/11

	GTP_GPIO_AS_INT(GTP_INT_PORT);

//[SM20][zihweishen] Modify for android M of GPIO setting begin 2015/11/11
//[SM20][zihweishen] end 2016/4/7
			rmi4_data->irq_enabled = false;
		}
	}

	return retval;
}

static void synaptics_rmi4_set_intr_mask(struct synaptics_rmi4_fn *fhandler,
		struct synaptics_rmi4_fn_desc *fd,
		unsigned int intr_count)
{
	unsigned char ii;
	unsigned char intr_offset;

	fhandler->intr_reg_num = (intr_count + 7) / 8;
	if (fhandler->intr_reg_num != 0)
		fhandler->intr_reg_num -= 1;

	/* Set an enable bit for each data source */
	intr_offset = intr_count % 8;
	fhandler->intr_mask = 0;
	for (ii = intr_offset;
			ii < ((fd->intr_src_count & MASK_3BIT) +
			intr_offset);
			ii++)
		fhandler->intr_mask |= 1 << ii;

	return;
}

static int synaptics_rmi4_f01_init(struct synaptics_rmi4_data *rmi4_data,
		struct synaptics_rmi4_fn *fhandler,
		struct synaptics_rmi4_fn_desc *fd,
		unsigned int intr_count)
{
	fhandler->fn_number = fd->fn_number;
	fhandler->num_of_data_sources = fd->intr_src_count;
	fhandler->data = NULL;
	fhandler->extra = NULL;

	synaptics_rmi4_set_intr_mask(fhandler, fd, intr_count);

	rmi4_data->f01_query_base_addr = fd->query_base_addr;
	rmi4_data->f01_ctrl_base_addr = fd->ctrl_base_addr;
	rmi4_data->f01_data_base_addr = fd->data_base_addr;
	rmi4_data->f01_cmd_base_addr = fd->cmd_base_addr;

	return 0;
}

 /**
 * synaptics_rmi4_f11_init()
 *
 * Called by synaptics_rmi4_query_device().
 *
 * This funtion parses information from the Function 11 registers
 * and determines the number of fingers supported, x and y data ranges,
 * offset to the associated interrupt status register, interrupt bit
 * mask, and gathers finger data acquisition capabilities from the query
 * registers.
 */
static int synaptics_rmi4_f11_init(struct synaptics_rmi4_data *rmi4_data,
		struct synaptics_rmi4_fn *fhandler,
		struct synaptics_rmi4_fn_desc *fd,
		unsigned int intr_count)
{
	int retval;
	unsigned char abs_data_size;
	unsigned char abs_data_blk_size;
	unsigned char query[F11_STD_QUERY_LEN];
	unsigned char control[F11_STD_CTRL_LEN];

	fhandler->fn_number = fd->fn_number;
	fhandler->num_of_data_sources = fd->intr_src_count;

	retval = synaptics_rmi4_i2c_read(rmi4_data,
			fhandler->full_addr.query_base,
			query,
			sizeof(query));
	if (retval < 0)
		return retval;

	/* Maximum number of fingers supported */
	if ((query[1] & MASK_3BIT) <= 4)
		fhandler->num_of_data_points = (query[1] & MASK_3BIT) + 1;
	else if ((query[1] & MASK_3BIT) == 5)
		fhandler->num_of_data_points = 10;

	rmi4_data->num_of_fingers = fhandler->num_of_data_points;

	retval = synaptics_rmi4_i2c_read(rmi4_data,
			fhandler->full_addr.ctrl_base,
			control,
			sizeof(control));
	if (retval < 0)
		return retval;

	/* Maximum x and y */
	rmi4_data->sensor_max_x = ((control[6] & MASK_8BIT) << 0) |
			((control[7] & MASK_4BIT) << 8);
	rmi4_data->sensor_max_y = ((control[8] & MASK_8BIT) << 0) |
			((control[9] & MASK_4BIT) << 8);
#ifdef TPD_HAVE_BUTTON
	rmi4_data->sensor_max_y = rmi4_data->sensor_max_y * TPD_DISPLAY_HEIGH_RATIO / TPD_TOUCH_HEIGH_RATIO;
#endif
	dev_dbg(&rmi4_data->i2c_client->dev,
			"%s: Function %02x max x = %d max y = %d\n",
			__func__, fhandler->fn_number,
			rmi4_data->sensor_max_x,
			rmi4_data->sensor_max_y);

	rmi4_data->max_touch_width = MAX_F11_TOUCH_WIDTH;

	synaptics_rmi4_set_intr_mask(fhandler, fd, intr_count);

	abs_data_size = query[5] & MASK_2BIT;
	abs_data_blk_size = 3 + (2 * (abs_data_size == 0 ? 1 : 0));
	fhandler->size_of_data_register_block = abs_data_blk_size;
	fhandler->data = NULL;
	fhandler->extra = NULL;

	return retval;
}

static int synaptics_rmi4_f12_set_enables(struct synaptics_rmi4_data *rmi4_data,
		unsigned short ctrl28)
{
	int retval;
	static unsigned short ctrl_28_address;

	if (ctrl28)
		ctrl_28_address = ctrl28;

	retval = synaptics_rmi4_i2c_write(rmi4_data,
			ctrl_28_address,
			&rmi4_data->report_enable,
			sizeof(rmi4_data->report_enable));
	if (retval < 0)
		return retval;

	return retval;
}

 /**
 * synaptics_rmi4_f12_init()
 *
 * Called by synaptics_rmi4_query_device().
 *
 * This funtion parses information from the Function 12 registers and
 * determines the number of fingers supported, offset to the data1
 * register, x and y data ranges, offset to the associated interrupt
 * status register, interrupt bit mask, and allocates memory resources
 * for finger data acquisition.
 */
static int synaptics_rmi4_f12_init(struct synaptics_rmi4_data *rmi4_data,
		struct synaptics_rmi4_fn *fhandler,
		struct synaptics_rmi4_fn_desc *fd,
		unsigned int intr_count)
{
	int retval;
	unsigned char size_of_2d_data;
	unsigned char size_of_query8;
	unsigned char ctrl_8_offset;
	unsigned char ctrl_23_offset;
	unsigned char ctrl_28_offset;
	unsigned char num_of_fingers;
	struct synaptics_rmi4_f12_extra_data *extra_data;
	struct synaptics_rmi4_f12_query_5 query_5;
	struct synaptics_rmi4_f12_query_8 query_8;
	struct synaptics_rmi4_f12_ctrl_8 ctrl_8;
	struct synaptics_rmi4_f12_ctrl_23 ctrl_23;

	fhandler->fn_number = fd->fn_number;
	fhandler->num_of_data_sources = fd->intr_src_count;
	fhandler->extra = kmalloc(sizeof(*extra_data), GFP_KERNEL);
	extra_data = (struct synaptics_rmi4_f12_extra_data *)fhandler->extra;
	size_of_2d_data = sizeof(struct synaptics_rmi4_f12_finger_data);

	retval = synaptics_rmi4_i2c_read(rmi4_data,
			fhandler->full_addr.query_base + 5,
			query_5.data,
			sizeof(query_5.data));
	if (retval < 0)
		return retval;

	ctrl_8_offset = query_5.ctrl0_is_present +
			query_5.ctrl1_is_present +
			query_5.ctrl2_is_present +
			query_5.ctrl3_is_present +
			query_5.ctrl4_is_present +
			query_5.ctrl5_is_present +
			query_5.ctrl6_is_present +
			query_5.ctrl7_is_present;

	ctrl_23_offset = ctrl_8_offset +
			query_5.ctrl8_is_present +
			query_5.ctrl9_is_present +
			query_5.ctrl10_is_present +
			query_5.ctrl11_is_present +
			query_5.ctrl12_is_present +
			query_5.ctrl13_is_present +
			query_5.ctrl14_is_present +
			query_5.ctrl15_is_present +
			query_5.ctrl16_is_present +
			query_5.ctrl17_is_present +
			query_5.ctrl18_is_present +
			query_5.ctrl19_is_present +
			query_5.ctrl20_is_present +
			query_5.ctrl21_is_present +
			query_5.ctrl22_is_present;

	ctrl_28_offset = ctrl_23_offset +
			query_5.ctrl23_is_present +
			query_5.ctrl24_is_present +
			query_5.ctrl25_is_present +
			query_5.ctrl26_is_present +
			query_5.ctrl27_is_present;

	retval = synaptics_rmi4_i2c_read(rmi4_data,
			fhandler->full_addr.ctrl_base + ctrl_23_offset,
			ctrl_23.data,
			sizeof(ctrl_23.data));
	if (retval < 0)
		return retval;

	/* Maximum number of fingers supported */
	fhandler->num_of_data_points = min(ctrl_23.max_reported_objects,
			(unsigned char)F12_FINGERS_TO_SUPPORT);

	num_of_fingers = fhandler->num_of_data_points;
	rmi4_data->num_of_fingers = num_of_fingers;

	retval = synaptics_rmi4_i2c_read(rmi4_data,
			fhandler->full_addr.query_base + 7,
			&size_of_query8,
			sizeof(size_of_query8));
	if (retval < 0)
		return retval;

	retval = synaptics_rmi4_i2c_read(rmi4_data,
			fhandler->full_addr.query_base + 8,
			query_8.data,
			size_of_query8);
	if (retval < 0)
		return retval;

	/* Determine the presence of the Data0 register */
	extra_data->data1_offset = query_8.data0_is_present;

	if ((size_of_query8 >= 3) && (query_8.data15_is_present)) {
		extra_data->data15_offset = query_8.data0_is_present +
				query_8.data1_is_present +
				query_8.data2_is_present +
				query_8.data3_is_present +
				query_8.data4_is_present +
				query_8.data5_is_present +
				query_8.data6_is_present +
				query_8.data7_is_present +
				query_8.data8_is_present +
				query_8.data9_is_present +
				query_8.data10_is_present +
				query_8.data11_is_present +
				query_8.data12_is_present +
				query_8.data13_is_present +
				query_8.data14_is_present;
		extra_data->data15_size = (num_of_fingers + 7) / 8;
	} else {
		extra_data->data15_size = 0;
	}

	rmi4_data->report_enable = RPT_DEFAULT;
#ifdef REPORT_2D_Z
	rmi4_data->report_enable |= RPT_Z;
#endif
#ifdef REPORT_2D_W
	rmi4_data->report_enable |= (RPT_WX | RPT_WY);
#endif

	retval = synaptics_rmi4_f12_set_enables(rmi4_data,
			fhandler->full_addr.ctrl_base + ctrl_28_offset);
	if (retval < 0)
		return retval;

	retval = synaptics_rmi4_i2c_read(rmi4_data,
			fhandler->full_addr.ctrl_base + ctrl_8_offset,
			ctrl_8.data,
			sizeof(ctrl_8.data));
	if (retval < 0)
		return retval;

	/* Maximum x and y */
	rmi4_data->sensor_max_x =
			((unsigned short)ctrl_8.max_x_coord_lsb << 0) |
			((unsigned short)ctrl_8.max_x_coord_msb << 8);
	rmi4_data->sensor_max_y =
			((unsigned short)ctrl_8.max_y_coord_lsb << 0) |
			((unsigned short)ctrl_8.max_y_coord_msb << 8);
#ifdef TPD_HAVE_BUTTON
	rmi4_data->sensor_max_y = rmi4_data->sensor_max_y * TPD_DISPLAY_HEIGH_RATIO / TPD_TOUCH_HEIGH_RATIO;
#endif
	dev_dbg(&rmi4_data->i2c_client->dev,
			"%s: Function %02x max x = %d max y = %d\n",
			__func__, fhandler->fn_number,
			rmi4_data->sensor_max_x,
			rmi4_data->sensor_max_y);

	rmi4_data->num_of_rx = ctrl_8.num_of_rx;
	rmi4_data->num_of_tx = ctrl_8.num_of_tx;
	rmi4_data->max_touch_width = max(rmi4_data->num_of_rx,
			rmi4_data->num_of_tx);

	synaptics_rmi4_set_intr_mask(fhandler, fd, intr_count);

	/* Allocate memory for finger data storage space */
	fhandler->data_size = num_of_fingers * size_of_2d_data;
	fhandler->data = kmalloc(fhandler->data_size, GFP_KERNEL);

	return retval;
}

static int synaptics_rmi4_f1a_alloc_mem(struct synaptics_rmi4_data *rmi4_data,
		struct synaptics_rmi4_fn *fhandler)
{
	int retval;
	struct synaptics_rmi4_f1a_handle *f1a;

	f1a = kzalloc(sizeof(*f1a), GFP_KERNEL);
	if (!f1a) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Failed to alloc mem for function handle\n",
				__func__);
		return -ENOMEM;
	}

	fhandler->data = (void *)f1a;
	fhandler->extra = NULL;

	retval = synaptics_rmi4_i2c_read(rmi4_data,
			fhandler->full_addr.query_base,
			f1a->button_query.data,
			sizeof(f1a->button_query.data));
	if (retval < 0) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Failed to read query registers\n",
				__func__);
		return retval;
	}

	f1a->max_count = f1a->button_query.max_button_count + 1;

	f1a->button_control.txrx_map = kzalloc(f1a->max_count * 2, GFP_KERNEL);
	if (!f1a->button_control.txrx_map) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Failed to alloc mem for tx rx mapping\n",
				__func__);
		return -ENOMEM;
	}

	f1a->button_bitmask_size = (f1a->max_count + 7) / 8;

	f1a->button_data_buffer = kcalloc(f1a->button_bitmask_size,
			sizeof(*(f1a->button_data_buffer)), GFP_KERNEL);
	if (!f1a->button_data_buffer) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Failed to alloc mem for data buffer\n",
				__func__);
		return -ENOMEM;
	}

	f1a->button_map = kcalloc(f1a->max_count,
			sizeof(*(f1a->button_map)), GFP_KERNEL);
	if (!f1a->button_map) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Failed to alloc mem for button map\n",
				__func__);
		return -ENOMEM;
	}

	return 0;
}

static int synaptics_rmi4_f1a_button_map(struct synaptics_rmi4_data *rmi4_data,
		struct synaptics_rmi4_fn *fhandler)
{
	int retval;
	unsigned char ii;
	unsigned char mapping_offset = 0;
	struct synaptics_rmi4_f1a_handle *f1a = fhandler->data;

	mapping_offset = f1a->button_query.has_general_control +
			f1a->button_query.has_interrupt_enable +
			f1a->button_query.has_multibutton_select;

	if (f1a->button_query.has_tx_rx_map) {
		retval = synaptics_rmi4_i2c_read(rmi4_data,
				fhandler->full_addr.ctrl_base + mapping_offset,
				f1a->button_control.txrx_map,
				sizeof(f1a->button_control.txrx_map));
		if (retval < 0) {
			dev_err(&rmi4_data->i2c_client->dev,
					"%s: Failed to read tx rx mapping\n",
					__func__);
			return retval;
		}

		rmi4_data->button_txrx_mapping = f1a->button_control.txrx_map;
	}

	if (cap_button_map.map) {
		if (cap_button_map.nbuttons != f1a->max_count) {
			f1a->valid_button_count = min(f1a->max_count,
					cap_button_map.nbuttons);
		} else {
			f1a->valid_button_count = f1a->max_count;
		}

		for (ii = 0; ii < f1a->valid_button_count; ii++)
			f1a->button_map[ii] = cap_button_map.map[ii];
	}
	return 0;
}

static void synaptics_rmi4_f1a_kfree(struct synaptics_rmi4_fn *fhandler)
{
	struct synaptics_rmi4_f1a_handle *f1a = fhandler->data;

	if (f1a) {
		kfree(f1a->button_control.txrx_map);
		kfree(f1a->button_data_buffer);
		kfree(f1a->button_map);
		kfree(f1a);
		fhandler->data = NULL;
	}

	return;
}

static int synaptics_rmi4_f1a_init(struct synaptics_rmi4_data *rmi4_data,
		struct synaptics_rmi4_fn *fhandler,
		struct synaptics_rmi4_fn_desc *fd,
		unsigned int intr_count)
{
	int retval;

	fhandler->fn_number = fd->fn_number;
	fhandler->num_of_data_sources = fd->intr_src_count;

	synaptics_rmi4_set_intr_mask(fhandler, fd, intr_count);

	retval = synaptics_rmi4_f1a_alloc_mem(rmi4_data, fhandler);
	if (retval < 0)
		goto error_exit;

	retval = synaptics_rmi4_f1a_button_map(rmi4_data, fhandler);
	if (retval < 0)
		goto error_exit;

	rmi4_data->button_0d_enabled = 1;

	return 0;

error_exit:
	synaptics_rmi4_f1a_kfree(fhandler);

	return retval;
}

static void synaptics_rmi4_empty_fn_list(struct synaptics_rmi4_data *rmi4_data)
{
	struct synaptics_rmi4_fn *fhandler;
	struct synaptics_rmi4_fn *fhandler_temp;
	struct synaptics_rmi4_device_info *rmi;

	rmi = &(rmi4_data->rmi4_mod_info);

	if (!list_empty(&rmi->support_fn_list)) {
		list_for_each_entry_safe(fhandler,
				fhandler_temp,
				&rmi->support_fn_list,
				link) {
			if (fhandler->fn_number == SYNAPTICS_RMI4_F1A) {
				synaptics_rmi4_f1a_kfree(fhandler);
			} else {
				kfree(fhandler->extra);
				kfree(fhandler->data);
			}
			list_del(&fhandler->link);
			kfree(fhandler);
		}
	}
	INIT_LIST_HEAD(&rmi->support_fn_list);

	return;
}

static int synaptics_rmi4_check_status(struct synaptics_rmi4_data *rmi4_data,
		bool *was_in_bl_mode)
{
	int retval;
	int timeout = CHECK_STATUS_TIMEOUT_MS;
	unsigned char command = 0x01;
	unsigned char intr_status;
	struct synaptics_rmi4_f01_device_status status;
	printk("[zihwei] synaptics_rmi4_i2c_write ");

	/* Do a device reset first */
	retval = synaptics_rmi4_i2c_write(rmi4_data,
			rmi4_data->f01_cmd_base_addr,
			&command,
			sizeof(command));
	if (retval < 0)
		return retval;

	msleep(DELAY_S7300_RESET_READY);

	retval = synaptics_rmi4_i2c_read(rmi4_data,
			rmi4_data->f01_data_base_addr,
			status.data,
			sizeof(status.data));
	if (retval < 0)
		return retval;

	while (status.status_code == STATUS_CRC_IN_PROGRESS) {
		if (timeout > 0)
			msleep(20);
		else
			return -1;

		retval = synaptics_rmi4_i2c_read(rmi4_data,
				rmi4_data->f01_data_base_addr,
				status.data,
				sizeof(status.data));
		if (retval < 0)
			return retval;

		timeout -= 20;
	}

	if (timeout != CHECK_STATUS_TIMEOUT_MS)
		*was_in_bl_mode = true;

	if (status.flash_prog == 1) {
		rmi4_data->flash_prog_mode = true;
		pr_notice("%s: In flash prog mode, status = 0x%02x\n",
				__func__,
				status.status_code);
	} else {
		rmi4_data->flash_prog_mode = false;
	}

	retval = synaptics_rmi4_i2c_read(rmi4_data,
			rmi4_data->f01_data_base_addr + 1,
			&intr_status,
			sizeof(intr_status));
	if (retval < 0) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Failed to read interrupt status\n",
				__func__);
		return retval;
	}

	return 0;
}

static void synaptics_rmi4_set_configured(struct synaptics_rmi4_data *rmi4_data)
{
	int retval;
	unsigned char device_ctrl;

	retval = synaptics_rmi4_i2c_read(rmi4_data,
			rmi4_data->f01_ctrl_base_addr,
			&device_ctrl,
			sizeof(device_ctrl));
	if (retval < 0) {
		dev_err(&(rmi4_data->input_dev->dev),
				"%s: Failed to set configured\n",
				__func__);
		return;
	}

	rmi4_data->no_sleep_setting = device_ctrl & NO_SLEEP_ON;
	device_ctrl |= CONFIGURED;

	retval = synaptics_rmi4_i2c_write(rmi4_data,
			rmi4_data->f01_ctrl_base_addr,
			&device_ctrl,
			sizeof(device_ctrl));
	if (retval < 0) {
		dev_err(&(rmi4_data->input_dev->dev),
				"%s: Failed to set configured\n",
				__func__);
	}

	return;
}

static int synaptics_rmi4_alloc_fh(struct synaptics_rmi4_fn **fhandler,
		struct synaptics_rmi4_fn_desc *rmi_fd, int page_number)
{
	*fhandler = kmalloc(sizeof(**fhandler), GFP_KERNEL);
	if (!(*fhandler))
		return -ENOMEM;

	(*fhandler)->full_addr.data_base =
			(rmi_fd->data_base_addr |
			(page_number << 8));
	(*fhandler)->full_addr.ctrl_base =
			(rmi_fd->ctrl_base_addr |
			(page_number << 8));
	(*fhandler)->full_addr.cmd_base =
			(rmi_fd->cmd_base_addr |
			(page_number << 8));
	(*fhandler)->full_addr.query_base =
			(rmi_fd->query_base_addr |
			(page_number << 8));

	return 0;
}

 /**
 * synaptics_rmi4_query_device()
 *
 * Called by synaptics_rmi4_probe().
 *
 * This funtion scans the page description table, records the offsets
 * to the register types of Function $01, sets up the function handlers
 * for Function $11 and Function $12, determines the number of interrupt
 * sources from the sensor, adds valid Functions with data inputs to the
 * Function linked list, parses information from the query registers of
 * Function $01, and enables the interrupt sources from the valid Functions
 * with data inputs.
 */
static int synaptics_rmi4_query_device(struct synaptics_rmi4_data *rmi4_data)
{
	int retval;
	unsigned char ii;
	unsigned char page_number;
	unsigned char intr_count;
	unsigned char f01_query[F01_STD_QUERY_LEN];
	unsigned short pdt_entry_addr;
	unsigned short intr_addr;
	bool was_in_bl_mode;
	struct synaptics_rmi4_fn_desc rmi_fd;
	struct synaptics_rmi4_fn *fhandler;
	struct synaptics_rmi4_device_info *rmi;

	rmi = &(rmi4_data->rmi4_mod_info);

rescan_pdt:
	was_in_bl_mode = false;
	intr_count = 0;
	INIT_LIST_HEAD(&rmi->support_fn_list);

	/* Scan the page description tables of the pages to service */
	for (page_number = 0; page_number < PAGES_TO_SERVICE; page_number++) {
		for (pdt_entry_addr = PDT_START; pdt_entry_addr > PDT_END;
				pdt_entry_addr -= PDT_ENTRY_SIZE) {
			pdt_entry_addr |= (page_number << 8);

			retval = synaptics_rmi4_i2c_read(rmi4_data,
					pdt_entry_addr,
					(unsigned char *)&rmi_fd,
					sizeof(rmi_fd));
			if (retval < 0)
				return retval;

			fhandler = NULL;

			if (rmi_fd.fn_number == 0) {
				dev_dbg(&rmi4_data->i2c_client->dev,
						"%s: Reached end of PDT\n",
						__func__);
				break;
			}

			dev_dbg(&rmi4_data->i2c_client->dev,
					"%s: F%02x found (page %d)\n",
					__func__, rmi_fd.fn_number,
					page_number);

			switch (rmi_fd.fn_number) {
			case SYNAPTICS_RMI4_F01:
				if (rmi_fd.intr_src_count == 0)
					break;

				retval = synaptics_rmi4_alloc_fh(&fhandler,
						&rmi_fd, page_number);
				if (retval < 0) {
					dev_err(&rmi4_data->i2c_client->dev,
							"%s: Failed to alloc for F%d\n",
							__func__,
							rmi_fd.fn_number);
					return retval;
				}

				retval = synaptics_rmi4_f01_init(rmi4_data,
						fhandler, &rmi_fd, intr_count);
				if (retval < 0)
					return retval;

				retval = synaptics_rmi4_check_status(rmi4_data,
						&was_in_bl_mode);
				if (retval < 0) {
					dev_err(&rmi4_data->i2c_client->dev,
							"%s: Failed to check status\n",
							__func__);
					return retval;
				}

				if (was_in_bl_mode) {
					kfree(fhandler);
					fhandler = NULL;
					goto rescan_pdt;
				}

				if (rmi4_data->flash_prog_mode)
					goto flash_prog_mode;

				break;
			case SYNAPTICS_RMI4_F11:
				if (rmi_fd.intr_src_count == 0)
					break;

				retval = synaptics_rmi4_alloc_fh(&fhandler,
						&rmi_fd, page_number);
				if (retval < 0) {
					dev_err(&rmi4_data->i2c_client->dev,
							"%s: Failed to alloc for F%d\n",
							__func__,
							rmi_fd.fn_number);
					return retval;
				}

				retval = synaptics_rmi4_f11_init(rmi4_data,
						fhandler, &rmi_fd, intr_count);
				if (retval < 0)
					return retval;
				break;
			case SYNAPTICS_RMI4_F12:
				if (rmi_fd.intr_src_count == 0)
					break;

				retval = synaptics_rmi4_alloc_fh(&fhandler,
						&rmi_fd, page_number);
				if (retval < 0) {
					dev_err(&rmi4_data->i2c_client->dev,
							"%s: Failed to alloc for F%d\n",
							__func__,
							rmi_fd.fn_number);
					return retval;
				}

				retval = synaptics_rmi4_f12_init(rmi4_data,
						fhandler, &rmi_fd, intr_count);
				if (retval < 0)
					return retval;
				break;
			case SYNAPTICS_RMI4_F1A:
				if (rmi_fd.intr_src_count == 0)
					break;

				retval = synaptics_rmi4_alloc_fh(&fhandler,
						&rmi_fd, page_number);
				if (retval < 0) {
					dev_err(&rmi4_data->i2c_client->dev,
							"%s: Failed to alloc for F%d\n",
							__func__,
							rmi_fd.fn_number);
					return retval;
				}

				retval = synaptics_rmi4_f1a_init(rmi4_data,
						fhandler, &rmi_fd, intr_count);
				if (retval < 0) {
#ifdef IGNORE_FN_INIT_FAILURE
					kfree(fhandler);
					fhandler = NULL;
#else
					return retval;
#endif
				}
				break;
/*[SM20][zihweishen] Modify read config ID and product ID addr for INNX and Truly 20160428 begin*/	
			case SYNAPTICS_RMI4_F34:
				rmi4_data->f34_ctrl_base = rmi_fd.ctrl_base_addr;
				break;
/*[SM20][zihweishen] 20160428 end*/
			}


			/* Accumulate the interrupt count */
			intr_count += (rmi_fd.intr_src_count & MASK_3BIT);

			if (fhandler && rmi_fd.intr_src_count) {
				list_add_tail(&fhandler->link,
						&rmi->support_fn_list);
			}
		}
	}

flash_prog_mode:
	rmi4_data->num_of_intr_regs = (intr_count + 7) / 8;
	dev_dbg(&rmi4_data->i2c_client->dev,
			"%s: Number of interrupt registers = %d\n",
			__func__, rmi4_data->num_of_intr_regs);

	retval = synaptics_rmi4_i2c_read(rmi4_data,
			rmi4_data->f01_query_base_addr,
			f01_query,
			sizeof(f01_query));
	if (retval < 0)
		return retval;

	/* RMI Version 4.0 currently supported */
	rmi->version_major = 4;
	rmi->version_minor = 0;

	rmi->manufacturer_id = f01_query[0];
	rmi->product_props = f01_query[1];
	rmi->product_info[0] = f01_query[2] & MASK_7BIT;
	rmi->product_info[1] = f01_query[3] & MASK_7BIT;
	rmi->date_code[0] = f01_query[4] & MASK_5BIT;
	rmi->date_code[1] = f01_query[5] & MASK_4BIT;
	rmi->date_code[2] = f01_query[6] & MASK_5BIT;
	rmi->tester_id = ((f01_query[7] & MASK_7BIT) << 8) |
			(f01_query[8] & MASK_7BIT);
	rmi->serial_number = ((f01_query[9] & MASK_7BIT) << 8) |
			(f01_query[10] & MASK_7BIT);
	memcpy(rmi->product_id_string, &f01_query[11], 10);

	if (rmi->manufacturer_id != 1) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Non-Synaptics device found, manufacturer ID = %d\n",
				__func__, rmi->manufacturer_id);
	}


	retval = synaptics_rmi4_i2c_read(rmi4_data,
			rmi4_data->f01_query_base_addr + F01_BUID_ID_OFFSET,
			rmi->build_id,
			sizeof(rmi->build_id));
	if (retval < 0)
		return retval;


	rmi4_data->firmware_id = (unsigned int)rmi->build_id[0] +
			(unsigned int)rmi->build_id[1] * 0x100 +
			(unsigned int)rmi->build_id[2] * 0x10000;

/*[Lavender][bozhi_lin] store touch vendor and firmware version to tpd_show_vendor_firmware 20150216 begin*/
#if defined(TPD_REPORT_VENDOR_FW)
	{
		unsigned char config_id[4]={0};
		char buf[80]={0};
/*[SM20][zihweishen] Modify read config ID and product ID addr for INNX and Truly 20160428 begin*/	
		synaptics_rmi4_i2c_read(rmi4_data,
				rmi4_data->f34_ctrl_base,
				config_id,
				sizeof(config_id));
/*[SM20][zihweishen] 20160428 end*/
		sprintf(buf, "Synaptics_0x%02x%02x%02x%02x",
				config_id[0], config_id[1], config_id[2], config_id[3]);

		if (tpd_show_vendor_firmware == NULL) {
			tpd_show_vendor_firmware = kmalloc(strlen(buf) + 1, GFP_ATOMIC);
		}
		if (tpd_show_vendor_firmware != NULL) {
			strcpy(tpd_show_vendor_firmware, buf);
		}
	}
//<2016/05/17-stevenchen, update lcm vendor information in /sys/module/tpd_misc/parameters/lcm_vendor
//<2015/10/15-stevenchen, store lcm vendor information in /sys/module/tpd_misc/parameters/lcm_vendor
	{
		char buf1[10]={0};
		
		if (!strncmp(rmi->product_id_string, "SM20TY", 6)) 
		{
			sprintf(buf1, "Truly");
		}
		else
		{
			sprintf(buf1, "Innolux");
		}

		if (lcm_vendor == NULL) {
			lcm_vendor = kmalloc(strlen(buf1) + 1, GFP_ATOMIC);
		}
		if (lcm_vendor != NULL) {
			strcpy(lcm_vendor, buf1);
		}
	}
//>2015/10/15-stevenchen
//>2016/05/17-stevenchen
#endif
/*[Lavender][bozhi_lin] 20150216 end*/

/*[Lavender][bozhi_lin] synaptics touch detect glove or finger in glove mode 20150720 begin*/
#if defined(LAVENDER_GLOVE)
	{
		unsigned char config_id[4]={0};
		unsigned int device_config_id;
/*[SM20][zihweishen] Modify read config ID and product ID addr for INNX and Truly 20160428 begin*/	
		synaptics_rmi4_i2c_read(rmi4_data,
				rmi4_data->f34_ctrl_base,
				config_id,
				sizeof(config_id));
/*[SM20][zihweishen] 20160428 end*/
		device_config_id =  (unsigned int)config_id[3] +
			(unsigned int)config_id[2] * 0x100 +
			(unsigned int)config_id[1] * 0x10000 +
			(unsigned int)config_id[0] * 0x1000000;
		
		printk("[B]%s(%d): device_config_id=0x%x\n", __func__, __LINE__, device_config_id);
		if ( device_config_id >= 0x00040008)
		{
			glovemode_support_detection = true;
		}
		else 
		{
			glovemode_support_detection = false;
		}
		printk("[B]%s(%d): glovemode_support_detection=%d\n", __func__, __LINE__, glovemode_support_detection);
	}
#endif
/*[Lavender][bozhi_lin] 20150720 end*/

	memset(rmi4_data->intr_mask, 0x00, sizeof(rmi4_data->intr_mask));

	/*
	 * Map out the interrupt bit masks for the interrupt sources
	 * from the registered function handlers.
	 */
	if (!list_empty(&rmi->support_fn_list)) {
		list_for_each_entry(fhandler, &rmi->support_fn_list, link) {
			if (fhandler->num_of_data_sources) {
				rmi4_data->intr_mask[fhandler->intr_reg_num] |=
						fhandler->intr_mask;
			}
		}
	}

	/* Enable the interrupt sources */
	for (ii = 0; ii < rmi4_data->num_of_intr_regs; ii++) {
		if (rmi4_data->intr_mask[ii] != 0x00) {
			dev_dbg(&rmi4_data->i2c_client->dev,
					"%s: Interrupt enable mask %d = 0x%02x\n",
					__func__, ii, rmi4_data->intr_mask[ii]);
			intr_addr = rmi4_data->f01_ctrl_base_addr + 1 + ii;
			retval = synaptics_rmi4_i2c_write(rmi4_data,
					intr_addr,
					&(rmi4_data->intr_mask[ii]),
					sizeof(rmi4_data->intr_mask[ii]));
			if (retval < 0)
				return retval;
		}
	}

	synaptics_rmi4_set_configured(rmi4_data);

	return 0;
}

static void synaptics_rmi4_set_params(struct synaptics_rmi4_data *rmi4_data)
{
	unsigned char ii;
	struct synaptics_rmi4_f1a_handle *f1a;
	struct synaptics_rmi4_fn *fhandler;
	struct synaptics_rmi4_device_info *rmi;

	rmi = &(rmi4_data->rmi4_mod_info);

	input_set_abs_params(rmi4_data->input_dev,
			ABS_MT_POSITION_X, 0,
			rmi4_data->sensor_max_x, 0, 0);
	input_set_abs_params(rmi4_data->input_dev,
			ABS_MT_POSITION_Y, 0,
			rmi4_data->sensor_max_y, 0, 0);
#ifdef REPORT_2D_W
	input_set_abs_params(rmi4_data->input_dev,
			ABS_MT_TOUCH_MAJOR, 0,
			rmi4_data->max_touch_width, 0, 0);
	input_set_abs_params(rmi4_data->input_dev,
			ABS_MT_TOUCH_MINOR, 0,
			rmi4_data->max_touch_width, 0, 0);
#endif

#if defined(SYNAPTICS_MT6752)
	input_set_abs_params(rmi4_data->input_dev, ABS_MT_TRACKING_ID, 0, 10, 0, 0);

#else
#ifdef TYPE_B_PROTOCOL
	input_mt_init_slots(rmi4_data->input_dev,
			rmi4_data->num_of_fingers);
#endif
#endif

	f1a = NULL;
	if (!list_empty(&rmi->support_fn_list)) {
		list_for_each_entry(fhandler, &rmi->support_fn_list, link) {
			if (fhandler->fn_number == SYNAPTICS_RMI4_F1A)
				f1a = fhandler->data;
		}
	}

	if (f1a) {
		for (ii = 0; ii < f1a->valid_button_count; ii++) {
			set_bit(f1a->button_map[ii],
					rmi4_data->input_dev->keybit);
			input_set_capability(rmi4_data->input_dev,
					EV_KEY, f1a->button_map[ii]);
		}
	}

	return;
}

static int synaptics_rmi4_set_input_dev(struct synaptics_rmi4_data *rmi4_data)
{
	int retval;
	//int temp;
	printk("[zihwei] : synaptics_rmi4_set_input_dev,");

	rmi4_data->input_dev = input_allocate_device();
	if (rmi4_data->input_dev == NULL) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Failed to allocate input device\n",
				__func__);
		retval = -ENOMEM;
		goto err_input_device;
	}

	retval = synaptics_rmi4_query_device(rmi4_data);
	printk("[elan] : retval %x,",  retval);
	if (retval < 0) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Failed to query device\n",
				__func__);
		goto err_query_device;
	}

	rmi4_data->input_dev->name = DRIVER_NAME;
	rmi4_data->input_dev->phys = INPUT_PHYS_NAME;
	rmi4_data->input_dev->id.product = SYNAPTICS_DSX_DRIVER_PRODUCT;
	rmi4_data->input_dev->id.version = SYNAPTICS_DSX_DRIVER_VERSION;
	rmi4_data->input_dev->id.bustype = BUS_I2C;
	rmi4_data->input_dev->dev.parent = &rmi4_data->i2c_client->dev;
	input_set_drvdata(rmi4_data->input_dev, rmi4_data);

	set_bit(EV_SYN, rmi4_data->input_dev->evbit);
	set_bit(EV_KEY, rmi4_data->input_dev->evbit);
	set_bit(EV_ABS, rmi4_data->input_dev->evbit);
	set_bit(BTN_TOUCH, rmi4_data->input_dev->keybit);
	set_bit(BTN_TOOL_FINGER, rmi4_data->input_dev->keybit);
#ifdef INPUT_PROP_DIRECT
	set_bit(INPUT_PROP_DIRECT, rmi4_data->input_dev->propbit);
#endif

	synaptics_rmi4_set_params(rmi4_data);

	retval = input_register_device(rmi4_data->input_dev);
	if (retval) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Failed to register input device\n",
				__func__);
		goto err_register_input;
	}

	return 0;

err_register_input:
err_query_device:
	synaptics_rmi4_empty_fn_list(rmi4_data);
	input_free_device(rmi4_data->input_dev);

err_input_device:
	return retval;
}

static int synaptics_rmi4_free_fingers(struct synaptics_rmi4_data *rmi4_data)
{
	//unsigned char ii;

#ifdef TYPE_B_PROTOCOL
	for (ii = 0; ii < rmi4_data->num_of_fingers; ii++) {
		input_mt_slot(rmi4_data->input_dev, ii);
		input_mt_report_slot_state(rmi4_data->input_dev,
				MT_TOOL_FINGER, 0);
	}
#endif
	input_report_key(rmi4_data->input_dev,
			BTN_TOUCH, 0);
	input_report_key(rmi4_data->input_dev,
			BTN_TOOL_FINGER, 0);
#ifndef TYPE_B_PROTOCOL
	input_mt_sync(rmi4_data->input_dev);
#endif
	input_sync(rmi4_data->input_dev);

	rmi4_data->fingers_on_2d = false;

	return 0;
}

static int synaptics_rmi4_reinit_device(struct synaptics_rmi4_data *rmi4_data)
{
	int retval;
	unsigned char ii;
	unsigned short intr_addr;
	struct synaptics_rmi4_fn *fhandler;
	struct synaptics_rmi4_exp_fhandler *exp_fhandler;
	struct synaptics_rmi4_device_info *rmi;

	rmi = &(rmi4_data->rmi4_mod_info);

	mutex_lock(&(rmi4_data->rmi4_reset_mutex));

	synaptics_rmi4_free_fingers(rmi4_data);

	if (!list_empty(&rmi->support_fn_list)) {
		list_for_each_entry(fhandler, &rmi->support_fn_list, link) {
			if (fhandler->fn_number == SYNAPTICS_RMI4_F12) {
				synaptics_rmi4_f12_set_enables(rmi4_data, 0);
				break;
			}
		}
	}

	for (ii = 0; ii < rmi4_data->num_of_intr_regs; ii++) {
		if (rmi4_data->intr_mask[ii] != 0x00) {
			dev_dbg(&rmi4_data->i2c_client->dev,
					"%s: Interrupt enable mask %d = 0x%02x\n",
					__func__, ii, rmi4_data->intr_mask[ii]);
			intr_addr = rmi4_data->f01_ctrl_base_addr + 1 + ii;
			retval = synaptics_rmi4_i2c_write(rmi4_data,
					intr_addr,
					&(rmi4_data->intr_mask[ii]),
					sizeof(rmi4_data->intr_mask[ii]));
			if (retval < 0)
				goto exit;
		}
	}

	mutex_lock(&exp_data.mutex);
	if (!list_empty(&exp_data.list)) {
		list_for_each_entry(exp_fhandler, &exp_data.list, link)
			if (exp_fhandler->exp_fn->reinit != NULL)
				exp_fhandler->exp_fn->reinit(rmi4_data);
	}
	mutex_unlock(&exp_data.mutex);

	synaptics_rmi4_set_configured(rmi4_data);

	retval = 0;

exit:
	mutex_unlock(&(rmi4_data->rmi4_reset_mutex));
	return retval;
}

static int synaptics_rmi4_reset_device(struct synaptics_rmi4_data *rmi4_data)
{
	int retval;
	//int temp;
	unsigned char command = 0x01;
	struct synaptics_rmi4_exp_fhandler *exp_fhandler;

	mutex_lock(&(rmi4_data->rmi4_reset_mutex));

/*[Lavender][bozhi_lin] enable touch build-in firmware upgrade mechanism 20150305 begin*/
	synaptics_rmi4_irq_enable(rmi4_data, false);
/*[Lavender][bozhi_lin] 20150305 end*/

	rmi4_data->touch_stopped = true;

	retval = synaptics_rmi4_i2c_write(rmi4_data,
			rmi4_data->f01_cmd_base_addr,
			&command,
			sizeof(command));
	if (retval < 0) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Failed to issue reset command, error = %d\n",
				__func__, retval);
		mutex_unlock(&(rmi4_data->rmi4_reset_mutex));
		return retval;
	}

	msleep(DELAY_S7300_RESET_READY);

	synaptics_rmi4_free_fingers(rmi4_data);

	synaptics_rmi4_empty_fn_list(rmi4_data);

	retval = synaptics_rmi4_query_device(rmi4_data);
	if (retval < 0) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Failed to query device\n",
				__func__);
		mutex_unlock(&(rmi4_data->rmi4_reset_mutex));
		return retval;
	}

	synaptics_rmi4_set_params(rmi4_data);

	mutex_lock(&exp_data.mutex);
	if (!list_empty(&exp_data.list)) {
		list_for_each_entry(exp_fhandler, &exp_data.list, link)
			if (exp_fhandler->exp_fn->reset != NULL)
				exp_fhandler->exp_fn->reset(rmi4_data);
	}
	mutex_unlock(&exp_data.mutex);

	rmi4_data->touch_stopped = false;
	
/*[Lavender][bozhi_lin] enable touch build-in firmware upgrade mechanism 20150305 begin*/
	synaptics_rmi4_irq_enable(rmi4_data, true);
/*[Lavender][bozhi_lin] 20150305 end*/

	mutex_unlock(&(rmi4_data->rmi4_reset_mutex));

	return 0;
}

/**
* synaptics_rmi4_exp_fn_work()
*
* Called by the kernel at the scheduled time.
*
* This function is a work thread that checks for the insertion and
* removal of other expansion Function modules such as rmi_dev and calls
* their initialization and removal callback functions accordingly.
*/
static void synaptics_rmi4_exp_fn_work(struct work_struct *work)
{
	int retval;
	struct synaptics_rmi4_exp_fhandler *exp_fhandler;
	struct synaptics_rmi4_exp_fhandler *exp_fhandler_temp;
	struct synaptics_rmi4_data *rmi4_data = exp_data.rmi4_data;

	mutex_lock(&exp_data.mutex);
	if (!list_empty(&exp_data.list)) {
		list_for_each_entry_safe(exp_fhandler,
				exp_fhandler_temp,
				&exp_data.list,
				link) {
			if ((exp_fhandler->exp_fn->init != NULL) &&
					exp_fhandler->insert) {
				retval = exp_fhandler->exp_fn->init(rmi4_data);
				if (retval < 0) {
					list_del(&exp_fhandler->link);
					kfree(exp_fhandler);
				} else {
					exp_fhandler->insert = false;
				}
			} else if ((exp_fhandler->exp_fn->remove != NULL) &&
					exp_fhandler->remove) {
				exp_fhandler->exp_fn->remove(rmi4_data);
				list_del(&exp_fhandler->link);
				kfree(exp_fhandler);
			}
		}
	}
	mutex_unlock(&exp_data.mutex);

	return;
}

/*
* synaptics_rmi4_new_function()
*
* Called by other expansion Function modules in their module init and
* module exit functions.
*
* This function is used by other expansion Function modules such as
* rmi_dev to register themselves with the driver by providing their
* initialization and removal callback function pointers so that they
* can be inserted or removed dynamically at module init and exit times,
* respectively.
*/

void synaptics_rmi4_new_function(struct synaptics_rmi4_exp_fn *exp_fn,
		bool insert)
{
	struct synaptics_rmi4_exp_fhandler *exp_fhandler;

	if (!exp_data.initialized) {
		mutex_init(&exp_data.mutex);
		INIT_LIST_HEAD(&exp_data.list);
		exp_data.initialized = true;
	}

	mutex_lock(&exp_data.mutex);
	if (insert) {
		exp_fhandler = kzalloc(sizeof(*exp_fhandler), GFP_KERNEL);
		if (!exp_fhandler) {
			pr_err("%s: Failed to alloc mem for expansion function\n",
					__func__);
			goto exit;
		}
		exp_fhandler->exp_fn = exp_fn;
		exp_fhandler->insert = true;
		exp_fhandler->remove = false;
		list_add_tail(&exp_fhandler->link, &exp_data.list);
	} else if (!list_empty(&exp_data.list)) {
		list_for_each_entry(exp_fhandler, &exp_data.list, link) {
			if (exp_fhandler->exp_fn->fn_type == exp_fn->fn_type) {
				exp_fhandler->insert = false;
				exp_fhandler->remove = true;
				goto exit;
			}
		}
	}

exit:
	mutex_unlock(&exp_data.mutex);

	if (exp_data.queue_work) {
		queue_delayed_work(exp_data.workqueue,
				&exp_data.work,
				msecs_to_jiffies(EXP_FN_WORK_DELAY_MS));
	}

	return;
}

EXPORT_SYMBOL(synaptics_rmi4_new_function);

  /*
 * synaptics_rmi4_probe()
 *
 * Called by the kernel when an association with an I2C device of the
 * same name is made (after doing i2c_add_driver).
 *
 * This funtion allocates and initializes the resources for the driver
 * as an input driver, turns on the power to the sensor, queries the
 * sensor for its supported Functions and characteristics, registers
 * the driver to the input subsystem, sets up the interrupt, handles
 * the registration of the early_suspend and late_resume functions,
 * and creates a work queue for detection of other expansion Function
 * modules.
 */
static int synaptics_rmi4_probe(struct i2c_client *client,
		const struct i2c_device_id *dev_id)
{
	int retval;
	unsigned char attr_count;
	struct synaptics_rmi4_data *rmi4_data;

	if (!i2c_check_functionality(client->adapter,
			I2C_FUNC_SMBUS_BYTE_DATA)) {
		dev_err(&client->dev,
				"%s: SMBus byte data not supported\n",
				__func__);
		return -EIO;
	}


	// gpio setting
	/*mt_set_gpio_mode(GPIO_CTP_EINT_PIN, GPIO_CTP_EINT_PIN_M_EINT);
	mt_set_gpio_dir(GPIO_CTP_EINT_PIN, GPIO_DIR_IN);
	mt_set_gpio_pull_enable(GPIO_CTP_EINT_PIN, GPIO_PULL_ENABLE);
	mt_set_gpio_pull_select(GPIO_CTP_EINT_PIN, GPIO_PULL_UP);*/

	//mt_set_gpio_mode(GPIO_CTP_RST_PIN, GPIO_CTP_RST_PIN_M_GPIO);
   	//mt_set_gpio_dir(GPIO_CTP_RST_PIN, GPIO_DIR_OUT);
   	
   //[SM20][zihweishen] Modify for android M of GPIO setting begin 2015/11/11
  	 GTP_GPIO_OUTPUT(GTP_RST_PORT, 1);//mt_set_gpio_out(GPIO_CTP_RST_PIN, GPIO_OUT_ONE);

	// power up sequence
	//[SM20][zihweishen] Modify PMIC MT6325 to MT6351 for TP_VDD begin 2015/10/14
	//hwPowerOn(MT6351_POWER_LDO_VLDO28, VOL_DEFAULT, "TP");
	syn_power_enable( SYN_ENABLE );

	//[SM20][zihweishen] Modify PMIC MT6325 to MT6351 for TP_VDD end 2015/10/14
	msleep(DELAY_S7300_BOOT_READY);
  	 GTP_GPIO_OUTPUT(GTP_RST_PORT, 0);//mt_set_gpio_out(GPIO_CTP_RST_PIN, GPIO_OUT_ZERO);
	msleep(DELAY_S7300_RESET);
  	 GTP_GPIO_OUTPUT(GTP_RST_PORT, 1);//mt_set_gpio_out(GPIO_CTP_RST_PIN, GPIO_OUT_ONE);
	msleep(DELAY_S7300_RESET_READY);
//[SM20][zihweishen] Modify for android M of GPIO setting end 2015/11/11


	rmi4_data = kzalloc(sizeof(*rmi4_data), GFP_KERNEL);
	if (!rmi4_data) {
		dev_err(&client->dev,
				"%s: Failed to alloc mem for rmi4_data\n",
				__func__);
		return -ENOMEM;
	}

	rmi4_data->i2c_client = client;
	rmi4_data->current_page = MASK_8BIT;
	rmi4_data->touch_stopped = false;
	rmi4_data->sensor_sleep = false;
	rmi4_data->irq_enabled = false;
	rmi4_data->fingers_on_2d = false;

	rmi4_data->i2c_read = synaptics_rmi4_i2c_read;
	rmi4_data->i2c_write = synaptics_rmi4_i2c_write;
	rmi4_data->irq_enable = synaptics_rmi4_irq_enable;
	rmi4_data->reset_device = synaptics_rmi4_reset_device;

	mutex_init(&(rmi4_data->rmi4_io_ctrl_mutex));
	mutex_init(&(rmi4_data->rmi4_reset_mutex));

	i2c_set_clientdata(client, rmi4_data);
	retval = synaptics_rmi4_set_input_dev(rmi4_data);
	
//[SM20][zihweishen]  Dynamic detect elan and Synaptics touch driver begin 2015/11/11
		if (retval < 0)
		{
	  //[SM20][zihweishen] Modify for disable touch LDO_VGP1 in suspend state begin 2015/10/14

	syn_power_enable( SYN_DISABLE );
	  //[SM20][zihweishen] Modify for disable touch LDO_VGP1 in suspend state end 2015/10/14
		  return -EIO;
		}
//[SM20][zihweishen]  Dynamic detect elan and Synaptics touch driver end 2015/11/11

	if (retval < 0) {
		dev_err(&client->dev,
				"%s: Failed to set up input device\n",
				__func__);
		goto err_set_input_dev;
	}

//[SM20][zihweishen] Add Touch ESD_CHECK begin 2016/02/5
#if defined( ESD_CHECK )
  INIT_DELAYED_WORK( &esd_work, synaptic_touch_esd_func );
  esd_wq = create_singlethread_workqueue( "esd_wq" );
  if( !esd_wq )
  {
    retval = -ENOMEM;
  }

  queue_delayed_work( esd_wq, &esd_work, delay );
#endif
//[SM20][zihweishen] end 2016/02/5

#ifdef CONFIG_HAS_EARLYSUSPEND
	rmi4_data->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	rmi4_data->early_suspend.suspend = synaptics_rmi4_early_suspend;
	rmi4_data->early_suspend.resume = synaptics_rmi4_late_resume;
	register_early_suspend(&rmi4_data->early_suspend);
#endif

	thread = kthread_run(touch_event_handler, rmi4_data, "synaptics-tpd");
	if ( IS_ERR(thread) ) {
		retval = PTR_ERR(thread);
		pr_err(" %s: failed to create kernel thread: %d\n",__func__, retval);
	}

	retval = synaptics_rmi4_irq_enable(rmi4_data, true);
	if (retval < 0) {
		dev_err(&client->dev,
				"%s: Failed to enable attention interrupt\n",
				__func__);
		goto err_enable_irq;
	}

	if (!exp_data.initialized) {
		mutex_init(&exp_data.mutex);
		INIT_LIST_HEAD(&exp_data.list);
		exp_data.initialized = true;
	}
	
/*[Lavender][poting_chang] GLOVE mode interface  20150303 begin*/
	#if defined( LAVENDER_GLOVE )
	Syna_touch_sysfs_init(); 
/*[Lavender][poting_chang]  20150302 end*/
	#endif
		
	exp_data.workqueue = create_singlethread_workqueue("dsx_exp_workqueue");
	INIT_DELAYED_WORK(&exp_data.work, synaptics_rmi4_exp_fn_work);
	exp_data.rmi4_data = rmi4_data;
	exp_data.queue_work = true;
	queue_delayed_work(exp_data.workqueue,
			&exp_data.work,
			msecs_to_jiffies(EXP_FN_WORK_DELAY_MS));

	for (attr_count = 0; attr_count < ARRAY_SIZE(attrs); attr_count++) {
		retval = sysfs_create_file(&rmi4_data->input_dev->dev.kobj,
				&attrs[attr_count].attr);
		if (retval < 0) {
			dev_err(&client->dev,
					"%s: Failed to create sysfs attributes\n",
					__func__);
			goto err_sysfs;
		}
	}

/*[Lavender][poting_chang] GLOVE mode interface  20150303 begin*/
#if defined( LAVENDER_GLOVE )
	rmi4_glove = rmi4_data;
#endif
/*[Lavender][poting_chang]  20150302 end*/
	
	tpd_load_status = 1;
	g_dev = &rmi4_data->input_dev->dev;

	printk("[zihwei] : probe OK");
	return retval;

err_sysfs:
	for (attr_count--; attr_count >= 0; attr_count--) {
		sysfs_remove_file(&rmi4_data->input_dev->dev.kobj,
				&attrs[attr_count].attr);
	}

	cancel_delayed_work_sync(&exp_data.work);
	flush_workqueue(exp_data.workqueue);
	destroy_workqueue(exp_data.workqueue);

	synaptics_rmi4_irq_enable(rmi4_data, false);

err_enable_irq:
#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&rmi4_data->early_suspend);
#endif

	synaptics_rmi4_empty_fn_list(rmi4_data);
	input_unregister_device(rmi4_data->input_dev);
	rmi4_data->input_dev = NULL;

err_set_input_dev:
	kfree(rmi4_data);

	return retval;
}

 /**
 * synaptics_rmi4_remove()
 *
 * Called by the kernel when the association with an I2C device of the
 * same name is broken (when the driver is unloaded).
 *
 * This funtion terminates the work queue, stops sensor data acquisition,
 * frees the interrupt, unregisters the driver from the input subsystem,
 * turns off the power to the sensor, and frees other allocated resources.
 */

//[SM20][zihweishen] Add Touch ESD_CHECK begin 2016/02/5
 #if defined( ESD_CHECK )
static void synaptic_touch_esd_func(struct work_struct *work)
{
//int   res;

  printk("[synaptic esd] %s: enter.......\n", __FUNCTION__);  /* elan_dlx */

  if( have_interrupts == 1 )
  {
    //printk("[elan esd] %s: had interrup not need check\n", __func__);
    printk("[synaptic esd] : had interrup not need check\n");
  }
  else
  {
   GTP_GPIO_OUTPUT(GTP_RST_PORT, 0);
    msleep( 10 );
   GTP_GPIO_OUTPUT(GTP_RST_PORT, 1);
    msleep( 100 );
  }

  have_interrupts = 0;
  queue_delayed_work( esd_wq, &esd_work, delay );
  printk("[synaptic esd] %s: exit.......\n", __FUNCTION__ );
}
#endif
//[SM20][zihweishen] end 2016/02/5

static int synaptics_rmi4_remove(struct i2c_client *client)
{
	unsigned char attr_count;
	struct synaptics_rmi4_data *rmi4_data = i2c_get_clientdata(client);

	for (attr_count = 0; attr_count < ARRAY_SIZE(attrs); attr_count++) {
		sysfs_remove_file(&rmi4_data->input_dev->dev.kobj,
				&attrs[attr_count].attr);
	}

	cancel_delayed_work_sync(&exp_data.work);
	flush_workqueue(exp_data.workqueue);
	destroy_workqueue(exp_data.workqueue);

	synaptics_rmi4_irq_enable(rmi4_data, false);

#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&rmi4_data->early_suspend);
#endif

	synaptics_rmi4_empty_fn_list(rmi4_data);
	input_unregister_device(rmi4_data->input_dev);
	rmi4_data->input_dev = NULL;

	kfree(rmi4_data);

	return 0;
}

#ifdef CONFIG_PM
 /**
 * synaptics_rmi4_sensor_sleep()
 *
 * Called by synaptics_rmi4_early_suspend() and synaptics_rmi4_suspend().
 *
 * This function stops finger data acquisition and puts the sensor to sleep.
 */
static void synaptics_rmi4_sensor_sleep(struct synaptics_rmi4_data *rmi4_data)
{
	int retval;
	unsigned char device_ctrl;

	retval = synaptics_rmi4_i2c_read(rmi4_data,
			rmi4_data->f01_ctrl_base_addr,
			&device_ctrl,
			sizeof(device_ctrl));
	if (retval < 0) {
		dev_err(&(rmi4_data->input_dev->dev),
				"%s: Failed to enter sleep mode\n",
				__func__);
		rmi4_data->sensor_sleep = false;
		return;
	}

	device_ctrl = (device_ctrl & ~MASK_3BIT);
	device_ctrl = (device_ctrl | NO_SLEEP_OFF | SENSOR_SLEEP);

	retval = synaptics_rmi4_i2c_write(rmi4_data,
			rmi4_data->f01_ctrl_base_addr,
			&device_ctrl,
			sizeof(device_ctrl));
	if (retval < 0) {
		dev_err(&(rmi4_data->input_dev->dev),
				"%s: Failed to enter sleep mode\n",
				__func__);
		rmi4_data->sensor_sleep = false;
		return;
	} else {
		rmi4_data->sensor_sleep = true;
	}

	return;
}

 /**
 * synaptics_rmi4_sensor_wake()
 *
 * Called by synaptics_rmi4_resume() and synaptics_rmi4_late_resume().
 *
 * This function wakes the sensor from sleep.
 */
static void synaptics_rmi4_sensor_wake(struct synaptics_rmi4_data *rmi4_data)
{
	int retval;
	unsigned char device_ctrl;
	unsigned char no_sleep_setting = rmi4_data->no_sleep_setting;

	retval = synaptics_rmi4_i2c_read(rmi4_data,
			rmi4_data->f01_ctrl_base_addr,
			&device_ctrl,
			sizeof(device_ctrl));
	if (retval < 0) {
		dev_err(&(rmi4_data->input_dev->dev),
				"%s: Failed to wake from sleep mode\n",
				__func__);
		rmi4_data->sensor_sleep = true;
		return;
	}

	device_ctrl = (device_ctrl & ~MASK_3BIT);
	device_ctrl = (device_ctrl | no_sleep_setting | NORMAL_OPERATION);

	retval = synaptics_rmi4_i2c_write(rmi4_data,
			rmi4_data->f01_ctrl_base_addr,
			&device_ctrl,
			sizeof(device_ctrl));
	if (retval < 0) {
		dev_err(&(rmi4_data->input_dev->dev),
				"%s: Failed to wake from sleep mode\n",
				__func__);
		rmi4_data->sensor_sleep = true;
		return;
	} else {
		rmi4_data->sensor_sleep = false;
	}

	return;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
 /**
 * synaptics_rmi4_early_suspend()
 *
 * Called by the kernel during the early suspend phase when the system
 * enters suspend.
 *
 * This function calls synaptics_rmi4_sensor_sleep() to stop finger
 * data acquisition and put the sensor to sleep.
 */
static void synaptics_rmi4_early_suspend(struct early_suspend *h)
{
	int ret;
	struct synaptics_rmi4_exp_fhandler *exp_fhandler;
	struct synaptics_rmi4_data *rmi4_data =
			container_of(h, struct synaptics_rmi4_data,
			early_suspend);

	if (rmi4_data->stay_awake) {
		rmi4_data->staying_awake = true;
		return;
	} else {
		rmi4_data->staying_awake = false;
	}

	rmi4_data->touch_stopped = true;
	synaptics_rmi4_irq_enable(rmi4_data, false);
	synaptics_rmi4_sensor_sleep(rmi4_data);
	synaptics_rmi4_free_fingers(rmi4_data);
/*[Lavender][bozhi_lin] synaptics touch detect glove or finger in glove mode 20150720 begin*/
#if defined(LAVENDER_GLOVE)

#else
//[SM20][zihweishen] Modify PMIC MT6325 to MT6351 for TP_VDD begin 2015/10/14
/*[Lavender][bozhi_lin] disable touch LDO_VGP1 in suspend state 20150327 begin*/
	ret = regulator_disable(tpd->reg);	/*disable regulator*/
	if (!ret) {
		printk("[B]%s(%d): Synaptics TPD power off fail", __func__, __LINE__);
	}
/*[Lavender][bozhi_lin] 20150327 end*/
//[SM20][zihweishen] Modify PMIC MT6325 to MT6351 for TP_VDD end 2015/10/14
#endif
/*[Lavender][bozhi_lin] 20150720 end*/
	mutex_lock(&exp_data.mutex);
	if (!list_empty(&exp_data.list)) {
		list_for_each_entry(exp_fhandler, &exp_data.list, link)
			if (exp_fhandler->exp_fn->early_suspend != NULL)
				exp_fhandler->exp_fn->early_suspend(rmi4_data);
	}
	mutex_unlock(&exp_data.mutex);

	if (rmi4_data->full_pm_cycle)
		synaptics_rmi4_suspend(&(rmi4_data->input_dev->dev));

	return;
}

 /**
 * synaptics_rmi4_late_resume()
 *
 * Called by the kernel during the late resume phase when the system
 * wakes up from suspend.
 *
 * This function goes through the sensor wake process if the system wakes
 * up from early suspend (without going into suspend).
 */
static void synaptics_rmi4_late_resume(struct early_suspend *h)
{
	int retval;
	struct synaptics_rmi4_exp_fhandler *exp_fhandler;
	struct synaptics_rmi4_data *rmi4_data =
			container_of(h, struct synaptics_rmi4_data,
			early_suspend);

	if (rmi4_data->staying_awake)
		return;

	if (rmi4_data->full_pm_cycle)
		synaptics_rmi4_resume(&(rmi4_data->input_dev->dev));

	if (rmi4_data->sensor_sleep == true) {
		synaptics_rmi4_sensor_wake(rmi4_data);
		synaptics_rmi4_irq_enable(rmi4_data, true);
		retval = synaptics_rmi4_reinit_device(rmi4_data);
		if (retval < 0) {
			dev_err(&rmi4_data->i2c_client->dev,
					"%s: Failed to reinit device\n",
					__func__);
		}
	}

	mutex_lock(&exp_data.mutex);
	if (!list_empty(&exp_data.list)) {
		list_for_each_entry(exp_fhandler, &exp_data.list, link)
			if (exp_fhandler->exp_fn->late_resume != NULL)
				exp_fhandler->exp_fn->late_resume(rmi4_data);
	}
	mutex_unlock(&exp_data.mutex);

	rmi4_data->touch_stopped = false;

	return;
}
#endif

 /**
 * synaptics_rmi4_suspend()
 *
 * Called by the kernel during the suspend phase when the system
 * enters suspend.
 *
 * This function stops finger data acquisition and puts the sensor to
 * sleep (if not already done so during the early suspend phase),
 * disables the interrupt, and turns off the power to the sensor.
 */
  //[SM20][zihweishen] Modify  for build error begin 2015/11/11
 static int synaptics_rmi4_dev_suspend(struct device *dev)
{
	struct synaptics_rmi4_exp_fhandler *exp_fhandler;
	struct synaptics_rmi4_data *rmi4_data = dev_get_drvdata(g_dev);

	if (rmi4_data->staying_awake)
		return 0;

	if (!rmi4_data->sensor_sleep) {
		rmi4_data->touch_stopped = true;
		synaptics_rmi4_irq_enable(rmi4_data, false);
		synaptics_rmi4_sensor_sleep(rmi4_data);
		synaptics_rmi4_free_fingers(rmi4_data);
	}

	mutex_lock(&exp_data.mutex);
	if (!list_empty(&exp_data.list)) {
		list_for_each_entry(exp_fhandler, &exp_data.list, link)
			if (exp_fhandler->exp_fn->suspend != NULL)
				exp_fhandler->exp_fn->suspend(rmi4_data);
	}
	mutex_unlock(&exp_data.mutex);
	tpd_halt = 1;
	return 0;
}
//[SM20][zihweishen] Modify  for build error end 2015/11/11

//[SM20][zihweishen] Modify touch screen randomly stops working begin 2016/8/23
static void synaptics_rmi4_suspend(struct device *h)
{
	struct synaptics_rmi4_exp_fhandler *exp_fhandler;
	struct synaptics_rmi4_data *rmi4_data = dev_get_drvdata(g_dev);

	if (rmi4_data->staying_awake)
		return;

	if (!rmi4_data->sensor_sleep) {
		rmi4_data->touch_stopped = true;
		synaptics_rmi4_irq_enable(rmi4_data, false);
		synaptics_rmi4_sensor_sleep(rmi4_data);
		synaptics_rmi4_free_fingers(rmi4_data);
	}

	mutex_lock(&exp_data.mutex);
	if (!list_empty(&exp_data.list)) {
		list_for_each_entry(exp_fhandler, &exp_data.list, link)
			if (exp_fhandler->exp_fn->suspend != NULL)
				exp_fhandler->exp_fn->suspend(rmi4_data);
	}
	mutex_unlock(&exp_data.mutex);
	tpd_halt = 1;
	synaptic_irq_disable();
	printk("zihwei 2GPIO1 sleep=%x\n",mt_get_gpio_value(1));
	return;
}
//[SM20][zihweishen] end 2016/8/23

 /**
 * synaptics_rmi4_resume()
 *
 * Called by the kernel during the resume phase when the system
 * wakes up from suspend.
 *
 * This function turns on the power to the sensor, wakes the sensor
 * from sleep, enables the interrupt, and starts finger data
 * acquisition.
 */
//[SM20][zihweishen] Add touch reset when phone resume begin 2016/5/17
//[SM20][zihweishen] Modify  for build error begin 2015/11/11
 static int synaptics_rmi4_dev_resume(struct device *dev)
{
	int retval,reset;
	unsigned char command = 0x01;
	//int ret;
	struct synaptics_rmi4_exp_fhandler *exp_fhandler;
	struct synaptics_rmi4_data *rmi4_data = dev_get_drvdata(g_dev);

	if (rmi4_data->staying_awake)
		return 0;

/*[Lavender][bozhi_lin] synaptics touch detect glove or finger in glove mode 20150720 begin*/
/*[Lavender][bozhi_lin] 20150720 end*/
	reset = synaptics_rmi4_i2c_write(rmi4_data,
			rmi4_data->f01_cmd_base_addr,
			&command,
			sizeof(command));

	synaptics_rmi4_sensor_wake(rmi4_data);
	synaptics_rmi4_irq_enable(rmi4_data, true);
	retval = synaptics_rmi4_reinit_device(rmi4_data);
	if (retval < 0) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Failed to reinit device\n",
				__func__);
		return retval;
	}

	mutex_lock(&exp_data.mutex);
	if (!list_empty(&exp_data.list)) {
		list_for_each_entry(exp_fhandler, &exp_data.list, link)
			if (exp_fhandler->exp_fn->resume != NULL)
				exp_fhandler->exp_fn->resume(rmi4_data);
	}
	mutex_unlock(&exp_data.mutex);

	rmi4_data->touch_stopped = false;
	tpd_halt = 0;

/*[Lavender][bozhi_lin] add check touch ap2 product id to only ap2 can enable glove mode 20150513 begin*/
#if defined( LAVENDER_GLOVE )
	if(glovemode)
		elan_glove_enable(true);
#endif
/*[Lavender][bozhi_lin] 20150513 end*/

	return 0;
}
//[SM20][zihweishen] Modify  for build error end 2015/11/11
//[SM20][zihweishen] Add touch reset when phone resume end 2016/5/17

//[SM20][zihweishen] Add touch reset when phone resume begin 2016/5/17

//[SM20][zihweishen] Modify touch screen randomly stops working begin 2016/8/23
static void synaptics_rmi4_resume(struct device *h)
{
		 int retval,reset,retval1;
		 unsigned char data[MAX_INTR_REGISTERS + 1];
		 unsigned char command = 0x01;
		 //int ret;
		 struct synaptics_rmi4_exp_fhandler *exp_fhandler;
		 struct synaptics_rmi4_data *rmi4_data = dev_get_drvdata(g_dev);
	 
		 if (rmi4_data->staying_awake)
			 return;
	 
	 /*[Lavender][bozhi_lin] synaptics touch detect glove or finger in glove mode 20150720 begin*/
	 /*[Lavender][bozhi_lin] 20150720 end*/
		 //synaptics_rmi4_irq_enable(rmi4_data, true);
		 reset = synaptics_rmi4_i2c_write(rmi4_data,
				 rmi4_data->f01_cmd_base_addr,
				 &command,
				 sizeof(command));

		 synaptics_rmi4_sensor_wake(rmi4_data);
		 synaptics_rmi4_irq_enable(rmi4_data, true);
		 retval = synaptics_rmi4_reinit_device(rmi4_data);
		 if (retval < 0) {
			 dev_err(&rmi4_data->i2c_client->dev,
					 "%s: Failed to reinit device\n",
					 __func__);
			 return;
		 }
	 
		 mutex_lock(&exp_data.mutex);
		 if (!list_empty(&exp_data.list)) {
			 list_for_each_entry(exp_fhandler, &exp_data.list, link)
				 if (exp_fhandler->exp_fn->resume != NULL)
					 exp_fhandler->exp_fn->resume(rmi4_data);
		 }
		 mutex_unlock(&exp_data.mutex);
	 
		 rmi4_data->touch_stopped = false;
		 tpd_halt = 0;
	 
	 /*[Lavender][bozhi_lin] add check touch ap2 product id to only ap2 can enable glove mode 20150513 begin*/
#if defined( LAVENDER_GLOVE )
		 if(glovemode)
			 elan_glove_enable(true);
#endif
	 /*[Lavender][bozhi_lin] 20150513 end*/
	 synaptic_irq_enable();
		 retval1 = synaptics_rmi4_i2c_read(rmi4_data,
				 rmi4_data->f01_data_base_addr,
				 data,
				 rmi4_data->num_of_intr_regs + 1);

		 printk("zihwei 2GPIO1 resume=%x\n",mt_get_gpio_value(1));

		 return;

}
//[SM20][zihweishen] Add touch reset when phone resume end 2016/5/17
//[SM20][zihweishen] end 2016/8/23

static const struct dev_pm_ops synaptics_rmi4_dev_pm_ops = {
	.suspend = synaptics_rmi4_dev_suspend,
	.resume  = synaptics_rmi4_dev_resume,
};
#endif

//[SM20][zihweishen] Modify for I2c register and device of android M  begin 2015/11/11
#define SYNAPTIC_DRIVER_NAME  "synaptics-tpd"

static const struct i2c_device_id synaptics_rmi4_id_table[] = {
	{SYNAPTIC_DRIVER_NAME, 0},
	{},
};
unsigned short syn_force[] = {0,TPD_I2C_ADDR,I2C_CLIENT_END,I2C_CLIENT_END};
static const unsigned short * const syn_forces[] = { syn_force, NULL };
//static int tpd_detect(struct i2c_client *client, struct i2c_board_info *info);
static const struct of_device_id tpd_of_match[] = {
	{.compatible = "mediatek,cap_touch_sy"},
	{},
};

MODULE_DEVICE_TABLE(i2c, synaptics_rmi4_id_table);

static struct i2c_driver tpd_i2c_driver = {
	    .driver = {
      .name   = SYNAPTIC_DRIVER_NAME,
      .of_match_table = tpd_of_match,
    },
	.probe = synaptics_rmi4_probe,
	.remove = synaptics_rmi4_remove,
	//.detect = tpd_detect,
	.driver.name = SYNAPTIC_DRIVER_NAME,
	.id_table = synaptics_rmi4_id_table,
	.address_list = (const unsigned short*) syn_forces,
};
//[SM20][zihweishen] Modify  for I2c register and device of android M  end 2015/11/11

static int tpd_local_init(void)
{

	if(i2c_add_driver(&tpd_i2c_driver)!=0)
	{
		TPD_DMESG("Error: unable to add i2c driver.\n");
		return -ENODEV;
	}
#ifdef TPD_HAVE_BUTTON     
	tpd_button_setting(TPD_KEY_COUNT, tpd_keys_local, tpd_keys_dim_local);// initialize tpd button data
#endif 
	boot_mode = get_boot_mode();
	if (boot_mode == 3) {
		boot_mode = NORMAL_BOOT;
	}  
	return 0;
}

static struct tpd_driver_t synaptics_rmi4_driver = {
	.tpd_device_name = "synaptics_tpd",
	.tpd_local_init = tpd_local_init,
	.suspend = synaptics_rmi4_suspend,
	.resume = synaptics_rmi4_resume,
#ifdef TPD_HAVE_BUTTON
	.tpd_have_button = 1,
#else
	.tpd_have_button = 0,
#endif		
};

//static struct i2c_board_info __initdata i2c_tpd={ I2C_BOARD_INFO("synaptics-tpd", (TPD_I2C_ADDR))};

 /**
 * synaptics_rmi4_init()
 *
 * Called by the kernel during do_initcalls (if built-in)
 * or when the driver is loaded (if a module).
 *
 * This function registers the driver to the I2C subsystem.
 *
 */
static int __init synaptics_rmi4_init(void)
{
//[SM20][zihweishen] Modify  to replace mt_eint_mask and mt_eint_registration begin 2015/11/11

	spin_lock_init(&syn_irq_flag_lock);
	printk("[SYNAPTICS]: DEVICE Touchscreen Driver rmi4 init\n");

	  tpd_get_dts_info();
//[SM20][zihweishen] Modify  to replace mt_eint_mask and mt_eint_registration end 2015/11/11
	if(tpd_driver_add(&synaptics_rmi4_driver) < 0){
		pr_err("Fail to add tpd driver\n");
		return -ENODEV;
	}

	return 0;
}

 /**
 * synaptics_rmi4_exit()
 *
 * Called by the kernel when the driver is unloaded.
 *
 * This funtion unregisters the driver from the I2C subsystem.
 *
 */
static void __exit synaptics_rmi4_exit(void)
{
	tpd_driver_remove(&synaptics_rmi4_driver);
	return;
}

module_init(synaptics_rmi4_init);
module_exit(synaptics_rmi4_exit);

MODULE_AUTHOR("Synaptics, Inc.");
MODULE_DESCRIPTION("Synaptics DSX I2C Touch Driver");
MODULE_LICENSE("GPL v2");
