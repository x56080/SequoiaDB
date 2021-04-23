
本章介绍桶相关的接口。

## GET Service

查询用户创建的所有存储桶

**请求语法**

```lang-rest
GET / HTTP/1.1
Host: ip:port
Date: date
Authorization: authorization string
```

**结果解析**

查询结果以 XML 形式在响应消息体中显示。

| 元素 | 说明 |
| ---- | ---- |
| ListAllMyBucketsResult| 包含 Owner 和 Buckets |
| Owner | 存储桶所有者，包含 ID 和 DisplayName，属于 ListAllMyBucketsResult |
| ID | 存储桶所有者的 ID，属于 ListAllMyBucketsResult.Owner |
| DisplayName | 存储桶所有者的名称，属于 ListAllMyBucketsResult.Owner |
| Buckets | 桶列表，包含若干 Bucket，属于 ListAllMyBucketsResult |
| Bucket | 存储桶，包含 Name 和 CreationDate，属于 ListAllMyBucketsResult.Buckets |
| Name | 存储桶名称，属于 ListAllMyBucketsResult.Buckets.Bucket |
| CreationDate | 存储桶创建时间，属于 ListAllMyBucketsResult.Buckets.Bucket |

**示例**

响应结果如下：

```lang-xml
<ListAllMyBucketsResult>
  <Owner>
    <DisplayName>username</DisplayName>
    <ID>34455</ID>
  </Owner>
  <Buckets>
    <Bucket>
      <Name>mybucket</Name>
      <CreationDate>2019-02-03T16:45:09.000Z</CreationDate>
    </Bucket>
    <Bucket>
      <Name>samples</Name>
      <CreationDate>2019-02-03T16:41:58.000Z</CreationDate>
    </Bucket>
  </Buckets>
</ListAllMyBucketsResult>
```

## PUT Bucket

创建存储桶

> **Note:**
>
> 存储桶名需在整个系统内唯一，长度在 3~63 之间。

**请求语法**

```lang-rest
PUT /bucketname HTTP/1.1
Host: ip:port
Content-Length: length
Date: date
Authorization: authorization string
<CreateBucketConfiguration>
  <LocationConstraint>Region</LocationConstraint>
</CreateBucketConfiguration>
```

**请求元素**

用户需要在请求消息体中使用 XML 形式指定存储桶创建的区域，如果不指定则存储桶创建在默认的区域上。

| 元素 | 说明 |
| ---- | ---- |
| CreateBucketConfiguration | 包含 LocationConstraint |
| LocationConstraint | 指定创建存储桶的区域，类型为 string |

**示例**

响应结果如下：

```lang-rest
HTTP/1.1 200 OK
Location: /bucketName
Content-Length: 0
Date: Fri, 16 Aug 2019 11:11:53 GMT
```


## DELETE Bucket

删除存储桶

**请求语法**

```lang-rest
DELETE /bucketname HTTP/1.1
Host: ip:port
Date: date
Authorization: authorization string
```

**示例**

响应结果如下：

```lang-rest
HTTP/1.1 204 No Content
Date: Fri, 16 Aug 2019 10:11:53 GMT
```


## HEAD Bucket

检查一个存储桶是否存在

**请求语法**

```lang-rest
HEAD /bucketname HTTP/1.1
Date: date
Authorization: authorization string
Host: ip:port
```

**示例**

响应结果如下：

```lang-rest
HTTP/1.1 200 OK
Date: Fri, 16 Aug 2019 10:10:53 GMT
```


## PUT Bucket versioning

修改存储桶的版本控制状态

**请求语法**

```lang-rest
PUT /bucketname?versioning HTTP/1.1
Host: ip:port
Content-Length: length
Date: date
Authorization: authorization string

<VersioningConfiguration>
  <Status>VersioningState</Status>
</VersioningConfiguration>
```

**参数说明**

| 参数名 | 说明 |
| ----   | ---- |
| versioning | 表示该请求为修改存储桶的版本控制状态 |

**请求元素**

在请求消息体中使用 XML 形式指定存储桶的版本控制状态。

