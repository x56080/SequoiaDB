## 名称

createNode - 在当前复制组中创建节点

## 语法

**rg.createNode( \<host\>, \<service\>, \<dbpath\>, \[config] )**

## 类别

SdbReplicaGroup

## 描述

在当前复制组中创建节点。

## 参数

| 参数名 | 参数类型 | 描述 | 是否必填 |
| ------ | -------- | ---- | -------- |
| host    | string | 指定节点的主机名 | 是 |
| service | int/string | 节点端口号   | 是 |
| dbpath  | string | 1. 数据文件路径，用于存放节点数据文件，请确保数据管理员（安装时创建，默认为sdbadmin）用户有写权限； <br> 2. 如果配置路径不以“/”开头，数据文件存放路径将是数据库管理员用户（默认为sdbadmin）的主目录（默认为/home/sequoiadb）+ 配置的路径。 | 是 |
| config  | Json 对象 |  节点配置信息，如配置日志大小，是否打开事务等，具体可参考[数据库配置][cluster_config]。 | 否 |

> **Note:**  
> rg.createNode() 方法的定义格式有四个参数：host，service，dbpath，config，如上表所示，host，dbpath 为字符串类型，Service 类型支持 int 或 string ，必填；最后一个是 Json 对象，选填。  
> 格式：( "<主机名>", "<端口号>", "<节点路径>, "[ { <configParam>: value, ... } ] )

## 返回值

无返回值，出错抛异常，并输出错误信息。可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息，通过 [getLastError()][getLastError] 获取错误码。关于错误处理可以参考[常见错误处理指南][faq]。

## 错误

错误信息记录在节点诊断日志（diaglog）中，可参考[错误码][error_code]。

| 错误码 | 可能的原因 | 解决方法 |
| -------- | ---------------------- | ------------------------------- |
| -15      | 网络错误     | 1. 检查 sdbcm 状态是否正常，如果状态异常，可以尝试重启；<br> 2. 检查 host 是否正确，网络是否能正常通信。 |
| -145     | 节点已存在   | 检查节点是否存在。 |
| -157     | 节点配置冲突 | 执行 netstat 命令检查节点端口是否已被占用。 |
| -3       | 权限错误     | 检查节点路径是否正确，路径权限是否正确。 |

## 版本

v2.0 及以上版本

## 示例

在复制组 group1 中创建一个 "hostname1:11830" 的节点，指定日志文件大小为64MB

```lang-javascript
> var rg = db.getRG("group1")
> rg.createNode( "hostname1", 11830, "/opt/sequoiadb/database/data/11830", { logfilesz: 64 } )
```

> **Note:**  
> 一个复制组中能创建多个节点，每个节点需要预留至少五个顺延的端口。因为系统为每个节点后台控制了五个通信接口。


[^_^]:
    本文使用的所有引用及链接
[cluster_config]:manual/Manual/Database_Configuration/configuration_parameters.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[faq]:manual/faq.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
