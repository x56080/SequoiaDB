sdbpasswd 是一个 SequoiaDB 数据库的密码管理工具。它可以将用户的密码保存到文件，并以此给其他 SequoiaDB 数据库工具提供基于文件的密码输入功能。

##权限需求##

运行 sdbpasswd 命令的用户必须对准备写入密码的加密文件具有读写权限，加密文件的权限设置为 600。

##连接需求##

sdbpasswd 不需要与数据库连接。

##选项##

| 参数        | 缩写 | 描述                                              | 是否必填 |
| ----------- | ---- | ------------------------------------------------- | -------- |
| --help      | -h   | 返回帮助说明                                      |    否    |
| --adduser   | -a   | 增加用户，支持使用 @ 符区分同名用户，格式：user@cluster1，最大值 256 个字符 |  adduser/removeuser 必选其一 |
| --removeuser| -r   | 删除用户                                          | adduser/removeuser 必选其一 |
| --password  | -p   | 用户密码，指定值则使用明文输入，不指定值则命令行提示输入，最大值 256 个字符 | 是 |
| --token     | -t   | 对保存的密码指定加密口令以增强安全性，最大值 256 个字符 | 否 |
| --file      | -f   | 指定加密文件用于密码的保存，默认为 ~/sequoiadb/passwd | 否 |

##用法##

1. 增加用户 sdbadmin，并指定密码为 sdbadmin

 ```lang-bash
 $ sdbpasswd --adduser sdbadmin --password sdbadmin
 ```

2. 增加用户 sdbadmin，并使用命令行提示的方式输入密码

 ```lang-bash
 $ sdbpasswd --adduser sdbadmin --password
 password:
 ```

3. 增加用户 sdbadmin，且通过 @ 符区分从属于不同集群的用户

 ```lang-bash
 $ sdbpasswd --adduser sdbadmin@db1 --password 123456
 $ sdbpasswd --adduser sdbadmin@db2 --password 654321
 ```
 > Note:
 >
 > 数据库工具在使用加密文件方式连接数据库时，会自动去掉 @ 及后面部分，使用原始用户名连接数据库。

4. 增加用户 sdbadmin，指定密码为 sdbadmin，并指定加密口令为 sequoiadb

 ```lang-bash
 $ sdbpasswd --adduser sdbadmin --password sdbadmin --token sequoiadb
 ```

5. 增加用户 sdbadmin，指定密码为 sdbadmin，并指定加密文件位置

 ```lang-bash
 $ sdbpasswd --adduser sdbadmin --password sdbadmin --file ./cipher
 ```

6. 删除用户 sdbadmin@db1

 ```lang-bash
 $ sdbpasswd --removeuser sdbadmin@db1
 ```
 > Note:
 >
 > 删除用户时，对于使用了 @ 符的用户，需要指定全名才能匹配删除。

##和数据库工具配合使用##

通过 sdbpasswd 增加密码后就可以在各个数据库工具使用加密文件的方式输入密码了。

下列工具都是通过指定用户名，再加上打开 cipher 开关来指定使用加密文件，指定的用户名对应通过 sdbpasswd 增加的用户名，对于参数的详细介绍请访问各工具的介绍页面：

|   工具名        | 使用加密文件的参数组合 |                                           
| --------------- | ---------------------- |
| sdbexprt        | --user sdbadmin --cipher true --token sequoiadb --cipherfile ./passwd          |
| sdbimprt        | --user sdbadmin --cipher true --token sequoiadb --cipherfile ./passwd          |
| sdbreplay       | --user sdbadmin --cipher true --token sequoiadb --cipherfile ./passwd          |
| sdbtop          | --usrname sdbadmin --cipher true --token sequoiadb --cipherfile ./passwd          |
| sdblobtool      | --usrname sdbadmin --cipher true --token sequoiadb --cipherfile ./passwd   |
| sdbinspect      | --auth sdbadmin --cipher true --token sequoiadb --cipherfile ./passwd   |
| Sequoiasql-pgsql| user 'sdbadmin', cipher 'on', token 'sequoiadb', cipherfile '/opt/sequoiadb/passwd'   |
 > Note:
 >
 > 在 sdbpasswd 增加密码时指定了 token 的情况下才需指定 --token，在使用了与默认值不同的加密文件路径时才需指定 --cipherfile。

###示例###

####sdbexprt:####

 ```lang-bash
 $ sdbexprt -s localhost -p 11810 --type csv --file foo.bar.csv --fields field1,fieldNotExist,field3 -c foo -l bar --user sdbadmin --cipher true --token sequoiadb --cipherfile ./passwd
 ```

####sdbimprt:####

 ```lang-bash
 $ sdbimprt -s localhost -p 11810 -c foo -l bar --file foo.bar.csv --type csv --headerline true --fields='c int,d string' --user sdbadmin --cipher true --token sequoiadb --cipherfile ./passwd
 ```

####sdbreplay:####

 ```lang-bash
 $ sdbreplay --hostname localhost --svcname 11810 --path 20000/archivelog/archivelog.0 --user sdbadmin --cipher true --token sequoiadb --cipherfile ./passwd
 ```

####sdbtop:####

 ```lang-bash
 $ sdbtop -i localhost -s 11810 --usrname sdbadmin --cipher true --token sequoiadb --cipherfile ./passwd
 ```

####sdblobtool:####

 ```lang-bash
 $ sdblobtool --operation export --hostname localhost --svcname 50000 --collection foo.bar --file /opt/mylob --usrname sdbadmin --cipher true --token sequoiadb --cipherfile ./passwd
 ```

####sdbinspect####

 ```lang-bash
 $ sdbinspect -d localhost:50000 -o item.bin --auth sdbadmin --cipher true --token sequoiadb --cipherfile ./passwd
 ```

####Sequoiasql-pgsql####

 ```lang-bash
 foo=# create server sdb_server foreign data wrapper sdb_fdw options(address '127.0.0.1', service '11810', user 'sdbUserName', cipher 'on', token 'sequoiadb', cipherfile '/opt/sequoiadb/passwd', preferedinstance 'A', transaction 'off');
 ```