#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

MODULE_INFO(vermagic, VERMAGIC_STRING);

__visible struct module __this_module
__attribute__((section(".gnu.linkonce.this_module"))) = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

#ifdef RETPOLINE
MODULE_INFO(retpoline, "Y");
#endif

static const struct modversion_info ____versions[]
__used
__attribute__((section("__versions"))) = {
	{ 0x59253af, __VMLINUX_SYMBOL_STR(module_layout) },
	{ 0x3cb3492a, __VMLINUX_SYMBOL_STR(param_ops_bool) },
	{ 0x48c1af0a, __VMLINUX_SYMBOL_STR(param_ops_uint) },
	{ 0xfe990052, __VMLINUX_SYMBOL_STR(gpio_free) },
	{ 0xd92ef1f7, __VMLINUX_SYMBOL_STR(gpiod_unexport) },
	{ 0xf20dabd8, __VMLINUX_SYMBOL_STR(free_irq) },
	{ 0xed2b0099, __VMLINUX_SYMBOL_STR(kernel_kobj) },
	{ 0xd6b8e852, __VMLINUX_SYMBOL_STR(request_threaded_irq) },
	{ 0x5394f626, __VMLINUX_SYMBOL_STR(gpiod_to_irq) },
	{ 0xb2396b63, __VMLINUX_SYMBOL_STR(gpiod_get_raw_value) },
	{ 0x26508223, __VMLINUX_SYMBOL_STR(gpiod_export) },
	{ 0xb0b40e2d, __VMLINUX_SYMBOL_STR(gpiod_direction_input) },
	{ 0x47229b5c, __VMLINUX_SYMBOL_STR(gpio_request) },
	{ 0xb01bd3a8, __VMLINUX_SYMBOL_STR(kobject_put) },
	{ 0x5bc78cae, __VMLINUX_SYMBOL_STR(sysfs_create_group) },
	{ 0x83a5047d, __VMLINUX_SYMBOL_STR(kobject_create_and_add) },
	{ 0x27e1a049, __VMLINUX_SYMBOL_STR(printk) },
	{ 0xda3d0d70, __VMLINUX_SYMBOL_STR(gpiod_set_debounce) },
	{ 0xf1ce739e, __VMLINUX_SYMBOL_STR(gpio_to_desc) },
	{ 0x20c55ae0, __VMLINUX_SYMBOL_STR(sscanf) },
	{ 0x211f68f1, __VMLINUX_SYMBOL_STR(getnstimeofday64) },
	{ 0x91715312, __VMLINUX_SYMBOL_STR(sprintf) },
	{ 0x1fdc7df2, __VMLINUX_SYMBOL_STR(_mcount) },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";


MODULE_INFO(srcversion, "72072ABAB7E95CBE2D9FED0");
