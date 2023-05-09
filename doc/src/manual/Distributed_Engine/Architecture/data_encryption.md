[^_^]:
    数据加密

SequoiaDB 巨杉数据库基于国家密码局认定的国产密码算法 SM2 和 SM4，提供集合级别的数据加密功能，以保障数据安全。开启该功能后，数据将以密文的形式写入集合，避免真实数据被非法用户窃取和滥用。

##原理##

SequoiaDB 采用双层数据加密法，先使用算法 SM4 生成加密密钥（DEK，Data Encryption Key），对数据进行加密；再通过算法 SM2 生成主密钥（MK，Master Key），对 DEK 进行二次加密。其中，主密钥为一个密钥对，包含公钥和私钥，系统将使用公钥对 DEK 进行加密。流程图如下：

![encryption][encryption]

数据解密分为两层进行，在第一层解密中，系统将使用 MK 私钥对加密后的 DEK 进行解密，并得到 DEK。第二层解密则使用该 DEK 对数据进行解密。流程图如下：

![decrypt][decrypt]

##密钥文件##

MK 密钥和 DEK 密钥的信息分别存储在目录 `<catalog_dbpath>/security/MK` 和 `<catalog_dbpath>/security/DEK` 下，下述主要介绍不同密钥文件的作用。

MK 密钥文件说明如下：

| 文件名 | 说明 |
| ------ | ---- |
| index  | 索引文件，用于匹配当前使用的主密钥文件 |
| mk\<tag\>.pem | MK 密钥文件，用于存储公钥和私钥 |

>**Note:**
>
> 文件 `mk<tag>.pem` 中的 tag 表示生成该文件时的时间戳。

DEK 密钥文件说明如下：

| 文件名 | 说明 |
| ------ | ---- |
| dek      | DEK 密钥文件，用于存储 DEK 密钥 |
| dek.sign | 验证信息文件，用于验证解密后的 DEK 是否正确 |

##参考##

数据加密功能默认为关闭状态，用户需手动开启，具体操作可参考[开启数据加密][data_encryption]。

[^_^]:
     本文使用的所有引用及链接
[encryption_and_decrypt]:images/Distributed_Engine/Architecture/encryption_and_decrypt.png
[encryption]:images/Distributed_Engine/Architecture/encryption.png
[decrypt]:images/Distributed_Engine/Architecture/decrypt.png
[data_encryption]:manual/Distributed_Engine/operation/open_encryption.md