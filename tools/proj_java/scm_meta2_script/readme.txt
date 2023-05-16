/**
 * SCM 元数据修复工具（阶段二）Readme
 * 修改：
 * 2023/5/11, Init
 */

一、工具介绍
SCM 元数据修复工具（阶段二）用于修复如下几个问题：
1）将 SCM 元数据表无效的 site_list 数组元素剔除
2）将 LOB 表的信息同步回 site_list 数组
3）将 LOB 表 状态 为 "Available: false" 的 LOB 打印到日志文件
注意：该工具只对阶段一输出的 update_succ 及 empty_site 文件涉及的 元数据 进行检测与修复，不会对整个集群的元数据进行检测与修复


二、工具结构目录
--+ readme.txt
--+ scm_meta2-xxx.jar
--+ bin
------+ scm_meta2_create_table.js
------+ scm_meta2_import_json.sh
------+ scm_meta2_export_json.sh


三、数据修复操作步骤

前提假设：
1）安装路径为 /opt/sequoiadb
2）SCM 元素表所在生产集群地址为：192.168.17.37:50000
3）LOB 表所在生产集群的地址由 scm.sites 文件提供
4）测试集群地址为：192.168.30.35:11810


操作准备：
1）创建一个用于工作的目录，假设为 /opt/workplace
2）将阶段二工具解压，并拷贝到 /opt/workplace
3）创建 /opt/workplace/out 目录，用于存放工具、脚本输出文件
4）将阶段一输出的 IBSFLOW_META.FILE_2021.update_succ 及 IBSFLOW_META.FILE_2021.empty_site 文件拷贝到 /opt/workplace
5）准备 scm.sites 及 IBSFLOW_META.FILE_2021.lob_tables 文件（其详情内容请参考开发文档）
6）准备一个测试环境，三组三节点，创建一个 名字叫 "domain3" 的域，包含这三个数据组


步骤一：连接生产集群，导出 SCM 元数据的 json 数据
要求：
1）提前创建由 -o 指定的输出目录

运行命令：
java -jar scm_meta2-xxx.jar -a exportmeta -c IBSFLOW_META.FILE_2021 --inputfile IBSFLOW_META.FILE_2021.update_succ,IBSFLOW_META.FILE_2021.empty_site -o out --host 192.168.17.37:50000 -u "abc" -w "abc"


步骤二：连接生产集群，导出 SCM LOB 信息的 json 数据
要求：
1）提前创建由 -o 指定的输出目录
2）不需要指定 --host 参数，地址将从 scm.sites 文件读取
3）IBSFLOW_META.FILE_2021.lob_tables 内容要准确，表不要多，也不要少

运行命令：
java -jar scm_meta2-xxx.jar -a exportlobinfo -c IBSFLOW_META.FILE_2021 --inputfile scm.sites,IBSFLOW_META.FILE_2021.lob_tables -o out -u "abc" -w "abc"


步骤三：在测试集群创建表及索引
要求：
1）需要提前准备一个叫 "domain3" 的域，其需要包含 3 个 groups
2）如果需要调整域的情况，请直接修改 scm_meta2_create_table.js 脚本

运行命令：
/opt/sequoiadb/bin/sdb -e ' var HOST = "192.168.30.35:11810"; var USER = "abc"; var PASSWD = "abc"; var METATABLE = "IBSFLOW_META.FILE_2021"; var FORCE = true;' -f bin/scm_meta2_create_table.js


步骤四：将 meta.json 与 lobinfo.json 文件 导入测试集群
要求：
1）-t 指定的类型需要与 -f 指定文件的后缀匹配

运行命令：
bin/scm_meta2_import_json.sh -s 192.168.30.35:11810 -u "abc" -w "abc" -t meta -f out/IBSFLOW_META.FILE_2021.meta.json

bin/scm_meta2_import_json.sh -s 192.168.30.35:11810 -u "abc" -w "abc" -t lobinfo -f out/IBSFLOW_META.FILE_2021.lobinfo.json


步骤五：在测试集群上，进行数据检测、修复
要求：
1）提前创建由 -o 指定的输出目录

运行命令：
java -jar scm_meta2-xxx.jar -a repair -c IBSFLOW_META.FILE_2021 -o out --host 192.168.17.37:50000 -u "abc" -w "abc" -j 10 -b 10000


步骤六：在测试集群上，将修复的元数据导出为 json 文件
要求：
1）提前创建由 -o 指定的输出目录

运行命令：
bin/scm_meta2_export_json.sh -s 192.168.30.35:11810 -u "abc" -w "abc" -c IBSFLOW_META.FILE_2021 -o out


步骤七：检查输出目录的 no_lob 及 invalid_lob 文件是否存在 oid，如果存在请与开发联系


步骤八：将 update.json 文件的元数据更新回生产集群
要求：
1）提前创建由 -o 指定的输出目录

运行命令：
java -jar scm_meta2-xxx.jar -a update -c IBSFLOW_META.FILE_2021 --inputfile out/IBSFLOW_META.FILE_2021.update.json -o out -u "abc" -w "abc" --host 192.168.17.37:50000


步骤九：检查输出目录的 update.update_fail 文件是否存在 oid，如果存在请与开发联系


步骤十：检查日志文件 scm_meta2.log 是否存在如下几种情况。若存在，请与开发联系
1）Lob status is unavailable
2）No record return for oid
3）非预期的 ERROR 日志
4）非预期的 WARN 日志
备注："非预期"指得是前线人员不确定是否有问题


四、修复后数据正确性检查步骤

步骤一：登录测试集群，将测试集群的 IBSFLOW_META_test 集合空间改名

> db.renameCS("IBSFLOW_META_test", "IBSFLOW_META_test_bak")


步骤二：重复 "数据修复操作步骤" 步骤一，导出修改后的 SCM 元数据的 json 文件
要求：
需要重新创建一个新的目录，把内容输出到新的目录

运行命令：
mkdir out2
java -jar scm_meta2-xxx.jar -a exportmeta -c IBSFLOW_META.FILE_2021 --inputfile IBSFLOW_META.FILE_2021.update_succ,IBSFLOW_META.FILE_2021.empty_site -o out2 --host 192.168.17.37:50000 -u "abc" -w "abc"


步骤三：重复 "数据修复操作步骤" 步骤三，在测试集群上重新创建表及索引


步骤四：将新的 meta.json 与 原来的 lobinfo.json 文件 导入测试集群
注意：meta 数据要指定 out2 目录的

运行命令：
bin/scm_meta2_import_json.sh -s 192.168.30.35:11810 -u "abc" -w "abc" -t meta -f out2/IBSFLOW_META.FILE_2021.meta.json

bin/scm_meta2_import_json.sh -s 192.168.30.35:11810 -u "abc" -w "abc" -t lobinfo -f out/IBSFLOW_META.FILE_2021.lobinfo.json


步骤五：在测试集群上，进行数据检测、修复
要求：
1）输出文件指定到新的 out2 目录

运行命令：
java -jar scm_meta2-xxx.jar -a repair -c IBSFLOW_META.FILE_2021 -o out2 --host 192.168.17.37:50000 -u "abc" -w "abc" -j 10 -b 10000


步骤六：登录 测试集群，对 IBSFLOW_META_test.FILE_2021_update 表记录数进行检查，预期返回 0 条
> db.IBSFLOW_META_test.FILE_2021_update.count()