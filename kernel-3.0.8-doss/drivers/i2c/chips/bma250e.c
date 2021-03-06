/* drivers/input/misc/bma250.c
 *
 * Bosh BMA 250. Digital, triaxial acceleration sensor.
 *
 * Copyright (C) 2010 Sony Ericsson Mobile Communications AB.
 * Copyright (C) 2012 Sony Mobile Communications AB.
 *
 * Author: Marcus Bauer <marcus.bauer@sonymobile.com>
 *         Tadashi Kubo <tadashi.kubo@sonymobile.com>
 *         Takashi Shiina <takashi.shiina@sonymobile.com>
 *
 * NOTE: This file has been created by Sony Ericsson Mobile Communications AB.
 *       This file has been modified by Sony Mobile Communications AB.
 *       This file contains code from: bma150.c
 *       The orginal bma150.c header is included below:
 *
 * Copyright (c) 2008-2009, Code Aurora Forum. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of Code Aurora Forum nor
 *       the names of its contributors may be used to endorse or promote
 *       products derived from this software without specific prior written
 *       permission.
 *
 * Alternatively, provided that this notice is retained in full, this software
 * may be relicensed by the recipient under the terms of the GNU General Public
 * License version 2 ("GPL") and only version 2, in which case the provisions of
 * the GPL apply INSTEAD OF those given above.  If the recipient relicenses the
 * software under the GPL, then the identification text in the MODULE_LICENSE
 * macro must be changed to reflect "GPLv2" instead of "Dual BSD/GPL".  Once a
 * recipient changes the license terms to the GPL, subsequent recipients shall
 * not relicense under alternate licensing terms, including the BSD or dual
 * BSD/GPL terms.  In addition, the following license statement immediately
 * below and between the words START and END shall also then apply when this
 * software is relicensed under the GPL:
 *
 * START
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License version 2 and only version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * END
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */
/*
 * Protocol driver for Bosch BMA250 accelerometer
 *
 */
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/ctype.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/irq.h>
#include <linux/platform_device.h>
#include <linux/spinlock.h>
#include <linux/interrupt.h>
#include <linux/fs.h>
#include <linux/spi/spi.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/uaccess.h>
#include <linux/debugfs.h>
#include <linux/earlysuspend.h>
#include <linux/linux_sensors.h>
#include <linux/gsensor.h>
#include <linux/miscdevice.h>
#include <linux/i2c/bma250e.h>
#include <linux/delay.h>

#define BMA250_VENDORID                  0x0001

//#define BMA250_SIMULATE_TEST

MODULE_LICENSE("GPL v2");
MODULE_ALIAS("bma250e");

/* bma250 register restrictions */
#define BMA250_LAST_REG 0x3F
#define BMA250_NA (1 << 8)
#define BMA250_RD (2 << 8)
#define BMA250_WR (4 << 8)
#define BMA250_RW (8 << 8)
static uint16_t bma250_mask[] = {
        0xFF | BMA250_RD, /* 00 */
        0x00 | BMA250_NA, /* 01 */
        0xC1 | BMA250_RD, /* 02 */
        0xFF | BMA250_RD, /* 03 */
        0xC1 | BMA250_RD, /* 04 */
        0xFF | BMA250_RD, /* 05 */
        0xC1 | BMA250_RD, /* 06 */
        0xFF | BMA250_RD, /* 07 */
        0x00 | BMA250_NA, /* 08 */
        0xFF | BMA250_RD, /* 09 */
        0xE0 | BMA250_RD, /* 0A */
        0xFF | BMA250_RD, /* 0B */
        0xFF | BMA250_RD, /* 0C */
        0x00 | BMA250_NA, /* 0D */
        0xFF | BMA250_RD, /* 0E */
        0x0F | BMA250_RW, /* 0F */
        0x1F | BMA250_RW, /* 10 */
        0xFE | BMA250_RW, /* 11 */
        0x60 | BMA250_RW, /* 12 */
        0xC0 | BMA250_RW, /* 13 */
        0xFF | BMA250_RW, /* 14 */
        0x00 | BMA250_NA, /* 15 */
        0xF7 | BMA250_RW, /* 16 */
        0x7F | BMA250_RW, /* 17 */
        0x0F | BMA250_RW, /* 18 */
        0xFF | BMA250_RW, /* 19 */
        0xD7 | BMA250_RW, /* 1A */
        0xFF | BMA250_RW, /* 1B */
        0x00 | BMA250_NA, /* 1C */
        0x00 | BMA250_NA, /* 1D */
        0x3F | BMA250_RW, /* 1E */
        0x00 | BMA250_NA, /* 1F */
        0x0F | BMA250_RW, /* 20 */
        0x8F | BMA250_RW, /* 21 */
        0xFF | BMA250_RW, /* 22 */
        0xFF | BMA250_RW, /* 23 */
        0xc7 | BMA250_RW, /* 24 */
        0xFF | BMA250_RW, /* 25 */
        0xFF | BMA250_RW, /* 26 */
        0xFF | BMA250_RW, /* 27 */
        0xFF | BMA250_RW, /* 28 */
        0xFF | BMA250_RW, /* 29 */
        0xC7 | BMA250_RW, /* 2A */
        0xDF | BMA250_RW, /* 2B */
        0x7F | BMA250_RW, /* 2C */
        0x7F | BMA250_RW, /* 2D */
        0x3F | BMA250_RW, /* 2E */
        0x37 | BMA250_RW, /* 2F */
        0x3f | BMA250_RW, /* 30 */
        0x00 | BMA250_NA, /* 31 */
        0x17 | BMA250_RW, /* 32 */
        0xFF | BMA250_RW, /* 33 */
        0x07 | BMA250_RW, /* 34 */
        0x00 | BMA250_NA, /* 35 */
        0xF7 | BMA250_RW, /* 36 */
        0x7F | BMA250_RW, /* 37 */
        0xFF | BMA250_RW, /* 38 */
        0xFF | BMA250_RW, /* 39 */
        0xFF | BMA250_RW, /* 3A */
        0xFF | BMA250_RW, /* 3B */
        0xFF | BMA250_RW, /* 3C */
        0x00 | BMA250_NA, /* 3D */
        0xC3 | BMA250_RW, /* 3E */
        0xFF | BMA250_RW, /* 3F */
};

