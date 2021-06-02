
本章介绍对象相关的接口

## PUT Object

上传一个对象到桶中，如果已有则覆盖

> **Note:**
>
> 当开启了版本控制，同一个名称的对象可以在系统中保留多个版本。系统会为每次上传的对象生成一个 version ID，并保留每个版本。

**请求语法**

```lang-rest
PUT /bucketname/ObjectName HTTP/1.1
Host: ip:port
Date: date
Authorization: authorization string
```

**请求头部**

| 头域 | 说明 |
| ---- | ---- |
| Cache-Control | 指定请求/响应链中的缓存属性 |
| Content-Disposition | 当获取对象时，该属性提示将对象保存为的文件名 |
| Content-Encoding | 对象的附加编码类型，例如压缩文档使用的 gzip 类型 |
| Content-MD5 | 对象内容（不包含头部）的 MD5 值经过 BASE64 编码后得到字符串，服务端收到对象后也会做同样的计算，比较 Content-MD5 和服务端计算得出的结果，可以防止上传的对象内容被篡改或不完整 |
| Content-Type | 请求内容的 MIME 类型 |
| Expect | 当 expect 设置为 100-continue，发送 put object 的请求时并不立刻发送对象内容，而是等收到 100 临时响应或等待超时再发送 |
| Expires | 缓存的超时时间 |
| x-amz-meta- | 自定义元数据 |

**结果解析**

响应信息通过 header 返回。

| 头域 | 说明 |
| ---- | ---- |
| ETag | 对象内容的 MD5 值转换为 16 进制之后生成的字符串 |
| x-amz-version-id | 版本号，当版本控制状态为 Enabled 时，该字段返回此次上传对象的版本号；当版本控制状态为 Suspended 时，该字段返回 null；当未开启或禁用版本控制，该字段不返回 | 

**示例**

上传一个对象

```lang-rest
PUT /bucketname/my-image.jpg HTTP/1.1
Host: ip:port
Date: Sat, 17 Aug 2019 17:50:00 GMT
Authorization: authorization string
Content-Type: text/plain
Content-Length: 11434
Expect: 100-continue
[11434 bytes of object data]
```

响应结果如下：

```lang-rest
HTTP/1.1 100 Continue

HTTP/1.1 200 OK
Date: Sat, 17 Aug 2019 17:50:00 GMT
ETag: "1b2cf535f27731c974343645a3985328"
Content-Length: 0
```

## PUT Object - Copy

从系统中已有的对象拷贝到目标对象

> **Note:**
>
> 该操作不需要从本地上传对象内容。

**请求语法**

```lang-rest
PUT /destinationbucket/destinationObject HTTP/1.1
Host: ip:port
x-amz-copy-source: /source_bucket/sourceObject
x-amz-metadata-directive: metadata_directive
x-amz-copy-source-if-match: etag
x-amz-copy-source-if-none-match: etag
x-amz-copy-source-if-unmodified-since: time_stamp
x-amz-copy-source-if-modified-since: time_stamp
<request metadata>
Authorization: authorization string
```
**请求头部**

