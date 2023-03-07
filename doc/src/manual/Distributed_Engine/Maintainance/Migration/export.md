[^_^]:
    数据导出

SequoiaDB 巨杉数据库支持使用 [sdbexprt 工具][sdbexprt]，将集合中的数据导出至 CSV 或 JSON 文件中。

下述将根据数据的格式分别介绍数据导出的具体操作步骤。示例中均以 `localhost:11810` 作为导入的目标集群协调节点地址，“sdbadmin”作为数据库用户名，及 `~/sequoiadb/passwd` 作为用户密码对应的密文文件路径。

##导出 CSV 文件##

CSV（Comma-Separated Values）是数据库间通用的数据交换格式之一，其文件以纯文本的形式存储表格数据。默认情况下记录间以换行符（\\n）分隔，字段间以逗号（,）分隔。CSV 格式定义可以参考 [RFC 4180][rfc4180] 文档。

###导出单个集合的数据###

1. 集合 sample.employee 存在如下数据：

    ```lang-json
    > db.sample.employee.find()
    {
      "_id": {
        "$oid": "5cd2dc7b294ffa8385000000"
      },
      "id": 1,
      "name": "Jack",
      "age": 18,
      "identity": "student",
      "phone_number": "18921222226",
      "email": "jack@example.com",
      "country": "China"
    }
    {
      "_id": {
        "$oid": "5cd2dc7b294ffa8385000001"
      },
      "id": 2,
      "name": "Mike",
      "age": 20,
      "identity": "student",
      "phone_number": "18923244255",
      "email": "mike@example.com",
      "country": "USA"
    }
    {
      "_id": {
        "$oid": "5cd2dc7b294ffa8385000002"
      },
      "id": 3,
      "name": "Woody",
      "age": 25,
      "identity": "worker",
      "phone_number": "18945253245",
      "email": "woody@example.com",
      "country": "China"
    }
    ```

2. 将集合的数据导出至 employee.csv 文件中，并通过 --fields 指定需导出的字段

    ```lang-bash
    $ sdbexprt --hosts "localhost:11810" --type csv --user sdbadmin --cipher true --cipherfile ~/sequoiadb/passwd --file employee.csv -c sample -l employee --fields id,name,age,identity,phone_number,email,country 
    ```

3. 查看导出后的文件

    ```lang-bash
    $ cat employee.csv 
    id,name,age,identity,phone_number,email,country
    1,"Jack",18,"student","18921222226","jack@example.com","China"
    2,"Mike",20,"student","18923244255","mike@example.com","USA"
    3,"Woody",25,"worker","18945253245","woody@example.com","China"
    ```

> **Note:**
>
> 导出集合数据时，默认导出全部记录。用户可通过参数 --filter 对需导出的记录进行过滤，更多参数说明可参考 [sdbexprt 工具][sdbexprt]。

###导出多个集合的数据###

1. 集合 sample.employee 存在如下数据：

    ```lang-json
    {id: 1, name: "Jack", age: 18}
    {id: 2, name: "Mike", age: 20}
    {id: 3, name: "Woody", age: 25}
    ```

    集合 sample.member 存在如下数据：

    ```lang-json
    {id: 101, name: "Alice", age: 28}
    {id: 102, name: "Mary", age: 37}
    {id: 103, name: "Helen", age: 19}
    ```

    集合 company.manager 存在如下数据：

    ```lang-json
    {id: 201, name: "Nancy", age: 83}
    {id: 202, name: "Mary", age: 62}
    {id: 203, name: "Eva", age: 22}
    ```

2. 将上述三个集合的数据均导出至 `/opt/sequoiadb/data` 目录中

    ```lang-bash
    $ sdbexprt --hosts "localhost:11810" --type csv --user sdbadmin --cipher true --cipherfile ~/sequoiadb/passwd --cscl sample.employee,sample.member,company.manager --fields sample.employee:id,name,age --fields sample.member:id,name,age --fields company.manager:id,name,age --dir /opt/sequoiadb/data
    ```

3. 查看导出后的目录

    ```lang-text
    $ ll /opt/sequoiadb/data
    -rw-r-----  1 sdbadmin sdbadmin_group   54 1月  26 21:08 company.manager.csv
    -rw-r-----  1 sdbadmin sdbadmin_group   49 1月  26 21:08 sample.employee.csv
    -rw-r-----  1 sdbadmin sdbadmin_group   56 1月  26 21:08 sample.member.csv
    ```

##导出 JSON 文件##

