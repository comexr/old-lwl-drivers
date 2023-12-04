/*!
 * Copyright (c) 2023 TUXEDO Computers GmbH <tux@tuxedocomputers.com>
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

#include "tuxedo_compatibility_check.h"

#include <linux/module.h>
#include <linux/dmi.h>
#include <asm/cpu_device_id.h>
#include <linux/mod_devicetable.h>
#include <asm/intel-family.h>

// Defines that might be missing in older kernel headers
#define INTEL_FAM6_SAPPHIRERAPIDS_X	0x8F
#define INTEL_FAM6_ALDERLAKE		0x97
#define INTEL_FAM6_ALDERLAKE_L		0x9A
#define INTEL_FAM6_ALDERLAKE_N		0xBE

// This check was not in place before 2023, so we continue to assume
// compatibility for devices <= Intel Core i 12th Gen and <= AMD Ryzen 5th Gen
static const struct x86_cpu_id skip_tuxedo_dmi_string_check_match[] __initconst = {
	X86_MATCH_INTEL_FAM6_MODEL(CORE_YONAH, NULL),
	X86_MATCH_INTEL_FAM6_MODEL(CORE2_MEROM, NULL),
	X86_MATCH_INTEL_FAM6_MODEL(CORE2_MEROM_L, NULL),
	X86_MATCH_INTEL_FAM6_MODEL(CORE2_PENRYN, NULL),
	X86_MATCH_INTEL_FAM6_MODEL(CORE2_DUNNINGTON, NULL),
	X86_MATCH_INTEL_FAM6_MODEL(NEHALEM, NULL),
	X86_MATCH_INTEL_FAM6_MODEL(NEHALEM_G, NULL),
	X86_MATCH_INTEL_FAM6_MODEL(NEHALEM_EP, NULL),
	X86_MATCH_INTEL_FAM6_MODEL(NEHALEM_EX, NULL),
	X86_MATCH_INTEL_FAM6_MODEL(WESTMERE, NULL),
	X86_MATCH_INTEL_FAM6_MODEL(WESTMERE_EP, NULL),
	X86_MATCH_INTEL_FAM6_MODEL(WESTMERE_EX, NULL),
	X86_MATCH_INTEL_FAM6_MODEL(SANDYBRIDGE, NULL),
	X86_MATCH_INTEL_FAM6_MODEL(SANDYBRIDGE_X, NULL),
	X86_MATCH_INTEL_FAM6_MODEL(IVYBRIDGE, NULL),
	X86_MATCH_INTEL_FAM6_MODEL(IVYBRIDGE_X, NULL),
	X86_MATCH_INTEL_FAM6_MODEL(HASWELL, NULL),
	X86_MATCH_INTEL_FAM6_MODEL(HASWELL_X, NULL),
	X86_MATCH_INTEL_FAM6_MODEL(HASWELL_L, NULL),
	X86_MATCH_INTEL_FAM6_MODEL(HASWELL_G, NULL),
	X86_MATCH_INTEL_FAM6_MODEL(BROADWELL, NULL),
	X86_MATCH_INTEL_FAM6_MODEL(BROADWELL_G, NULL),
	X86_MATCH_INTEL_FAM6_MODEL(BROADWELL_X, NULL),
	X86_MATCH_INTEL_FAM6_MODEL(BROADWELL_D, NULL),
	X86_MATCH_INTEL_FAM6_MODEL(SKYLAKE_L, NULL),
	X86_MATCH_INTEL_FAM6_MODEL(SKYLAKE, NULL),
	X86_MATCH_INTEL_FAM6_MODEL(SKYLAKE_X, NULL),
	X86_MATCH_INTEL_FAM6_MODEL(KABYLAKE_L, NULL),
	X86_MATCH_INTEL_FAM6_MODEL(KABYLAKE, NULL),
	X86_MATCH_INTEL_FAM6_MODEL(COMETLAKE, NULL),
	X86_MATCH_INTEL_FAM6_MODEL(COMETLAKE_L, NULL),
	X86_MATCH_INTEL_FAM6_MODEL(CANNONLAKE_L, NULL),
	X86_MATCH_INTEL_FAM6_MODEL(ICELAKE_X, NULL),
	X86_MATCH_INTEL_FAM6_MODEL(ICELAKE_D, NULL),
	X86_MATCH_INTEL_FAM6_MODEL(ICELAKE, NULL),
	X86_MATCH_INTEL_FAM6_MODEL(ICELAKE_L, NULL),
	X86_MATCH_INTEL_FAM6_MODEL(ICELAKE_NNPI, NULL),
	X86_MATCH_INTEL_FAM6_MODEL(LAKEFIELD, NULL),
	X86_MATCH_INTEL_FAM6_MODEL(ROCKETLAKE, NULL),
	X86_MATCH_INTEL_FAM6_MODEL(TIGERLAKE_L, NULL),
	X86_MATCH_INTEL_FAM6_MODEL(TIGERLAKE, NULL),
	X86_MATCH_INTEL_FAM6_MODEL(SAPPHIRERAPIDS_X, NULL),
	X86_MATCH_INTEL_FAM6_MODEL(ALDERLAKE, NULL),
	X86_MATCH_INTEL_FAM6_MODEL(ALDERLAKE_L, NULL),
	X86_MATCH_INTEL_FAM6_MODEL(ALDERLAKE_N, NULL),
	X86_MATCH_INTEL_FAM6_MODEL(ATOM_BONNELL, NULL),
	X86_MATCH_INTEL_FAM6_MODEL(ATOM_BONNELL_MID, NULL),
	X86_MATCH_INTEL_FAM6_MODEL(ATOM_SALTWELL, NULL),
	X86_MATCH_INTEL_FAM6_MODEL(ATOM_SALTWELL_MID, NULL),
	X86_MATCH_INTEL_FAM6_MODEL(ATOM_SALTWELL_TABLET, NULL),
	X86_MATCH_INTEL_FAM6_MODEL(ATOM_SILVERMONT, NULL),
	X86_MATCH_INTEL_FAM6_MODEL(ATOM_SILVERMONT_D, NULL),
	X86_MATCH_INTEL_FAM6_MODEL(ATOM_SILVERMONT_MID, NULL),
	X86_MATCH_INTEL_FAM6_MODEL(ATOM_AIRMONT, NULL),
	X86_MATCH_INTEL_FAM6_MODEL(ATOM_AIRMONT_MID, NULL),
	X86_MATCH_INTEL_FAM6_MODEL(ATOM_AIRMONT_NP, NULL),
	X86_MATCH_INTEL_FAM6_MODEL(ATOM_GOLDMONT, NULL),
	X86_MATCH_INTEL_FAM6_MODEL(ATOM_GOLDMONT_D, NULL),
	X86_MATCH_INTEL_FAM6_MODEL(ATOM_GOLDMONT_PLUS, NULL),
	X86_MATCH_INTEL_FAM6_MODEL(ATOM_TREMONT_D, NULL),
	X86_MATCH_INTEL_FAM6_MODEL(ATOM_TREMONT, NULL),
	X86_MATCH_INTEL_FAM6_MODEL(ATOM_TREMONT_L, NULL),
	X86_MATCH_INTEL_FAM6_MODEL(XEON_PHI_KNL, NULL),
	X86_MATCH_INTEL_FAM6_MODEL(XEON_PHI_KNM, NULL),
	X86_MATCH_VENDOR_FAM_MODEL(INTEL, 5, INTEL_FAM5_QUARK_X1000, NULL),
	X86_MATCH_VENDOR_FAM(AMD, 5, NULL),
	X86_MATCH_VENDOR_FAM(AMD, 6, NULL),
	X86_MATCH_VENDOR_FAM(AMD, 15, NULL),
	X86_MATCH_VENDOR_FAM(AMD, 16, NULL),
	X86_MATCH_VENDOR_FAM(AMD, 17, NULL),
	X86_MATCH_VENDOR_FAM(AMD, 18, NULL),
	X86_MATCH_VENDOR_FAM(AMD, 19, NULL),
	X86_MATCH_VENDOR_FAM(AMD, 20, NULL),
	X86_MATCH_VENDOR_FAM(AMD, 21, NULL),
	X86_MATCH_VENDOR_FAM(AMD, 22, NULL),
	X86_MATCH_VENDOR_FAM(AMD, 23, NULL),
	X86_MATCH_VENDOR_FAM(AMD, 24, NULL),
	X86_MATCH_VENDOR_FAM_MODEL(AMD, 25, 0x01, NULL),
	X86_MATCH_VENDOR_FAM_MODEL(AMD, 25, 0x08, NULL),
	X86_MATCH_VENDOR_FAM_MODEL(AMD, 25, 0x21, NULL),
	X86_MATCH_VENDOR_FAM_MODEL(AMD, 25, 0x40, NULL),
	X86_MATCH_VENDOR_FAM_MODEL(AMD, 25, 0x44, NULL),
	X86_MATCH_VENDOR_FAM_MODEL(AMD, 25, 0x50, NULL),
	{ }
};

// Placeholder for potential future exemptions
static const struct x86_cpu_id force_tuxedo_dmi_string_check_match[] __initconst = {
	{ }
};

// Going forward we only run the drivers on in house tested devices
static const struct dmi_system_id tuxedo_dmi_string_match[] __initconst = {
	{
		.matches = {
			DMI_MATCH(DMI_SYS_VENDOR, "TUXEDO"),
		},
	},
	{
		.matches = {
			DMI_MATCH(DMI_BOARD_VENDOR, "TUXEDO"),
		},
	},
	{
		.matches = {
			DMI_MATCH(DMI_CHASSIS_VENDOR, "TUXEDO"),
		},
	},
	{ }
};

bool tuxedo_is_compatible(void) {
	if (dmi_check_system(tuxedo_dmi_string_match)
	    || (x86_match_cpu(skip_tuxedo_dmi_string_check_match)
	    && !x86_match_cpu(force_tuxedo_dmi_string_check_match))) {
		return true;
	}
	return false;
}
EXPORT_SYMBOL(tuxedo_is_compatible);

MODULE_AUTHOR("TUXEDO Computers GmbH <tux@tuxedocomputers.com>");
MODULE_DESCRIPTION("Provide check for other modules if driver package is known compatible");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0.0");
