`stpstart` 是一个用于启动 STP 节点的工具。

> **Note:**
>
> * 也可以通过 `stp --daemon` 来启动 STP 节点，请参考[后台模式](database_management/stp/tools/stp.md#后台模式)

##权限需求##

无

##连接需求##

无

##选项##

| 参数 | 缩写 | 描述 |
| ---- | ---- | ---- |
| --help | -h | 返回 `stpstart` 的用法和帮助 |
| --version | | 返回 `stpstart` 的版本信息 |
| --confpath | -c | 指定 STP 的配置目录 |
| --options | | 指定启动 STP 时额外的配置项 |

##用法##

- 通过 `stpstart` 启动 STP 节点

```
bin/stpstart
```

- 通过 `stpstart` 使用 conf/stp 下的配置启动 STP 节点

```
bin/stpstart --confpath conf/stp
```

- 通过 `stpstart` 启动 STP 节点，并且额外配置 diaglevel 为 5 （DEBUG 级别）

```
bin/stpstart --options "--diaglevel=5"
```
