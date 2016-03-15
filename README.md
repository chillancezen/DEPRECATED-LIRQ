# LIRQ
a softirq-like irq mechanism special for general-purpose thread level 
----------

```sh
# when first loading the module you will get the following info
root@cute-meeeow:/mnt/projects/LIRQ# make;make install
root@cute-meeeow:/mnt/projects/LIRQ# dmesg  |tail
         [ 1005.721218] [x]cpu init worker:0
         [ 1005.721304] [x]cpu init worker:1
root@cute-meeeow:/mnt/projects/LIRQ# cat /proc/lirqs 
cpu  0   :       0
cpu  1   :       0

root@cute-meeeow:/mnt/projects/LIRQ/test# make ;make install 
root@cute-meeeow:/mnt/projects/LIRQ/test# dmesg |tail
[ 1005.721218] [x]cpu init worker:0
[ 1005.721304] [x]cpu init worker:1
[ 1293.229109] tasking:3
[ 1293.229114] tasking:2
[ 1293.229116] tasking:1
[ 1293.252208] kfree---
# from test  code we let the context schedule the task three times ,so we will have 3 output lines
#again ,we will check the proc file
root@cute-meeeow:/mnt/projects/LIRQ/test# cat /proc/lirqs 
cpu  0   :       1
cpu  1   :       0
#CPU 0 did this job.
```
this is all the thing about LIRQ,this is totally based on general kthread ,so compared with really softirqs ,less prioritized maybe.but in most cases ,it 's OK ,beacause it schedule the tasks in an atomic context since I disable CPU preemption.
for any questions please drop me a letter: chillancezen@gmail.com or jzheng.bjtu@hotmail.com