| 头域 | 说明 |
| ---- | ---- |
| x-amz-copy-source | 必须携带的头部，复制对象的源对象地址，包含源存储桶和源对象，例如：`/source_bucket/sourceObject` ，默认复制源对象的最新版本；如果要指定版本复制，则需要增加版本号，例如：`/source_bucket/sourceObject?versionId=3344` |
|  x-amz-metadata-directive | 指定是否从源对象复制元数据到目标对象，取值包括"COPY"和"REPLACE"，默认值为"COPY" <br> 当指定为"COPY"时，从源对象复制元数据到目标对象；当指定为"REPLACE"时，源对象的元数据都不会复制到目标对象，目标对象使用复制对象请求中携带的元数据 |
| x-amz-copy-if-modified-since | 时间，只有当源对象的创建时间在此时间后才进行复制 |
| x-amz-copy-if-unmodified-since | 时间，只有当源对象的创建时间在此之前才进行复制 |
| x-amz-copy-if-match | ETag，只有当源对象的 ETag 与此 ETag 匹配才进行复制 |
| x-amz-copy-if-none-match | ETag，只有当源对象的 ETag 与此 ETag 不匹配才进行复制 |
| Cache-Control | 指定请求/响应链中的缓存属性 |
| Content-Disposition | 当获取对象时，该属性提示将对象保存为的文件名 |
| Content-Encoding | 对象的附加编码类型，例如压缩文档使用的 gzip 类型 |
| Content-MD5 | 对象内容（不包含头部）计算 MD5 值后经过 BASE64 编码得到字符串，服务端收到对象后也会做同样的计算，比较 Content-MD5 和服务端计算得出的结果是否一致，可以防止上传的对象内容被篡改或不完整 |
| Expires | 缓存的超时时间 |
| x-amz-meta- | 自定义元数据 |

**结果解析**

版本号在响应 header 中体现。

| 头域 | 说明 |
| ---- | ---- |
| x-amz-version-id | 复制后生成对象的版本号 |
| x-amz-copy-source-version-id | 源对象的版本号，复制后对象的 ETag 和创建时间在消息体中以 XML 形式体现 |
| CopyObjectResult | 包含 ETag 和 LastModified |
| ETag | 新对象内容计算 MD5 值后转换为 16 进制得到的字符串，与源对象一致 |
| LastModified | 新对象的创建时间 |

**示例**

复制指定的版本

```lang-rest
PUT /bucketname/my-second-image.jpg HTTP/1.1
Host: ip:port
Date: Sat, 17 Aug 2019 17:50:00 GMT
x-amz-copy-source: /bucketname/my-image.jpg?versionId=3344
Authorization: authorization string
```

响应结果如下：

```lang-rest
HTTP/1.1 200 OK
x-amz-version-id:5656
x-amz-copy-source-version-id:3344
Date: Sat, 17 Aug 2019 17:50:00 GMT

<CopyObjectResult>
   <LastModified>2019-08-17T17:50:00</LastModified>
   <ETag>"9b2cf535f27731c974343645a3985328"</ETag>
</CopyObjectResult>
```

## GET Object

获取对象内容

**请求语法**

```lang-rest
GET /bucketname/ObjectName HTTP/1.1
Host: ip:port
Date: date
Authorization: authorization string
```

**请求参数**

| 参数名 | 说明 |
| ----   | ---- |
| versionId | 获取指定版本的对象时通过此参数指定版本号 |
| response-content-type | 指定响应消息中的 Content-Type 头部值 | 
| response-content-language | 指定响应消息中的 Content-Language 头部值 |
| response-expires | 指定响应消息中的 Expires 头部值 |
| response-cache-control | 指定响应消息中的 Cache-Control 头部值 |
| response-content-disposition | 指定响应消息中的 Content-Disposition 头部值 |
| response-content-encoding | 指定响应消息中的 Content-Encoding 头部值 |

**请求头部**

| 头域 | 说明 |
| ---- | ---- |
| Range | 下载指定位置的字节数 |
| If-Modified-Since | 指定时间，只有在指定时间之后更新过，才返回对象，否则返回 304 |
| If-Unmodified-Since | 指定时间，只有在指定时间之前未更新，才返回对象，否则返回 412 | 
| If-Match | 指定 ETag，只有对象的 ETag 和 ETag 匹配，才返回对象，否则返回 412 |
| If-None-Match | 指定 ETag，只有对象的 ETag 和 ETag 不匹配，才返回对象，否则返回 304 |

**结果解析**

响应信息通过 header 返回。

| 头域 | 说明 |
| ---- | ---- |
| x-amz-version-id | 获取的对象的版本号 |
| x-amz-meta- | 对象的自定义元数据，与上传对象时的设置一致 |
| x-amz-delete-marker | 当获取的对象是一个删除标记，响应中会携带该头部且值为 true；当获取的对象不是删除标记，则不会携带该头部 |

