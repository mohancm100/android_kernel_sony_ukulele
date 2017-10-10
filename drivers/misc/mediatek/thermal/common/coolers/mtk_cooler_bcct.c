#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/kobject.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <asm/uaccess.h>
#include <linux/err.h>
#include <linux/syscalls.h>
#include "mt-plat/mtk_thermal_monitor.h"
#include <charging.h>
#include <tmp_battery.h>
#include <linux/uidgid.h>

/* ************************************ */
/* Weak functions */
/* ************************************ */
int __attribute__ ((weak))
get_bat_charging_current_level(void)
{
	pr_err("E_WF: %s doesn't exist\n", __func__);
	return 500;
}

CHARGER_TYPE __attribute__ ((weak))
mt_get_charger_type(void)
{
	pr_err("E_WF: %s doesn't exist\n", __func__);
	return STANDARD_HOST;
}

int __attribute__ ((weak))
set_bat_charging_current_limit(int current_limit)
{
	pr_err("E_WF: %s doesn't exist\n", __func__);
	return 0;
}
/* ************************************ */

#define mtk_cooler_bcct_dprintk_always(fmt, args...) \
pr_debug("[thermal/cooler/bcct]" fmt, ##args)

#define mtk_cooler_bcct_dprintk(fmt, args...) \
do { \
	if (1 == cl_bcct_klog_on) \
		pr_debug("[thermal/cooler/bcct]" fmt, ##args); \
} while (0)

#define MAX_NUM_INSTANCE_MTK_COOLER_BCCT  3

#define MTK_CL_BCCT_GET_LIMIT(limit, state) \
{(limit) = (short) (((unsigned long) (state))>>16); }

#define MTK_CL_BCCT_SET_LIMIT(limit, state) \
{(state) = ((((unsigned long) (state))&0xFFFF) | ((short) limit<<16)); }

#define MTK_CL_BCCT_GET_CURR_STATE(curr_state, state) \
{(curr_state) = (((unsigned long) (state))&0xFFFF); }

#define MTK_CL_BCCT_SET_CURR_STATE(curr_state, state) \
do { \
	if (0 == (curr_state)) \
		state &= ~0x1; \
	else \
		state |= 0x1; \
} while (0)

static kuid_t uid = KUIDT_INIT(0);
static kgid_t gid = KGIDT_INIT(1000);

#define MIN(_a_, _b_) ((_a_) > (_b_) ? (_b_) : (_a_))
#define MAX(_a_, _b_) ((_a_) > (_b_) ? (_a_) : (_b_))

/* Charger Limiter
 * Charger Limiter provides API to limit charger IC input current and
 * battery charging current. It arbitrates the limitation from users and sets
 * limitation to charger driver via two API functions:
 *	set_chr_input_current_limit()
 *	set_bat_charging_current_limit()
 */
int chrlmt_chr_input_curr_limit = -1; /**< -1 is unlimit, unit is mA. */
int chrlmt_bat_chr_curr_limit = -1; /**< -1 is unlimit, unit is mA. */

struct chrlmt_handle {
	int chr_input_curr_limit;
	int bat_chr_curr_limit;
};

/* temp solution, use list instead */
#define CHR_LMT_MAX_USER_COUNT	(4)

static struct chrlmt_handle *chrlmt_registered_users[CHR_LMT_MAX_USER_COUNT] = { 0 };

static int chrlmt_register(struct chrlmt_handle *handle)
{
	int i;

	if (!handle)
		return -1;

	handle->chr_input_curr_limit = -1;
	handle->bat_chr_curr_limit = -1;

	/* find an empty entry */
	for (i = CHR_LMT_MAX_USER_COUNT; --i >= 0; )
		if (!chrlmt_registered_users[i]) {
			chrlmt_registered_users[i] = handle;
			return 0;
		}

	return -1;
}

static int chrlmt_unregister(struct chrlmt_handle *handle)
{
	return -1;
}

static int chrlmt_set_limit(struct chrlmt_handle *handle, int chr_input_curr_limit, int bat_char_curr_limit)
{
	int i;
	int min_char_input_curr_limit = 0xFFFFFF;
	int min_bat_char_curr_limit = 0xFFFFFF;

	if (!handle)
		return -1;

	handle->chr_input_curr_limit = chr_input_curr_limit;
	handle->bat_chr_curr_limit = bat_char_curr_limit;

	for (i = CHR_LMT_MAX_USER_COUNT; --i >= 0; )
		if (chrlmt_registered_users[i]) {
			if (chrlmt_registered_users[i]->chr_input_curr_limit > -1)
				min_char_input_curr_limit =
					MIN(chrlmt_registered_users[i]->chr_input_curr_limit
						, min_char_input_curr_limit);
			if (chrlmt_registered_users[i]->bat_chr_curr_limit > -1)
				min_bat_char_curr_limit =
					MIN(chrlmt_registered_users[i]->bat_chr_curr_limit
						, min_bat_char_curr_limit);
		}

	if (min_char_input_curr_limit == 0xFFFFFF)
		min_char_input_curr_limit = -1;
	if (min_bat_char_curr_limit == 0xFFFFFF)
		min_bat_char_curr_limit = -1;

	if ((min_char_input_curr_limit != chrlmt_chr_input_curr_limit) ||
		(min_bat_char_curr_limit != chrlmt_bat_chr_curr_limit)) {
		chrlmt_chr_input_curr_limit = min_char_input_curr_limit;
		chrlmt_bat_chr_curr_limit = min_bat_char_curr_limit;

#ifdef CONFIG_MTK_SWITCH_INPUT_OUTPUT_CURRENT_SUPPORT
		set_chr_input_current_limit(chrlmt_chr_input_curr_limit);
#endif
		set_bat_charging_current_limit(chrlmt_bat_chr_curr_limit);

		mtk_cooler_bcct_dprintk_always("%s %p %d %d\n", __func__
			, handle, chrlmt_chr_input_curr_limit, chrlmt_bat_chr_curr_limit);
	}

	return 0;
}

static int cl_bcct_klog_on;
static struct thermal_cooling_device *cl_bcct_dev[MAX_NUM_INSTANCE_MTK_COOLER_BCCT] = { 0 };
static unsigned long cl_bcct_state[MAX_NUM_INSTANCE_MTK_COOLER_BCCT] = { 0 };
static struct chrlmt_handle cl_bcct_chrlmt_handle;

static int cl_bcct_cur_limit = 65535;

static void mtk_cl_bcct_set_bcct_limit(void)
{
	/* TODO: optimize */
	int i = 0;
	int min_limit = 65535;

	for (; i < MAX_NUM_INSTANCE_MTK_COOLER_BCCT; i++) {
		unsigned long curr_state;

		MTK_CL_BCCT_GET_CURR_STATE(curr_state, cl_bcct_state[i]);
		if (1 == curr_state) {

			int limit;

			MTK_CL_BCCT_GET_LIMIT(limit, cl_bcct_state[i]);
			if ((min_limit > limit) && (limit > 0))
				min_limit = limit;
		}
	}

	if (min_limit != cl_bcct_cur_limit) {
		cl_bcct_cur_limit = min_limit;

		if (65535 <= cl_bcct_cur_limit) {
			chrlmt_set_limit(&cl_bcct_chrlmt_handle, -1, -1);
			mtk_cooler_bcct_dprintk("%s limit=-1\n", __func__);
		} else {
			chrlmt_set_limit(&cl_bcct_chrlmt_handle, -1, cl_bcct_cur_limit);
			mtk_cooler_bcct_dprintk("%s limit=%d\n", __func__
				, cl_bcct_cur_limit);
		}

		mtk_cooler_bcct_dprintk("%s real limit=%d\n", __func__
				, get_bat_charging_current_level() / 100);

	}
}

static int mtk_cl_bcct_get_max_state(struct thermal_cooling_device *cdev, unsigned long *state)
{
	*state = 1;
	mtk_cooler_bcct_dprintk("%s %s %lu\n", __func__, cdev->type, *state);
	return 0;
}

static int mtk_cl_bcct_get_cur_state(struct thermal_cooling_device *cdev, unsigned long *state)
{
	MTK_CL_BCCT_GET_CURR_STATE(*state, *((unsigned long *)cdev->devdata));
	mtk_cooler_bcct_dprintk("%s %s %lu\n", __func__, cdev->type, *state);
	mtk_cooler_bcct_dprintk("%s %s limit=%d\n", __func__, cdev->type,
				get_bat_charging_current_level() / 100);
	return 0;
}

static int mtk_cl_bcct_set_cur_state(struct thermal_cooling_device *cdev, unsigned long state)
{
	mtk_cooler_bcct_dprintk("%s %s %lu\n", __func__, cdev->type, state);
	MTK_CL_BCCT_SET_CURR_STATE(state, *((unsigned long *)cdev->devdata));
	mtk_cl_bcct_set_bcct_limit();
	mtk_cooler_bcct_dprintk("%s %s limit=%d\n", __func__, cdev->type,
				get_bat_charging_current_level() / 100);

	return 0;
}

/* bind fan callbacks to fan device */
static struct thermal_cooling_device_ops mtk_cl_bcct_ops = {
	.get_max_state = mtk_cl_bcct_get_max_state,
	.get_cur_state = mtk_cl_bcct_get_cur_state,
	.set_cur_state = mtk_cl_bcct_set_cur_state,
};

static int mtk_cooler_bcct_register_ltf(void)
{
	int i;

	mtk_cooler_bcct_dprintk("%s\n", __func__);

	chrlmt_register(&cl_bcct_chrlmt_handle);

	for (i = MAX_NUM_INSTANCE_MTK_COOLER_BCCT; i-- > 0;) {
		char temp[20] = { 0 };

		sprintf(temp, "mtk-cl-bcct%02d", i);
		/* put bcct state to cooler devdata */
		cl_bcct_dev[i] = mtk_thermal_cooling_device_register(temp, (void *)&cl_bcct_state[i],
								     &mtk_cl_bcct_ops);
	}

	return 0;
}

static void mtk_cooler_bcct_unregister_ltf(void)
{
	int i;

	mtk_cooler_bcct_dprintk("%s\n", __func__);

	for (i = MAX_NUM_INSTANCE_MTK_COOLER_BCCT; i-- > 0;) {
		if (cl_bcct_dev[i]) {
			mtk_thermal_cooling_device_unregister(cl_bcct_dev[i]);
			cl_bcct_dev[i] = NULL;
			cl_bcct_state[i] = 0;
		}
	}

	chrlmt_unregister(&cl_bcct_chrlmt_handle);
}

static struct thermal_cooling_device *cl_abcct_dev;
static unsigned long cl_abcct_state;
static struct chrlmt_handle abcct_chrlmt_handle;
static long abcct_prev_temp;
static long abcct_curr_temp;
static long abcct_target_temp = 48000;
static long abcct_kp = 1000;
static long abcct_ki = 3000;
static long abcct_kd = 10000;
static int abcct_max_bat_chr_curr_limit = 3000;
static int abcct_min_bat_chr_curr_limit = 200;
static int abcct_cur_bat_chr_curr_limit;
static long abcct_iterm;

static int mtk_cl_abcct_get_max_state(struct thermal_cooling_device *cdev, unsigned long *state)
{
	*state = 1;
	mtk_cooler_bcct_dprintk("%s %s %lu\n", __func__, cdev->type, *state);
	return 0;
}

static int mtk_cl_abcct_get_cur_state(struct thermal_cooling_device *cdev, unsigned long *state)
{
	*state = cl_abcct_state;
	mtk_cooler_bcct_dprintk("%s %s %lu\n", __func__, cdev->type, *state);
	return 0;
}

static int mtk_cl_abcct_set_cur_state(struct thermal_cooling_device *cdev, unsigned long state)
{
	cl_abcct_state = state;
	mtk_cooler_bcct_dprintk("%s %s %lu\n", __func__, cdev->type, state);
	return 0;
}

/* bind fan callbacks to fan device */
static struct thermal_cooling_device_ops mtk_cl_abcct_ops = {
	.get_max_state = mtk_cl_abcct_get_max_state,
	.get_cur_state = mtk_cl_abcct_get_cur_state,
	.set_cur_state = mtk_cl_abcct_set_cur_state,
};

static int mtk_cl_abcct_set_cur_temp(struct thermal_cooling_device *cdev, unsigned long temp)
{
	long delta, pterm, dterm;
	int limit;

	/* based on temp and state to do ATM */
	abcct_prev_temp = abcct_curr_temp;
	abcct_curr_temp = (long) temp;

	if (cl_abcct_state == 0) {
		abcct_iterm = 0;
		abcct_cur_bat_chr_curr_limit = abcct_max_bat_chr_curr_limit;
		chrlmt_set_limit(&abcct_chrlmt_handle, -1, -1);
		return 0;
	}

	pterm = abcct_target_temp - abcct_curr_temp;
	abcct_iterm += pterm;
	dterm = abcct_prev_temp - abcct_curr_temp;

	delta = pterm/abcct_kp + abcct_iterm/abcct_ki + dterm/abcct_kd;

	limit = abcct_cur_bat_chr_curr_limit + (int) delta;
	limit = (limit / 50) * 50; /* Align limit to 50mA to avoid redundant calls to chrlmt. */
	limit = MIN(abcct_max_bat_chr_curr_limit, limit);
	limit = MAX(abcct_min_bat_chr_curr_limit, limit);
	abcct_cur_bat_chr_curr_limit = limit;

	mtk_cooler_bcct_dprintk("%s %ld %ld %ld %ld %ld %d\n"
		, __func__, abcct_curr_temp, pterm, abcct_iterm, dterm, delta, limit);

	chrlmt_set_limit(&abcct_chrlmt_handle, -1, limit);

	return 0;
}

static struct thermal_cooling_device_ops_extra mtk_cl_abcct_ops_ext = {
	.set_cur_temp = mtk_cl_abcct_set_cur_temp
};

static int mtk_cooler_abcct_register_ltf(void)
{
	mtk_cooler_bcct_dprintk("%s\n", __func__);

	chrlmt_register(&abcct_chrlmt_handle);

	if (!cl_abcct_dev)
		cl_abcct_dev = mtk_thermal_cooling_device_register_wrapper_extra(
			"abcct", (void *)NULL, &mtk_cl_abcct_ops, &mtk_cl_abcct_ops_ext);

	return 0;
}

static void mtk_cooler_abcct_unregister_ltf(void)
{
	mtk_cooler_bcct_dprintk("%s\n", __func__);

	if (cl_abcct_dev) {
		mtk_thermal_cooling_device_unregister(cl_abcct_dev);
		cl_abcct_dev = NULL;
		cl_abcct_state = 0;
	}

	chrlmt_unregister(&abcct_chrlmt_handle);
}

static ssize_t _cl_bcct_write(struct file *filp, const char __user *buf, size_t len, loff_t *data)
{
	/* int ret = 0; */
	char tmp[128] = { 0 };
	int klog_on, limit0, limit1, limit2;

	len = (len < (128 - 1)) ? len : (128 - 1);
	/* write data to the buffer */
	if (copy_from_user(tmp, buf, len))
		return -EFAULT;

    /**
     * sscanf format <klog_on> <mtk-cl-bcct00 limit> <mtk-cl-bcct01 limit> ...
     * <klog_on> can only be 0 or 1
     * <mtk-cl-bcct00 limit> can only be positive integer or -1 to denote no limit
     */

	if (NULL == data) {
		mtk_cooler_bcct_dprintk("%s null data\n", __func__);
		return -EINVAL;
	}
	/* WARNING: Modify here if MTK_THERMAL_MONITOR_COOLER_MAX_EXTRA_CONDITIONS is changed to other than 3 */
#if (3 == MAX_NUM_INSTANCE_MTK_COOLER_BCCT)
	MTK_CL_BCCT_SET_LIMIT(-1, cl_bcct_state[0]);
	MTK_CL_BCCT_SET_LIMIT(-1, cl_bcct_state[1]);
	MTK_CL_BCCT_SET_LIMIT(-1, cl_bcct_state[2]);

	if (1 <= sscanf(tmp, "%d %d %d %d", &klog_on, &limit0, &limit1, &limit2)) {
		if (klog_on == 0 || klog_on == 1)
			cl_bcct_klog_on = klog_on;

		if (limit0 >= -1)
			MTK_CL_BCCT_SET_LIMIT(limit0, cl_bcct_state[0]);
		if (limit1 >= -1)
			MTK_CL_BCCT_SET_LIMIT(limit1, cl_bcct_state[1]);
		if (limit2 >= -1)
			MTK_CL_BCCT_SET_LIMIT(limit2, cl_bcct_state[2]);

		return len;
	}
#else
#error "Change correspondent part when changing MAX_NUM_INSTANCE_MTK_COOLER_BCCT!"
#endif
	mtk_cooler_bcct_dprintk("%s bad argument\n", __func__);
	return -EINVAL;
}

static int _cl_bcct_read(struct seq_file *m, void *v)
{
    /**
     * The format to print out:
     *  kernel_log <0 or 1>
     *  <mtk-cl-bcct<ID>> <bcc limit>
     *  ..
     */

	mtk_cooler_bcct_dprintk("%s\n", __func__);

	{
		int i = 0;

		seq_printf(m, "klog %d\n", cl_bcct_klog_on);
		seq_printf(m, "curr_limit %d\n", cl_bcct_cur_limit);

		for (; i < MAX_NUM_INSTANCE_MTK_COOLER_BCCT; i++) {
			int limit;
			unsigned int curr_state;

			MTK_CL_BCCT_GET_LIMIT(limit, cl_bcct_state[i]);
			MTK_CL_BCCT_GET_CURR_STATE(curr_state, cl_bcct_state[i]);

			seq_printf(m, "mtk-cl-bcct%02d %d mA, state %d\n", i, limit, curr_state);
		}
	}

	return 0;
}

static int _cl_bcct_open(struct inode *inode, struct file *file)
{
	return single_open(file, _cl_bcct_read, PDE_DATA(inode));
}

static const struct file_operations _cl_bcct_fops = {
	.owner = THIS_MODULE,
	.open = _cl_bcct_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.write = _cl_bcct_write,
	.release = single_release,
};

static ssize_t _cl_abcct_write(struct file *filp, const char __user *buf, size_t len, loff_t *data)
{
	/* int ret = 0; */
	char tmp[128] = { 0 };
	long _abcct_target_temp, _abcct_kp, _abcct_ki, _abcct_kd;
	int _max_cur, _min_cur;

	len = (len < (128 - 1)) ? len : (128 - 1);
	/* write data to the buffer */
	if (copy_from_user(tmp, buf, len))
		return -EFAULT;

	if (NULL == data) {
		mtk_cooler_bcct_dprintk("%s null data\n", __func__);
		return -EINVAL;
	}

	if (6 <= sscanf(tmp, "%ld %ld %ld %ld %d %d"
		, &_abcct_target_temp, &_abcct_kp, &_abcct_ki, &_abcct_kd
		, &_max_cur, &_min_cur)) {

		abcct_target_temp = _abcct_target_temp;
		abcct_kp = _abcct_kp;
		abcct_ki = _abcct_ki;
		abcct_kd = _abcct_kd;
		abcct_max_bat_chr_curr_limit = _max_cur;
		abcct_min_bat_chr_curr_limit = _min_cur;
		abcct_cur_bat_chr_curr_limit = abcct_max_bat_chr_curr_limit;
		abcct_iterm = 0;

		return len;
	}

	mtk_cooler_bcct_dprintk("%s bad argument\n", __func__);
	return -EINVAL;
}

static int _cl_abcct_read(struct seq_file *m, void *v)
{
	mtk_cooler_bcct_dprintk("%s\n", __func__);

	seq_printf(m, "abcct_target_temp %ld\n", abcct_target_temp);
	seq_printf(m, "abcct_kp %ld\n", abcct_kp);
	seq_printf(m, "abcct_ki %ld\n", abcct_ki);
	seq_printf(m, "abcct_kd %ld\n", abcct_kd);
	seq_printf(m, "abcct_max_bat_chr_curr_limit %d\n", abcct_max_bat_chr_curr_limit);
	seq_printf(m, "abcct_min_bat_chr_curr_limit %d\n", abcct_min_bat_chr_curr_limit);

	return 0;
}

static int _cl_abcct_open(struct inode *inode, struct file *file)
{
	return single_open(file, _cl_abcct_read, PDE_DATA(inode));
}

static const struct file_operations _cl_abcct_fops = {
	.owner = THIS_MODULE,
	.open = _cl_abcct_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.write = _cl_abcct_write,
	.release = single_release,
};

static int __init mtk_cooler_bcct_init(void)
{
	int err = 0;
	int i;

	for (i = MAX_NUM_INSTANCE_MTK_COOLER_BCCT; i-- > 0;) {
		cl_bcct_dev[i] = NULL;
		cl_bcct_state[i] = 0;
	}

	/* cl_bcct_dev = NULL; */

	mtk_cooler_bcct_dprintk("%s\n", __func__);

	err = mtk_cooler_bcct_register_ltf();
	if (err)
		goto err_unreg;

	err = mtk_cooler_abcct_register_ltf();
	if (err)
		goto err_unreg;

	/* create a proc file */
	{
		struct proc_dir_entry *entry = NULL;
		struct proc_dir_entry *dir_entry = NULL;

		dir_entry = mtk_thermal_get_proc_drv_therm_dir_entry();
		if (!dir_entry) {
			mtk_cooler_bcct_dprintk("[%s]: mkdir /proc/driver/thermal failed\n",
						__func__);
		}

		entry =
		    proc_create("clbcct", S_IRUGO | S_IWUSR | S_IWGRP, dir_entry, &_cl_bcct_fops);
		if (!entry)
			mtk_cooler_bcct_dprintk_always("%s clbcct creation failed\n", __func__);
		else
			proc_set_user(entry, uid, gid);

		entry =
		    proc_create("clabcct", S_IRUGO | S_IWUSR | S_IWGRP, dir_entry, &_cl_abcct_fops);
		if (!entry)
			mtk_cooler_bcct_dprintk_always("%s clabcct creation failed\n", __func__);
		else
			proc_set_user(entry, uid, gid);
	}
	return 0;

err_unreg:
	mtk_cooler_bcct_unregister_ltf();
	return err;
}

static void __exit mtk_cooler_bcct_exit(void)
{
	mtk_cooler_bcct_dprintk("%s\n", __func__);

	/* remove the proc file */
	remove_proc_entry("driver/thermal/clbcct", NULL);
	remove_proc_entry("driver/thermal/clabcct", NULL);

	mtk_cooler_bcct_unregister_ltf();
	mtk_cooler_abcct_unregister_ltf();
}
module_init(mtk_cooler_bcct_init);
module_exit(mtk_cooler_bcct_exit);
