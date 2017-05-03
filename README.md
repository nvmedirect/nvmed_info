# nvmed_info

A user-level tool for obtaining various information on the NVMe SSD.


## Introduction

__nvmed_info__ shows the following information for the specified NVMe SSD in a textual form. To use __nvmed_info__, you should first install the NVMeDirect framework. All the information is currently compliant to the NVM Express Specification 1.2.1.

- The result of the IDENTIFY CONTROLLER command (cf. NVMe Spec. Ch. 5.11)
- The result of the IDENTIFY NAMESPACE command (cf. NVMe Spec. Ch. 5.11)
- The status of PCI Express Registers (cf. NVMe Spec. Ch. 2)
- The status of Controller Registers (cf. NVMe Spec. Ch. 3)
- The result of GET FEATURES command (cf. NVMe Spec. Ch. 5.9)
- The result of GET LOG PAGE command (cf. NVMe Spec. Ch. 5.10)

## How to Install

1. Install the NVMeDirect framework referring to [this instruction](https://github.com/nvmedirect/nvmedirect)
You need the following files to build __nvmed_info__.
```shell
/usr/local/include/nvme_hdr.h
/usr/local/include/nvmed.h
/usr/local/include/lib_nvmed.h
/usr/local/lib/libnvmed.so
```

2. Build __nvmed_info__
```shell
$ make
```

3. Move __nvmed_info__ to the appropriate path
```shell
$ mv nvmed_info /usr/local/bin
```

## How to Run
- Usage:
```shell
$ sudo nvmed_info [dev] [command] [subcommand] [args]
```
- You should be a root to run __nvmed_info__
- __`dev`__: The target device you want to examine such as `/dev/nvme0n1`
- __`command`__: The following commands are available. Note that you can specify any number of characters that can distinguish between commands. The command shown in parenthesis denotes the default one when none was specified.
```shell
   [identify]:          for IDENTIFY command
   pci:                 for PCI Express and Controller registers
   features:            for GET FEATURES command
   logs:                for GET LOG PAGE command
   all:                 for all of the above
```
- __`subcommand`__: The available subcommands depend on the __`command`__. The following subcommands are available. The subcommand shown in parenthesis denotes the default one when none was specified. 
```shell
   identify 
       [controller]:    for IDENTIFY CONTROLLER command
       namespace:       for IDENTIFY NAMESPACE command
                        (The following [args] specifies the namespace ID.)
   pci		
       [nvme]:          for NVMe Controller registers
       config:          for PCIe Config registers
   features
       [get]:           for GET FEATURES command
   logs
       [get]:           for GET LOG PAGE command
```

## Examples:
- Shows all the information
```shell
$ sudo nvmed_info /dev/nvme0n1 all
```

- Shows the result of IDENTIFY CONTROLLER command
```shell
$ sudo nvmed_info /dev/nvme0n1 identify controller      # or
$ sudo nvmed_info /dev/nvme0n1 i c                      # or
$ sudo nvmed_info /dev/nvme0n1 i                        # or
$ sudo nvmed_info /dev/nvme0n1 
```

- Show the result of IDENTIFY NAMESPACE command
```shell
$ sudo nvmed_info /dev/nvme0n1 identify name            # or
$ sudo nvmed_info /dev/nvme0n1 i n                      # or
$ sudo nvmed_info /dev/nvme0n1 i n 1               
```

- Shows the NVMe Controller registers
```shell
$ sudo nvmed_info /dev/nvme0n1 pci                      # or
$ sudo nvmed_info /dev/nvme0n1 p n                      # or
$ sudo nvmed_info /dev/nvme0n1 p					
```

- Shows the PCIe Config registers
```shell
$ sudo nvmed_info /dev/nvme0n1 pci config               # or
$ sudo nvmed_info /dev/nvme0n1 p c 				
```

- Shows the result of GET FEATURES command
```shell
$ sudo nvmed_info /dev/nvme0n1 features                 # or
$ sudo nvmed_info /dev/nvme0n1 f
```

- Shows the result of GET LOG PAGE command
```shell
$ sudo nvmed_info /dev/nvme0n1 logs                     # or
$ sudo nvmed_info /dev/nvme0n1 l
```


## Disclaimer

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

For bugs or feature requests, you are kindly invited to [file an issue](https://github.com/nvmedirect/nvmed_info/issues). 