**示例**

- 获取一个对象

    ```lang-rest
    GET /bucketname/ObjectName HTTP/1.1
    Host: ip:port
    Date: Sat, 17 Aug 2019 17:50:00 GMT
    Authorization: authorization string
    ```
    
    响应结果如下：
    
    ```lang-rest
    HTTP/1.1 200 OK
    Date: Sat, 17 Aug 2019 17:50:00 GMT
    Last-Modified: Sat, 17 Aug 2019 17:40:00 GMT
    ETag: "fba9dede5f27731c9771645a39863328"
    Content-Length: 434234
    
    [434234 bytes of object data]
    ```

- 指定版本号获取一个对象

    ```lang-rest
    GET /bucketname/myObject?versionId=4433 HTTP/1.1
    Host: ip:port
    Date: Sat, 17 Aug 2019 17:50:00 GMT
    Authorization: authorization string
    ```
    
    响应结果如下：
    
    ```lang-rest
    HTTP/1.1 200 OK
    Date: Sat, 17 Aug 2019 17:50:00 GMT
    Last-Modified: Sat, 17 Aug 2019 17:40:00 GMT
    x-amz-version-id: 4433
    ETag: "fba9dede5f27731c9771645a39863328"
    Content-Length: 434234
    Content-Type: text/plain
    
    [434234 bytes of object data]
    ```

## HEAD Object

获取对象的元数据信息，不获取对象内容 

**请求语法**

```lang-rest
HEAD /bucketname/ObjectName HTTP/1.1
Host: ip:port
Authorization: authorization string
```

**请求参数**

| 参数名 | 说明 |
| ----   | ---- |
| versionId | 获取指定版本的对象时通过此参数指定版本号 |

**请求头部**

| 头域 | 说明 |
| ---- | ---- |
| Range | 下载指定位置的字节数 |
| If-Modified-Since | 指定时间，只有在指定时间之后更新才返回对象，否则返回 304 |
| If-Unmodified-Since | 指定时间，只有在指定时间之前未更新才返回对象，否则返回 412 |
| If-Match | 指定 ETag，只有对象的 ETag 和 ETag 匹配才返回对象，否则返回 412 |
| If-None-Match | 指定 ETag，只有对象的 ETag 和 ETag 不匹配，才返回对象，否则返回 304 |

**结果解析**

响应信息通过 header 返回。

| 头域 | 说明 |
| ---- | ---- |
| x-amz-version-id | 获取的对象的版本号 | 
| x-amz-meta- | 对象的自定义元数据，与上传对象时的设置一致 |

**示例**

- 获取对象的元数据

    ```lang-rest
    HEAD /bucketname/my-image.jpg HTTP/1.1
    Host: ip:port
    Date: Sat, 17 Aug 2019 17:50:00 GMT
    Authorization: authorization string
    ```
    
    响应结果如下：
    
    ```lang-rest
    HTTP/1.1 200 OK
    x-amz-version-id: 3344
    Date: Sat, 17 Aug 2019 17:50:00 GMT
    Last-Modified: Sat, 17 Aug 2019 17:40:00 GMT
    ETag: "fba9dede5f27731c9771645a39863328"
    Content-Length: 434234
    Content-Type: text/plain
    ```

- 获取指定版本对象的元数据

    ```lang-rest
    HEAD /bucketname/my-image.jpg?versionId=3344 HTTP/1.1
    Host: ip:port
    Date: Sat, 17 Aug 2019 17:55:00 GMT
    Authorization: authorization string
    ```
    
    响应结果如下：
    
    ```lang-rest
    HTTP/1.1 200 OK
    x-amz-version-id: 3344
    Date: Sat, 17 Aug 2019 17:55:00 GMT
    Last-Modified: Sat, 17 Aug 2019 17:40:00 GMT
    ETag: "fba9dede5f27731c9771645a39863328"
    Content-Length: 434234
    Content-Type: text/plain
    ```

## DELETE Object