| 元素 | 说明 |
| ---- | ---- |
| VersioningConfiguration | 包含 Status |
| Status | 版本控制状态，有效值为 Suspended\|Enabled，属于 VersioningConfiguration |

**示例**

响应结果如下：

```lang-rest
HTTP/1.1 200 OK
Date: Wed, 01 Mar  2006 12:00:00 GMT
```


## GET Bucket versioning

查询桶的版本控制状态

**请求语法**

```lang-rest
GET /bucketname?versioning HTTP/1.1
Host: ip:port
Content-Length: length
Date: date
Authorization: authorization string
```

**结果解析**

查询结果以 XML 形式在响应消息头中显示。

| 元素 | 说明 |
| ---- | ---- |
| VersioningConfiguration | 包含 Status |
| Status | 版本控制状态，有效值为 Suspended|Enabled，属于 VersioningConfiguration |

**示例**

- 打开版本控制开关，查询结果如下：

    ```lang-xml
    <VersioningConfiguration>
      <Status>Enabled</Status>
    </VersioningConfiguration>
    ```

- 禁用版本控制，查询结果如下：

    ```lang-xml
    <VersioningConfiguration>
      <Status>Suspended</Status>
    </VersioningConfiguration>
    ```

- 从未开启或禁用过版本控制，查询结果如下：

    ```lang-xml
    <VersioningConfiguration/>
    ```

## GET Bucket location

查询桶所在的区域

**请求语法**

```lang-rest
GET /bucketname?location HTTP/1.1
Host: ip:port
Date: date
Authorization: authorization string
```

**结果解析**

查询结果以 XML 格式在响应消息体中显示。

| 元素 | 说明 |
| ---- | ---- |
| LocationConstraint | 桶所在的区域 |

**示例**

- 已经配置了区域的存储桶，查询结果如下：

    ```lang-xml
    <LocationConstraint>region</LocationConstraint>
    ```

- 未配置区域的存储桶，查询结果如下：

    ```lang-xml
    <LocationConstraint/>
    ```


## GET Bucket (List Objects) Version 1

查询存储桶内对象列表

**请求语法**

```lang-rest
GET /bucketname HTTP/1.1
Host: ip:port
Date: date
Authorization: authorization string
```

**请求参数**

| 参数名 | 说明 |
| ----   | ---- |
| prefix | 前缀，类型为 string，返回具有前缀的对象列表， |
| delimiter | 分隔符，类型为 string，如果指定 prefix，则 prefix 后第一次出现的分隔符之间包含相同字符串的所有键都被分组在一个 CommonPrefixes；如果未指定 prefix 参数，则子字符串从对象名称的开头开始|
| marker | 指定在存储桶中列出对象要开始的键，类型为 string，返回对象键按照 UTF-8 二进制顺序从该标记后的键开始按顺序排列 |
|max-keys | 设置响应中返回的最大键数，类型为 string，默认值为 1000，如果要查询返回数量少于 1000，可以填写其他值，填写超过 1000 的值，仍然按照  1000 条返回 | 
| encoding-type | 响应结果编码类型，只支持 url，由于对象名称可以包含任意字符，但是 XML 对某些特别的字符无法解析，所以需要对响应中的对象名称进行编码 |

**结果解析**

查询结果以 XML 形式在响应消息体中显示。

