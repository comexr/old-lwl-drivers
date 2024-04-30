/*!
 * Copyright (c) 2024 TUXEDO Computers GmbH <tux@tuxedocomputers.com>
 *
 * This file is part of tuxedo-drivers.
 *
 * tuxedo-drivers is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software.  If not, see <https://www.gnu.org/licenses/>.
 */
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/dmi.h>
#include "tuxi_acpi.h"

#define FAN_SET_DUTY_MAX 255

struct driver_data_t {
	struct platform_device *pdev;
};

static ssize_t fan1_pwm_show(struct device *dev,
			     struct device_attribute *attr, char *buffer);

static ssize_t fan1_pwm_store(struct device *dev,
			      struct device_attribute *attr,
			      const char *buffer, size_t size);

static ssize_t fan1_pwm_enable_show(struct device *dev,
				    struct device_attribute *attr, char *buffer);

static ssize_t fan1_pwm_enable_store(struct device *dev,
				     struct device_attribute *attr,
				     const char *buffer, size_t size);

static ssize_t fan2_pwm_show(struct device *dev,
			     struct device_attribute *attr, char *buffer);

static ssize_t fan2_pwm_store(struct device *dev,
			      struct device_attribute *attr,
			      const char *buffer, size_t size);

static ssize_t fan2_pwm_enable_show(struct device *dev,
				    struct device_attribute *attr, char *buffer);

static ssize_t fan2_pwm_enable_store(struct device *dev,
				     struct device_attribute *attr,
				     const char *buffer, size_t size);

struct fan_control_attrs_t {
	struct device_attribute fan1_pwm;
	struct device_attribute fan1_pwm_enable;
	struct device_attribute fan2_pwm;
	struct device_attribute fan2_pwm_enable;
};

struct fan_control_attrs_t fan_control_attrs = {
	.fan1_pwm = __ATTR(fan1_pwm, 0644, fan1_pwm_show, fan1_pwm_store),
	.fan1_pwm_enable = __ATTR(fan1_pwm_enable, 0644, fan1_pwm_enable_show, fan1_pwm_enable_store),
	.fan2_pwm = __ATTR(fan2_pwm, 0644, fan2_pwm_show, fan2_pwm_store),
	.fan2_pwm_enable = __ATTR(fan2_pwm_enable, 0644, fan2_pwm_enable_show, fan2_pwm_enable_store),
};

static struct attribute *fan_control_attrs_list[] = {
	&fan_control_attrs.fan1_pwm.attr,
	&fan_control_attrs.fan1_pwm_enable.attr,
	&fan_control_attrs.fan2_pwm.attr,
	&fan_control_attrs.fan2_pwm_enable.attr,
	NULL
};

static struct attribute_group fan_control_attr_group = {
	.attrs = fan_control_attrs_list
};

static ssize_t fan1_pwm_show(struct device *dev,
			     struct device_attribute *attr, char *buffer)
{
	u8 pwm_data, duty_data;
	duty_data = tuxi_get_fan_speed(0, &duty_data);
	pwm_data = (duty_data * 0xff) / FAN_SET_DUTY_MAX;
	sysfs_emit(buffer, "%d\n", pwm_data);
	return strlen(buffer);
}

static ssize_t fan1_pwm_store(struct device *dev,
			      struct device_attribute *attr,
			      const char *buffer, size_t size)
{
	u8 pwm_data, duty_data;
	int err;

	if (kstrtou8(buffer, 0, &pwm_data))
		return -EINVAL;

	duty_data = (pwm_data * FAN_SET_DUTY_MAX) / 0xff;

	err = tuxi_set_fan_speed(0, duty_data);
	if (err)
		return err;

	return size;
}

static ssize_t fan1_pwm_enable_show(struct device *dev,
				    struct device_attribute *attr, char *buffer)
{
	enum tuxi_fan_mode mode;
	u8 enable_hwmon;
	int err;

	err = tuxi_get_fan_mode(&mode);
	if (mode == MANUAL) {
		enable_hwmon = 1;
	} else {
		enable_hwmon = 2;
	}
	sysfs_emit(buffer, "%d\n", enable_hwmon);

	return strlen(buffer);
}

static ssize_t fan1_pwm_enable_store(struct device *dev,
				     struct device_attribute *attr,
				     const char *buffer, size_t size)
{
	enum tuxi_fan_mode mode;
	u8 enable_hwmon;
	int err;

	if (kstrtou8(buffer, 0, &enable_hwmon))
		return -EINVAL;

	if (enable_hwmon == 1)
		mode = MANUAL;
	else
		mode = AUTO;

	err = tuxi_set_fan_mode(mode);
	if (err)
		return err;

	return size;
}