删除对象

> **Note:**
>
> 当用户打开了版本控制，删除对象时会生成一个删除标记，原来的对象还保存在系统中。如果用户需要永久删除对应版本，可以指定版本号进行删除。

**请求语法**

```lang-rest
DELETE /bucketname/ObjectName HTTP/1.1
Host: ip:port
Date: date
Authorization: authorization string
```

**请求参数**

| 参数名 | 说明 |
| ----   | ---- |
| versionId | 指定版本号，用于删除指定版本的对象 |

**结果解析**

响应信息通过 header 返回。

| 头域 | 说明 |
| ---- | ---- |
| x-amz-delete-marker | 1. 当删除操作生成一个删除标记时，会返回该头部且值为 true <br> 2. 当通过指定版本号删除对象时，如果删除的是一个删除标记，则会返回该头部且值为 true |
| x-amz-version-id | 1. 当删除操作生成一个删除标记时，该头部记录删除标记的版本号 <br> 2. 当通过指定版本号删除对象时，该头部记录被删除的版本号 |

**示例**

- 删除一个未开启版本控制的桶内的对象

    ```lang-rest
    DELETE /bucketname/my-second-image.jpg HTTP/1.1
    Host: ip:port
    Date: Sat, 17 Aug 2019 17:55:00 GMT
    Authorization: authorization string
    Content-Type: text/plain
    ```
    
    响应结果如下：
    
    ```lang-rest
    HTTP/1.1 204 NoContent
    Date: Sat, 17 Aug 2019 17:55:00 GMT
    Content-Length: 0
    ```

- 删除指定版本对象

    ```lang-rest
    DELETE /bucketname/my-third-image.jpg?versionId=4455 HTTP/1.1
    Host: ip:port
    Date: Sat, 17 Aug 2019 17:58:00 GMT
    Authorization: authorization string
    ```
    
    响应结果如下：
    
    ```lang-rest
    HTTP/1.1 204 NoContent
    x-amz-version-id: 4455
    Date: Sat, 17 Aug 2019 17:58:00 GMT
    Content-Length: 0
    ```

## DELETE Objects

删除多个对象

**请求语法**

```lang-rest
POST /bucketname?delete HTTP/1.1
Host: ip:port
Date: date
Authorization: authorization string

<Delete>
   <Object>
      <Key>string</Key>
      <VersionId>string</VersionId>
   </Object>
   <Quiet>boolean</Quiet>
</Delete>
```

**请求元素**



用户需要在请求消息体中使用 XML 形式指定待删除的对象列表，系统对每个待删除的对象都会执行单独的删除操作，并将删除结果返回，单个对象的删除操作参考 DELETE object。

| 元素 | 说明 |
| ---- | ---- |
| Delete| 包含 Object 和 Quiet |
| Object | 待删除的对象，包含 Key 和 VersionId |
| Key | 对象名称 |
| VersionId | 对象版本号，可选 |
| Quiet | 静默标志，boolean类型，当请求中包含该字段且值为true时，返回结果中仅包含删除失败的对象的结果。默认不携带此标志，返回结果中包含全部删除对象的操作结果。 |

**结果解析**

响应消息体中返回 XML 形式的结果，包含各个对象的删除结果。

| 元素 | 说明 |
| ---- | ---- |
| DeleteResult | 包含 Deleted 和 Error |
| Deleted | 删除成功的记录，包含 Key，VersionId，DeleteMarker 和 DeleteMarkerVersionId |
| Key | 已删除的对象名称 |
| VersionId | 指定删除的对象版本号 |
| DeleteMarker | 当删除操作产生了 DeleteMarker 或指定版本号删除的对象是一个DeleteMarker 时，该字段为true。  | 
| DeleteMarkerVersionId | 当删除操作产生了 DeleteMarker 或指定版本号删除的对象是一个DeleteMarker 时，该字段为新产生或指定的 VersionId |
| Error | 删除失败的记录，包括Code, Key, Message, VersionId |
| Code | 错误码 | 
| Key | 删除失败的对象名称 | 
| Message | 失败的描述 |
| VersionId | 指定删除的对象版本号 |

