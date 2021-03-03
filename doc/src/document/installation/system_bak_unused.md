在安装 SequoiaDB 产品之前，请确保您选择的系统满足必须的操作系统，硬件，通信，磁盘和内存的要求。sdbpreck命令将检查系统是否满足安全先决条件。

##硬件要求##

+--------+----------------------------------------------------------------------+-----------------------------------------------------------------------+
| 需求项 | 要求                                                                 | 建议                                                                  |
+========+======================================================================+=======================================================================+
| CPU    |                                                                      | 建议采用 X64（64 位 AMD64 和 Intel EM64T 处理器 ）或者 PowerPC 处理器 |
|        |  -   x86（Intel Pentium、Intel Xeon 和 AMD）32位 Intel 和 AMD 处理器 |                                                                       |
|        |  -   x64（64位 AMD64 和 Intel EM64T 处理器）                         |                                                                       |
|        |  -   PowerPC 7 或者 PowerPC 7+ 处理器                                |                                                                       |
+--------+----------------------------------------------------------------------+-----------------------------------------------------------------------+
| 磁盘   | 至少 10GB 空间                                                       | 建议大于 100GB 磁盘空间                                               |
+--------+----------------------------------------------------------------------+-----------------------------------------------------------------------+
| 内存   | 至少 1GB                                                             | 大于 2GB 物理内存                                                     |
+--------+----------------------------------------------------------------------+-----------------------------------------------------------------------+
| 网卡   | 配备至少 1 张网卡                                                    | 建议至少配置 1GE 网卡                                                 |
+--------+----------------------------------------------------------------------+-----------------------------------------------------------------------+

##受支持的操作系统##

+----------------+------------------------------------------------------------+
| 系统类型       | 系统列表                                                   |
+================+============================================================+
| Linux          |                                                            |
|                | 	-   Red Hat Enterprise Linux (RHEL) 6                     |
|                | 	-   SUSE Linux Enterprise Server (SLES) 11 Service Pack 1 |
|                | 	-   SUSE Linux Enterprise Server (SLES) 11 Service Pack 2 |
|                | 	-   SUSE Linux Enterprise Server (SLES) 12 Service Pack 1 |
|                | 	-   Ubuntu 12                                             |
|                | 	-   CentOS 6                                              |
+----------------+------------------------------------------------------------+
| Power PC Linux |                                                            |
|                |  -   Red Hat Enterprise Linux (RHEL) 6                     |
|                |  -   SUSE Linux Enterprise Server (SLES)11 Service Pack 1  |
|                |  -   SUSE Linux Enterprise Server (SLES)11 Service Pack 2  |
+----------------+------------------------------------------------------------+

##软件要求##

###Linux 系统要求###

