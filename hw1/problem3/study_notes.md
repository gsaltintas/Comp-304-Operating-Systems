## Kernel Code
Sample kernel code can be found [here](example/simple.c). 
- List all kernel modules that are currently loaded by `lsmod` 
    - 3 columns: name, size, where the module is being used
- Module entry point function must return an integer: *0: success*, otherwise *failure*.
- Module entry and exit points don't have any parameteres.
- Macros to register module entry and exit points:
```c
module_init()
module_exit()
```
- Call `make` to compile the module
- Call `clean` to clean generated files

## Loading and Removing Kernel
- Kernel modules are loaded with `insmod` command: 
```bash
sudo insmod simple.ko
``` 
- Check if the kernel module has loaded with `lsmod | grep simple ` command
- Check contents of kernel module's messages with (You should see the message "Loading Module.")
```bash
dmesg | grep simple
```
- To remove the module, invoke
```bash
sudo rmmod simple   # .ko suffix is unnecessary
``` 
You should see the message "Removing Module"
- Check with `lsmod` and `dmesg` whether the module has been removed.
- Periodically clean kernel log buffer with
```bash
sudo dmesg -c
```