| 元素 | 说明 |
| ---- | ---- |
| ListBucketResult | 包含存储桶信息、查询条件和查询的对象信息 |
| Name | 存储桶名称 |
| Prefix | 查询的 prefix 条件 |
| Delimiter | 查询的 delimiter 条件 |
| Marker | 查询的 marker 条件 |
| MaxKeys | 查询的 maxKeys 条件 |
| Encoding-Type | 查询的 encoding-type 条件 |
| IsTruncated | 如果该字段为 true，说明由于条数限制，本次没有查询完所有符合条件的结果，可以使用 NextMarker 作为下一次查询的 Marker 条件继续查询剩余内容 |
| NextMarker | 当 IsTruncated 为 true 时，该字段返回的是本次查询的最后一条的记录 |
| CommonPrefixes | 当查询条件指定了 Delimter 时，Prefix 后面第一次出现 Delimiter 的位置（包括 Delimiter）之前的内容作为 CommonPrefix，当有多个对象具有相同的 CommonPrefix 时，只返回一条 CommonPrefix，计数一次，对象信息不返回 |
| Prefix | CommonPrefix 包含的前缀，属于 ListBucketResult.CommonPrefixes |
| Contents | 包含对象的元数据 | 
| Key | 对象的名称，属于 ListBucketResult.Contents |
| LastModified | 创建对象的时间，属于 ListBucketResult.Contents |
| ETag | 对象的 MD5 值，属于 ListBucketResult.Contents |
| Size | 对象的大小，单位为字节，属于 ListBucketResult.Contents |
| Owner | 存储桶的所有者，属于 ListBucketResult.Contents |
| ID | 存储桶所有者的 ID，属于 ListBucketResult.Contents.Owner |
| DisplayName | 桶所有者的名字，属于 ListBucketResult.Contents.Owner |


**示例**

- 不携带查询条件，查询存储桶内所有记录

    ```lang-rest
    GET /bucketname HTTP/1.1
    Host: ip:port
    Date: date
    Authorization: authorization string
    ```
   
    查询结果如下：
   
    ```lang-xml
    <ListBucketResult>
        <Name>bucketname</Name>
        <Prefix/>
        <Marker/>
        <MaxKeys>1000</MaxKeys>
        <IsTruncated>false</IsTruncated>
        <Contents>
            <Key>my-image.jpg</Key>
            <LastModified>2019-08-12T17:50:30.000Z</LastModified>
            <ETag>"fba9dede5f27731c9771645a39863328"</ETag>
            <Size>434234</Size>
            <Owner>
                <ID>125664</ID>
                <DisplayName>username</DisplayName>
            </Owner>
        </Contents>
        <Contents>
           <Key>my-third-image.jpg</Key>
             <LastModified>2019-08-12T17:51:30.000Z</LastModified>
             <ETag>"1b2cf535f27731c974343645a3985328"</ETag>
             <Size>64994</Size>
             <Owner>
                <ID>125664</ID>
                <DisplayName>username</DisplayName>
            </Owner>
        </Contents>
    </ListBucketResult>
    ```

- 本次请求指定 prefix 为 N，起始位置为 Ned，并只返回 100 条记录

    ```lang-rest
    GET /mybucket?prefix=N&marker=Ned&max-keys=100 HTTP/1.1
    Host: iP:port
    Date: date
    Authorization: authorization string
    ```
    
    查询结果如下：
    
    ```lang-xml
    <ListBucketResult>
      <Name>mybucket</Name>
      <Prefix>N</Prefix>
      <Marker>Ned</Marker>
      <MaxKeys>100</MaxKeys>
      <IsTruncated>false</IsTruncated>
      <Contents>
        <Key>Nelson</Key>
        <LastModified>2019-08-12T12:00:00.000Z</LastModified>
        <ETag>"828ef3fdfa96f00ad9f27c383fc9ac7f"</ETag>
        <Size>5</Size>
        <Owner>
          <ID>125664</ID>
          <DisplayName>username</DisplayName>
         </Owner>
      </Contents>
      <Contents>
        <Key>Neo</Key>
        <LastModified>2019-08-12T12:01:00.000Z</LastModified>
        <ETag>"828ef3fdfa96f00ad9f27c383fc9ac7f"</ETag>
        <Size>4</Size>
         <Owner>
          <ID>125664</ID>
          <DisplayName>username</DisplayName>
        </Owner>
     </Contents>
    </ListBucketResult>
    ```