JSON（JavaScript Object Notation）是一种轻量级的数据交换格式。在 JSON 文件中，记录间需以左右花括号分隔，字符串类型的数据需包含在两个双引号（""）之间。当字符串类型的数据包含特殊字符时，需要使用反斜杠（\\）进行转义。

###导出单个集合的数据###

1. 集合 sample.employee 存在如下数据：

    ```lang-json
    > db.sample.employee.find()
    {
      "_id": {
        "$oid": "5cd2dc7b294ffa8385000000"
      },
      "id": 1,
      "name": "Jack",
      "age": 18,
      "identity": "student",
      "phone_number": "18921222226",
      "email": "jack@example.com",
      "country": "China"
    }
    {
      "_id": {
        "$oid": "5cd2dc7b294ffa8385000001"
      },
      "id": 2,
      "name": "Mike",
      "age": 20,
      "identity": "student",
      "phone_number": "18923244255",
      "email": "mike@example.com",
      "country": "USA"
    }
    {
      "_id": {
        "$oid": "5cd2dc7b294ffa8385000002"
      },
      "id": 3,
      "name": "Woody",
      "age": 25,
      "identity": "worker",
      "phone_number": "18945253245",
      "email": "woody@example.com",
      "country": "China"
    }
    ```

2. 将集合的数据导出至 `employee.json` 文件中，并通过 --fields 指定需导出的字段

    ```lang-bash
    $ sdbexprt --hosts "localhost:11810" --type json --user sdbadmin --cipher true --cipherfile ~/sequoiadb/passwd --file employee.json -c sample -l employee --fields id,name,age,identity,phone_number,email,country 
    ```

3. 查看导出后的文件

    ```lang-bash
    $ cat employee.json
    { "id": 1, "name": "Jack", "age": 18, "identity": "student", "phone_number": "18921222226", "email": "jack@example.com", "country": "China" }
    { "id": 2, "name": "Mike", "age": 20, "identity": "student", "phone_number": "18923244255", "email": "mike@example.com", "country": "USA" }
    { "id": 3, "name": "Woody", "age": 25, "identity": "worker", "phone_number": "18945253245", "email": "woody@example.com", "country": "China" }
    ```

> **Note:**
>
> 导出集合数据时，默认导出全部记录。用户可通过参数 --filter 对需导出的记录进行过滤，更多参数说明可参考 [sdbexprt 工具][sdbexprt]。


###导出多个集合的数据###

1. 集合 sample.employee 存在如下数据：

    ```lang-json
    {id: 1, name: "Jack", age: 18}
    {id: 2, name: "Mike", age: 20}
    {id: 3, name: "Woody", age: 25}
    ```

    集合 sample.member 存在如下数据：

    ```lang-json
    {id: 101, name: "Alice", age: 28}
    {id: 102, name: "Mary", age: 37}
    {id: 103, name: "Helen", age: 19}
    ```

    集合 company.manager 存在如下数据：

    ```lang-json
    {id: 201, name: "Nancy", age: 83}
    {id: 202, name: "Mary", age: 62}
    {id: 203, name: "Eva", age: 22}
    ```

2. 将上述三个集合的数据均导出至 `/opt/sequoiadb/data` 目录中

    ```lang-bash
    $ sdbexprt --hosts "localhost:11810" --type json --user sdbadmin --cipher true --cipherfile ~/sequoiadb/passwd --cscl sample.employee,sample.member,company.manager --fields sample.employee:id,name,age --fields sample.member:id,name,age --fields company.manager:id,name,age --dir /opt/sequoiadb/data
    ```

3. 查看导出后的目录

    ```lang-text
    $ ll /opt/sequoiadb/data
    -rw-r-----  1 sdbadmin sdbadmin_group   54 1月  26 21:08 company.manager.json
    -rw-r-----  1 sdbadmin sdbadmin_group   49 1月  26 21:08 sample.employee.json
    -rw-r-----  1 sdbadmin sdbadmin_group   56 1月  26 21:08 sample.member.json
    ```




[^_^]:
     本文使用的所有引用及链接
[sdbimprt]:manual/Distributed_Engine/Maintainance/Mgmt_Tools/sdbimprt.md
[sdbexprt]:manual/Distributed_Engine/Maintainance/Mgmt_Tools/sdbexprt.md
[csv_data_type]:manual/Distributed_Engine/Maintainance/Mgmt_Tools/sdbimprt.md#csv_data_type
[collection_space]:manual/Distributed_Engine/Architecture/Data_Model/collection_space.md
[collection]:manual/Distributed_Engine/Architecture/Data_Model/collection.md
[rfc4180]:https://www.rfc-editor.org/rfc/rfc4180.txt