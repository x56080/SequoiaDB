[^_^]:
    概述
    作者：吴艳
    时间：20190320
    评审意见
    王涛：时间：
    许建辉：时间：
    市场部：时间：20190521


sdbdpsdump 是 SequoiaDB 巨杉数据库的同步日志文件 dump 工具，该工具可以解析 replicalog 同步日志文件的内容，并且给出结果报告。

运行需求
----

- sdbdpsdump 不需要与数据库连接。
- 运行 sdbdpsdump 命令的用户必须对数据库的 replicalog 同步日志文件拥有读权限。

连接

语法规则
----
```lang-text
sdbdpsdump [--source | -s arg][--output | -o arg][ --type | -t arg]

sdbdpsdump [--source | -s arg][--name | -n arg][--output | -o arg][ --type | -t arg]

sdbdpsdump [--source | -s arg][--lsn | -l arg][--ahead | -a arg][--output | -o arg]

sdbdpsdump [--source | -s arg][--meta | -m]

sdbdpsdump [--source | -s arg][--last | -e arg] 

sdbdpsdump --help | -h

sdbdpsdump --version | -v
```

参数说明
----

- **--help, -h**
  
 返回基本帮助和用法文本


- **--version, -v**  

 返回工具的编译版本

- **--meta, -m**  

 指定在解析日志文件时，只解析日志文件的元数据信息，只能单独使用

- **--type, -t**  

 指定只解析日志文件中指定类型的日志：<br />

 - 数据插入<br />
 - 数据更新<br />
 - 数据删除<br />
 - 创建集合空间<br />
 - 删除集合空间<br />
 - 创建集合
 - 删除集合
 - 创建索引<br />
 - 删除索引<br />
 - 集合重命名<br />
 - 集合 truncate<br />
 - 事务提交<br />
 - 事务回滚<br />
 - 清空 Catalog 缓存<br />
 - 写入 LOB 数据<br />
 - 删除 LOB 数据<br />
 - 修改 LOB 数据<br />
 - LOB 数据 truncate<br />
 - 修改集合/集合空间/域的属性<br />
 - 添加集合/集合空间的 UniqueID<br />

- **--name, -n**  

 指定集合空间或者集合名，只解析和指定名相关的集合或者集合空间的日志

- **--lsn, -l**  

 指定 LSN 值，只解析 LSN 的日志，可与 -a 和 -b 联用

- **--last, -e**  

 指定解析日志文件最后的 N 条日志，N 为指定的数值（需要 -s 指定到文件）

- **--source, -s**  

 指定日志文件的目录或路径，默认为当前目录

- **--output, -o**  

 指定输出文件，默认为屏幕输出

- **--ahead, -a**  

 指定解析指定 LSN 之前的 N 条日志，N 为指定数值，默认值为 20，必须与 -l 或 --lsn 联用

- **--back, -b**  

 指定解析指定 lsn 之后的 N 条日志，N 为指定数值，默认值为 20，必须与 -l 或 --lsn 联用

> **Note：**
>
> 无论用户是否指定 --meta 选项，sdbdpsdump 都会打印日志文件的元数据信息。

示例
----

