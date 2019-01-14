##参数说明##

  参数名                缩写   类型      说明
  --------------------- ------ --------- ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
  --help                -h     --        打印帮助
  --dbpath              -d     str       1.指定数据文件存放路径。2.如果不指定，则默认为当前路径。
  --indexpath           -i     str       1.指定索引文件存放路径。2.如果不指定，则默认与'dbpath'相同。
  --confpath            -c     str       1.指定配置文件路径（不包含文件名），系统会在confpath下寻找sdb.conf。 2.sdb.conf中填入需要的配置项，配制方法为：参数名 = 参数值。如 svcname=11810；diaglevel=3 3.如果不指定此参数，系统默认在当前路径寻找sdb.conf。 4.sdb.conf可以不存在。
  --logpath             -l     str       1.副本节点在进行数据同步时会生成同步日志。此参数用来指定同步日志的路径。 2.如果不指定，则默认路径为：数据文件路径/replicalog
  --diagpath            --     str       1.指定诊断日志存放目录。 2.如果不指定，则默认为：数据文件路径/diaglog
  --diagnum             --     num       1.指定诊断日志文件最大数量。 2.如果不指定，则默认为：20，-1表示不限制。
  --bkuppath            --     str       1.指定备份文件生成目录。 2.如果不指定，则默认为：数据文件路径/bakfile
  --maxpool             --     str       1.指定线程池内线程数量。 2.如果不指定，则默认为0。
  --svcname             -p     str       1.指定本地服务端口。 2.如果不指定则默认为11810端口用于编目节点，11800用于协调节点，11820用于数据节点。
  --replname            -r     str       1.指定数据同步平面端口。 2.如果不指定则默认为svcname+1。
  --shardname           -a     str       1.指定shard平面端口。 2.如果不指定则默认为svcname+2。
  --catalogname         -x     str       1.指定catalog平面端口。 2.如果不指定则默认为svcname+3。
  --httpname            -s     str       1.指定http端口。 2.如果不指定则默认为svcname+4。
  --diaglevel           -v     num       1.指定诊断日志打印级别。SequoiaDB中诊断日志从0-5分别代表：SEVERE, ERROR, EVENT, WARNING, INFO, DEBUG。 2.如果不指定，则默认为WARNING。
  --role                -o     str       1.指定服务角色。SequoiaDB分别以data/coord/catalog/standalone代表：数据节点/协调节点/编目节点/单机。 2.如果不指定则默认为单机。
  --catalogaddr         -t     str       1.指定编目节点的地址。配置形式为"hostname1:catalogname1,hostname2:catalogname2,..."。 2.需要至少指定一个编目节点的地址。
  --logfilesz           -f     num       1.指定同步日志文件的大小。合法输入为64（MB）- 2048（MB）。 2.如果不指定，则默认为64（MB）。
  --logfilenum          -n     num       1.指定同步日志文件的数量。 2.如果不指定，则默认为20。
  --transactionon       -e     boolean   1.指定是否打开事务。2.如果不指定，则默认为false。
  --numpreload          --     num       页面预加载代理数据，默认值为0，取值范围：[0,100]
  --maxprefpool         --     num       数据预取代理池最大数量,默认值:0,取值范围:[0,1000]
  --maxreplsync         --     num       日志同步最大并发数量，默认值:10,取值范围:[0,200], 0表示不启用日志并发同步
  --logbuffsize         --     num       复制日志内存页面数,默认值:1024,取值范围:[512,1024000],但日志总内存大小不能超过日志总文件大小;每个页面大小为64KB
  --tmppath             --     num       数据库临时文件目录，默认为'数据库路径'+'/tmp'
  --sortbuf             --     num       排序缓存大小(MB),默认值256,最小值128
  --hjbuf               --     num       哈希连接缓存大小(MB),默认值128,最小值64
  --syncstrategy        --     str       副本组之间数据同步控制策略,取值:none,keepnormal,keepall,默认为keepnormal。
  --preferedinstance    --     str       1.指定执行读请求时优先选择的实例 2.如果不指定，则默认为随机选择任意实例。 3.取值列表： M--可读写实例 S--只读实例 A--任意实例 1-7--第n个实例
  --numpagecleaners     --     num       数据库启动时需要开启的脏页清除器数量 0意味着不启动任何脏页清除器，默认为1，取值范围：[0, 50]。
  --pagecleaninterval   --     num       对每个集合空间的进行脏页清除的最小时间间隔 单位：毫秒，默认：10000，最小：1000
  --lobpath             --     str       1.指定大对象存放目录。 2.如果不指定，则默认为：数据文件路径
  --directioinlob       --     boolean   在大对象功能中关闭文件系统缓存，如果不指定，默认值为"false"
  --sparsefile          --     boolean   当扩展文件时，使用稀疏文件功能，如果不指定，默认值为"false"
  --weight              --     num       节点选举权重，默认值为10，取值范围[1, 100]
  --usessl              --     boolean   允许客户端使用 SSL 连接（仅限企业版），默认为 false
  --auth                --     boolean   开启鉴权功能，默认为 true
  --planbuckets         --     num       访问计划缓存内桶的个数。当其为零时Sdb将不会缓存任何访问计划
  --arbiter             --     boolean   将节点设置成为一个仲裁节点。默认为false。
  --transactiontimeout  --     num       事务锁等待超时时间（单位：秒）,默认为:60,取值范围[0,3600]
  --omaddr              --     str       指定om节点的地址。配置形式为"hostname:omservicename"。
  --dataerrorop         --     num       1.节点在无法继续正常增量同步而可能触发全量同步时的处理操作，默认为1。取值列表：0--不作任何处理，保持节点运行 1--自动从该数据组的其它节点进行全量同步 2--该节点停止运行

**Note:**

SequoiaDB支持命令行方式及配置文件方式。当两种方式并存时，命令行参数将会覆盖配置文件中的相同的配置项。

同步日志的总大小（logfilesz * logfilenum）决定了在同步过程中的容错能力。日志越大则进行全量恢复的可能性越小。