/*
 * Data returned from accelerometer.
 * Temp is in units of 0.5 degrees C
 */
struct bma250_accel_data {
        short            accel_x;
        short            accel_y;
        short            accel_z;
};

struct driver_data {
        struct input_dev            *ip_dev;
        struct i2c_client           *ic_dev;
        unsigned char                shift;
        struct delayed_work          work_data;
        unsigned int                 rate;
        unsigned long                delay_jiffies;
        unsigned int                 range;
        unsigned int                 bw_sel;
        struct list_head             next_dd;
        struct dentry               *dbfs_root;
        struct dentry               *dbfs_regs;
        struct bma250_platform_data *pdata;
	atomic_t		     enabled;

	struct early_suspend early_suspend;
        struct miscdevice misc_device;
};


#define BMA_DEBUG

#ifdef BMA_DEBUG
static int verbose = 0;
#define dbg( fmt, args... )  do{ if( verbose ) printk( fmt, ##args); }while(0)
#else
#define dbg( fmt, args... )
#endif

static struct mutex               bma250_power_lock;
static struct mutex               bma250_dd_lock;
static struct list_head           dd_list;

static int bma250_power_down(struct driver_data *dd);
static int bma250_power_up(struct driver_data *dd);
static int bma250_config(struct driver_data *dd);

struct driver_data *bma250_ic_get_data(struct i2c_client *ic_dev)
{
        return i2c_get_clientdata(ic_dev);
}

void bma250_ic_set_data(struct i2c_client *ic_dev, struct driver_data *data)
{
        i2c_set_clientdata(ic_dev, data);
}

#define BMA250_I2C_RETRYS	5

#ifndef BMA250_SIMULATE_TEST
int bma250_ic_read(struct i2c_client *ic_dev, u8 reg, u8 *buf, int len)
{
        int rc;
	int i;

	for (i = 0; i < BMA250_I2C_RETRYS; i++) {
		rc = i2c_smbus_read_i2c_block_data(ic_dev, reg, len, buf);
		if (rc > 0)
			return 0;
	}

        return rc;
}

int bma250_ic_write(struct i2c_client *ic_dev, u8 reg, u8 val)
{
        int rc;
	int i;

        if (reg > BMA250_LAST_REG) {
                printk(KERN_ERR "%s: reg 0x%.02X, out of range.\n",
                        __func__, reg);
                return -EINVAL;
        }
        if (bma250_mask[reg] & BMA250_NA) {
                printk(KERN_ERR "%s: reg 0x%.02X, reserved.\n",
                        __func__, reg);
                return -EINVAL;
        }
        if (bma250_mask[reg] & BMA250_RD) {
                printk(KERN_ERR "%s: reg 0x%.02X, read only.\n",
                        __func__, reg);
                return -EINVAL;

        }
        if (val & ~bma250_mask[reg]) {
                printk(KERN_ERR "%s: reg 0x%.02X val 0x%.02X, out of bounds.\n",
                        __func__, reg, val);
                return -EINVAL;
        }

	for (i = 0; i < BMA250_I2C_RETRYS; i++) {
		rc = i2c_smbus_write_byte_data(ic_dev, reg, val);
		if (rc == 0)
			return 0;
	}

        return rc;
}
#else  /* BMA250_SIMULATE_TEST */
int bma250_ic_read(struct i2c_client *ic_dev, u8 reg, u8 *buf, int len) {
	*buf = BMA250_MODE_NOSLEEP;

	return 0;
}

int bma250_ic_write(struct i2c_client *ic_dev, u8 reg, u8 val) {
	return 0;
}
#endif	/* BMA250_SIMULATE_TEST */

/* 10bit to 16 bit will give a 6 bit offset, shift
   it down to represent 256 lsb/g for alla ranges */