* sdbdpsdump 解析指定目录 `/opt/sequoiadb/database/data/11860/replicalog` 中的 replicalog 文件，并只解析类型为 1（数据插入）的日志，输出到当前目录 `out.log`

   ```lang-bash
   $ sdbdpsdump -s /opt/sequoiadb/database/data/11860/replicalog/sequoiadbLog.0 -o out.log -t 1
   ```

   输出如下日志文件元数据信息，包括起始日志文件、结束日志文件、起始 LSN 、最后的日志 LSN、日志有效空间等：

   ```lang-text
   =======================================
       Log Files in total: 1
       LogFile begin     : sequoiadbLog.0
       LogFile work      : sequoiadbLog.0
           begin Lsn     : 0x00000000
           current Lsn   : 0x00000644
           expect Lsn    : 0x00000710
   =======================================
   
   Log File Name: sequoiadbLog.0
   Logic ID     : 0
   First LSN    : 0x00000000
   Last  LSN    : 0x00000644
   Valid Size   : 1808 bytes
   Rest Size    : 67107056 bytes
   
   parse file : [/opt/sequoiadb/database/data/11860/replicalog/sequoiadbLog.0]
   
   0x00007FFF40BC1290 : 5344 424C 4F47 4844 0000 0000 0000 0000   SDBLOGHD........
   0x00007FFF40BC12A0 : 0100 0000 0000 0000 0000 0000 0100 0000   ................
   0x00007FFF40BC12B0 : 0000 0004 0000 0000 1400 0000 0000 0000   ................
   0x00007FFF40BC12C0 : 0000 0000 0000 0000 0000 0000 0000 0000   ................
   *
   
    Head   : SDBLOGHD
    FirstLSN: 0x0000000000000000(0)
    LogID  : 0
    
   0x00000000019EE900 : 2001 0000 0000 0000 A400 0000 0000 0000    ...............
   0x00000000019EE910 : 7000 0000 0100 0000 0100 0000 0000 0000   p...............
   0x00000000019EE920 : C910 0000 0073 616D 706C 652E 656D 706C   .....sample.empl
   0x00000000019EE930 : 6F79 6565 0001 3500 0000 3500 0000 075F   oyee..5...5...._
   0x00000000019EE940 : 6964 005C 9496 946E 6107 74AC 9A1C 6510   id.\...na.t...e.
   0x00000000019EE950 : 6100 0000 0000 106E 6F00 0000 0000 0274   a......no......t
   0x00000000019EE960 : 6573 7400 0600 0000 7465 7374 3000 0000   est.....test0...
   
   ```
   
   dump 解析数据插入的日志，记录日志版本、当前日志 LSN 、上一条日志 LSN 、日志长度 Length 、日志类型 Type 、集合空间名 FullName、插入记录等
   
   ```lang-text
   Version: 0x00000001(1)
    LSN    : 0x0000000000000120(288)
    PreLSN : 0x00000000000000a4(164)
    Length : 112
    Type   : INSERT(1)
    FullName : sample.employee
    Insert : { "_id": { "$oid": "5c9496946e610774ac9a1c65" }, "a": 0, "no": 0, "test": "test0" }
   
   0x00000000019EE900 : 9001 0000 0000 0000 2001 0000 0000 0000   ........ .......
   0x00000000019EE910 : 7000 0000 0100 0000 0100 0000 0000 0000   p...............
   0x00000000019EE920 : C910 0000 0073 616D 706C 652E 656D 706C   .....sample.empl
   0x00000000019EE930 : 6F79 6565 0001 3500 0000 3500 0000 075F   oyee..5...5...._
   0x00000000019EE940 : 6964 005C 9496 946E 6107 74AC 9A1C 6610   id.\...na.t...f.
   0x00000000019EE950 : 6100 0100 0000 106E 6F00 0100 0000 0274   a......no......t
   0x00000000019EE960 : 6573 7400 0600 0000 7465 7374 3100 0000   est.....test1...
   ```

* 指定日志文件目录 `/opt/sequoiadb/database/data/11860/replicalog/`，指定只解析类型为 2（数据更新）的日志，输出到当前目录 `update.log`

   ```lang-bash
   $ sdbdpsdump -s /opt/sequoiadb/database/data/11860/replicalog/ -o update.log -t 2
   ```

   dump 解析数据更新的日志，记录日志版本、当前日志 LSN 、上一条日志 LSN 、日志长度 Length 、日志类型 Type 、集合空间名 FullName、更新记录等

   ```lang-text
    Version: 0x00000001(1)
    LSN    : 0x0000000000000580(1408)
    PreLSN : 0x0000000000000510(1296)
    Length : 196
    Type   : UPDATE(2)
    FullName : sample.employee
    Orig id : { "_id": { "$oid": "5c9496946e610774ac9a1c67" } }
    Orig   : { "$unset": { "b": "" } }
    New id  : { "_id": { "$oid": "5c9496946e610774ac9a1c67" } }
    New    : { "$set": { "b": "testupdate" } }
    Old ShardingKey: { "a": 2 }
   
   0x0000000001A73B90 : 4406 0000 0000 0000 8005 0000 0000 0000   D...............
   0x0000000001A73BA0 : CC00 0000 0100 0000 0200 0000 0000 0000   ................
   0x0000000001A73BB0 : C910 0000 0073 616D 706C 652E 656D 706C   .....sample.empl
   0x0000000001A73BC0 : 6F79 6565 0001 1600 0000 1600 0000 075F   oyee..........._
   0x0000000001A73BD0 : 6964 005C 9496 946E 6107 74AC 9A1C 6800   id.\...na.t...h.
   0x0000000001A73BE0 : 0220 0000 0020 0000 0003 2473 6574 0015   . ... ....$set..
   0x0000000001A73BF0 : 0000 0002 7465 7374 0006 0000 0074 6573   ....test.....tes
   0x0000000001A73C00 : 7433 0000 0003 1600 0000 1600 0000 075F   t3............._
   0x0000000001A73C10 : 6964 005C 9496 946E 6107 74AC 9A1C 6800   id.\...na.t...h.
   0x0000000001A73C20 : 0425 0000 0025 0000 0003 2473 6574 001A   .%...%....$set..
   0x0000000001A73C30 : 0000 0002 7465 7374 000B 0000 0074 6573   ....test.....tes
   0x0000000001A73C40 : 7475 7064 6174 6500 0000 050C 0000 000C   tupdate.........
   0x0000000001A73C50 : 0000 0010 6100 0300 0000 0000             ....a.......
   ```