static ssize_t fan2_pwm_show(struct device *dev,
			     struct device_attribute *attr, char *buffer)
{
	u8 pwm_data, duty_data;
	duty_data = tuxi_get_fan_speed(1, &duty_data);
	pwm_data = (duty_data * 0xff) / FAN_SET_DUTY_MAX;
	sysfs_emit(buffer, "%d\n", pwm_data);
	return strlen(buffer);
}

static ssize_t fan2_pwm_store(struct device *dev,
			      struct device_attribute *attr,
			      const char *buffer, size_t size)
{
	u8 pwm_data, duty_data;
	int err;

	if (kstrtou8(buffer, 0, &pwm_data))
		return -EINVAL;

	duty_data = (pwm_data * FAN_SET_DUTY_MAX) / 0xff;

	err = tuxi_set_fan_speed(1, duty_data);
	if (err)
		return err;

	return size;
}

static ssize_t fan2_pwm_enable_show(struct device *dev,
				    struct device_attribute *attr, char *buffer)
{
	enum tuxi_fan_mode mode;
	u8 enable_hwmon;
	int err;

	err = tuxi_get_fan_mode(&mode);
	if (mode == MANUAL) {
		enable_hwmon = 1;
	} else {
		enable_hwmon = 2;
	}
	sysfs_emit(buffer, "%d\n", enable_hwmon);

	return strlen(buffer);
}

static ssize_t fan2_pwm_enable_store(struct device *dev,
				     struct device_attribute *attr,
				     const char *buffer, size_t size)
{
	enum tuxi_fan_mode mode;
	u8 enable_hwmon;
	int err;

	if (kstrtou8(buffer, 0, &enable_hwmon))
		return -EINVAL;

	if (enable_hwmon == 1)
		mode = MANUAL;
	else
		mode = AUTO;

	err = tuxi_set_fan_mode(mode);
	if (err)
		return err;

	return size;
}

static int __init tuxedo_tuxi_fan_control_probe(struct platform_device *pdev)
{
	int err;
	struct driver_data_t *driver_data = devm_kzalloc(&pdev->dev, sizeof(*driver_data), GFP_KERNEL);
	if (!driver_data)
		return -ENOMEM;

	pr_debug("driver probe\n");

	dev_set_drvdata(&pdev->dev, driver_data);

	driver_data->pdev = pdev;

	err = sysfs_create_group(&driver_data->pdev->dev.kobj, &fan_control_attr_group);
	if (err) {
		pr_err("create group failed\n");
		platform_device_unregister(driver_data->pdev);
		return err;
	}

	return 0;
}

static int tuxedo_tuxi_fan_control_remove(struct platform_device *pdev)
{
	pr_debug("driver remove\n");
	struct driver_data_t *driver_data = dev_get_drvdata(&pdev->dev);
	sysfs_remove_group(&driver_data->pdev->dev.kobj, &fan_control_attr_group);

	return 0;
}

static struct platform_device *tuxedo_tuxi_fan_control_device;
static struct platform_driver tuxedo_tuxi_fan_control_driver = {
	.driver.name = "tuxedo_fan_control",
	.remove = tuxedo_tuxi_fan_control_remove,
};

static int __init tuxedo_tuxi_fan_control_init(void)
{
	tuxedo_tuxi_fan_control_device =
		platform_create_bundle(&tuxedo_tuxi_fan_control_driver,
				       tuxedo_tuxi_fan_control_probe, NULL, 0, NULL, 0);

	if (IS_ERR(tuxedo_tuxi_fan_control_device))
		return PTR_ERR(tuxedo_tuxi_fan_control_device);

	return 0;
}

static void __exit tuxedo_tuxi_fan_control_exit(void)
{
	platform_device_unregister(tuxedo_tuxi_fan_control_device);
	platform_driver_unregister(&tuxedo_tuxi_fan_control_driver);
}

module_init(tuxedo_tuxi_fan_control_init);
module_exit(tuxedo_tuxi_fan_control_exit);

MODULE_AUTHOR("TUXEDO Computers GmbH <tux@tuxedocomputers.com>");
MODULE_DESCRIPTION("TUXEDO Computers TUXI fan control driver");
MODULE_LICENSE("GPL");
