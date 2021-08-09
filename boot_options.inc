// X(id, keyword, description, shortcut, type)

X(PXEN, "pxen", "Network (PXE boot)", 'n', OPT_TYPE_BOOLEAN)
X(USBEN, "usben", "USB boot", 'u', OPT_TYPE_BOOLEAN)
X(SCON, "scon", "Serial console", 't', OPT_TYPE_BOOLEAN)
X(COM2EN, "com2en", "Redirect console output to COM2", 'k', OPT_TYPE_BOOLEAN)
X(UARTC, "uartc", "UART C / GPIO[0..7]", 'o', OPT_TYPE_TOGGLE)
X(UARTD, "uartd", "UART D / GPIO[10..17]", 'p', OPT_TYPE_TOGGLE)
X(MPCIE2_CLK, "mpcie2_clk", "Force mPCIe2 slot CLK (GPP3 PCIe)", 'm', OPT_TYPE_BOOLEAN)
X(EHCIEN, "ehcien", "EHCI0 controller", 'h', OPT_TYPE_BOOLEAN)
X(BOOSTEN, "boosten", "Core Performance Boost", 'l', OPT_TYPE_BOOLEAN)
X(WATCHDOG, "watchdog", "Watchdog", 'i', OPT_TYPE_HEX4)
X(SD3MODE, "sd3mode", "SD 3.0 mode", 'j', OPT_TYPE_BOOLEAN)
X(PCIEREVERSE, "pciereverse", "Reverse order of PCI addresses", 'g', OPT_TYPE_BOOLEAN)
X(IOMMU, "iommu", "IOMMU", 'v', OPT_TYPE_BOOLEAN)
X(PCIEPM, "pciepm", "PCIe power management features", 'y', OPT_TYPE_BOOLEAN)

/* vim: set filetype=c : */
