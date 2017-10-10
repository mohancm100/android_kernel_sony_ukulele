#ifndef _CUST_BAT_H_
#define _CUST_BAT_H_

/* stop charging while in talking mode */
#define STOP_CHARGING_IN_TAKLING
// <<< 2016/02/02-youchihwang. Battery. The battery voltage configuration of the talking mode.
#define TALKING_RECHARGE_VOLTAGE 3950
// >>> 2016/02/02-youchihwang. Battery. The battery voltage configuration of the talking mode.
#define TALKING_SYNC_TIME		   60

/* Battery Temperature Protection */
#define MTK_TEMPERATURE_RECHARGE_SUPPORT
// <<< 2016/05/09-youchihwang. Battery. Setting the temperature of pop over temperature protection message.
#define HIGH_TEMPERATURE_WARNNING_MESSAGE_TEMPERATURE 64
// >>> 2016/05/09-youchihwang. Battery. Setting the temperature of pop over temperature protection message.
// <<< 2016/05/11-youchihwang. Battery. Setting the temperature of pop under temperature protection message.
// <<< 2016/05/23-youchihwang. Battery. Setting the temperature of pop under temperature protection message.
#define LOW_TEMPERATURE_WARNNING_MESSAGE_TEMPERATURE -21
// >>> 2016/05/23-youchihwang. Battery. Setting the temperature of pop under temperature protection message.
// >>> 2016/05/11-youchihwang. Battery. Setting the temperature of pop under temperature protection message.
// <<< 2016/05/23-youchihwang. Battery. Setting the temperature of pop under temperature protection message.
#define LOW_BATTERY_TEMPERATURE_SHUTDOWN_TEMPERATURE -23
// >>> 2016/05/23-youchihwang. Battery. Setting the temperature of pop under temperature protection message.
#define MAX_CHARGE_TEMPERATURE_MINUS_X_DEGREE	47
#define MIN_CHARGE_TEMPERATURE  0
#define MIN_CHARGE_TEMPERATURE_PLUS_X_DEGREE	6
#define ERR_CHARGE_TEMPERATURE  0xFF

/* Linear Charging Threshold */
#define V_PRE2CC_THRES			3400	/* mV */
// <<< 2016/02/02-youchihwang. Battery. The battery voltage configuration of the talking mode.
#define V_CC2TOPOFF_THRES		4000
// >>> 2016/02/02-youchihwang. Battery. The battery voltage configuration of the talking mode.
#define RECHARGING_VOLTAGE      4110
#define CHARGING_FULL_CURRENT    150	/* mA */

/* Charging Current Setting */
/* #define CONFIG_USB_IF */
#define USB_CHARGER_CURRENT_SUSPEND			0/* def CONFIG_USB_IF */
#define USB_CHARGER_CURRENT_UNCONFIGURED	CHARGE_CURRENT_70_00_MA/* 70mA */
#define USB_CHARGER_CURRENT_CONFIGURED		CHARGE_CURRENT_500_00_MA/* 500mA */

// <<< 2016/02/03-youchihwang. Battery. DMS06708494 [So][VBJ][ESTA]Dut happened low battery and shudown but DUT is always connected with PC during ESTA stability test.
//                             Increasing charger input current & battery charging current for ESTA tset.
// <<< 2016/03/06-youchihwang. Battery. Setting PC USB port charging parameters. Safety concern
#define    USB_CHARGER_CURRENT    CHARGE_CURRENT_500_00_MA    /* 500mA */
// >>> 2016/03/06-youchihwang. Battery. Setting PC USB port charging parameters. Safety concern
// >>> 2016/02/03-youchihwang. Battery. DMS06708494 [So][VBJ][ESTA]Dut happened low battery and shudown but DUT is always connected with PC during ESTA stability test.

// <<< 2016/03/06-youchihwang. Battery. Setting PC USB port charging parameters
#define USB_CHARGER_BATTERY_CHARGING_CURRENT    CHARGE_CURRENT_768_00_MA
// >>> 2016/03/06-youchihwang. Battery. Setting PC USB port charging parameters

/* #define AC_CHARGER_CURRENT					CHARGE_CURRENT_650_00_MA */
// <<< 2016/03/06-youchihwang. Battery. Setting ac charger charging parameters
#define AC_CHARGER_CURRENT					CHARGE_CURRENT_2112_00_MA
// <<< 2016/03/07-youchihwang. Battery. Setting ac charger charging parameters
#define AC_CHARGER_INPUT_CURRENT				CHARGE_CURRENT_1550_00_MA
// >>> 2016/03/07-youchihwang. Battery. Setting ac charger charging parameters
// >>> 2016/03/06-youchihwang. Battery. Setting ac charger charging parameters
#define NON_STD_AC_CHARGER_CURRENT			CHARGE_CURRENT_500_00_MA
#define CHARGING_HOST_CHARGER_CURRENT       CHARGE_CURRENT_650_00_MA
#define APPLE_0_5A_CHARGER_CURRENT          CHARGE_CURRENT_500_00_MA
#define APPLE_1_0A_CHARGER_CURRENT          CHARGE_CURRENT_650_00_MA
#define APPLE_2_1A_CHARGER_CURRENT          CHARGE_CURRENT_800_00_MA