+--------------------------------+-------------------------------------------------------------+----------------------------------------------------+
| 配置项                         | 配置方法                                                    | 验证方法                                           |
+================================+=============================================================+====================================================+
| 配置主机名                     | 1.使用 root 权限登陆，执行 hostname sdbserver1              | 执行 hostname 命令，确认打印信息是否为“sdbserver1” |
|                                |（sdbserver1为主机名称，可根据需要修改。）；                 |                                                    |
+--------------------------------+-------------------------------------------------------------+----------------------------------------------------+
|                                | -   对于 SUSE：                                             |                                                    |
|                                |      2.  打开 /etc/HOSTNAME 文件；<br>                      |                                                    |
|                                |          vi /etc/HOSTNAME<br>                               |                                                    |
|                                |      3.  修改文件内容，配置为主机名称；<br>                 |                                                    |
|                                |          sdbserver1 （主机名称）<br>                        |                                                    |
|                                |      4.  按 : wq 保存退出；                                 |                                                    |
|                                | -   对于 RedHat：                                           |                                                    |
|                                |      2.  打开 /etc/sysconfig/network 文件；<br>             |                                                    |
|                                |          vi /etc/sysconfig/network <br>                     |                                                    |
|                                |      3.  将 HOSTNAME 一行修改为 HOSTNAME = sdbserver1，其中 |                                                    |
|                                |          sdbserver1 为新主机名；                            |                                                    |
|                                |      4.  按 : wq 保存退出；                                 |                                                    |
|                                | -   对于 Ubuntu：                                           |                                                    |
|                                |      2.  打开 /etc/hostname 文件；<br>                      |                                                    |
|                                |          vi /etc/hostname<br>                               |                                                    |
|                                |      3.  修改文件内容，配置为主机名称；<br>                 |                                                    |
|                                |          sdbserver1<br>                                     |                                                    |
|                                |      4.  按 : wq 保存退出；                                 |                                                    |
+--------------------------------+-------------------------------------------------------------+----------------------------------------------------+
| 配置物理机之间通过主机名可连接 |                                                             |                                                    |
|                                | -   使用 root 权限，打开 /etc/hosts 文件<br>                | 1.ping sdbserver1（本机主机名）可以 ping 通      |
|                                |     vi /etc/hosts<br>                                       | 2.ping sdbserver2（远端主机名）可以 ping 通      |
|                                | -   修改 /etc/hosts，将服务器节点的主机名与IP映射关系配置到 |                                                    |
|                                |     该文件中<br>                                            |                                                    |
|                                |     192.168.20.200 sdbserver1<br>                           |                                                    |
|                                |     192.168.20.201 sdbserver2<br>                           |                                                    |
|                                |     192.168.20.202 sdbserver3<br>                           |                                                    |
|                                | -   保存退出                                                |                                                    |
+--------------------------------+-------------------------------------------------------------+----------------------------------------------------+
| 关闭防火墙 (需要管理员权限)    |                                                             | -   对于 SUSE：                                    |
|                                | 	-   对于 SUSE：                                            | 		chkconfig -list &#124 grep fire；           |
|                                | 		1.  SuSEfirewall2 stop；                               | -   对于 RedHat：                                  |
|                                | 		2.  chkconfig SuSEfirewall2\_setup；                   | 		service iptables status；                   |
|                                | 	-   对于 RedHat：                                          | -   对于 Ubuntu：                                  |
|                                | 		1.  service iptables stop；                            | 		ufw status；                                |
|                                | 		2.  chkconfig iptables off；                           |                                                    |
|                                | 	-   对于 Ubuntu：                                          |                                                    |
|                                | 		1.  ufw disable；                                      |                                                    |
+--------------------------------+-------------------------------------------------------------+----------------------------------------------------+

**Note: **

1.每台作为数据库服务器的机器都需要配置;

2.社区版要求系统安装glibc 2.15以及libstdc++ 6.0.18以上版本。

###Linux 推荐配置###

-   调整 ulimit

    在配置文件 /etc/security/limits.conf 中设置：

<pre class="prettyprint lang-diy">
#&lt;domain&gt;      &lt;type&gt;    &lt;item&gt;     &lt;value&gt;
*                    soft        core             0
*                    soft        data             unlimited
*                    soft       fsize             unlimited
*                    soft         rss             unlimited
*                    soft          as             unlimited</pre>

**参数说明：**

**core**：数据库出现故障时产生 core 文件用于故障诊断，生产系统建议关闭；

**data**：数据库进程所允许分配的数据内存大小；

**fsize**：数据库进程所允许寻址的文件大小；

**rss**：数据库进程所允许的最大 resident set 大小；

**as**：数据库进程所允许最大虚拟内存寻址空间限制；

	在配置文件 /etc/security/limits.d/90-nproc.conf 中设置：

<pre class="prettyprint lang-diy">
#&lt;domain&gt;      &lt;type&gt;    &lt;item&gt;     &lt;value&gt;
*                   soft       nproc             unlimited</pre>

**参数说明：**

**nproc：**数据库所允许的最大线程数限制；

**Note:**

1.每台作为数据库服务器的机器都需要配置；

2.更改配置后需重新登录使得配置生效。

