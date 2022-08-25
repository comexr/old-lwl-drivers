/*!
 * Copyright (c) 2018-2020 TUXEDO Computers GmbH <tux@tuxedocomputers.com>
 *
 * This file is part of tuxedo-keyboard.
 *
 * tuxedo-keyboard is free software: you can redistribute it and/or modify
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

#ifndef CLEVO_LEDS_H
#define CLEVO_LEDS_H

#include <linux/types.h>
#include <linux/platform_device.h>

enum clevo_kb_backlight_types {
	CLEVO_KB_BACKLIGHT_TYPE_NONE = 0x00,
	CLEVO_KB_BACKLIGHT_TYPE_FIXED_COLOR = 0x01,
	CLEVO_KB_BACKLIGHT_TYPE_3_ZONE_RGB = 0x02,
	CLEVO_KB_BACKLIGHT_TYPE_1_ZONE_RGB = 0x06,
	CLEVO_KB_BACKLIGHT_TYPE_PER_KEY_RGB = 0xf3
};

int clevo_leds_init(struct platform_device *dev);
int clevo_leds_remove(struct platform_device *dev);
enum clevo_kb_backlight_types clevo_leds_get_backlight_type(void);
void clevo_leds_set_brightness_extern(u32 brightness);
void clevo_leds_set_color_extern(u32 color);

// TODO The following should go into a seperate .c file, but for this to work more reworking is required in the tuxedo_keyboard structure.

#include "clevo_leds.h"

#include "clevo_interfaces.h"

#include <linux/leds.h>
#include <linux/led-class-multicolor.h>

#define CLEVO_KBD_BRIGHTNESS_MIN	0x00
#define CLEVO_KBD_BRIGHTNESS_MAX	0xff
#define CLEVO_KBD_BRIGHTNESS_DEFAULT	(CLEVO_KBD_BRIGHTNESS_MAX * 0.5)

#define CLEVO_KB_COLOR_DEFAULT_RED	0xff
#define CLEVO_KB_COLOR_DEFAULT_GREEN	0xff
#define CLEVO_KB_COLOR_DEFAULT_BLUE	0xff
#define CLEVO_KB_COLOR_DEFAULT		((CLEVO_KB_COLOR_DEFAULT_RED << 16) + (CLEVO_KB_COLOR_DEFAULT_GREEN << 8) + CLEVO_KB_COLOR_DEFAULT_BLUE)

// The very first Clevos with keyboard backlight did have fixed color, but not yet the CLEVO_METHOD_ID_GET_SPECS. To
// not break these, we set this as default for the time being, better having an extra sysfs entry without function than
// a missing one. This is a temporary fix until we find a way to identify these early keyboard backlight devices.
static enum clevo_kb_backlight_types clevo_kb_backlight_type = CLEVO_KB_BACKLIGHT_TYPE_FIXED_COLOR;
static bool leds_initiaized;

static int clevo_evaluate_set_brightness(u8 brightness)
{
	pr_debug("Set brightness on %d\n", brightness);

	return clevo_evaluate_method (CLEVO_CMD_SET_KB_LEDS, 0xF4000000 | brightness, NULL);
}

static int clevo_evaluate_set_color(u32 zone, u32 color)
{
	u32 cset = ((color & 0x0000FF) << 16) | ((color & 0xFF0000) >> 8) | ((color & 0x00FF00) >> 8);
	u32 clevo_submethod_arg = zone | cset;

	pr_debug("Set Color 0x%08x for region 0x%08x\n", color, zone);

	return clevo_evaluate_method(CLEVO_CMD_SET_KB_LEDS, clevo_submethod_arg, NULL);
}

static void clevo_leds_set_brightness(struct led_classdev *led_cdev __always_unused, enum led_brightness brightness) {
	int ret = clevo_evaluate_set_brightness(brightness);
	if (ret) {
		pr_debug("clevo_leds_set_brightness(): clevo_evaluate_set_brightness() failed\n");
		return;
	}
	led_cdev->brightness = brightness;
}

static void clevo_leds_set_brightness_mc(struct led_classdev *led_cdev, enum led_brightness brightness) {
	int ret;
	u32 zone, color;
	struct led_classdev_mc *mcled_cdev = lcdev_to_mccdev(led_cdev);

	ret = clevo_evaluate_set_brightness(CLEVO_KBD_BRIGHTNESS_MAX);
	if (ret) {
		pr_debug("clevo_leds_set_brightness_mc(): clevo_evaluate_set_brightness() failed\n");
		return;
	}

	zone = mcled_cdev->subled_info[0].channel;

	led_mc_calc_color_components(mcled_cdev, brightness);
	color = (mcled_cdev->subled_info[0].brightness << 16) +
		(mcled_cdev->subled_info[1].brightness << 8) +
		mcled_cdev->subled_info[2].brightness;

	ret = clevo_evaluate_set_color(zone, color);
	if (ret) {
		pr_debug("clevo_leds_set_brightness_mc(): clevo_evaluate_set_color() failed\n");
		return;
	}
	led_cdev->brightness = brightness;
}

static struct led_classdev clevo_led_cdev = {
	.name = "white:" LED_FUNCTION_KBD_BACKLIGHT,
	.max_brightness = CLEVO_KBD_BRIGHTNESS_MAX,
	.brightness_set = &clevo_leds_set_brightness,
	.brightness = CLEVO_KBD_BRIGHTNESS_DEFAULT
};

static struct mc_subled clevo_mcled_cdevs_subleds[3][3] = {
	{
		{
			.color_index = LED_COLOR_ID_RED,
			.brightness = CLEVO_KBD_BRIGHTNESS_DEFAULT,
			.intensity = CLEVO_KB_COLOR_DEFAULT_RED,
			.channel = CLEVO_CMD_SET_KB_LEDS_SUB_RGB_ZONE_0
		},
		{
			.color_index = LED_COLOR_ID_GREEN,
			.brightness = CLEVO_KBD_BRIGHTNESS_DEFAULT,
			.intensity = CLEVO_KB_COLOR_DEFAULT_GREEN,
			.channel = CLEVO_CMD_SET_KB_LEDS_SUB_RGB_ZONE_0
		},
		{
			.color_index = LED_COLOR_ID_BLUE,
			.brightness = CLEVO_KBD_BRIGHTNESS_DEFAULT,
			.intensity = CLEVO_KB_COLOR_DEFAULT_BLUE,
			.channel = CLEVO_CMD_SET_KB_LEDS_SUB_RGB_ZONE_0
		}
	},
	{
		{
			.color_index = LED_COLOR_ID_RED,
			.brightness = CLEVO_KBD_BRIGHTNESS_DEFAULT,
			.intensity = CLEVO_KB_COLOR_DEFAULT_RED,
			.channel = CLEVO_CMD_SET_KB_LEDS_SUB_RGB_ZONE_1
		},
		{
			.color_index = LED_COLOR_ID_GREEN,
			.brightness = CLEVO_KBD_BRIGHTNESS_DEFAULT,
			.intensity = CLEVO_KB_COLOR_DEFAULT_GREEN,
			.channel = CLEVO_CMD_SET_KB_LEDS_SUB_RGB_ZONE_1
		},
		{
			.color_index = LED_COLOR_ID_BLUE,
			.brightness = CLEVO_KBD_BRIGHTNESS_DEFAULT,
			.intensity = CLEVO_KB_COLOR_DEFAULT_BLUE,
			.channel = CLEVO_CMD_SET_KB_LEDS_SUB_RGB_ZONE_1
		}
	},
	{
		{
			.color_index = LED_COLOR_ID_RED,
			.brightness = CLEVO_KBD_BRIGHTNESS_DEFAULT,
			.intensity = CLEVO_KB_COLOR_DEFAULT_RED,
			.channel = CLEVO_CMD_SET_KB_LEDS_SUB_RGB_ZONE_2
		},
		{
			.color_index = LED_COLOR_ID_GREEN,
			.brightness = CLEVO_KBD_BRIGHTNESS_DEFAULT,
			.intensity = CLEVO_KB_COLOR_DEFAULT_GREEN,
			.channel = CLEVO_CMD_SET_KB_LEDS_SUB_RGB_ZONE_2
		},
		{
			.color_index = LED_COLOR_ID_BLUE,
			.brightness = CLEVO_KBD_BRIGHTNESS_DEFAULT,
			.intensity = CLEVO_KB_COLOR_DEFAULT_BLUE,
			.channel = CLEVO_CMD_SET_KB_LEDS_SUB_RGB_ZONE_2
		}
	}
};

static struct led_classdev_mc clevo_mcled_cdevs[3] = {
	{
		.led_cdev.name = "rgb:" LED_FUNCTION_KBD_BACKLIGHT,
		.led_cdev.max_brightness = CLEVO_KBD_BRIGHTNESS_MAX,
		.led_cdev.brightness_set = &clevo_leds_set_brightness_mc,
		.led_cdev.brightness = CLEVO_KBD_BRIGHTNESS_DEFAULT,
		.num_colors = 3,
		.subled_info = clevo_mcled_cdevs_subleds[0]
	},
	{
		.led_cdev.name = "rgb:" LED_FUNCTION_KBD_BACKLIGHT,
		.led_cdev.max_brightness = CLEVO_KBD_BRIGHTNESS_MAX,
		.led_cdev.brightness_set = &clevo_leds_set_brightness_mc,
		.led_cdev.brightness = CLEVO_KBD_BRIGHTNESS_DEFAULT,
		.num_colors = 3,
		.subled_info = clevo_mcled_cdevs_subleds[1]
	},
	{
		.led_cdev.name = "rgb:" LED_FUNCTION_KBD_BACKLIGHT,
		.led_cdev.max_brightness = CLEVO_KBD_BRIGHTNESS_MAX,
		.led_cdev.brightness_set = &clevo_leds_set_brightness_mc,
		.led_cdev.brightness = CLEVO_KBD_BRIGHTNESS_DEFAULT,
		.num_colors = 3,
		.subled_info = clevo_mcled_cdevs_subleds[2]
	}
};

int clevo_leds_init(struct platform_device *dev)
{
	int ret;
	u32 status;
	union acpi_object *result;

	status = clevo_evaluate_method2(CLEVO_CMD_GET_SPECS, 0, &result);
	if (!status) {
		if (result->type == ACPI_TYPE_BUFFER) {
			pr_debug("CLEVO_CMD_GET_SPECS successful\n");
			         clevo_kb_backlight_type = result->buffer.pointer[0x0f];
		}
		else {
			pr_err("CLEVO_CMD_GET_SPECS return value has wrong type\n");
		}
		ACPI_FREE(result);
	}
	else {
		pr_err("CLEVO_CMD_GET_SPECS failed\n");
	}
	pr_debug("Keyboard backlight type: 0x%02x\n", clevo_kb_backlight_type);

	clevo_leds_set_brightness_extern(CLEVO_KBD_BRIGHTNESS_DEFAULT);
	clevo_leds_set_color_extern(CLEVO_KB_COLOR_DEFAULT);

	if (clevo_kb_backlight_type == CLEVO_KB_BACKLIGHT_TYPE_FIXED_COLOR) {
		pr_debug("Registering fixed color leds interface\n");
		ret = led_classdev_register(&dev->dev, &clevo_led_cdev);
		if(ret) {
			pr_err("Registering fixed color leds interface failed\n");
			return ret;
		}
	}
	else if (clevo_kb_backlight_type == CLEVO_KB_BACKLIGHT_TYPE_1_ZONE_RGB) {
		pr_debug("Registering single zone rgb leds interface\n");
		ret = devm_led_classdev_multicolor_register(&dev->dev, &clevo_mcled_cdevs[0]);
		if(ret) {
			pr_err("Registering single zone rgb leds interface failed\n");
			return ret;
		}
	}
	else if (clevo_kb_backlight_type == CLEVO_KB_BACKLIGHT_TYPE_3_ZONE_RGB) {
		pr_debug("Registering three zone rgb leds interface\n");
		ret = devm_led_classdev_multicolor_register(&dev->dev, &clevo_mcled_cdevs[0]);
		if(ret) {
			pr_err("Registering three zone rgb zone 0 leds interface failed\n");
			return ret;
		}
		ret = devm_led_classdev_multicolor_register(&dev->dev, &clevo_mcled_cdevs[1]);
		if(ret) {
			pr_err("Registering three zone rgb zone 1 leds interface failed\n");
			devm_led_classdev_multicolor_unregister(&dev->dev, &clevo_mcled_cdevs[0]);
			return ret;
		}
		ret = devm_led_classdev_multicolor_register(&dev->dev, &clevo_mcled_cdevs[2]);
		if(ret) {
			pr_err("Registering three zone rgb zone 2 leds interface failed\n");
			devm_led_classdev_multicolor_unregister(&dev->dev, &clevo_mcled_cdevs[0]);
			devm_led_classdev_multicolor_unregister(&dev->dev, &clevo_mcled_cdevs[1]);
			return ret;
		}
	}

	leds_initiaized = true;
	return 0;
}
EXPORT_SYMBOL(clevo_leds_init);

int clevo_leds_remove(struct platform_device *dev) {
	if (leds_initiaized) {
		if (clevo_kb_backlight_type == CLEVO_KB_BACKLIGHT_TYPE_FIXED_COLOR) {
			led_classdev_unregister(&clevo_led_cdev);
		}
		else if (clevo_kb_backlight_type == CLEVO_KB_BACKLIGHT_TYPE_1_ZONE_RGB) {
			devm_led_classdev_multicolor_unregister(&dev->dev, &clevo_mcled_cdevs[0]);
		}
		else if (clevo_kb_backlight_type == CLEVO_KB_BACKLIGHT_TYPE_3_ZONE_RGB) {
			devm_led_classdev_multicolor_unregister(&dev->dev, &clevo_mcled_cdevs[0]);
			devm_led_classdev_multicolor_unregister(&dev->dev, &clevo_mcled_cdevs[1]);
			devm_led_classdev_multicolor_unregister(&dev->dev, &clevo_mcled_cdevs[2]);
		}
	}

	return 0;
}
EXPORT_SYMBOL(clevo_leds_remove);

enum clevo_kb_backlight_types clevo_leds_get_backlight_type() {
	return clevo_kb_backlight_type;
}
EXPORT_SYMBOL(clevo_leds_get_backlight_type);

void clevo_leds_set_brightness_extern(u32 brightness) {
	if (clevo_kb_backlight_type == CLEVO_KB_BACKLIGHT_TYPE_FIXED_COLOR) {
		clevo_led_cdev.brightness_set(&clevo_led_cdev, brightness);
	}
	else if (clevo_kb_backlight_type == CLEVO_KB_BACKLIGHT_TYPE_1_ZONE_RGB) {
		clevo_mcled_cdevs[0].led_cdev.brightness_set(&clevo_mcled_cdevs[0].led_cdev, brightness);
	}
	else if (clevo_kb_backlight_type == CLEVO_KB_BACKLIGHT_TYPE_3_ZONE_RGB) {
		clevo_mcled_cdevs[0].led_cdev.brightness_set(&clevo_mcled_cdevs[0].led_cdev, brightness);
		clevo_mcled_cdevs[1].led_cdev.brightness_set(&clevo_mcled_cdevs[1].led_cdev, brightness);
		clevo_mcled_cdevs[2].led_cdev.brightness_set(&clevo_mcled_cdevs[2].led_cdev, brightness);
	}
}
EXPORT_SYMBOL(clevo_leds_set_brightness_extern);

void clevo_leds_set_color_extern(u32 color) {
	if (clevo_kb_backlight_type == CLEVO_KB_BACKLIGHT_TYPE_1_ZONE_RGB) {
		clevo_mcled_cdevs[0].subled_info[0].intensity = (color >> 16) & 0xff;
		clevo_mcled_cdevs[0].subled_info[1].intensity = (color >> 8) & 0xff;
		clevo_mcled_cdevs[0].subled_info[2].intensity = color & 0xff;
		clevo_mcled_cdevs[0].led_cdev.brightness_set(&clevo_mcled_cdevs[0].led_cdev, clevo_mcled_cdevs[0].led_cdev.brightness);
	}
	else if (clevo_kb_backlight_type == CLEVO_KB_BACKLIGHT_TYPE_3_ZONE_RGB) {
		clevo_mcled_cdevs[0].subled_info[0].intensity = (color >> 16) & 0xff;
		clevo_mcled_cdevs[0].subled_info[1].intensity = (color >> 8) & 0xff;
		clevo_mcled_cdevs[0].subled_info[2].intensity = color & 0xff;
		clevo_mcled_cdevs[0].led_cdev.brightness_set(&clevo_mcled_cdevs[0].led_cdev, clevo_mcled_cdevs[0].led_cdev.brightness);
		clevo_mcled_cdevs[1].subled_info[0].intensity = (color >> 16) & 0xff;
		clevo_mcled_cdevs[1].subled_info[1].intensity = (color >> 8) & 0xff;
		clevo_mcled_cdevs[1].subled_info[2].intensity = color & 0xff;
		clevo_mcled_cdevs[1].led_cdev.brightness_set(&clevo_mcled_cdevs[1].led_cdev, clevo_mcled_cdevs[1].led_cdev.brightness);
		clevo_mcled_cdevs[2].subled_info[0].intensity = (color >> 16) & 0xff;
		clevo_mcled_cdevs[2].subled_info[1].intensity = (color >> 8) & 0xff;
		clevo_mcled_cdevs[2].subled_info[2].intensity = color & 0xff;
		clevo_mcled_cdevs[2].led_cdev.brightness_set(&clevo_mcled_cdevs[2].led_cdev, clevo_mcled_cdevs[2].led_cdev.brightness);
	}
}
EXPORT_SYMBOL(clevo_leds_set_color_extern);

MODULE_LICENSE("GPL");

#endif // CLEVO_LEDS_H
