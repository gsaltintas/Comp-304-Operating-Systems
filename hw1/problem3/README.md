## HW 1 - Problem 3
- Run `make` which will generate the kernel modules for both `problem1_a` and `problem1_b`. 
- Run 
```bash
sudo insmod problem1_a.ko   	# loads the module
dmesg				# outputs kernel log buffer
sudo rmmod problem1_a		# removes module
```
-For `problem1_b` run first
```bash
sudo insmod problem1_a.ko mypid=PIDNUMBER	# To find a valid PID number you may execute pstree -p command
```