-   调整内核参数

	1.  使用下列命令输出当前 vm 配置，并将其归档保存：

	<pre class="prettyprint lang-javascript">
	> cat /proc/sys/vm/swappiness
	> cat /proc/sys/vm/dirty_ratio
	> cat /proc/sys/vm/dirty_background_ratio
	> cat /proc/sys/vm/dirty_expire_centisecs
	> cat /proc/sys/vm/vfs_cache_pressure
	> cat /proc/sys/vm/min_free_kbytes
	> cat /proc/sys/vm/zone_reclaim_mode
	> cat /proc/sys/vm/overcommit_memory</pre>

	2.  添加下列参数至 /etc/sysctl.conf 文件调整内核参数：

	<pre class="prettyprint lang-diy">
	vm.swappiness = 0
	vm.dirty_ratio = 100
	vm.dirty_background_ratio = 40
	vm.dirty_expire_centisecs = 3000
	vm.vfs_cache_pressure = 200
	vm.min_free_kbytes = &lt;物理内存大小的8%，单位KB&gt;
	vm.zone_reclaim_mode = 0
	vm.overcommit_memory = 0</pre>

	**Note:**
		
	当数据库可用物理内存不足 8GB 时不需使用 vm.swappiness = 0；上述 dirty 类参数只是建议值，具体系统设置时请按原则（控制系统的 flush 进程只采用脏页超时机制刷新脏页，而不采用脏页比例超支刷新脏页）进行设置。

	3.  执行如下命令，使配置生效：

	<pre class="prettyprint lang-javascript">
	> /sbin/sysctl -p</pre>

	4.	停用transparent_hugepage，编辑/etc/rc.local，在第一行“#!/bin/sh”的下一行重启一行添加如下两行内容：

	<pre class="prettyprint lang-diy">
	echo never > /sys/kernel/mm/transparent_hugepage/enabled
	echo never > /sys/kernel/mm/transparent_hugepage/defrag</pre>
	
	5.  执行如下命令，使配置生效：
	
    <pre class="prettyprint lang-javascript">
	> source /etc/rc.local</pre>

    6.	分别执行如下两条命令，输出结果中都有“[never]”则表示成功关闭了transparent_hugepage，如果是“never”并且有“[always]”或者“[madvise]”则关闭失败：

	<pre class="prettyprint lang-javascript">
	> cat /sys/kernel/mm/transparent_hugepage/enabled
	> cat /sys/kernel/mm/transparent_hugepage/defrag</pre>

	**Note:** 

	每台作为数据库服务器的机器都需要配置。

-   数据库目录结构

    用户应尽可能使数据目录，索引目录与日志目录存放在不同物理磁盘中，以减少顺序 I/O 与随机 I/O 之间的竞争。并且保证安装路径上的文件夹具有可读和可执行权限。

-   NUMA的影响
    
    Linux系统默认开启NUMA，NUMA默认的内存分配策略是优先在进程所在CPU的本地内存中分配，会导致CPU节点之间内存分配不均衡，当某个CPU节点的内存不足时，会导致swap产生，而不是从远程节点分配内存。这种内存分配策略的初衷是好的，为了内存更接近需要它的线程，但不适合数据库这种大规模内存使用的应用场景。我们建议用户在使用SequoiaDB时关闭NUMA或者调整NUMA的内存分配策略。