- 桶内已经有如下对象：

   ```lang-text
   sample.jpg
    photos/2006/January/sample.jpg
    photos/2006/February/sample2.jpg
    photos/2006/February/sample3.jpg
    photos/2006/February/sample4.jpg
    ```
    
    本次请求携带分隔符/
    
    ```lang-test
    GET /mybucket-2?delimiter=/ HTTP/1.1
    Host: ip:port
    Date: date
    Authorization: authorization string
    ```
    
    查询结果如下：
    
    ```lang-xml
    <ListBucketResult>
      <Name>mybucket-2</Name>
      <Prefix/>
      <Marker/>
      <MaxKeys>1000</MaxKeys>
      <Delimiter>/</Delimiter>
      <IsTruncated>false</IsTruncated>
      <Contents>
        <Key>sample.jpg</Key>
        <LastModified>2019-08-12T12:01:00.000Z</LastModified>
        <ETag>"bf1d737a4d46a19f3bced6905cc8b902"</ETag>
        <Size>142863</Size>
        <Owner>
          <ID>canonical-user-id</ID>
          <DisplayName>display-name</DisplayName>
        </Owner>
      </Contents>
      <CommonPrefixes>
        <Prefix>photos/</Prefix>
      </CommonPrefixes>
    </ListBucketResult>
    ```

## GET Bucket (List Objects) Version 2

查询桶内对象列表

**请求语法**

```
GET /bucketname?list-type=2 HTTP/1.1
Host: ip:port
Date: date
Authorization: authorization string
```

**请求参数**

| 参数名 | 说明 |
| ----   | ---- |
| list-type | 固定设置为 2，List Objects 的第二个版本 |
| prefix | 前缀，类型为 string，返回具有前缀的对象列表 | 
| delimiter | 分隔符，类型为 string，如果指定 prefix，则 prefix 后第一次出现的分隔符之间包含相同字符串的所有键都被分组在一个 CommonPrefixes；如果未指定 prefix 参数，则子字符串从对象名称的开头开始 |
| start-after | 指定在存储桶中列出对象要开始的键，类型为 string，返回对象键按照 UTF-8 二进制顺序从该标记后的键开始按顺序排列 |
| max-keys | 设置响应中返回的最大键数，类型为 string，默认值 1000，如果要查询返回数量少于 1000，可以填写其他值，填写超过 1000 的值，仍然按照 1000 条返回 |
| encoding-type | 响应结果编码类型，只支持 url，由于对象名称可以包含任意字符，但是 XML 对某些特别的字符无法解析，所以需要对响应中的对象名称进行编码 |
| continuation-token | 当响应结果被截断，还有部分未返回时，响应结果中会包含 NextContinuationToken，要列出下一组对象，可以使用 NextContinuationToken 下一个请求中的元素作为 continuation-token |
| fetch-owner | 默认情况下，结果中不会返回 Owner 信息，如果要在响应中包含 Owner 信息，将该参数置为 true |

**结果解析**

查询结果以 XML 形式在响应消息体中显示。

| 元素 | 说明 |
| ---- | ---- |
| ListBucketResult | 包含存储桶信息、查询条件和查询的对象信息 |
| Name | 存储桶名称 |
| Prefix | 查询的 prefix 条件 |
| Delimiter | 查询的 delimiter 条件 |
| StartAfter | 查询的 start-after 条件 |
| ContinuationToken | 查询的 continuation-token 条件 |
| MaxKeys | 查询的 maxKeys 条件 |
| Encoding-Type | 查询的 encoding-type 条件 |
| KeyCount | 本次查询返回的记录数 |
| IsTruncated | 如果该字段为 true，说明由于条数限制，本次没有查询完所有符合条件的结果，可以使用 NextMarker 作为下一次查询的 Marker 条件继续查询剩余内容 |
| NextContinuationToken | 当 IsTruncated 为 true 时，NextContinuationToken 记录位置，下一次请求在 continuation-token 携带该令牌继续查询下一组记录 |
| CommonPrefixes | 当查询条件指定了 Delimter 时，Prefix 后面第一次出现 Delimiter 的位置（包括 Delimiter）之前的内容作为 CommonPrefix，当有多个对象具有相同的 CommonPrefix 时，只返回一条 CommonPrefix，计数一次，对象信息不返回 |
| Prefix | CommonPrefix 包含的前缀，属于 ListBucketResult.CommonPrefixes |
| Contents | 包含对象的元数据 |
| Key | 对象的名称，属于 ListBucketResult.Contents |
| LastModified | 创建对象的时间，属于 ListBucketResult.Contents |
| ETag | 对象的 MD5 值，属于 ListBucketResult.Contents |
| Size | 对象的大小，单位为字节，属于 ListBucketResult.Contents |
| Owner | 存储桶的所有者，属于 ListBucketResult.Contents |
| ID | 存储桶所有者的 ID，属于 ListBucketResult.Contents.Owner |
| DisplayName | 桶所有者的名字，属于 ListBucketResult.Contents.Owner |

