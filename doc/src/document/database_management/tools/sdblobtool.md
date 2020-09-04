sdblobtool 是 SequoiaDB 集合大对象管理工具。

##功能列表##

设置 sdblobtool 的 --operation 选项可以使用不同功能：

| 设置      | 功能 | 描述                                   |
| --------- | ---- | -------------------------------------- |
| export    | 导出 | 将集合中的大对象导出至本地文件         |
| import    | 导入 | 将本地文件中的大对象导入至集合         |
| migration | 迁移 | 将一个集合中的大对象复制到另一个集合中 |

##选项##

**导出选项**

| 名称         | 描述                        | 默认值    | 是否必填 |
| ------------ | --------------------------- | --------- | -------- |
| --hostname   | 指定协调节点（Coord）地址   | localhost | 否       |
| --svcname    | 指定协调节点（Coord）服务名 | 11810     | 否       |
| --usrname    | 指定数据库用户              |           | 否       |
| --passwd     | 指定数据库用户密码，如果不使用该参数指定密码，工具会通过交互式界面提示用户输入密码 |              | 否       |
| --cipher     | 是否使用密文模式输入密码。关于密文模式的介绍，详细可参考[密码管理](database_management/security/system_security.md) |   false，不使用密文模式输入密码 | 否       |
| --token      | 指定加密令牌                |           | 否       |
| --cipherfile | 指定密文文件路径            | `~/sequoiadb/passwd` | 否       |
| --operation  | 指定操作类型                |           | 是，设置为 export |
| --file       | 指定本地文件全路径          |           | 是       |
| --collection | 需要导出的集合全名          |           | 是       |
| --prefer     | 指定优先选择的实例          | M         | 否       |
| --ssl        | 是否使用 SSL 连接           | false，不使用 SSL 连接 | 否       |
  
>   **Note:**
>
>   *   --prefer 的取值 m 或 M 指 master，s 或 S 表示 slave，a 或 A 表示 anyone，1 ~ 7 表示 node1 ~ node7，默认值为 M。

**导入选项**

| 名称         | 描述                        | 默认值    | 是否必填 |
| ------------ | --------------------------- | --------- | -------- |
| --hostname   | 指定协调节点（Coord）地址   | localhost | 否       |
| --svcname    | 指定协调节点（Coord）服务名 | 11810     | 否       |
| --usrname    | 指定数据库用户              |           | 否       |
| --passwd     | 指定数据库用户密码，如果不使用该参数指定密码，工具会通过交互式界面提示用户输入密码 |           | 否       |
| --cipher     | 是否使用密文模式输入密码。关于密文模式的介绍，详细可参考[密码管理](database_management/security/system_security.md) |   false，不使用密文模式输入密码 | 否       |
| --token      | 指定加密令牌                |           | 否       |
| --cipherfile | 指定密文文件路径            | `~/sequoiadb/passwd` | 否       |
| --operation  | 指定操作类型                |           | 是，设置为 import |
| --file       | 指定本地文件全路径          |           | 是       |
| --collection | 指定需要导入的集合全名      |           | 是       |
| --ignorefe   | 当前大对象如果已经存在于集合中，是否忽略这个错误并开始导入下一条记录 | false | 否 |
| --ssl        | 是否使用 SSL 连接        | false，不使用 SSL 连接 | 否       |

>   **Note:**
>
>   *   当需要开启 --ignorefe 时只需要添加 --ignorefe 即可，不需要为其制定具体值。下同。
>   *   本地文件必须为导出生成的文件。

**迁移选项**

| 名称            | 描述                            | 默认值    | 是否必填 |
| --------------- | ------------------------------- | --------- | -------- |
| --hostname      | 指定协调节点（Coord）地址       | localhost | 否       |
| --svcname       | 指定协调节点（Coord）服务名     | 11810     | 否       |
| --usrname       | 指定数据库用户                   |           | 否       |
| --passwd        | 指定数据库用户密码，如果不使用该参数指定密码，工具会通过交互式界面提示用户输入密码 |           | 否       |
| --cipher        | 是否使用密文模式输入密码。关于密文模式的介绍，详细可参考[密码管理](database_management/security/system_security.md) |   false，不使用密文模式输入密码 | 否       |
| --token         | 指定加密令牌                 |           | 否       |
| --cipherfile    | 指定密文文件路径             | `~/sequoiadb/passwd` | 否       |
| --operation     | 指定操作类型                     |           | 是，设置为 migration |
| --file          | 指定本地文件全路径               |           | 是       |
| --collection    | 指定需要导出的集合全名           |           | 是       |
| --dsthost       | 指定目标协调节点（Coord） 地址   | localhost | 否       |
| --dstservice    | 指定目标协调节点（Coord） 服务名 | 11810     | 否       |
| --dstusrname    | 指定目标用户名                   |           | 否       |
| --dstpasswd     | 指定目标密码                     |           | 否       |
| --dstcollection | 需要导入的目标集合全名       |           | 是       |
| --ignorefe      | 当前大对象如果已经存在于集合中，是否忽略这个错误并开始导入下一条记录 | false | 否 |
| --ssl           | 是否使用 SSL 连接            | false，不使用 SSL 连接 | 否       |

##日志##

使用 sdblobtool 时会在用户的当前运行目录产生日志文件 `sdblobtool.log`，当发生错误时可以在日志中查看详细信息。

##常见错误##

| 错误码（rc） | 描述 | 应对措施 |
| ------------ | ---- | -------- |
| -5           | 本地文件已存在或者集合中存在相同 OID 的大对象 | - 如果是导出操作，检查本地文件是否已经存在<br> - 如果是导入或者迁移操作，检查目标集合中是否已存在相同 OID 的大对象 |
| -6           | 参数不合法 | 检查参数是否输入错误 |
| -10          | 系统错误 | 需要根据日志进行错误排查 |
| -15          | 无法连接到指定地址 | - 检查地址相关参数是否填写正确<br> - 检查数据库是否正常启动<br>- 如果使用主机名作为参数，检查本地主机名列表是否配置正确<br> - 检查防火墙是否开启 |
| -23          | 集合不存在 | - 检查集合相关参数是否填写正确<br> - 检查相关集合是否存在 |

##示例##

* 将集合 SAMPLE.EMPLOYEE 中的大对象导出至本地文件 `mylob` 中

   ```lang-bash
   $ ./sdblobtool --operation export --hostname hostname1 --svcname 11810 --collection SAMPLE.EMPLOYEE --file /opt/mylob
   ```

* 将本地文件 `mylob` 中的大对象导入至集合 SAMPLE.EMPLOYEE 中，当遇到已存在的大对象时直接跳过

   ```lang-bash
   $ ./sdblobtool --operation import --hostname hostname1 --svcname 11810 --collection SAMPLE.EMPLOYEE --file /opt/mylob --ignorefe
   ```

* 将集合中的大对象复制到另一个集合中

   ```lang-bash
   $ ./sdblobtool --operation migration --hostname hostname1 --svcname 11810 --collection SAMPLE.EMPLOYEE --dsthost hostname2 --dstservice 11810 --dstcollection SAMPLE.EMPLOYEE
   ```