**示例**

- 在未开启版本控制的桶内删除多个对象

   ```lang-rest
   POST /bucketname?delete HTTP/1.1
   Host: ip:port
   Date: date
   Authorization: authorization string

   <Delete>
      <Object>
         <Key>key1</Key>
      </Object>
      <Object>
         <Key>key2</Key>
      </Object>
   </Delete>  
   ```

   删除成功，响应结果如下：

   ```lang-rest
   <DeleteResult>
      <Deleted>
         <Key>key1</Key>
      </Deleted>
      <Deleted>
         <Key>key2</Key>
      </Deleted>
   </DeleteResult>
   ```

- 在开启版本控制的桶内删除多个对象

   ```lang-rest
   POST /bucketname?delete HTTP/1.1
   Host: ip:port
   Date: date
   Authorization: authorization string

   <Delete>
      <Object>
         <Key>key1</Key>
      </Object>
      <Object>
         <Key>key2</Key>
      </Object>
   </Delete>  
   ```

   生成DeleteMarker，响应结果如下：

   ```lang-rest
   <DeleteResult>
      <Deleted>
         <Key>key1</Key>
         <DeleteMarker>true</DeleteMarker>
         <DeleteMarkerVersionId>1</DeleteMarkerVersionId>
      </Deleted>
      <Deleted>
         <Key>key2</Key>
         <DeleteMarker>true</DeleteMarker>
         <DeleteMarkerVersionId>1</DeleteMarkerVersionId>
      </Deleted>
   </DeleteResult>
   ```
- 指定版本删除多个对象

   ```lang-rest
   POST /bucketname?delete HTTP/1.1
   Host: ip:port
   Date: date
   Authorization: authorization string

   <Delete>
      <Object>
         <Key>key1</Key>
         <VersionId>0</VersionId>
      </Object>
      <Object>
         <Key>key2</Key>
         <VersionId>0</VersionId>
      </Object>
      <Object>
         <Key>key1</Key>
         <VersionId>1</VersionId>
      </Object>
   </Delete> 
   ```

   删除成功，响应结果如下：

   ```lang-rest
   <DeleteResult>
      <Deleted>
         <Key>key1</Key>
         <VersionId>0</VersionId>
      </Deleted>
      <Deleted>
         <Key>key2</Key>
         <VersionId>0</VersionId>
      </Deleted>
      <Deleted>
         <Key>key1</Key>
         <VersionId>1</VersionId>
         <DeleteMarker>true</DeleteMarker>
         <DeleteMarkerVersionId>1</DeleteMarkerVersionId>
      </Deleted>
   </DeleteResult>
   ```


## Initiate Multipart Upload

初始化分段上传，获得 upload ID

**请求语法**

```lang-rest
POST /bucketname/ObjectName?uploads HTTP/1.1
Host: ip:port
Date: date
Authorization: authorization string
```

**请求头部**

初始化时携带的元数据，在合并分段上传生成一个完整对象时作为对象的元数据。

| 头域 | 说明 |
| ---- | ---- |
| Cache-Control | 指定请求/响应链中的缓存属性 |
| Content-Disposition | 当获取对象时，该属性提示将对象保存为的文件名 |
| Content-Encoding | 对象的附加编码类型，例如压缩文档使用的 gzip 类型 |
| Content-Type | 请求内容的 MIME 类型 |
| Expires | 缓存的超时时间 |
| x-amz-meta- | 自定义元数据 |

**结果解析**

响应消息体中返回 XML 形式的结果，包含 upload ID。

| 元素 | 说明 |
| ---- | ---- |
| InitiateMultipartUploadResult | 包含初始化分段的结果 |
| Bucket | 初始化分段上传对象所在存储桶 |
| Key | 初始化分段上传的对象名称 |
| UploadId | 初始化分段上传的 ID，用来唯一标识一个分段上传请求 | 