**示例**

- 不携带查询条件，查询存储桶内所有记录

    ```lang-rest
    GET /bucketname?list-type=2 HTTP/1.1
    Host: ip:port
    Date: Sat, 17 Aug 2019 17:51:00 GMT
    Authorization: authorization string
    ```
    
    查询结果如下：
    
    ```lang-xml
    <ListBucketResult>
        <Name>bucketname</Name>
        <Prefix/>
        <Marker/>
        <KeyCount>205</KeyCount>
        <MaxKeys>1000</MaxKeys>
        <IsTruncated>false</IsTruncated>
        <Contents>
            <Key>my-image.jpg</Key>
            <LastModified>2019-08-12T17:50:30.000Z</LastModified>
            <ETag>"fba9dede5f27731c9771645a39863328"</ETag>
            <Size>434234</Size>
            <Owner>
                <ID>125664</ID>
                <DisplayName>username</DisplayName>
            </Owner>
        </Contents>
        <Contents>
           ...
        </Contents>
        ...
    </ListBucketResult>
    ```

- 指定 prefix 为 N，起始位置为 Ned，并只返回 100 条记录

    ```lang-rest
    GET /mybucket?list-type=2&prefix=N&start-after=Ned&max-keys=100 HTTP/1.1
    Host: iP:port
    Date: Sat, 17 Aug 2019 17:45:00 GMT
    Authorization: authorization string
    ```
    
    查询结果如下，实际查询到两条符合条件的记录：
    
    ```lang-xml
    <ListBucketResult>
      <Name>mybucket</Name>
      <Prefix>N</Prefix>
      <Marker>Ned</Marker>
      <KeyCount>2</KeyCount>
      <MaxKeys>100</MaxKeys>
      <IsTruncated>false</IsTruncated>
      <Contents>
        <Key>Nelson</Key>
        <LastModified>2019-08-12T12:00:00.000Z</LastModified>
        <ETag>"828ef3fdfa96f00ad9f27c383fc9ac7f"</ETag>
        <Size>5</Size>
        <Owner>
          <ID>125664</ID>
          <DisplayName>username</DisplayName>
         </Owner>
      </Contents>
      <Contents>
        <Key>Neo</Key>
        <LastModified>2019-08-12T12:01:00.000Z</LastModified>
        <ETag>"828ef3fdfa96f00ad9f27c383fc9ac7f"</ETag>
        <Size>4</Size>
         <Owner>
          <ID>125664</ID>
          <DisplayName>username</DisplayName>
        </Owner>
     </Contents>
    </ListBucketResult>
    ```

- 桶内已经有如下对象

   ```lang-text
   sample.jpg
   photos/2006/January/sample.jpg
    photos/2006/February/sample2.jpg
    photos/2006/February/sample3.jpg
    photos/2006/February/sample4.jpg
    ```
    
    本次请求携带分隔符/
    
    ```lang-rest
    GET /mybucket-2?list-type=2&delimiter=/ HTTP/1.1
    Host: ip:port
    Date: date
    Authorization: authorization string
    ```
    
    查询结果如下：
    
    ```lang-xml
    <ListBucketResult>
      <Name>mybucket-2</Name>
      <Prefix/>
      <Marker/>
      <KeyCount>2</KeyCount>
      <MaxKeys>1000</MaxKeys>
      <Delimiter>/</Delimiter>
      <IsTruncated>false</IsTruncated>
      <Contents>
        <Key>sample.jpg</Key>
        <LastModified>2019-08-12T12:01:00.000Z</LastModified>
        <ETag>"bf1d737a4d46a19f3bced6905cc8b902"</ETag>
        <Size>142863</Size>
        <Owner>
          <ID>canonical-user-id</ID>
          <DisplayName>display-name</DisplayName>
        </Owner>
      </Contents>
      <CommonPrefixes>
        <Prefix>photos/</Prefix>
      </CommonPrefixes>
    </ListBucketResult>
    ```

