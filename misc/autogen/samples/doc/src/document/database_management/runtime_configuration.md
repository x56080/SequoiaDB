##参数说明##

|参数名|缩写|类型|生效类型|生效策略|说明|
|---|---|---|---|---|---|
|--dbpath|-d|str|||1. 指定数据文件存放路径。2. 如果不指定，则默认为当前路径。|
|--indexpath|-i|str|||1. 指定索引文件存放路径。2. 如果不指定，则默认与'dbpath'相同。|
|--confpath|-c|str|||1. 指定配置文件路径（不包含文件名），系统会在confpath下寻找sdb.conf。<br/>             2. sdb.conf中填入需要的配置项，配制方法为：参数名 = 参数值。如 svcname=11810；diaglevel=3<br/>             3. 如果不指定此参数，系统默认在当前路径寻找sdb.conf。<br/>             4. sdb.conf可以不存在。|
|--logpath|-l|str|||1. 副本节点在进行数据同步时会生成同步日志。此参数用来指定同步日志的路径。<br/>             2. 如果不指定，则默认路径为：数据文件路径/replicalog|
|--diagpath||str|重启生效||1. 指定诊断日志存放目录。<br/>             2. 如果不指定，则默认为：数据文件路径/diaglog|
|--auditpath||str|重启生效||1. 指定审计日志存放目录。<br/>             2. 如果不指定，则默认为：数据文件路径/diaglog|
|--diagnum||num|在线生效|当前文件写满时生效|1. 指定诊断日志文件最大数量。<br/>             2. 如果不指定，则默认为：20，-1表示不限制，取值范围[-1, 2^31 - 1]。|
|--auditnum||num|在线生效|当前文件写满时生效|1. 指定审计日志文件最大数量。<br/>             2. 如果不指定，则默认为：20，-1表示不限制，取值范围[-1, 2^31 - 1]。|
|--bkuppath||str|||1. 指定备份文件生成目录。<br/>              2. 如果不指定，则默认为：数据文件路径/bakfile|
|--maxpool||num|在线生效||1. 指定线程池内线程数量。<br/>             2. 如果不指定，则默认为50，取值范围是[0,10000]。|
|--svcname|-p|str|||1. 指定本地服务端口。<br/>             2. 如果不指定则默认为11810端口用于编目节点，11800用于协调节点，11820用于数据节点。|
|--replname|-r|str|||1. 指定数据同步平面端口。<br/>             2. 如果不指定则默认为svcname+1。|
|--shardname|-a|str|||1. 指定shard平面端口。<br/>             2. 如果不指定则默认为svcname+2。|
|--catalogname|-x|str|||1. 指定catalog平面端口。<br/>             2. 如果不指定则默认为svcname+3。|
|--httpname|-s|str|||1. 指定http端口。<br/>             2. 如果不指定则默认为svcname+4。|
|--diaglevel|-v|num|在线生效||1. 指定诊断日志打印级别。SequoiaDB中诊断日志从0-5分别代表：SEVERE, ERROR, EVENT, WARNING, INFO, DEBUG。<br/>             2. 如果不指定，则默认为WARNING。|
|--auditmask||str|在线生效|新连接生效|1. 指定审计日志打印掩码。SequoiaDB中审计日志类型有：ACCESS,CLUSTER,SYSTEM,DML,DDL,DCL,DQL,INSERT,DELETE,UPDATE,OTHER。<br/>             2. 如果不指定，则默认为"SYSTEM\|DDL\|DCL", ALL取值表示开启所有,NONE关闭全部。|
|--role|-o|str|||1. 指定服务角色。SequoiaDB分别以data/coord/catalog/standalone代表：数据节点/协调节点/编目节点/单机。<br/>             2. 如果不指定则默认为单机。|
|--catalogaddr|-t|str|||1. 指定编目节点的地址。配置形式为"hostname1:catalogname1,hostname2:catalogname2,..."。<br/>             2. 需要至少指定一个编目节点的地址。|
|--logfilesz|-f|num|||1. 指定同步日志文件的大小。合法输入为64（MB）- 2048（MB）。<br/>             2. 如果不指定，则默认为64（MB）。<br/>             3. 同步日志的总大小（logfilesz * logfilenum）决定了在同步过程中的容错能力，日志越大则触发全量同步的可能性越小。|
|--logfilenum|-n|num|||1. 指定同步日志文件的数量。<br/>              2. 如果不指定，则默认为20。|
|--transactionon|-e|boolean|重启生效||1. 指定是否打开事务。2. 如果不指定，则默认为true。|
|--transactiontimeout||num|在线生效||1. 事务锁等待超时时间（单位：秒）,默认为:60,取值范围[0,3600]|
|--numpreload||num|重启生效||页面预加载代理数据，默认值为0，取值范围：[0,100]|
|--maxprefpool||num|重启生效||数据预取代理池最大数量,默认值:0,取值范围:[0,1000]|
|--maxreplsync||num|在线生效||日志同步最大并发数量，默认值:10,取值范围:[0,200], 0表示不启用日志并发同步|
|--logbuffsize||num|重启生效||复制日志内存页面数,默认值:1024,取值范围:[512,1024000],但日志总内存大小不能超过日志总文件大小;每个页面大小为64KB|
|--tmppath||str|重启生效||数据库临时文件目录，默认为'数据库路径'+'/tmp'|
|--sortbuf||num|在线生效|下次查询生效|排序缓存大小(MB),默认值256,最小值128,取值范围[128, 2^31 - 1]|
|--hjbuf||num|在线生效|下次查询生效|哈希连接缓存大小(MB),默认值128,最小值64,取值范围[64, 2^31 - 1]|
|--syncstrategy||str|在线生效||1. 副本组之间数据同步控制策略。<br/>             2. 取值列表：<br/>             ● none: 不开启同步控制策略。若主节点处理数据的能力远超备节点同步数据的能力，则在写操作繁忙的场景下易导致备节点发生全量同步。<br/>             ● keepnormal: 主动降低主节点相对于正常节点的处理速度（可能会造成性能影响），以避免全量同步的发生。<br/>             ● keepall: 主动降低主节点相对于所有节点的处理速度（可能会造成性能影响），以避免全量同步的发生。<br/>             3. keepnormal和keepall的区别在于，当有节点异常时keepall会降低主节点的处理速度，而keepnormal不受异常节点的影响。<br/>             4. 如果不指定，默认为keepnormal。|
|--preferedinstance||str|在线生效|新连接生效|1. 指定执行读请求时优先选择的实例<br/>             2. 如果不指定，则默认值为M，即选择可读写实例。<br/>             3. 取值列表：<br/>             ● "M", "m": 可读写实例（主实例）；如果多个 1-255 的实例和 "M" 一起指定，则满足指定实例中的主实例会优先选择；如果多个 1-255 的实例和 "M" 或 "m" 一起指定，则当没有满足指定的实例时选择主实例。<br/>             ● "S", "s": 只读实例（备实例）；如果多个 1-255 的实例和 "S" 一起指定，则满足指定实例中的备实例会被优先选择；如果多个 1-255 的实例和 "S" 或 "s" 一起指定，则当没有满足指定的实例时选择备实例。<br/>             ● "A", "a": 任意实例。<br/>             ● 1-255: 通过 --instanceid 指定实例 ID 的实例。<br/>             ● 如果指定多个 "M", "m", "S", "s", "A", "a" 实例，则只有第一个生效。<br/>             4. 如果没有匹配的实例，将随机选择。|
|--preferedinstancemode||str|在线生效|新连接生效|1. 指定当多个实例符合 --preferedinstance 的条件时的选择模式。<br/>           2. 取值列表：<br/>                ● "random": 从候选的实例中随机选择。<br/>                ● "ordered": 从候选的实例中按照 --perferedinstance 的顺序进行选择。|
|--preferedstrict||boolean|在线生效|新连接生效|指定节点选择是否为严格模式，当为严格模式时，节点只能从 preferedinstance 指定的ID中选取，默认为false。|
|--instanceid||num|重启生效||节点的实例 ID，用于 --preferedinstance 进行实例选择。|
|--lobpath||str|||1. 指定大对象存放路径。<br/>             2. 如果不指定，则默认为：数据文件路径|
|--lobmetapath||str|||1. 指定大对象元数据存放路径。<br/>             2. 如果不指定，则默认与'lobpath'保持一致|
|--directioinlob||boolean|在线生效|新建集合空间生效|在大对象功能中关闭文件系统缓存，如果不指定，默认值为"false"|
|--sparsefile||boolean|在线生效||当扩展文件时，使用稀疏文件功能，如果不指定，默认值为"false"|
|--weight||num|在线生效||节点选举权重, 默认值为10, 取值范围[1, 100]|
|--usessl||boolean|在线生效|新连接生效|允许客户端使用SSL连接（仅限企业版），默认为false|
|--auth||boolean|在线生效||开启鉴权功能.默认为true|
|--arbiter||boolean|||将节点设置成为一个仲裁节点。默认为false。|
|--planbuckets||num|在线生效|下次查询生效|1. 访问计划缓存内桶的个数。<br/>             2. 当其为0时数据库将不会缓存任何访问计划。<br/>             3. 默认为500，最大值为4096。<br/>             4. SequoiaDB 内部自动向上取整为 0, 128, 256, 512, 1024, 2048, 4096。<br/>             5. 每个桶中平均最多可以放置 3 个访问计划缓存。|
|--optimeout||num|在线生效||判定操作中断的时间(ms),默认值:60000, 0表示不超时，取值范围[0, 2^31 - 1]|
|--overflowratio||num|在线生效||记录大小预留空间扩展比(%),默认为12,取值范围:[0,10000]|
|--omaddr||str|||1. 指定om节点的地址。配置形式为"hostname:omservicename"。|
|--maxcachesize||num|在线生效||节点缓存最大值，单位为MB，默认值为0，取值范围[0, 2^31 - 1]（注意：该配置目前仅对Lob功能生效）|
|--maxcachejob||num|在线生效||1. 后台缓存任务线程的最大数量，默认值为10，取值范围为[2, 200]。<br/>             2. 后台缓存任务线程主要执行同步脏页至文件，回收和释放空闲内存页。<br/>             3. 后台缓存任务线程根据缓存的负载情况自动启动和退出。<br/>             4. 该配置目前仅对Lob功能生效。|
|--cachemergesz||num|在线生效||每一个集合空间用于合并页的缓存大小，默认为0，取值范围:[0,64],单位为MB（注意：该配置目前仅对Lob功能生效）|
|--pagealloctimeout||num|在线生效||申请缓存页的超时时间，默认为0，取值范围:[0,3600000],单位为毫秒|
|--maxsyncjob||num|在线生效||1. 后台数据同步任务线程的最大数量，默认值为10，取值范围为[2, 200]。<br/>             2. 后台数据同步任务线程主要执行同步脏数据和日志至文件。<br/>             3. 后台数据同步任务线程根据负载情况自动启动和退出。|
|--syncinterval||num|在线生效||1. 后台数据同步周期，单位毫秒。<br/>             2. 对于编目节点和om节点，默认值为10000，取值范围(0, 60000]<br/>             3. 对于其它类型的节点，默认值为10000，0表示不按周期触发数据同步，取值范围[0, 2^31 - 1]|
|--syncrecordnum||num|在线生效||1. 后台数据同步触发记录数。<br/>             2. 对于编目节点和om节点，默认值为10，取值范围(0, 1000]<br/>             3. 对于其它类型的节点，默认值为0，0表示不按记录数触发数据同步，取值范围[0, 2^31 - 1]|
|--syncdeep||boolean|在线生效||1. 数据同步是否开启深度刷盘。<br/>             2. 如果不指定，则默认为false。|
|--archiveon||boolean|重启生效||开启复制日志归档功能，默认值为false。|
|--archivecompresson||boolean|在线生效|新归档文件生效|开启复制日志归档压缩功能，默认值为true。|
|--archivepath||str|||1. 此参数用来指定归档日志的路径。<br/>             2. 如果不指定，则默认路径为：数据文件路径/archivelog。|
|--archivetimeout||num|在线生效||判定未归档的超时时间(秒)，默认值：600，0表示不超时, 取值范围[0, 2^31 - 1]。|
|--archiveexpired||num|在线生效||归档日志文件的过期时间(小时)，默认值:240，0表示不过期, 取值范围[0, 11930464]。|
|--archivequota||num|在线生效||归档日志目录的磁盘配额(GB)，默认值：10，0表示没有限制，取值范围[0, 2^31 - 1]。|
|--dataerrorop||num|在线生效||1. 节点在无法继续正常增量同步而可能触发全量同步时的处理操作，取值为 0/1/2。 缺省为1。<br/>           2. 取值列表：<br/>             ● 0: 不作任何处理，保持节点运行。<br/>             ● 1: 自动从该数据组的其它节点进行全量同步。<br/>             ● 2: 该节点停止运行。|
|--maxconn||num|在线生效||指定允许连接到引擎的客户端的最大数量，取值范围为[0,30000], 默认值是0，0表示不限制|
|--plancachelevel||num|在线生效|下次查询生效|1. 指定查询计划的缓存级别，默认是 3。<br/>           2. 取值列表：<br/>             ● 0: 不缓存查询计划。<br/>             ● 1: 缓存原查询计划。<br/>             ● 2: 缓存泛化后的查询计划。<br/>             ● 3: 缓存参数化的查询计划。<br/>             ● 4: 缓存参数化并带操作符模糊匹配的查询计划。<br/>         |
|--svcscheduler||num|重启生效||1. 指定任务调度器类型，默认是0。<br/>           2. 取值列表：<br/>             ● 0: 不开启。<br/>             ● 1: 先入先出。<br/>             ● 2: 基于优先级调度。<br/>             ● 3: 基于容器调度。<br/>         |
|--svcmaxconcurrency||num|在线生效||1. 指定任务执行的最大并发数，0表示不限制。默认值为100，取值范围[0, 2^31 - 1]。<br/>             2. 当'svcscheduler'取值为0时，该参数不生效。<br/>         |
|--transisolation||num|在线生效|下一次事务中生效|1. 指定事务隔离级别，默认是1。<br/>           2. 取值列表：<br/>             ● 0: RU，读未提交。<br/>             ● 1: RC, 读已提交。<br/>             ● 2: RS，读稳定性。<br/>         |
|--translockwait||boolean|在线生效|下一次事务中生效|1. 指定事务在RC隔离级别下记录锁的等待行为，默认是false。需要与transisolation配合使用<br/>              2. 取值列表：<br/>              ● false: 不等待记录锁，直接从系统读取最后一次提交的版本。<br/>              ● true: 等待记录锁，读取最新提交版本的数据<br/>        |
|--transautocommit||boolean|在线生效|下一次事务中生效|是否开启自动事务提交，默认是false。只有当 transaction 开启时取值才会生效。|
|--transautorollback||boolean|在线生效|下一次事务中生效|1.事务操作失败是否自动回滚该事务。默认为true。只有当 transaction 开启时取值才会生效。|
|--transuserbs||boolean|在线生效|下一次事务中生效|事务操作是否使用回滚段。默认为true。只有当 transaction 开启时取值才会有效。|
|--transrccount||boolean|在线生效|下一次事务中生效|是否使用读已提交来处理 count() 查询。默认为 true。只有当 transaction 开启时，且隔离级别为读已提交 RC 或者读稳定性 RS，取值才会有效。|
|--logwritemod||str|在线生效||复制日志写模式，取值：increment,full,默认为increment。为increment时，复制日志只保存更新记录的增量信息；为full时，复制日志将保存更新记录的完整信息。|
|--logtimeon||boolean|在线生效||开启复制日志保存时间信息功能，默认为false。|
|--maxsocketpernode|||在线生效||两个节点之前的最大连接数，取值范围：[1,100]，默认为5。|
|--maxsocketperthread|||在线生效||一个线程驱动最大连接数，0表示不限制，默认为1。|
|--maxsocketthread|||在线生效||通信最大驱动线程数，取值范围[1,100]，默认10。|