/* Precise Tunning */
#define BATTERY_AVERAGE_DATA_NUMBER	3
#define BATTERY_AVERAGE_SIZE 30

/* charger error check */
/* #define BAT_LOW_TEMP_PROTECT_ENABLE         // stop charging if temp < MIN_CHARGE_TEMPERATURE */
#define V_CHARGER_ENABLE 0				/* 1:ON , 0:OFF	*/
#define V_CHARGER_MAX 6500				/* 6.5 V */
#define V_CHARGER_MIN 4400				/* 4.4 V	*/

/* Tracking TIME */
#define ONEHUNDRED_PERCENT_TRACKING_TIME	10	/* 10 second	*/
#define NPERCENT_TRACKING_TIME 20	/* 20 second	*/
#define SYNC_TO_REAL_TRACKING_TIME 60	/* 60 second	*/
// <<< 2016/03/22-youchihwang. Battery. Setting shutdown off voltage
#define V_0PERCENT_TRACKING							3450 //3450mV
// >>> 2016/03/22-youchihwang. Battery. Setting shutdown off voltage

/*#define CUST_SYSTEM_OFF_VOLTAGE 3300
#define SYSTEM_OFF_VOLTAGE CUST_SYSTEM_OFF_VOLTAGE*/

/* Battery Notify */
#define BATTERY_NOTIFY_CASE_0001_VCHARGER
#define BATTERY_NOTIFY_CASE_0002_VBATTEMP
/* #define BATTERY_NOTIFY_CASE_0003_ICHARGING */
// <<< 2016/02/25-youchihwang. (Vbat) over charging protection mmi warning message
#define BATTERY_NOTIFY_CASE_0004_VBAT
// >>> 2016/02/25-youchihwang. (Vbat) over charging protection mmi warning message
/* #define BATTERY_NOTIFY_CASE_0005_TOTAL_CHARGINGTIME */

/* High battery support */
#define HIGH_BATTERY_VOLTAGE_SUPPORT

/* JEITA parameter */
/*#define MTK_JEITA_STANDARD_SUPPORT*/
#define CUST_SOC_JEITA_SYNC_TIME 30
#define JEITA_RECHARGE_VOLTAGE  4110	/* for linear charging */
#ifdef HIGH_BATTERY_VOLTAGE_SUPPORT
// <<< 2016/02/22-youchihwang. Setting charging voltage according to JEITA
#define JEITA_TEMP_ABOVE_POS_60_CV_VOLTAGE		BATTERY_VOLT_04_208000_V
#define JEITA_TEMP_POS_45_TO_POS_60_CV_VOLTAGE		BATTERY_VOLT_04_208000_V
#define JEITA_TEMP_POS_10_TO_POS_45_CV_VOLTAGE		BATTERY_VOLT_04_304000_V
#define JEITA_TEMP_POS_0_TO_POS_10_CV_VOLTAGE		BATTERY_VOLT_04_208000_V
#define JEITA_TEMP_BELOW_POS_0_CV_VOLTAGE		BATTERY_VOLT_04_208000_V
#define JEITA_TEMP_NEG_10_TO_POS_0_CV_VOLTAGE		BATTERY_VOLT_04_200000_V
#define JEITA_TEMP_BELOW_NEG_10_CV_VOLTAGE		BATTERY_VOLT_04_200000_V
// >>> 2016/02/22-youchihwang. Setting charging voltage according to JEITA
#else
#define JEITA_TEMP_ABOVE_POS_60_CV_VOLTAGE		BATTERY_VOLT_04_100000_V
#define JEITA_TEMP_POS_45_TO_POS_60_CV_VOLTAGE	BATTERY_VOLT_04_100000_V
#define JEITA_TEMP_POS_10_TO_POS_45_CV_VOLTAGE	BATTERY_VOLT_04_200000_V
#define JEITA_TEMP_POS_0_TO_POS_10_CV_VOLTAGE	BATTERY_VOLT_04_100000_V
#define JEITA_TEMP_NEG_10_TO_POS_0_CV_VOLTAGE	BATTERY_VOLT_03_900000_V
#define JEITA_TEMP_BELOW_NEG_10_CV_VOLTAGE		BATTERY_VOLT_03_900000_V
#endif
/* For JEITA Linear Charging only */
#define JEITA_NEG_10_TO_POS_0_FULL_CURRENT  120/* mA */
#define JEITA_TEMP_POS_45_TO_POS_60_RECHARGE_VOLTAGE  4000
#define JEITA_TEMP_POS_10_TO_POS_45_RECHARGE_VOLTAGE  4100
#define JEITA_TEMP_POS_0_TO_POS_10_RECHARGE_VOLTAGE   4000
#define JEITA_TEMP_NEG_10_TO_POS_0_RECHARGE_VOLTAGE   3800
#define JEITA_TEMP_POS_45_TO_POS_60_CC2TOPOFF_THRESHOLD	4050
#define JEITA_TEMP_POS_10_TO_POS_45_CC2TOPOFF_THRESHOLD	4050
#define JEITA_TEMP_POS_0_TO_POS_10_CC2TOPOFF_THRESHOLD	4050
#define JEITA_TEMP_NEG_10_TO_POS_0_CC2TOPOFF_THRESHOLD	3850


