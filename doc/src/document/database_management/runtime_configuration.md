##参数说明##

|参数名|缩写|类型|说明|
|---|---|---|---|
|--help|-h|--|打印帮助|
|--dbpath|-d|str|1.指定数据文件存放路径。2.如果不指定，则默认为当前路径。|
|--indexpath|-i|str|1.指定索引文件存放路径。2.如果不指定，则默认与'dbpath'相同。|
|--confpath|-c|str|1.指定配置文件路径（不包含文件名），系统会在confpath下寻找sdb.conf。<br/>             2.sdb.conf中填入需要的配置项，配制方法为：参数名 = 参数值。如 svcname=11810；diaglevel=3<br/>             3.如果不指定此参数，系统默认在当前路径寻找sdb.conf。<br/>             4.sdb.conf可以不存在。|
|--logpath|-l|str|1.副本节点在进行数据同步时会生成同步日志。此参数用来指定同步日志的路径。<br/>             2.如果不指定，则默认路径为：数据文件路径/replicalog|
|--diagpath|--|str|1.指定诊断日志存放目录。<br/>             2.如果不指定，则默认为：数据文件路径/diaglog|
|--auditpath|--|str|1.指定审计日志存放目录。<br/>             2.如果不指定，则默认为：数据文件路径/diaglog|
|--diagnum|--|num|1.指定诊断日志文件最大数量。<br/>             2.如果不指定，则默认为：20，-1表示不限制。|
|--auditnum|--|num|1.指定审计日志文件最大数量。<br/>             2.如果不指定，则默认为：20，-1表示不限制。|
|--bkuppath|--|str|1.指定备份文件生成目录。<br/>              2.如果不指定，则默认为：数据文件路径/bakfile|
|--maxpool|--|str|1.指定线程池内线程数量。<br/>             2.如果不指定，则默认为20，取值范围是[0,10000]。|
|--svcname|-p|str|1.指定本地服务端口。<br/>             2.如果不指定则默认为11810端口用于编目节点，11800用于协调节点，11820用于数据节点。|
|--replname|-r|str|1.指定数据同步平面端口。<br/>             2.如果不指定则默认为svcname+1。|
|--shardname|-a|str|1.指定shard平面端口。<br/>             2.如果不指定则默认为svcname+2。|
|--catalogname|-x|str|1.指定catalog平面端口。<br/>             2.如果不指定则默认为svcname+3。|
|--httpname|-s|str|1.指定http端口。<br/>             2.如果不指定则默认为svcname+4。|
|--diaglevel|-v|num|1.指定诊断日志打印级别。SequoiaDB中诊断日志从0-5分别代表：SEVERE, ERROR, EVENT, WARNING, INFO, DEBUG。<br/>             2.如果不指定，则默认为WARNING。|
|--auditmask|--|str|1.指定审计日志打印掩码。SequoiaDB中审计日志类型有：ACCESS,CLUSTER,SYSTEM,DML,DDL,DCL,DQL,INSERT,DELETE,UPDATE,OTHER。<br/>             2.如果不指定，则默认为"SYSTEM|DDL|DCL", ALL取值表示开启所有,NONE关闭全部。|
|--role|-o|str|1.指定服务角色。SequoiaDB分别以data/coord/catalog/standalone代表：数据节点/协调节点/编目节点/单机。<br/>             2.如果不指定则默认为单机。|
|--catalogaddr|-t|str|1.指定编目节点的地址。配置形式为"hostname1:catalogname1,hostname2:catalogname2,..."。<br/>             2.需要至少指定一个编目节点的地址。|
|--logfilesz|-f|num|1.指定同步日志文件的大小。合法输入为64（MB）- 2048（MB）。<br/>             2.如果不指定，则默认为64（MB）。|
|--logfilenum|-n|num|1.指定同步日志文件的数量。<br/>              2.如果不指定，则默认为20。|
|--transactionon|-e|boolean|1.指定是否打开事务。2.如果不指定，则默认为false。|
|--transactiontimeout|--|num|事务锁等待超时时间（单位：秒）,默认为:60,取值范围[0,3600]|
|--numpreload|--|num|页面预加载代理数据，默认值为0，取值范围：[0,100]|
|--maxprefpool|--|num|数据预取代理池最大数量,默认值:0,取值范围:[0,1000]|
|--maxreplsync|--|num|日志同步最大并发数量，默认值:10,取值范围:[0,200], 0表示不启用日志并发同步|
|--logbuffsize|--|num|复制日志内存页面数,默认值:1024,取值范围:[512,1024000],但日志总内存大小不能超过日志总文件大小;每个页面大小为64KB|
|--tmppath|--|str|数据库临时文件目录，默认为'数据库路径'+'/tmp'|
|--sortbuf|--|num|排序缓存大小(MB),默认值256,最小值128|
|--hjbuf|--|num|哈希连接缓存大小(MB),默认值128,最小值64|
|--syncstrategy|--|str|副本组之间数据同步控制策略,取值:none,keepnormal,keepall,默认为keepnormal。|
|--preferedinstance|--|str|1.指定执行读请求时优先选择的实例<br/>             2.如果不指定，则默认为随机选择任意实例。<br/>             3.取值列表：<br/>                    M--可读写实例<br/>                    S--只读实例<br/>                    A--任意实例<br/>                    1-7--第n个实例|
|--lobpath|--|str|1.指定大对象存放路径。<br/>             2.如果不指定，则默认为：数据文件路径|
|--lobmetapath|--|str|1.指定大对象元数据存放路径。<br/>             2.如果不指定，则默认与'lobpath'保持一致|
|--directioinlob|--|boolean|在大对象功能中关闭文件系统缓存，如果不指定，默认值为"false"|
|--sparsefile|--|boolean|当扩展文件时，使用稀疏文件功能，如果不指定，默认值为"false"|
|--weight|--|num|节点选举权重, 默认值为10, 取值范围[1, 100]|
|--usessl|--|boolean|允许客户端使用SSL连接（仅限企业版），默认为false|
|--auth|--|boolean|开启鉴权功能.默认为true|
|--arbiter|--|boolean|将节点设置成为一个仲裁节点。默认为false。|
|--planbuckets|--|num|访问计划缓存内桶的个数。当其为零时Sdb将不会缓存任何访问计划。默认为500。|
|--optimeout|--|num|判定操作中断的时间(ms),默认值:300000, 0表示不超时|
|--overflowratio|--|num|记录大小预留空间扩展比(%),默认为12,取值范围:[0,10000]|
|--omaddr|--|str|1.指定om节点的地址。配置形式为"hostname:omservicename"。|
|--maxcachesize|--|num|节点层缓层最大大小，单位为MB，默认值为0|
|--maxcachejob|--|num|1.后台缓存任务线程的最大数量，默认值为10，取值范围为[2, 200]。<br/>             2.后台缓存任务线程主要执行同步脏页至文件，回收和释放空闲内存页。<br/>             3.后台缓存任务线程根据缓存的负载情况自动启动和退出。|
|--maxsyncjob|--|num|1.后台数据同步任务线程的最大数量，默认值为10，取值范围为[2, 200]。<br/>             2.后台数据同步任务线程主要执行同步脏数据和日志至文件。<br/>             3.后台数据同步任务线程根据负载情况自动启动和退出。|
|--syncinterval|--|num|后台数据同步周期，单位毫秒，默认值为10000，0表示不按周期触发数据同步|
|--syncrecordnum|--|num|后台数据同步触发记录数，默认值为0，0表示不按记录数触发数据同步|
|--syncdeep|--|boolean|1. 数据同步是否开启深度刷盘。<br/>             2. 如果不指定，则默认为false。|
|--archiveon|--|boolean|开启复制日志归档功能，默认值为false。|
|--archivecompresson|--|boolean|开启复制日志归档压缩功能，默认值为true。|
|--archivepath|--|str|1.此参数用来指定归档日志的路径。<br/>             2.如果不指定，则默认路径为：数据文件路径/archivelog。|
|--archivetimeout|--|num|判定未归档的超时时间(秒)，默认值：600，0表示不超时。|
|--archiveexpired|--|num|归档日志文件的过期时间(小时)，默认值:240，0表示不过期。|
|--archivequota|--|num|归档日志目录的磁盘配额(GB)，默认值：10，0表示没有限制。|

>**Note:**  
>1. SequoiaDB支持命令行方式及配置文件方式。当两种方式并存时，命令行参数将会覆盖配置文件中的相同的配置项。  
>2. 同步日志的总大小（logfilesz * logfilenum）决定了在同步过程中的容错能力。日志越大则进行全量恢复的可能性越小。  