**示例**

初始化分段上传后，响应结果如下：

```lang-xml
HTTP/1.1 200 OK
Date: Sat, 17 Aug 2019 17:59:00 GMT
Content-Length: 151

<InitiateMultipartUploadResult>
  <Bucket>bucketname</Bucket>
  <Key>ObjectName</Key>
  <UploadId>56778</UploadId>
</InitiateMultipartUploadResult>
```

## Upload Part

上传分段

**请求语法**

```lang-rest
PUT /bucketname/ObjectName?partNumber=PartNumber&uploadId=UploadId HTTP/1.1
Host: ip:port
Date: date
Content-Length: Size
Authorization: authorization string
```

**请求参数**

| 参数名 | 说明 |
| ----   | ---- |
| partNumber | 分段编号，有效范围为 1~10000 |
| uploadId | upload ID，可以在 Initiate Multipart Upload 获得 |

**示例**

上传一个分段

```lang-rest
PUT /bucketname/ObjectName?partNumber=1&uploadId=56778 HTTP/1.1
Host: ip:port
Date: Sat, 17 Aug 2019 18:05:00 GMT
Content-Length: 10485760
Content-MD5: pUNXr/BjKK5G2UKvaRRrOA==
Authorization: authorization string

[10485760 bytes of object data]
```

响应结果如下：

```lang-rest
HTTP/1.1 200 OK
Date: Sat, 17 Aug 2019 18:05:00 GMT
ETag: "b54357faf0632cce46e942fa68356b38"
Content-Length: 0
```

## List Parts

查询分段列表

**请求语法**

```lang-rest
GET /bucketname/ObjectName?uploadId=UploadId HTTP/1.1
Host: ip:port
Date: Date
Authorization: authorization string
```

**请求参数**

| 参数名 | 说明 |
| ----   | ---- |
| uploadId | upload ID，可以在 Initiate Multipart Upload 获得 |
| max-parts | 一次返回的最大分段数 |
| part-number?-marker | 查询的起始位置 |
| encoding-type | 响应结果编码类型，只支持 url；由于对象名称可以包含任意字符，但是 XML 对某些特别的字符无法解析，所以需要对响应中的对象名称进行编码 | 

**结果解析**

查询结果以 XML 形式返回。

| 元素 | 说明 |
| ---- | ---- |
| ListPartsResult | 包含查询分段列表结果 |
| Bucket | 分段上传的存储桶名称 |
| Key | 分段上传的对象名称 |
| UploadId | 分段上传请求的 upload ID |
| Initiator | 分段上传的发起者 |
| Owner | 存储桶的拥有者 |
| DisplayName | 用户的名称 |
| ID | 用户 ID | 
| PartNumberMarker | 查询的 part-number?-marker 条件 |
| MaxParts | 查询的 max-parts 条件 |
| IsTruncated | 是否被截断 | 
| NextPartNumberMarker | 当 IsTruncated 为 true 时，该字段记录下一次查询的起始位置 |
| Encoding-Type | 查询的 encoding-type 条件 |
| Part | 包含分段内容 |
| PartNumber | 分段编号 | 
| LastModified | 分段的最新修改时间 |
| ETag | 分段的 ETag |
| Size | 分段的大小 |

**示例**

查询 upload ID 为 56778 的分段列表，指定 max-parts 为 2，part-number-marker 为 1

```lang-rest
GET /bucketname/ObjectName?uploadId=56778&max-parts=2&part-number-marker=1 HTTP/1.1
Host: ip:port
Date: Sat, 17 Aug 2019 18:10:00 GMT
Authorization: authorization string
```

响应结果如下：

