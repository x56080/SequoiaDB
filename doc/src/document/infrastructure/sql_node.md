##概念##

  SQL节点是系统提供SQL访问能力的逻辑节点，兼容所有标准SQL2003语法，并且完全兼容PostgrepSQL语法。

  SQL节点将接收的外部请求进行SQL解析，生成内部的执行计划，将执行计划下发至协调节点，并汇总协调节点的应答进行外部响应。

  SQL节点支持水平伸缩，节点相互独立，一次外部请求只能在一个SQL节点内完成，因此，可以根据外部应用的压力来规划SQL节点的规模。

  SQL节点需要进行一定的配置，才可以对接至指定的 DB 引擎。

##操作##

  > **Note：**  
  > 在进行下列操作前，请确保 SequoiaSQL 已经安装，并将当前目录切换至 SequoiaSQL 安装的根目录。

- 创建SQL节点

  请参考[创建SQL实例](sql_engine/sequoiasql/install/install_deploy.md)  
  TODO

- 启动SQL节点

  TODO

- 查看SQL节点

  TODO

- 配置对接DB引擎

  TODO

- 停止SQL节点

  TODO

- 删除SQL节点

  TODO