static inline u8 bma250_range2shift(u8 range)
{
        switch (range) {
        case BMA250_RANGE_16G: /* 31.25mg/lsb * 32lsb ~= 1g  */
                return 3;
                break;
        case BMA250_RANGE_8G: /* 15.63mg/lsb * 64lsb ~= 1g */
                return 4;
                break;
        case BMA250_RANGE_4G: /* 7.81mg/lsb * 128lsb ~= 1g */
                return 5;
                break;
        default:
        case BMA250_RANGE_2G: /* 3.91mg/lsb * 256lsb ~= 1g */
                return 6;
                break;
        }
}

static inline u8 bma250_range_to_jz_range(u8 range) {
	switch (range) {
        case BMA250_RANGE_16G:
                return GSENSOR_16G;
                break;
        case BMA250_RANGE_8G:
                return GSENSOR_8G;
                break;
        case BMA250_RANGE_4G:
                return GSENSOR_4G;
                break;
        default:
        case BMA250_RANGE_2G:
                return GSENSOR_2G;
                break;
        }
}

static inline int bma250_bw_handler(struct driver_data *dd)
{
        if (dd->rate > 100)
                dd->bw_sel = BMA250_BW_7_81HZ;
        else if (dd->rate > 50)
                dd->bw_sel = BMA250_BW_15_63HZ;
        else
                dd->bw_sel = BMA250_BW_31_25HZ;

        return bma250_ic_write(dd->ic_dev, BMA250_BW_SEL_REG, dd->bw_sel);
}

static inline int bma250_range_handler(struct driver_data *dd)
{
        int rc = 0;
        u8 threshold, duration = 0;

        if (dd->range == BMA250_RANGE_16G)
                threshold = 2;
        else if (dd->range == BMA250_RANGE_8G)
                threshold = 3;
        else if (dd->range == BMA250_RANGE_4G)
                threshold = 4;
        else
                threshold = 5;

        rc = bma250_ic_write(dd->ic_dev, BMA250_RANGE_REG, dd->range);
        if (rc)
                goto range_error;

        /* threshold definition for the slope int, g-range dependant */
        rc = bma250_ic_write(dd->ic_dev, BMA250_SLOPE_THR, threshold);
        if (rc)
                goto range_error;

        /* number of samples (n + 1) to be evaluted for slope int */
        rc = bma250_ic_write(dd->ic_dev, BMA250_SLOPE_DUR, duration);
        if (rc)
                goto range_error;

        dd->shift = bma250_range2shift(dd->range);
        return rc;

range_error:
        dev_err(&dd->ip_dev->dev,
                "%s: device failed, error %d\n", __func__, rc);
        return rc;
}

static ssize_t bma250_rate_show(struct device *dev,
                                        struct device_attribute *attr,
                                        char *buf)
{
        struct driver_data *dd = dev_get_drvdata(dev);

        return snprintf(buf, PAGE_SIZE, "%d\n", dd->rate);
}

static ssize_t bma250_rate_store(struct device *dev,
                                        struct device_attribute *attr,
                                        const char *buf, size_t count)
{
        struct driver_data *dd = dev_get_drvdata(dev);
        int rc;
        unsigned long val;

        rc = strict_strtoul(buf, 10, &val);
        if (rc)
                return rc;

	if ((val >= 1) && (val <= 10000)) {
		dd->rate = (unsigned int)val;
		dd->delay_jiffies = msecs_to_jiffies(dd->rate);

		cancel_delayed_work(&dd->work_data);
                bma250_config(dd);

		return strnlen(buf, count);
	}
        return -EINVAL;
}

static ssize_t bma250_enable_show(struct device *dev,
                                        struct device_attribute *attr,
                                        char *buf)
{
        struct driver_data *dd = dev_get_drvdata(dev);

        return snprintf(buf, PAGE_SIZE, "%d\n", atomic_read(&dd->enabled));
}

static ssize_t bma250_enable_store(struct device *dev,
                                        struct device_attribute *attr,
                                        const char *buf, size_t count)
{
        struct driver_data *dd = dev_get_drvdata(dev);
        int rc;
        unsigned long val;

        rc = strict_strtoul(buf, 10, &val);
        if (rc)
                return rc;

	if( val <= 0 ){
		bma250_power_down(dd);
	}else{
		rc = bma250_power_up(dd);
		if (rc == 0)
			bma250_config(dd);
	}

        return count;
}

#ifdef BMA_DEBUG
static ssize_t bma250_verbose_show(struct device *dev,
                                        struct device_attribute *attr,
                                        char *buf)
{
        return snprintf(buf, PAGE_SIZE, "%d\n", verbose );
}