## GET Bucket Object versions

查询桶内对象的所有版本

**请求语法**

```lang-rest
GET /bucketname?versions HTTP/1.1
Host: ip:port
Date: date
Authorization: authorization string
```

**请求参数**

| 参数名 | 说明 |
| ----   | ---- |
| prefix | 前缀，类型为 string，返回具有前缀的对象列表 |
| delimiter | 分隔符，类型为 string，如果指定 prefix，则 prefix 后第一次出现的分隔符之间包含相同字符串的所有键都被分组在一个 CommonPrefixes；如果未指定 prefix 参数，则子字符串从对象名称的开头开始 |
| key-marker | 指定在存储桶中列出对象要开始的键，类型为 string，返回对象键按照 UTF-8 二进制顺序从该标记后的键开始按顺序排列 |
| version-id-marker | 指定起始位置的 version，仅在指定了 key-marker 的情况下有效 |
| max-keys | 设置响应中返回的最大键数，类型为 string，默认值 1000，如果要查询返回数量少于 1000，可以填写其他值，填写超过 1000 的值，仍然按照 1000 条返回  |
| encoding-type | 响应结果编码类型，只支持 url，由于对象名称可以包含任意字符，但是 XML 对某些特别的字符无法解析，所以需要对响应中的对象名称进行编码 |

**结果解析**

查询结果以 XML 形式在响应消息体中显示。

| 元素 | 说明 |
| ---- | ---- |
| ListVersionsResult | 包含存储桶信息、查询条件和查询的对象版本信息 |
| Name | 存储桶名称 |
| Prefix | 查询的 prefix 条件 |
| Delimiter | 查询的 delimiter 条件 |
| KeyMarker | 查询的 key-marker 条件 |
| VersionIDMarker | 查询的 version-id-marker 条件 |
| MaxKeys | 查询的 maxKeys 条件 |
| Encoding-Type | 查询的 encoding-type 条件 |
| IsTruncated | 如果该字段为 true，说明由于条数限制，本次没有查询完所有符合条件的结果，可以使用 NextMarker 作为下一次查询的 Marker 条件继续查询剩余内容 |
| NextKeyMarker | 当 IsTruncated 为 true 时，NextKeyMarker 记录本次返回的最后一个对象或者 CommonPrefix |
| NextVersionIdMarker | 当 IsTruncated 为 true 时，NextVersionIdMarker 记录本次返回的最后一条记录的 version |
| CommonPrefixes | 当查询条件指定了 Delimter 时，Prefix 后面第一次出现 Delimiter 的位置（包括 Delimiter）之前的内容作为 CommonPrefix，当有多个对象具有相同的 CommonPrefix 时，只返回一条 CommonPrefix，计数一次，对象信息不返回 |
| Prefix | CommonPrefix 包含的前缀，属于 ListVersionsResult.CommonPrefixes |
| Version | 包含对象版本的元数据 |
| DeleteMarker | 包含删除标记 |
| Key | 对象的名称，属于 ListVersionsResult.Version\|ListVersionsResult.DeleteMarker |
| VersionId | 对象的版本号，属于 ListVersionsResult.Version\|ListVersionsResult.DeleteMarker |
| IsLatest | 是否是最新版本，属于 ListVersionsResult.Version\|ListVersionsResult.DeleteMarker |
| LastModified | 创建对象的时间，属于 ListVersionsResult.Version\|ListVersionsResult.DeleteMarker |
| ETag | 对象的 MD5 值，属于 ListVersionsResult.Version |
| Size | 对象的大小，单位为字节，属于 ListVersionsResult.Version |
| Owner | 存储桶的所有者，属于 ListVersionsResult.Version\|ListVersionsResult.DeleteMarker |
| ID | 存储桶所有者的 ID，属于 ListVersionsResult.Version.Owner\|ListVersionsResult.DeleteMarker.Owner |
| DisplayName | 桶所有者的名字，属于 ListVersionsResult.Version.Owner\|ListVersionsResult.DeleteMarker.Owner |

**示例**

