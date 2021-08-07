// X(keyword, description, shortcut, type)

X("pxen", "Network/PXE boot", 'n', OPT_TYPE_BOOLEAN)
X("usben", "USB boot", 'u', OPT_TYPE_BOOLEAN)
X("scon", "Serial console", 't', OPT_TYPE_BOOLEAN)
X("com2en", "Redirect console output to COM2", 'k', OPT_TYPE_BOOLEAN)
X("uartc", "UART C / GPIO[0..7]", 'o', OPT_TYPE_TOGGLE)
X("uartd", "UART D / GPIO[10..17]", 'p', OPT_TYPE_TOGGLE)
X("mpcie2_clk", "Force mPCIe2 slot CLK (GPP3 PCIe)", 'm', OPT_TYPE_BOOLEAN)
X("ehcien", "EHCI0 controller", 'h', OPT_TYPE_BOOLEAN)
X("boosten", "Core Performance Boost", 'l', OPT_TYPE_BOOLEAN)
X("watchdog", "Watchdog", 'i', OPT_TYPE_HEX4)
X("sd3mode", "SD 3.0 mode", 'j', OPT_TYPE_BOOLEAN)
X("pciereverse", "Reverse order of PCI addresses", 'g', OPT_TYPE_BOOLEAN)
X("iommu", "IOMMU", 'v', OPT_TYPE_BOOLEAN)
X("pciepm", "PCIe power management features", 'y', OPT_TYPE_BOOLEAN)

/* vim: set filetype=c : */