static ssize_t bma250_verbose_store(struct device *dev,
                                        struct device_attribute *attr,
                                        const char *buf, size_t count)
{
        int rc;
        unsigned long val;

        rc = strict_strtoul(buf, 10, &val);
        if (rc)
                return rc;

	if( val ==1 ){
		verbose = 1;
	}else if( val == 0 ){
		verbose = 0;
	}

        return count;
}
#endif
static struct device_attribute attributes[] = {
        __ATTR(enable,      S_IRUGO|S_IWUGO, bma250_enable_show, bma250_enable_store),
        __ATTR(delay,       S_IRUGO|S_IWUGO, bma250_rate_show, bma250_rate_store),
#ifdef BMA_DEBUG
        __ATTR(verbose,      S_IRUGO|S_IWUGO, bma250_verbose_show, bma250_verbose_store),
#endif
};

static int add_sysfs_interfaces(struct device *dev)
{
        int i;

        for (i = 0; i < ARRAY_SIZE(attributes); i++)
                if (device_create_file(dev, attributes + i))
                        goto undo;
        return 0;
undo:
        for (; i >= 0 ; i--)
                device_remove_file(dev, attributes + i);
        dev_err(dev, "%s: failed to create sysfs interface\n", __func__);
        return -ENODEV;
}

static void remove_sysfs_interfaces(struct device *dev)
{
        int i;

        for (i = 0; i < ARRAY_SIZE(attributes); i++)
                device_remove_file(dev, attributes + i);
}

#if defined(CONFIG_DEBUG_FS)
static int bma250_dbfs_open(struct inode *inode, struct file *fp)
{
        fp->private_data = inode->i_private;
        return 0;
}

static ssize_t bma250_dbfs_write(struct file *fp, const char __user *buf,
                                 size_t count, loff_t *f_pos)
{
        u8                          *p;
        u8                          *np;
        u8                          *mbuf;
        int                          rc;
        unsigned int                 val;
        u8                           reg;
        u8                           data;
        struct driver_data          *dd;

        /* format of write data is "A[A] D[D]" eg. "AA DD", "A D" etc
           where A is address in hex, D is data in hex.
           Multiple address/data pairs may be separated by spaces.
        */
        if (count < 3)
                return 0;

        dd = fp->private_data;

        mbuf = kzalloc(count, GFP_KERNEL);
        if (!mbuf) {
                rc = -ENOMEM;
                goto dbfs_write_exit;
        }

        if (copy_from_user(mbuf, buf, count)) {
                rc = -EFAULT;
                goto dbfs_write_exit_copy;
        }

        p = mbuf;

        while (isspace(*p))
                p++;
        do {
                val = simple_strtoul(p, (char **)&np, 16);
                if ((val > BMA250_LAST_REG) || (p == np)) {
                        rc = -EINVAL;
                        goto dbfs_write_exit_copy;
                }
                while (isspace(*np) && ((np - mbuf) < count))
                        np++;
                p = np;
                reg = (u8)val;

                val = simple_strtoul(p, (char **)&np, 16);
                if ((val > 0xFF)  || (p == np)) {
                        rc = -EINVAL;
                        goto dbfs_write_exit_copy;
                }
                while (isspace(*np) && ((np - mbuf) < count))
                        np++;
                p = np;
                data = (u8)val;

                rc = bma250_ic_write(dd->ic_dev, reg, data);

                /* update here to avoid checking g-range at each interrupt */
                if ((!rc) && (reg == BMA250_RANGE_REG))
                        dd->shift = bma250_range2shift(data);

        } while (!rc && (np - mbuf) < count);

        if (rc)
                goto dbfs_write_exit;
        kfree(mbuf);

        return count;

dbfs_write_exit_copy:
        kfree(mbuf);
dbfs_write_exit:
        return rc;
}

static ssize_t bma250_dbfs_read(struct file *fp, char __user *buf,
                                size_t count, loff_t *f_pos)
{
        u8                           rx;
        u8                           mbuf[8];
        int                          rc;
        int                          copy_size;
        struct driver_data          *dd;

        dd = fp->private_data;
        if ((int)*f_pos > BMA250_LAST_REG) {
                rc = 0;
                goto dbfs_read_exit;
        }

        rc = bma250_ic_read(dd->ic_dev, (u8)*f_pos, &rx, 1);
        if (rc)
                goto dbfs_read_exit;

        snprintf(mbuf, ARRAY_SIZE(mbuf), "%02x %02x\n", (u8)*f_pos, rx);
        copy_size = min(count, strlen(mbuf) + 1);
        if (copy_to_user(buf, mbuf, copy_size))
                return -EFAULT;
        (*f_pos)++;

        return copy_size;
dbfs_read_exit:
        return rc;
}

static const struct file_operations dbfs_fops = {
        .owner    = THIS_MODULE,
        .open     = bma250_dbfs_open,
        .read     = bma250_dbfs_read,
        .write    = bma250_dbfs_write,
};

