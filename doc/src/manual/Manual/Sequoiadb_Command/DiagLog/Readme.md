DiagLog 类主要用于集群日志搜索、下载和分析，包含的函数如下：

| 名称 | 描述 |
|------|------|
| [DiagLog()][DiagLog] | 集群日志对象 |
| [search()][search] | 设置运行模式为 search，在集群日志中搜索相关内容 |
| [collect()][collect] | 设置运行模式为 collect，在集群日志中搜索相关内容，并把涉及的日志文件收集到本地 |
| [analyze()][analyze] | 设置运行模式为 analyze，分析收集回来的集群日志，分析维度包含错误出现次数，错误出现时间 |
| [lastFile()][lastFile] | 设置 search() 仅搜索最近的日志文件 |
| [lastest()][lastest] | 设置 search() 仅搜索最近一定时间的日志 |
| [timeBegin()][timeBegin] | 设置 search() 中搜索日志的起始时间 |
| [timeEnd()][timeEnd] | 设置 search() 中搜索日志的结束时间 |
| [error()][error] | 设置 search() 中搜索的错误码 |
| [diaglevel()][diaglevel] | 设置 search() 中过滤的日志级别 |
| [keypattern()][keypattern] | 设置 search() 中搜索的关键字 |
| [pid()][pid] | 设置 search() 中搜索的 pid |
| [tid()][tid] | 设置 search() 中搜索的 tid |
| [after()][after] | 设置 search() 结果上下文的下文条数 |
| [before()][before] | 设置 search() 结果上下文的上文条数 |
| [limit()][limit] | 限制 search() 返回的结果条数 |
| [original()][original] | 设置 search() 返回的结果为原始日志格式 |
| [output()][output] | 设置 search() 和 analyze() 的输出路径 |
| [path()][path] | 设置 collect() 的输出路径、search() 和 analyze() 的读取路径 |
| [snapshot()][snapshot] | 设置 collect() 收集 snapshot 快照 |
| [core()][core] | 设置 collect() 收集 core 文件 |
| [trap()][trap] | 设置 collect() 收集 trap 文件 |
| [all()][all] | 设置 collect() 收集 trap、core 文件和所有的 snapshot |
| [compress()][compress] | 设置 collect() 的压缩方式 |
| [next()][next] | 展示 search() 搜索的日志结果 |
| [close()][close] | 关闭 DiagLog 对象打开的文件 |
| [run()][run] | 以当前设置的参数运行 |
| [reset()][reset] | 重置 DiagLog 对象中的参数 |
| [conn()][conn] | 设置 DiagLog 对象中 Sdb 连接 |

[^_^]:
     本文使用的所有引用及链接
[DiagLog]:manual/Manual/Sequoiadb_Command/DiagLog/DiagLog.md
[search]:manual/Manual/Sequoiadb_Command/DiagLog/search.md
[collect]:manual/Manual/Sequoiadb_Command/DiagLog/collect.md
[analyze]:manual/Manual/Sequoiadb_Command/DiagLog/analyze.md
[lastFile]:manual/Manual/Sequoiadb_Command/DiagLog/lastFile.md
[lastest]:manual/Manual/Sequoiadb_Command/DiagLog/lastest.md
[timeBegin]:manual/Manual/Sequoiadb_Command/DiagLog/timeBegin.md
[timeEnd]:manual/Manual/Sequoiadb_Command/DiagLog/timeEnd.md
[error]:manual/Manual/Sequoiadb_Command/DiagLog/error.md
[diaglevel]:manual/Manual/Sequoiadb_Command/DiagLog/diaglevel.md
[keypattern]:manual/Manual/Sequoiadb_Command/DiagLog/keypattern.md
[pid]:manual/Manual/Sequoiadb_Command/DiagLog/pid.md
[tid]:manual/Manual/Sequoiadb_Command/DiagLog/tid.md
[after]:manual/Manual/Sequoiadb_Command/DiagLog/after.md
[before]:manual/Manual/Sequoiadb_Command/DiagLog/before.md
[limit]:manual/Manual/Sequoiadb_Command/DiagLog/limit.md
[original]:manual/Manual/Sequoiadb_Command/DiagLog/original.md
[output]:manual/Manual/Sequoiadb_Command/DiagLog/output.md
[path]:manual/Manual/Sequoiadb_Command/DiagLog/path.md
[snapshot]:manual/Manual/Sequoiadb_Command/DiagLog/snapshot.md
[trap]:manual/Manual/Sequoiadb_Command/DiagLog/trap.md
[core]:manual/Manual/Sequoiadb_Command/DiagLog/core.md
[all]:manual/Manual/Sequoiadb_Command/DiagLog/all.md
[compress]:manual/Manual/Sequoiadb_Command/DiagLog/compress.md
[next]:manual/Manual/Sequoiadb_Command/DiagLog/next.md
[close]:manual/Manual/Sequoiadb_Command/DiagLog/close.md
[run]:manual/Manual/Sequoiadb_Command/DiagLog/run.md
[reset]:manual/Manual/Sequoiadb_Command/DiagLog/reset.md
[reset]:manual/Manual/Sequoiadb_Command/DiagLog/conn.md
