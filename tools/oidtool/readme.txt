一、工具介绍

sdboidtool 工具（后续简称为：工具）用于 检查 及 修复 SEQUOIADBMAINSTREAM-8889 问题单提及的记录 OID 问题。
详细情况请参考相关问题单。

工具由三部分组成：

* 执行程序
--
 |--- sdboidtool.js
 |--- conf.js
 |--- lib
 |--- bin

* 输出目录
--
 |--- result

* 日志目录
--
 |--- log

二、工具使用

1. 条件说明

使用当前工具前，客户需要确保集群所有业务都已经将 java 驱动升级到 v3.4.9/v3.6.2/v5.0.4 及以上版本

2. 执行步骤

处理该 OID　问题过程，需要执行如下步骤：

步骤一：解压 oidtool.tar.gz 到 SequoiaDB 安装目录下的 tools 目录。假设 SequoiaDB 安装目录
       为 /opt/sequoiadb 目录，后续所有操作全部基于这个假设
[sdbadmin@p-7605 ~]$ tar -zxf oidtool.tar.gz
[sdbadmin@p-7605 ~]$ mv oidtool /opt/sequoiadb/tools/

步骤二：进入工具工作目录
[sdbadmin@p-7605 ~]$ cd /opt/sequoiadb/tools/oidtool

步骤三：在 conf.js 文件设置集群的信息，及工具的工作时间段

说明：
* 工作时间段设置只对 ACTION 为 "check" 或 "repair" 有效
* 工作时间段应该设置为业务低峰期

步骤四：使用工具，生成 `可疑集合列表` 文件（即：init.result 文件）
[sdbadmin@p-7605 ~]$ /opt/sequoiadb/bin/sdb -e ' var ACTION = "init"; ' -f sdboidtool.js

说明：
* 该步骤只需要成功执行一次。如果集合数为 10w, 该步骤大约在半小时内能够完成
* 成功执行后，将在 result 目录下生成 init.result 文件 及 init.report 文件（该步骤的报告信息）

步骤五：使用工具，对 init.result 文件的集合进行检测，并生成 `待修复集合列表` 文件（即：check.result 文件）
[sdbadmin@p-7605 ~]$ /opt/sequoiadb/bin/sdb -e ' var ACTION = "check"; ' -f sdboidtool.js

说明：
* 该步骤将在指定的工作时间内，逐行检测 init.result 文件记录的集合，并将成功检测的集合
  写入 check.result 文件
* 如果在工作时间内无法检测完毕所有集合，可在 conf.js 重新设置工作时间段，然后重复执当前步骤，
  工具会跳过已经完成检测的集合
* 检测结果会写入报告文件（check.report）
* 若 check.report 显示 "Has check all collections in init.result file: true",
  说明该步骤已经完成所有可疑集合的检测。此时，如果 check.result 文件包含内容，需要进行 步骤五 进行
  数据修复

步骤六（可选）：使用工具，对 check.result 文件的集合进行修复，并生成 `完成修复集合列表` 文件（即：repair.result 文件）
[sdbadmin@p-7605 ~]$ /opt/sequoiadb/bin/sdb -e ' var ACTION = "repair"; ' -f sdboidtool.js

说明：
* 在该步骤修复集合过程，要求集合 LSN 不能改变（即不能进行 写操作）。否则该集合将修复失败
* 在该步骤修复集合过程，会发生集合 rename, 会导致集合有若干秒短暂的不可访问时间间隙。
  这间隙期间，外部的查询将获取 -23（集合不存在）的报错
* 该步骤将在指定的工作时间内，逐行修复 check.result 文件记录的集合，并将成功修复的集合
  写入 repair.result 文件
* 如果在工作时间内无法检测完毕所有集合，可在 conf.js 重新设置工作时间段，然后重复执当前步骤，
  工具会跳过已经完成修复的集合
* 检测结果会写入报告文件（repair.report）
* 若 repair.report 显示 "Has repaired all collections in check.result file: true",
  说明该步骤已经完成所有集合的修复
* 该步骤将不处理以下这几类集合：
  1）含 lob 的集合
  2）含自增字段的集合
  3）含数据源的集合
  4）含全文索引的集合
  5）不含 $id 索引的集合