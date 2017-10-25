##获取驱动开发包##

从 [SequoiaDB](http://download.sequoiadb.com/cn/index-cat_id-2) 下载对应操作系统版本的 SequoiaDB 驱动开发包；解压驱动开发包，从 driver/CSharp/ 目录中获取 sequoiadb.dll 链接库，然后，在 Visual Studio 中引用该链接库，或者在命令行编译时指定引用该链接库，比如“csc /target:exe /reference:sequoiadb.dll Find.cs Common.cs”，即可使用相关 API。在安装目录下的 smaples\\C# 目录可以找到 CSharp 驱动的完整示例。

##BSON 库 API##

SequoiaDB 数据库的 CSharp 驱动使用了第三方公司 MongoDB 提供的 CSharp BSON 库，详细介绍可以参照 [MongoDB 官方文档](http://docs.mongodb.org/ecosystem/tutorial/use-csharp-driver/#the-bson-library)

##Visual Studio 版本支持##

当前版本的 CSharp 驱动可在以下版本的 Visual Studio 中使用

-   Visual Studio 2010

##.NET Framework 版本支持##

当前版本的 CSharp 驱动在 .NET Framework4.0 中生成，可在以下版本的 .NET Framework 中使用

-   .NET Framework 4.0
