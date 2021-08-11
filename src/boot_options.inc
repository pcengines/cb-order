/* SPDX-License-Identifier: GPL-2.0-or-later */

/* X(keyword, description, shortcut, type) */

X("boosten", "Core Performance Boost", 'L', OPT_TYPE_BOOLEAN)
X("com2en", "Redirect console output to COM2", 'K', OPT_TYPE_BOOLEAN)
X("ehcien", "EHCI0 controller", 'H', OPT_TYPE_BOOLEAN)
X("iommu", "IOMMU", 'V', OPT_TYPE_BOOLEAN)
X("mpcie2_clk", "Force mPCIe2 slot CLK (GPP3 PCIe)", 'M', OPT_TYPE_BOOLEAN)
X("pciepm", "PCIe power management features", 'Y', OPT_TYPE_BOOLEAN)
X("pciereverse", "Reverse order of PCI addresses", 'G', OPT_TYPE_BOOLEAN)
X("pxen", "Network (PXE boot)", 'N', OPT_TYPE_BOOLEAN)
X("scon", "Serial console", 'T', OPT_TYPE_BOOLEAN)
X("sd3mode", "SD 3.0 mode", 'J', OPT_TYPE_BOOLEAN)
X("uartc", "UART C / GPIO[0..7]", 'O', OPT_TYPE_TOGGLE)
X("uartd", "UART D / GPIO[10..17]", 'P', OPT_TYPE_TOGGLE)
X("usben", "USB boot", 'U', OPT_TYPE_BOOLEAN)
X("watchdog", "Watchdog", 'I', OPT_TYPE_HEX4)

/* vim: set filetype=c : */
