#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

MODULE_INFO(vermagic, VERMAGIC_STRING);

struct module __this_module
__attribute__((section(".gnu.linkonce.this_module"))) = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

static const struct modversion_info ____versions[]
__used
__attribute__((section("__versions"))) = {
	{ 0x28950ef1, __VMLINUX_SYMBOL_STR(module_layout) },
	{ 0x35b6b772, __VMLINUX_SYMBOL_STR(param_ops_charp) },
	{ 0x3418a135, __VMLINUX_SYMBOL_STR(print_openlist) },
	{ 0xc0718c6e, __VMLINUX_SYMBOL_STR(print_blacklist) },
	{ 0xdbda9d0, __VMLINUX_SYMBOL_STR(delete_open_whitelist) },
	{ 0x29f642f, __VMLINUX_SYMBOL_STR(delete_open_blacklist) },
	{ 0x9c166a26, __VMLINUX_SYMBOL_STR(insert_open_whitelist) },
	{ 0x9334a7d9, __VMLINUX_SYMBOL_STR(insert_open_blacklist) },
	{ 0x19236fc4, __VMLINUX_SYMBOL_STR(delete_whitelist) },
	{ 0x1601a23b, __VMLINUX_SYMBOL_STR(delete_blacklist) },
	{ 0xcbd3fafb, __VMLINUX_SYMBOL_STR(insert_whitelist) },
	{ 0xc4f13704, __VMLINUX_SYMBOL_STR(insert_blacklist) },
	{ 0x27e1a049, __VMLINUX_SYMBOL_STR(printk) },
	{ 0xe2d5255a, __VMLINUX_SYMBOL_STR(strcmp) },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=para";


MODULE_INFO(srcversion, "7F3D934070CE86A700A638F");
MODULE_INFO(rhelversion, "7.6");
#ifdef RETPOLINE
	MODULE_INFO(retpoline, "Y");
#endif
#ifdef CONFIG_MPROFILE_KERNEL
	MODULE_INFO(mprofile, "Y");
#endif