```lang-xml
HTTP/1.1 200 OK
Date: Sat, 17 Aug 2019 18:10:00 GMT
Content-Length: 838

<ListPartsResult>
  <Bucket>bucketname</Bucket>
  <Key>ObjectName</Key>
  <UploadId>56778</UploadId>
  <Initiator>
    <DisplayName>username</DisplayName>
    <ID>34455</ID>
  </Initiator>
  <Owner>
    <DisplayName>username</DisplayName>
    <ID>34455</ID>
  </Owner>
  <PartNumberMarker>1</PartNumberMarker>
  <NextPartNumberMarker>3</NextPartNumberMarker>
  <MaxParts>2</MaxParts>
  <IsTruncated>true</IsTruncated>
  <Part>
    <PartNumber>2</PartNumber>
    <LastModified>2019-08-1T17:06:06.000Z</LastModified>
    <ETag>"7778aef83f66abc1fa1e8477f296d394"</ETag>
    <Size>10485760</Size>
  </Part>
  <Part>
    <PartNumber>3</PartNumber>
    <LastModified>2019-08-1T17:06:23.000Z</LastModified>
    <ETag>"aaaa18db4cc2f85cedef654fccc4a4x8"</ETag>
    <Size>10485760</Size>
  </Part>
</ListPartsResult>
```

## Complete Multipart Upload

完成分段上传，合并分段

**请求语法**

```lang-rest
POST /bucketname/ObjectName?uploadId=UploadId HTTP/1.1
Host: ip:port
Date: Date
Content-Length: Size
Authorization: authorization string

<CompleteMultipartUpload>
  <Part>
    <PartNumber>PartNumber</PartNumber>
    <ETag>ETag</ETag>
  </Part>
  ...
</CompleteMultipartUpload>
```

**请求元素**

| 元素 | 说明 |
| ---- | ---- |
| CompleteMultipartUpload | 包含所有要合并的的分段信息 |
| Part | 一个分段 |
| PartNumber | 分段编码 |
| ETag | 分段的 ETag |

**结果解析**

响应消息体中包含 XML 形式的合并结果，包含合并后对象的 ETag。

| 元素 | 说明 |
| ---- | ---- |
| CompleteMultipartUploadResult | 合并分段结果 |
| Location | 合并后对象的地址 |
| Bucket | 存储桶名称 |
| Key | 对象名称 |
| ETag | 合并后对象的 ETag，不一定是合并后完整对象的 MD5 值 |

**示例**

完成分段上传，合并分段

```lang-rest
POST /bucketname/ObjectName?uploadId=56778 HTTP/1.1
Host: ip:port
Date: Sat, 17 Aug 2019 18:10:30 GMT
Content-Length: 391
Authorization: authorization string

<CompleteMultipartUpload>
  <Part>
    <PartNumber>1</PartNumber>
    <ETag>"a54357aff0632cce46d942af68356b38"</ETag>
  </Part>
  <Part>
    <PartNumber>2</PartNumber>
    <ETag>"0c78aef83f66abc1fa1e8477f296d394"</ETag>
  </Part>
  <Part>
    <PartNumber>3</PartNumber>
    <ETag>"acbd18db4cc2f85cedef654fccc4a4d8"</ETag>
  </Part>
</CompleteMultipartUpload>
```

响应结果如下：

```lang-xml
HTTP/1.1 200 OK
Date: Sat, 17 Aug 2019 18:10:30 GMT

<CompleteMultipartUploadResult>
  <Location>http://ip:port/bucketname/ObjectName</Location>
  <Bucket>bucketname</Bucket>
  <Key>ObjectName</Key>
  <ETag>"3858f62230ac3c915f300c664312c11f-9"</ETag>
</CompleteMultipartUploadResult>
```

## Abort Multipart Upload

取消分段上传

**请求语法**

```lang-rest
DELETE /bucketname/ObjectName?uploadId=UploadId HTTP/1.1
Host: ip:port
Date: Date
Authorization: authorization string
```

**请求参数**

| 参数名 | 说明 |
| ----   | ---- |
| uploadId | 待取消的 upload ID |

**示例**

取消分段上传响应结果如下：

```lang-rest
HTTP/1.1 204 OK
Date: Sat, 17 Aug 2019 18:15:30 GMT
Content-Length: 0
```