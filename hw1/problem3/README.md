## HW 1 - Problem 3
- Run `make` which will generate the kernel modules for both `p3a` and `p1b`. 
- Run 
```bash
sudo insmod p3a.ko   	# loads the module
dmesg				# outputs kernel log buffer
sudo rmmod p3a		# removes module
```
-For `p3b` run first
```bash
sudo insmod p3a.ko mypid=PIDNUMBER	# To find a valid PID number you may execute pstree -p command
```