static void __devinit bma250_create_dbfs_entry(struct driver_data *dd)
{
        char buf[16];

        snprintf(buf, sizeof(buf), BMA250_NAME);
        dd->dbfs_root = debugfs_create_dir(buf, NULL);
        if (dd->dbfs_root <= (struct dentry *)NULL) {
                dd->dbfs_root = NULL;
                goto dbfs_err_root;
        }

        dd->dbfs_regs = debugfs_create_file("registers",
                                           S_IRUGO | S_IWUGO,
                                           dd->dbfs_root, dd,
                                           &dbfs_fops);
        if (dd->dbfs_regs <= (struct dentry *)NULL) {
                dd->dbfs_regs = NULL;
                goto dbfs_err_regs;
        }
        return;

dbfs_err_regs:
        debugfs_remove(dd->dbfs_root);
dbfs_err_root:
        return;
}

static void bma250_remove_dbfs_entry(struct driver_data *dd)
{
        if (dd->dbfs_regs)
                debugfs_remove(dd->dbfs_regs);
        if (dd->dbfs_root)
                debugfs_remove(dd->dbfs_root);
}
#else
static void __devinit bma250_create_dbfs_entry(struct driver_data *dd) { }

static void __devexit bma250_remove_dbfs_entry(struct driver_data *dd) { }
#endif

static void bma250_hw_shutdown(struct driver_data *dd)
{
        dd->pdata->teardown(&dd->ic_dev->dev);
        dd->pdata->hw_config(0);
}

static int bma250_hw_setup(struct driver_data *dd)
{
        int rc = 0;

        dd->pdata->hw_config(1);
        rc = dd->pdata->setup(&dd->ic_dev->dev);
        if (rc)
                dd->pdata->hw_config(0);
        return rc;
}

static int bma250_power_down(struct driver_data *dd)
{
        int rc = 0;

	// printk("===>enter %s:%d\n", __func__, __LINE__);

        mutex_lock(&bma250_power_lock);

	if (atomic_read(&dd->enabled) == 0)
		goto out;

        rc = bma250_ic_write(dd->ic_dev, BMA250_MODE_CTRL_REG,
                                BMA250_MODE_SUSPEND);
        bma250_hw_shutdown(dd);

	atomic_set(&dd->enabled, 0);
	cancel_delayed_work(&dd->work_data);

out:
        mutex_unlock(&bma250_power_lock);

	// printk("===>enter %s:%d\n", __func__, __LINE__);

        return rc;
}

static int bma250_power_up(struct driver_data *dd)
{
        int rc = 0;
	int retry;

	// printk("===>enter %s:%d\n", __func__, __LINE__);

        mutex_lock(&bma250_power_lock);

	if (atomic_read(&dd->enabled))
		goto out;

        rc = bma250_hw_setup(dd);

        if (rc)
                goto hw_setup_error;
        rc = bma250_ic_write(dd->ic_dev, BMA250_RESET_REG, BMA250_RESET);
        if (rc)
                goto power_up_error;

	msleep(10);

	retry = 5;
	while( retry-- ){
		char rx;
		rc = bma250_ic_write(dd->ic_dev, BMA250_MODE_CTRL_REG,
				BMA250_MODE_NOSLEEP);

		if( !rc ){
			rx = 0;
			rc = bma250_ic_read(dd->ic_dev, BMA250_MODE_CTRL_REG, &rx, 1);
			if( !rc && rx == BMA250_MODE_NOSLEEP ){
				break;
			}else if( !rc && rx == BMA250_MODE_SUSPEND){
				printk( "powerup fail, retry...\n" );
			}
		}
		msleep(50);
	}

	if( !retry ){
                goto power_up_error;
	}

	atomic_set(&dd->enabled, 1);

out:
        mutex_unlock(&bma250_power_lock);

	// printk("===>enter %s:%d\n", __func__, __LINE__);

        return rc;

power_up_error:
        bma250_hw_shutdown(dd);
hw_setup_error:
        mutex_unlock(&bma250_power_lock);
        return rc;
}

static int __devinit bma250_hwid(struct driver_data *dd)
{
        int rc;
        u8 rx_buf[2];

        rc = bma250_ic_read(dd->ic_dev, BMA250_CHIP_ID_REG, rx_buf, 2);
        if (rc)
                goto config_exit;

	/* TODO: check config_id here if you want */

        printk(KERN_INFO "bma250: detected chip id %x, rev 0x%X\n", rx_buf[0], rx_buf[1]);

config_exit:
        return rc;
}