- 查询存储桶内所有版本

    ```lang-rest
    GET /bucketname?versions HTTP/1.1
    Host: ip:port
    Date: date
    Authorization: authorization string
    ```
    
    查询结果如下：
    
    ```lang-xml
    <ListVersionsResult>
        <Name>bucket</Name>
        <Prefix>my</Prefix>
        <KeyMarker/>
        <VersionIdMarker/>
        <MaxKeys>1000</MaxKeys>
        <IsTruncated>false</IsTruncated>
        <Version>
            <Key>my-image.jpg</Key>
            <VersionId>234</VersionId>
            <IsLatest>true</IsLatest>
            <LastModified>2019-08-16T17:50:32.000Z</LastModified>
            <ETag>"fba9dede5f27731c9771645a39863328"</ETag>
            <Size>434234</Size>
            <Owner>
                <ID>125664</ID>
                <DisplayName>username</DisplayName>
            </Owner>
        </Version>
        <DeleteMarker>
            <Key>my-second-image.jpg</Key>
            <VersionId>55566666</VersionId>
            <IsLatest>true</IsLatest>
            <LastModified>2019-08-16T17:50:31.000Z</LastModified>
            <Owner>
                <ID>125664</ID>
                <DisplayName>username</DisplayName>
            </Owner>
        </DeleteMarker>
        <Version>
            <Key>my-second-image.jpg</Key>
            <VersionId>45667</VersionId>
            <IsLatest>false</IsLatest>
            <LastModified>2019-08-16T17:50:30.000Z</LastModified>
            <ETag>"9b2cf535f27731c974343645a3985328"</ETag>
            <Size>166434</Size>
            <Owner>
                <ID>125664</ID>
                <DisplayName>username</DisplayName>
            </Owner>
        </Version>
    </ListVersionsResult>
    ```

- 携带分隔符/进行查询

    ```lang-rest
    GET /mybucket-2?versions&delimiter=/ HTTP/1.1
    Host: ip:port
    Date: date
    Authorization: authorization string
    ```
    
    响应结果如下：
    
    ```lang-xml
    <ListVersionsResult>
      <Name>mvbucketwithversionon1</Name>
      <Prefix/>
      <KeyMarker/>
      <VersionIdMarker/>
      <MaxKeys>1000</MaxKeys>
      <Delimiter>/</Delimiter>
      <IsTruncated>false</IsTruncated>
      <Version>
        <Key>Sample.jpg</Key>
        <VersionId>toxMzQlBsGyGCz1YuMWMp90cdXLzqOCH</VersionId>
        <IsLatest>true</IsLatest>
        <LastModified>2019-02-02T18:46:20.000Z</LastModified>
        <ETag>"3305f2cfc46c0f04559748bb039d69ae"</ETag>
        <Size>3191</Size>
        <Owner>
            <ID>125664</ID>
            <DisplayName>username</DisplayName>
        </Owner>
      </Version>
      <CommonPrefixes>
        <Prefix>photos/</Prefix>
      </CommonPrefixes>
      <CommonPrefixes>
        <Prefix>videos/</Prefix>
      </CommonPrefixes>
    </ListVersionsResult>
    ```

## List Multipart Uploads

查询桶内所有已初始化未完成的分段上传请求

**请求语法**

```lang-rest
GET /bucketname?uploads HTTP/1.1
Host: ip:port
Date: Date
Authorization: authorization string
```

**请求参数**

| 参数 | 说明 |
| ---- | ---- |
| prefix | 前缀，类型为 string，返回具有前缀的对象列表 |
| delimiter | 分隔符，类型为 string，如果指定 prefix，则 prefix 后第一次出现的分隔符之间包含相同字符串的所有键都被分组在一个 CommonPrefixes；如果未指定 prefix 参数，则子字符串从对象名称的开头开始 |
| key-marker | 指定在存储桶中列出对象要开始的键，类型为 string，返回对象键按照 UTF-8 二进制顺序从该标记后的键开始按顺序排列 |
| upload-id-marker | 指定起始位置的 uploadId，仅在指定了 key-marker 的情况下有效 |
| max-uploads | 设置响应中返回的最大键数，类型为 string，默认值 1000，如果要查询返回数量少于 1000，可以填写其他值，填写超过 1000 的值，仍然按照 1000 条返回 |
| encoding-type | 对响应内容进行的编码方法，只支持 url，由于对象名称可以包含任意字符，但是 XML 对某些特别的字符无法解析，所以需要对响应中的对象名称进行编码 |