/* For CV_E1_INTERNAL */
#define CV_E1_INTERNAL

/* Disable Battery check for HQA */
#ifdef CONFIG_MTK_DISABLE_POWER_ON_OFF_VOLTAGE_LIMITATION
#define CONFIG_DIS_CHECK_BATTERY
#endif

#ifdef CONFIG_MTK_FAN5405_SUPPORT
#define FAN5405_BUSNUM 1
#endif

/*VINDPM moved from cust_pe.h to de-relating from PE+*/
#define SWITCH_CHR_VINDPM_5V 0x13  /* 4.5V */
#define SWITCH_CHR_VINDPM_7V 0x25  /* 6.3V */
#define SWITCH_CHR_VINDPM_9V 0x37  /* 8.1V */
#define SWITCH_CHR_VINDPM_12V 0x54 /* 11.0 set this tp prevent adapters from failure and reset*/

/*Added switch chr OPTIONS for BQ25896 on Jade*/
/*switch charger input/output current separation; moved to Kconfig.driver*/
/*#define CONFIG_MTK_SWITCH_INPUT_OUTPUT_CURRENT_SUPPORT*/
/*Dynamic CV using BIF
* Note: CONFIG_MTK_BAT_BIF_SUPPORT=yes, otherwise default constant CV
*/
/*#define CONFIG_MTK_DYNAMIC_BAT_CV_SUPPORT*/
/*Define this macro for thermal team experiment:no current limitation*/
#define CONFIG_MTK_THERMAL_TEST_SUPPORT
/*enable to save charger in detection power by turning off Chr clock*/
#define CONFIG_MTK_CHRIND_CLK_PDN_SUPPORT

/*Added battery_common options for jade*/
/*enable this to change thread wakeup period to 10 secs to avoid suspend failure*/
#define CONFIG_MTK_I2C_CHR_SUPPORT

#define BATTERY_MODULE_INIT

#if defined(CONFIG_MTK_BQ24196_SUPPORT)\
	|| defined(CONFIG_MTK_BQ24296_SUPPORT)\
	|| defined(CONFIG_MTK_BQ24160_SUPPORT)\
	|| defined(CONFIG_MTK_BQ25896_SUPPORT)\
	|| defined(CONFIG_MTK_BQ24261_SUPPORT)
#define SWCHR_POWER_PATH
#endif

#if defined(CONFIG_MTK_FAN5402_SUPPORT) \
	|| defined(CONFIG_MTK_FAN5405_SUPPORT) \
	|| defined(CONFIG_MTK_BQ24158_SUPPORT) \
	|| defined(CONFIG_MTK_BQ24196_SUPPORT) \
	|| defined(CONFIG_MTK_BQ24296_SUPPORT) \
	|| defined(CONFIG_MTK_NCP1851_SUPPORT) \
	|| defined(CONFIG_MTK_NCP1854_SUPPORT) \
	|| defined(CONFIG_MTK_BQ24160_SUPPORT) \
	|| defined(CONFIG_MTK_BQ24157_SUPPORT) \
	|| defined(CONFIG_MTK_BQ24250_SUPPORT) \
	|| defined(CONFIG_MTK_BQ24261_SUPPORT)
#define EXTERNAL_SWCHR_SUPPORT
#endif

// <<< 2015/12/04-youchihwang, Disabling HW OVP PMIC ChargerIn HV detection for charger
#define DISABLE_HW_OVP_PMIC_ENABLES_CHARGERIN_HV_DETECTION
// >>> 2015/12/04-youchihwang, Disabling HW OVP PMIC ChargerIn HV detection for charger

#endif /* _CUST_BAT_H_ */