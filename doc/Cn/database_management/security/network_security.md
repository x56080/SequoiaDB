SequoiaDB 通过配置可以支持 SSL。SequoiaDB 客户端和 SequoiaDB 实例直接可以使用 SSL 加密连接。

##SequoiaDB 支持##

若要使用 SSL 加密连接，需要 SequoiaDB 1.12 或之后的版本。

目前该特性仅限于 SequoiaDB 企业版，社区版暂不支持。

##客户端支持##

所有官方支持的客户端驱动都支持 SSL，包括 C、C++、Java、Python、C#、PHP、REST API 及 SDB Shell。

##配置 SequoiaDB 使用 SSL##

在安装部署时，通过配置参数开启 SequoiaDB 对 SSL 连接的支持：

--usessl，默认值为 false，设为 true 时开启 SSL，允许客户端通过 SSL 加密连接，同时仍然接受非 SSL 加密连接。

参考如下：

<pre class="prettyprint lang-diy">
sequoiadb --usessl=true</pre>

也支持使用配置文件配置，参见[数据库配置](SdbDoc_Cn/database_management/runtime_configuration.html)。

SequoiaDB 在开启 SSL 后会自动创建证书，不需要用户指定。

##客户端使用 SSL##

客户端必须与开启 SSL 的 SequoiaDB 配合才能使用 SSL加密连接。所有官方支持的客户端驱动都支持 SSL。

-   C

    C 驱动接口使用 sdbSecureConnect() 和 sdbSecureConnect1() 建立 SSL连接，使用方式与 sdbConnect() 和 sdbConnect1() 相同。

    参见 [C 驱动](SdbDoc_Cn/driver/c_driver/c_driver.html)。

-   C++

    C++ 驱动中类 sdb 的构造函数有参数 useSSL，设为 true 时使用 SSL 连接。

    参见 [C++驱动](SdbDoc_Cn/driver/cpp_driver/cpp_driver.html)。

-   Java

    Java 驱动中类 com.sequoiadb.net.ConfigOptions 有接口 setUseSSL(boolean useSSL)，设为 true 时使用 SSL 连接。

    参见 [Java驱动](SdbDoc_Cn/driver/java_driver/java_driver.html)。

-   Python

    Python 驱动中类 client 构造函数有可选参数 ssl，设为 true 时使用 SSL连接。

    参见 [Python驱动](SdbDoc_Cn/driver/python_driver/python_driver.html)。

-   C#

    C# 驱动中类 SequoiaDB.ConfigOptions 有属性 UseSSL，设为 true 时使用SSL 连接。

    参见 [C#驱动](SdbDoc_Cn/driver/csharp_driver/csharp_driver.html)。

-   PHP

    PHP 驱动中有类 SecureSdb，该类是 SequoiaDB 的子类，类 SecureSdb的对象使用 SSL 连接。

    参见 [PHP驱动](SdbDoc_Cn/driver/php_driver/php_driver.html)。

-   REST API

    REST API 支持 https。

    参见 [REST接口](SdbDoc_Cn/driver/rest/overview.html)。

-   sdb shell

    sdb shell 中共有类 SecureSdb，该类是 Sdb 的子类，类 SecureSdb的对象使用 SSL 连接。


##工具支持##

sdbexprt、sdbimprt、sdblobtool、sdbtop 支持 SSL连接。

参见：[sdbexprt](SdbDoc_Cn/database_management/tools/data_migration_tool.html),[sdbimprt](SdbDoc_Cn/database_management/tools/data_migration_tool.html),[sdblobtool](SdbDoc_Cn/database_management/tools/sdblob.html),[sdbtop](SdbDoc_Cn/database_management/tools/sdbtop.html)。