>**Note:**  
>1. “生效类型”为在线生效的配置能进行在线修改，不需要重启就能生效。  
>2. “生效类型”为重启生效的配置能进行在线修改，需要重启后生效。  
>3. “生效类型”为空的配置不能进行在线修改，需要用户对相应的文件进行管理，重启后才能生效。请参考[特殊配置项修改](database_management/Special_configuration_modify#特殊配置项修改)。  

##参数配置##
SequoiaDB支持命令行方式及配置文件方式进行参数配置。

###命令行方式配置###

在启动sequoiadb时传入配置参数值：

```lang-bash
$ ./sequoiadb --businessname yyy --catalogaddr ubuntu-wjm:30003,ubuntu-wjm:30013,ubuntu-wjm:30023 --clustername xxx --dbpath /home/users/wjm/sequoiadb/trunk/50000 --diaglevel 3 --role coord --svcname 50000
```

###配置文件方式配置###

在启动sequoiadb时传入配置文件路径：

```lang-bash
$ ./sequoiadb -c ../conf/local/50000/
```

配置文件内容如下：

```lang-ini
businessname=yyy
catalogaddr=ubuntu-wjm:30003,ubuntu-wjm:30013,ubuntu-wjm:30023
clustername=xxx
dbpath=/home/users/wjm/sequoiadb/trunk/50000
diaglevel=3
role=coord
svcname=50000
```

>**Note:**  
>1. 当两种方式并存时，命令行参数将会覆盖配置文件中的相同的配置项。 

###配置动态生效###
使用 [updateConf()](reference/Sequoiadb_command/Sdb/updateConf.md) 以及 [deleteConf()](reference/Sequoiadb_command/Sdb/deleteConf.md) 在线修改配置。

使用 [reloadConf()](reference/Sequoiadb_command/Sdb/reloadConf.md) 重新加载配置文件，并进行配置动态生效，只支持“生效类型”列为“在线生效”的配置项，其他配置项会被忽略。“生效策略”若无其他说明，则默认为立即生效。