static int bma250_config(struct driver_data *dd)
{
        int rc;

	// printk("===>enter %s:%d\n", __func__, __LINE__);

        rc = bma250_bw_handler(dd);
        if (rc)
                goto config_error;

        rc = bma250_range_handler(dd);
        if (rc)
                goto config_error;

	// printk("===>enter %s:%d\n", __func__, __LINE__);

        schedule_delayed_work(&dd->work_data, dd->delay_jiffies);
        return rc;

config_error:
        bma250_power_down(dd);
        return rc;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void bma250_early_suspend(struct early_suspend *handler)
{
        struct driver_data *dd =
		container_of(handler, struct driver_data, early_suspend);

        if (dd->ip_dev->users) {
		bma250_power_down(dd);
	}

        dd->pdata->power_mode(0);
}

static void bma250_late_resume(struct early_suspend *handler)
{
	struct driver_data *dd =
		container_of(handler, struct driver_data, early_suspend);
        int rc = 0;

        dd->pdata->power_mode(1);

        if (dd->ip_dev->users) {
		rc = bma250_power_up(dd);
                if (rc == 0)
			bma250_config(dd);
        }
}
#endif /* CONFIG_HAS_EARLYSUSPEND */

#ifdef BMA250_SIMULATE_TEST
struct simu_gravity  {
	short x;
	short y;
	short z;
};

static struct simu_gravity simu_gs[] = {
	{ -17, 981, 4 },
	{ -148, 969, 40 },
	{ -415, 881, 111 },
	{ -673, 693, 168 },
	{ -890, 254, 324 },
	{ -789, -237, 532 },
	{ -123, -740, 632 },
	{ -836, -445, -255 },
	{ -850, 287, -396 },
	{ -614, -751, -142 },
	{ -250, -947, -44 },
	{ 783, -590, 14 },
	{ 896, -399, 0 },
	{ 947, -254, -17 },
	{ 980, 17, -17 },
	{ 892, 399, -78 },
	{ 673, 705, -107 },
	{ 484, 849, -77 },
	{ 164, 966, -47 },
	{ -295, 933, 68 },
	{ -635, 740, 101 },
	{ -827, 520, 87 },
	{ -922, 319, 97 },
	{ -977, 85, 0 },
	{ -947, -254, 17 },
	{ -803, -562, 14 },
	{ -519, -832, -27 },
	{ -204, -959, 11 },
	{ -185, -963, -29 },
	{ -192, -959, -70 },
	{ -75, -977, -41 },
	{ -24, -980, -25 },
	{ 8, -981, 15 },
};

static void simu_get_next_g(short *x, short *y, short *z) {
	static unsigned long last_get_time = 0;;
	static int last_idx = 0;

	if (last_get_time == 0)
		last_get_time = jiffies;

	if (time_after(jiffies, last_get_time + 5 * HZ)) {
		last_get_time = jiffies;
		last_idx ++;
		if (last_idx >= (sizeof(simu_gs) / sizeof(struct simu_gravity)))
			last_idx = 0;
	}

	*x = simu_gs[last_idx].x;
	*y = simu_gs[last_idx].y;
	*z = simu_gs[last_idx].z;

	printk("===>(%d, %d, %d)\n", *x, *y, *z);
}
#endif	/* BMA250_SIMULATE_TEST */

#ifdef CONFIG_SENSORS_ORI
extern void orientation_report_values(int x,int y,int z);
#endif

static inline int bma250_report_data(struct driver_data *dd)
{
        int                         rc = 0;
        u8                          rx_buf[7];
        struct bma250_accel_data    data;

	// printk("===>enter %s:%d\n", __func__, __LINE__);

#ifndef BMA250_SIMULATE_TEST
        rc = bma250_ic_read(dd->ic_dev, BMA250_X_AXIS_LSB_REG, rx_buf, 6);
        if (rc)
                goto report_error;

        /* 10bit signed to 16bit signed (aka. x64) */
        data.accel_x = ((rx_buf[1] << 8) | (rx_buf[0] & 0xc0));
        data.accel_y = ((rx_buf[3] << 8) | (rx_buf[2] & 0xc0));
        data.accel_z = ((rx_buf[5] << 8) | (rx_buf[4] & 0xc0));

        /* sensitivty 256lsb/g for all g-ranges */
        //data.accel_x = data.accel_x >> dd->shift;
        //data.accel_y = data.accel_y >> dd->shift;
        //data.accel_z = data.accel_z >> dd->shift;

	data.accel_x = (data.accel_x >> 6) * 4; /* 100 / 25 */
        data.accel_y = (data.accel_y >> 6) * 4;
        data.accel_z = (data.accel_z >> 6) * 4;

        data.accel_x = -data.accel_x;
	data.accel_z = -data.accel_z;
#else
	simu_get_next_g(&data.accel_x, &data.accel_y, &data.accel_z);
#endif

        //printk( "%s report: %d, %d, %d\n", __func__, data.accel_x, data.accel_y, data.accel_z);
        input_report_abs(dd->ip_dev, ABS_X, data.accel_x);
        input_report_abs(dd->ip_dev, ABS_Y, data.accel_y);
        input_report_abs(dd->ip_dev, ABS_Z, data.accel_z);
        input_sync(dd->ip_dev);

#ifdef CONFIG_SENSORS_ORI
	orientation_report_values(data.accel_x, data.accel_y, data.accel_z);
#endif

        return rc;

report_error:
        dev_err(&dd->ip_dev->dev,
                "%s: device failed, error %d\n", __func__, rc);
        dbg( "%s: device failed, error %d\n", __func__, rc);
        return rc;
}

static void bma250_work_f(struct work_struct *work)
{
        int                         rc;
        struct driver_data         *dd = container_of(work, struct driver_data,
                                                        work_data.work);

	// printk("===>enter %s:%d\n", __func__, __LINE__);

        mutex_lock(&bma250_power_lock);

        if (atomic_read(&dd->enabled)) {
                rc = bma250_report_data(dd);
                if (rc)
                        goto work_error;
                schedule_delayed_work(&dd->work_data, dd->delay_jiffies);
        }
work_error:
        mutex_unlock(&bma250_power_lock);
        return ;
}

#if 0
static int bma250_open(struct input_dev *dev)
{
        int                 rc = 0;
        struct driver_data *dd = input_get_drvdata(dev);

        rc = bma250_power_up(dd);
        if (rc)
                return rc;
        rc = bma250_config(dd);

        return rc;
}

static void bma250_release(struct input_dev *dev)
{
        struct driver_data *dd = input_get_drvdata(dev);

        bma250_power_down(dd);
}
#endif

static int bma250_misc_open(struct inode *inode, struct file *file) {
	int err;
	// printk("===>enter %s:%d\n", __func__, __LINE__);
	err = nonseekable_open(inode, file);
	if (err < 0)
		return err;

	return 0;
}

static struct linux_sensor_t hardware_data_bma250 = {
	"bma250e 3-axis Accelerometer",
	"ST sensor",
	SENSOR_TYPE_ACCELEROMETER,
	1, 1024, 1, 1, { }
};

long bma250_misc_ioctl(struct file *file, unsigned int cmd, unsigned long arg) {
	struct miscdevice *dev = file->private_data;
	struct driver_data *dd  = container_of(dev, struct driver_data, misc_device);
	void __user *argp = (void __user *) arg;
	int data;
	int rc;

	// printk("===>enter %s:%d cmd = %u\n", __func__, __LINE__, cmd);

	switch (cmd) {
	case SENSOR_IOCTL_GET_DELAY:
		data = (int) dd->rate;
		if (copy_to_user(argp, &data, sizeof(data)))
			return -EFAULT;
		break;
	case SENSOR_IOCTL_SET_DELAY:
		if (atomic_read(&dd->enabled)) {
			if (copy_from_user(&data, argp, sizeof(data)))
				return -EFAULT;

			// printk("===>enter %s:%d data = %d\n", __func__, __LINE__, data);

			data *= 10;  //for Sensor_new

			if (data < 1) data = 1;
			if (data > 10000) data = 10000;

			dd->rate = (unsigned int)data;
			dd->delay_jiffies = msecs_to_jiffies(dd->rate);
			cancel_delayed_work(&dd->work_data);

			bma250_config(dd);
		}
		break;
	case SENSOR_IOCTL_SET_ACTIVE:
		if (copy_from_user(&data, argp, sizeof(data)))
			return -EFAULT;

		// printk("===>enter %s:%d data = %d\n", __func__, __LINE__, data);

		if (data > 1)
			return -EINVAL;

		if (data) {
			rc = bma250_power_up(dd);
			if (rc)
				return rc;
			rc = bma250_config(dd);
		} else {
			rc = bma250_power_down(dd);
		}

		return rc;

	case SENSOR_IOCTL_GET_ACTIVE:
		data = atomic_read(&dd->enabled);
		if (copy_to_user(argp, &data, sizeof(data)))
			return -EINVAL;

		break;

	case SENSOR_IOCTL_GET_DATA:
		// printk("===>enter %s:%d\n", __func__, __LINE__);
		if (copy_to_user(argp, &hardware_data_bma250,
					sizeof(hardware_data_bma250)))
			return -EINVAL;
		break;

	case SENSOR_IOCTL_GET_DATA_MAXRANGE:
	{
		u8 range = bma250_range_to_jz_range(dd->range);
		if (copy_to_user(argp, &range, 1))
			return -EFAULT;

		break;

	}

	case SENSOR_IOCTL_WAKE:
		input_event(dd->ip_dev, EV_SYN, SYN_CONFIG, 0);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static const struct file_operations bma250_misc_fops = {
	.owner = THIS_MODULE,
	.open = bma250_misc_open,
	.unlocked_ioctl = bma250_misc_ioctl,
};

static int bma250_probe(struct i2c_client *ic_dev,
					  const struct i2c_device_id *id)
{
        struct driver_data *dd;
        int                 rc;
        struct bma250_platform_data *pdata = ic_dev->dev.platform_data;

	// printk("===>enter %s:%d\n", __func__, __LINE__);

        if (!pdata || !pdata->power_mode || !pdata->hw_config ||
                        !pdata->setup || !pdata->teardown)
                return -ENODEV;

        dd = kzalloc(sizeof(struct driver_data), GFP_KERNEL);
        if (!dd) {
                rc = -ENOMEM;
                goto probe_exit;
        }

        mutex_lock(&bma250_dd_lock);
        list_add_tail(&dd->next_dd, &dd_list);
        mutex_unlock(&bma250_dd_lock);
        dd->ic_dev = ic_dev;

        INIT_DELAYED_WORK(&dd->work_data, bma250_work_f);

        dd->pdata = pdata;

        /* initial configuration */
        dd->rate = pdata->rate;
        dd->delay_jiffies = msecs_to_jiffies(dd->rate);
        dd->bw_sel = pdata->reg->bw_sel;
        dd->range = pdata->reg->range;

	if( pdata->gpio_cfg )
		pdata->gpio_cfg(&ic_dev->dev);

	{
		struct regulator *vgsensor;

		vgsensor = regulator_get(NULL, "vcc_gsensor");
		if (!IS_ERR(vgsensor)) {
			regulator_enable(vgsensor);
		}
	}

        pdata->power_mode(1);
        rc = bma250_power_up(dd);

        if (rc)
                goto probe_err_cfg;
        rc = bma250_hwid(dd);

        bma250_power_down(dd);
        if (rc)
                goto probe_err_cfg;

	atomic_set(&dd->enabled,0);

        bma250_create_dbfs_entry(dd);
        bma250_ic_set_data(ic_dev, dd);

        dd->ip_dev = input_allocate_device();
        if (!dd->ip_dev) {
                rc = -ENOMEM;
                goto probe_err_reg;
        }
        input_set_drvdata(dd->ip_dev, dd);
#if 0
        dd->ip_dev->open       = bma250_open;
        dd->ip_dev->close      = bma250_release;
        dd->ip_dev->name       = BMA250_NAME;
#else
	dd->ip_dev->name       = "g_sensor";
#endif
        dd->ip_dev->id.vendor  = BMA250_VENDORID;
        dd->ip_dev->id.product = 1;
        dd->ip_dev->id.version = 1;
        __set_bit(EV_ABS,       dd->ip_dev->evbit);
        __set_bit(ABS_X,        dd->ip_dev->absbit);
        __set_bit(ABS_Y,        dd->ip_dev->absbit);
        __set_bit(ABS_Z,        dd->ip_dev->absbit);
        input_set_abs_params(dd->ip_dev, ABS_X, -4096, 4095, 0, 0);
        input_set_abs_params(dd->ip_dev, ABS_Y, -4096, 4095, 0, 0);
        input_set_abs_params(dd->ip_dev, ABS_Z, -4096, 4095, 0, 0);

        rc = input_register_device(dd->ip_dev);
        if (rc) {
                input_free_device(dd->ip_dev);
                goto probe_err_reg;
        }

        rc = add_sysfs_interfaces(&dd->ip_dev->dev);
        if (rc)
                goto probe_err_sysfs;

	dd->misc_device.minor = MISC_DYNAMIC_MINOR;
	dd->misc_device.name = "g_sensor";
	dd->misc_device.fops = &bma250_misc_fops;

	misc_register(&dd->misc_device);

#ifdef CONFIG_HAS_EARLYSUSPEND
	dd->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	dd->early_suspend.suspend = bma250_early_suspend;
	dd->early_suspend.resume = bma250_late_resume;
	register_early_suspend(&dd->early_suspend);
#endif

	// printk("===>enter %s:%d\n", __func__, __LINE__);

        return rc;

probe_err_sysfs:
        input_unregister_device(dd->ip_dev);
probe_err_reg:
        bma250_remove_dbfs_entry(dd);
        bma250_ic_set_data(ic_dev, NULL);
probe_err_cfg:
        mutex_lock(&bma250_dd_lock);
        list_del(&dd->next_dd);
        mutex_unlock(&bma250_dd_lock);
        kfree(dd);
probe_exit:

        return rc;
}

static int __devexit bma250_remove(struct i2c_client *ic_dev)
{
        struct driver_data *dd = bma250_ic_get_data(ic_dev);
        int                 rc;

	misc_deregister(&dd->misc_device);

        rc = bma250_power_down(dd);
        if (rc)
                dev_err(&dd->ip_dev->dev,
                        "%s: power down failed with error %d\n",
                        __func__, rc);
        dd->pdata->power_mode(0);
        remove_sysfs_interfaces(&dd->ip_dev->dev);
        input_unregister_device(dd->ip_dev);
        bma250_remove_dbfs_entry(dd);
        bma250_ic_set_data(ic_dev, NULL);
        mutex_lock(&bma250_dd_lock);
        list_del(&dd->next_dd);
        mutex_unlock(&bma250_dd_lock);
        kfree(dd);
        return 0;
}

static const struct i2c_device_id bma250_i2c_id[] = {
        {BMA250_NAME, 0},
        {}
};

static struct i2c_driver bma250_driver = {
        .driver = {
                .name  = BMA250_NAME,
                .owner = THIS_MODULE,
        },
        .probe         = bma250_probe,
        .remove        = __devexit_p(bma250_remove),
        .id_table      = bma250_i2c_id,
};


static int __init bma250_init(void)
{
        INIT_LIST_HEAD(&dd_list);
        mutex_init(&bma250_dd_lock);
        mutex_init(&bma250_power_lock);
        return i2c_add_driver(&bma250_driver);
}
module_init(bma250_init);

static void __exit bma250_exit(void)
{
        i2c_del_driver(&bma250_driver);
}
module_exit(bma250_exit);