**结果解析**

查询结果在响应消息体中以 XML 形式体现。

| 元素 | 说明 |
| ---- | ---- |
| ListMultipartUploadsResult | 包含桶信息、查询条件和未完成的分段上传信息 |
| Bucket | 存储桶名称 |
| Prefix | 查询的 prefix 条件 |
| Delimiter | 查询的 delimiter 条件 |
| KeyMarker | 查询的 key-marker 条件 |
| UploadIdMarker | 查询的 upload-id-marker 条件 |
| MaxUploads | 查询的 maxUploads 条件 |
| Encoding-Type | 查询的 encoding-type 条件 |
| IsTruncated | 如果该字段为 true，说明由于条数限制，本次没有查询完所有符合条件的结果，可以使用 NextMarker 作为下一次查询的 Marker 条件继续查询剩余内容 |
| NextKeyMarker | 当 IsTruncated 为 true 时，NextKeyMarker 记录本次返回的最后一个对象或者 CommonPrefix |
| NextUploadIdMarker | 当 IsTruncated 为 true 时，NextUploadIdMarker 记录本次返回的最后一条记录的 uploadId |
| CommonPrefixes | 当查询条件指定了 Delimter 时，Prefix 后面第一次出现 Delimiter 的位置（包括 Delimiter）之前的内容作为 CommonPrefix，当有多个对象具有相同的 CommonPrefix 时，只返回一条 CommonPrefix，计数一次，对象信息不返回 |
| Prefix | CommonPrefix 包含的前缀，属于 ListMultipartUploadsResult.CommonPrefixes |
| Upload | 包含分段上传信息 |
| Key | 对象的名称，属于 ListMultipartUploadsResult.Upload | 
| UploadId | 分段上传的 uploadId，属于 ListMultipartUploadsResult.Upload |
| Initiated | 分段上传的的初始化时间，属于 ListMultipartUploadsResult.Upload | 
| Owner | 存储桶的所有者，属于 ListMultipartUploadsResult.Upload |
| ID | 存储桶所有者的 ID，属于 ListMultipartUploadsResult.Upload.Owner | ListMultipartUploadsResult.Upload.Initiated |
| DisplayName | 桶所有者的名字，属于 ListMultipartUploadsResult.Upload.Owner |ListMultipartUploadsResult.Upload.Initiated |

**示例**

查询 uploads，携带分隔符/

```lang-rest
GET /example-bucket?uploads&delimiter=/ HTTP/1.1
Host: ip:port
Date: Sat, 17 Aug 2019 20:34:56 GMT
Authorization: authorization string
```

查询结果如下：

```lang-xml
<ListMultipartUploadsResult>
  <Bucket>example-bucket</Bucket>
  <KeyMarker/>
  <UploadIdMarker/>
  <NextKeyMarker>sample.jpg</NextKeyMarker>
  <NextUploadIdMarker>4444</NextUploadIdMarker>
  <Delimiter>/</Delimiter>
  <Prefix/>
  <MaxUploads>1000</MaxUploads>
  <IsTruncated>false</IsTruncated>
  <Upload>
    <Key>sample.jpg</Key>
    <UploadId>4444</UploadId>
    <Initiator>
      <ID>2234</ID>
      <DisplayName>s3-nickname</DisplayName>
    </Initiator>
    <Owner>
      <ID>2234</ID>
      <DisplayName>s3-nickname</DisplayName>
    </Owner>
    <Initiated>2019-08-16T19:24:17.000Z</Initiated>
  </Upload>
  <CommonPrefixes>
    <Prefix>photos/</Prefix>
  </CommonPrefixes>
  <CommonPrefixes>
    <Prefix>videos/</Prefix>
  </CommonPrefixes>
</ListMultipartUploadsResult>
```