* 指定日志文件目录 `/opt/sequoiadb/database/data/11860/replicalog/`，解析指定集合 sample.employee 中所有操作类型日志，输出到当前目录 `out.log`

   ```lang-bash
   $ sdbdpsdump -s /opt/sequoiadb/database/data/11860/replicalog/ -o out.log -n sample.employee
   ```

   集合 sample.employee 中执行插入、更新、删除数据、创建索引等操作，dump 解析该集合日志内容如下，包括数据插入、更新、删除、创建索引等

   ```lang-text
   Version: 0x00000018(24)
    LSN    : 0x00000010b305055c(71722927452)
    PreLSN : 0x00000010b3050504(71722927364)
    Length : 88
    Type   : INSERT(1)
    FullName : sample.employee
    Insert : { "_id": { "$oid": "5cc69c950fb91f653845a92d" }, "a": 9 }
   
   0x0000000000CCA7E0 : B405 05B3 1000 0000 5C05 05B3 1000 0000   ........\.......
   0x0000000000CCA7F0 : 5800 0000 1800 0000 0300 0000 0000 0000   X...............
   0x0000000000CCA800 : C910 0000 0073 616D 706C 652E 656D 706C   .....sample.empl
   0x0000000000CCA810 : 6F79 6565 0001 1D00 0000 1D00 0000 075F   oyee..........._
   0x0000000000CCA820 : 6964 005C C69C 950F B91F 6538 45A9 2610   id.\......e8E.&.
   0x0000000000CCA830 : 6100 0200 0000 0000                       a.......
   
   
    Version: 0x00000018(24)
    LSN    : 0x00000010b30505b4(71722927540)
    PreLSN : 0x00000010b305055c(71722927452)
    Length : 88
    Type   : DELETE(3)
    CLName : sample.employee
    Orig   : { "_id": { "$oid": "5cc69c950fb91f653845a926" }, "a": 2 }
   
   0x0000000000CCA7E0 : 0C06 05B3 1000 0000 B405 05B3 1000 0000   ................
   0x0000000000CCA7F0 : AC00 0000 1800 0000 0200 0000 0000 0000   ................
   0x0000000000CCA800 : C910 0000 0073 616D 706C 652E 656D 706C   .....sample.empl
   0x0000000000CCA810 : 6F79 6565 0001 1600 0000 1600 0000 075F   oyee..........._
   0x0000000000CCA820 : 6964 005C C69C 950F B91F 6538 45A9 2500   id.\......e8E.%.
   0x0000000000CCA830 : 021A 0000 001A 0000 0003 2475 6E73 6574   ..........$unset
   0x0000000000CCA840 : 000D 0000 0002 6300 0100 0000 0000 0003   ......c.........
   0x0000000000CCA850 : 1600 0000 1600 0000 075F 6964 005C C69C   ........._id.\..
   0x0000000000CCA860 : 950F B91F 6538 45A9 2500 041C 0000 001C   ....e8E.%.......
   0x0000000000CCA870 : 0000 0003 2473 6574 0011 0000 0002 6300   ....$set......c.
   0x0000000000CCA880 : 0500 0000 7465 7374 0000 0000             ....test....
   
   
    Version: 0x00000018(24)
    LSN    : 0x00000010b305060c(71722927628)
    PreLSN : 0x00000010b30505b4(71722927540)
    Length : 172
    Type   : UPDATE(2)
    FullName : sample.employee
    Orig id : { "_id": { "$oid": "5cc69c950fb91f653845a925" } }
    Orig   : { "$unset": { "c": "" } }
    New id  : { "_id": { "$oid": "5cc69c950fb91f653845a925" } }
    New    : { "$set": { "c": "test" } }
   
   0x0000000000CCA7E0 : B806 05B3 1000 0000 0C06 05B3 1000 0000   ................
   0x0000000000CCA7F0 : 8C00 0000 1800 0000 0800 0000 0000 0000   ................
   0x0000000000CCA800 : C910 0000 0073 616D 706C 652E 656D 706C   .....sample.empl
   0x0000000000CCA810 : 6F79 6565 0001 4F00 0000 4F00 0000 075F   oyee..O...O...._
   0x0000000000CCA820 : 6964 005C C69C CDEB A8A8 886D F90D 6E03   id.\.......m..n.
   0x0000000000CCA830 : 6B65 7900 0C00 0000 1061 0001 0000 0000   key......a......
   0x0000000000CCA840 : 026E 616D 6500 0A00 0000 7465 7374 696E   .name.....testin
   0x0000000000CCA850 : 6465 7800 0875 6E69 7175 6500 0008 656E   dex..unique...en
   0x0000000000CCA860 : 666F 7263 6564 0000 0000 0000             forced......
   
   
    Version: 0x00000018(24)
    LSN    : 0x00000010b30506b8(71722927800)
    PreLSN : 0x00000010b305060c(71722927628)
    Length : 140
    Type   : IX CREATE(8)
    CLName : sample.employee
    IXDef  : { "_id": { "$oid": "5cc69ccdeba8a8886df90d6e" }, "key": { "a": 1 }, "name": "testindex", "unique": false, "enforced": false }
   ```

[^_^]:
    TODO：replicalog同步日志如果确认统一更新为事务日志，后面需要更新为事务日志并补充对应链接