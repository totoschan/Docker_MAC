# Docker_MAC
Docker MAC based on kernel probe and SElinux

## Background
In the kprobe-based container access control scenario, the control module is loaded into the kernel in the form of a kernel module, and checkpoints are set for sensitive system calls to determine whether the process in the container complies with rules.
## Environment

```
OS: Centos 7.5、Ubuntu16.04
kernel version：3.10.11+、4.12.3、4.15.0
Docker
SELinux
```


## Compiling
Download the codes and 

```
make
```

## Usage and Example
- "container_name" is the target Docker container init process(no.1 process), container_shim is the shim process' pid, list_type means whether blacklist or whitelist you use

```
 insmod para.ko container_name=5771 containerd_shim=5755 list_type=0 openlist_type=0 dir_path="/home/chan/tmp/config/"
```
- install the update model to update the process list you want control. There are 8 parameters you can choose.


```
 insmod update_list.ko blacklist_insert="/usr/bin/ps" open_blacklist_insert="/test/hello:cat"
 
                blacklist_insert 
		whitelist_insert 
		blacklist_delete
		whitelist_delete
		open_blacklist_insert
		open_whitelist_insert
		open_blacklist_delete
		open_whitelist_delete
```
## Author
totoschan(Chen Zhi) @ICA/CAEP