-   关闭NUMA
    
    关闭Linux系统的NUMA的方法主要有两种，一种是通过BIOS禁用NUMA；另一种是通过修改gurb的配置文件，CentOS、SUSE、Ubuntu的grub配置文件有差异，同一款Linux的不同版本配置也略有不同，此处会介绍CentOS6.4和Ubuntu12.04的配置方法以供参考，SUSE和CentOS修改方法类似。

    1.  开机按快捷键进入BIOS设置界面，关闭NUMA。不同品牌的主板或服务器，具体操作略有差异，此处不作详细介绍。
    
    2.  CentOS6.4的grub配置文件修改，以root权限编辑/etc/grub.conf，找到kernel行，该行类似如下（不同的版本内容略有差异，但开头一定有“kernel /vmlinuz-”）：
    
    <pre class="prettyprint lang-diy">
    kernel /vmlinuz-2.6.32-358.el6.x86_64 ro root=/dev/mapper/vg_centos64001-lv_root rd_NO_LUKS rd_LVM_LV=vg_centos64001/lv_root rd_NO_MD rd_LVM_LV=vg_centos64001/lv_swap crashkernel=128M LANG=zh_CN.UTF-8  KEYBOARDTYPE=pc KEYTABLE=us rd_NO_DM rhgb quiet</pre>

    在kernel行的末尾，空格再添加“numa=off”，如果有多个kernel行，则每个kernel行都要添加。

    3.  Ubuntu12.04的grub文件修改，以root权限编辑/boot/grub/grub.cfg，找到Linux引导行，该行类似如下（不同版本内容略有差异，但开头一定有“linux   /boot/vmlinuz-”）：
    
    <pre class="prettyprint lang-diy">
    linux   /boot/vmlinuz-3.2.0-31-generic root=UUID=92191cd8-3690-4cd4-9f42-95d392c9d828 ro</pre>

    在Linux引导行的末尾，空格再添加“numa=off”，如果有多个Linux引导行，则每个Linux引导行都要添加。

    4.   验证NUMA是否成功关闭，shell执行如下命令：
    
    <pre class="prettyprint lang-javascript">
    > numastat</pre>

    如果输出结果中只有node0，则表示成功禁用了NUMA，如果有node1出现则失败。

-   通过numactl以全交叉内存分配策略启动数据库

    当您安装好SequoiaDB后，可以通过numactl以全交叉内存分配策略来启动数据库服务，这样就可以在不关闭NUMA的情况下，避免在高负荷运转时系统卡死甚至崩溃。
    
    以全交叉内存分配策略启动程序的命令形式：

    <pre class="prettyprint lang-javascript">
    > numactl --interleave=all &lt;command&gt;</pre>
    
    以全交叉内存分配策略启动sdbcm服务的命令形式。请先检查sdbcm服务是否正在运行，如果正在运行，请先关闭再使用
    numactl --interleave=all的方式启动服务：

    检查sdbcm服务是否正在运行：    
    <pre class="prettyprint lang-javascript">
    > service sdbcm status</pre>
    
    关闭sdbcm服务：
    <pre class="prettyprint lang-javascript">
    > service sdbcm stop</pre>
    
    以全交叉内存分配策略启动sdbcm服务：
    <pre class="prettyprint lang-javascript">
    > numactl --interleave=all service sdbcm start</pre>
    
    确认sdb相关进程的内存分配策略是否为全交叉的方法，示例：
    
    1.   找到sdb先关的进程号：
    
    <pre class="prettyprint lang-javascript">
    > ps -ef | grep sdb</pre>
    
    2.   根据进程号来查看该进程的内存分配策略，假设进程号为8888：
    
    <pre class="prettyprint lang-javascript">
    > cat /proc/8888/numa_maps</pre>
    
    3.   上一步骤中的输出结果如果每一行的第一个空格后面是“interleave”则表示设置成功。

-   关闭影响服务器硬件性能发挥的Linux服务
    
    Linux系统自启动了一些自认为节能环保的服务，这些服务是为移动设备设计的并不适合服务器，尤其是运行数据库的服务器。这些服务的运行在一定程度上会使得系统不能完全发挥出硬件具有的性能。如：用于优化中断分配的irqbalance，用于cpu自动降频的cpuspeed、cpufreqd、powerd等。建议关闭这些服务。当系统允许效率低于预期时，请使用“top”和“ps -ef | grep xxx”命令检查这些服务是否占用过多的资源。

    临时关闭irqbalance和cpuspeed：

    <pre class="prettyprint lang-javascript">
    > service irqbalance stop
    > service irqbalance stop</pre>
    
    永远关闭irqbalance和cpuspeed（该方法要重启系统才能生效，如不能重启，先执行上文中的临时关闭的命令）：

    <pre class="prettyprint lang-javascript">
    > chkconfig irqbalance off
    > chkconfig cpuspeed off</pre>
    
    
    
    
    

    


    


