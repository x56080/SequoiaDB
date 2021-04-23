本章介绍区域相关接口。

## Create Region

增加一个区域或更新区域配置

> **Note:**
>
> 用户创建区域时可对区域内数据存储位置进行配置，即指定集合空间的生成方式，生成方式分为指定模式和自动创建模式。不能够同时指定两种模式，更新区域配置是也不能修改模式，更新区域配置只能够修改自动创建模式下的集合空间生成规则。

**请求语法**

```lang-rest
POST /region/?Action=CreateRegion&RegionName={regionname} HTTP/1.1
Host: ip:port
Content-Length: length
Date: date
Authorization: authorization string

<RegionConfiguration>
  <DataCSShardingType>year</DataCSShardingType>
  <DataCLShardingType>month</DataCLShardingType>
  <DataDomain>domain1</DataDomain>
  <MetaDomain>domain2</MetaDomain>
</RegionConfiguration>
```

**参数说明**

| 参数名 | 说明 |
| ----   | ---- |
| Action | 固定为 CreateRegion，表示该操作为创建一个区域 |
| RegionName | 指定区域名称 |

**请求元素**

用户可以在请求消息体中使用 XML 形式指定区域的配置。

| 元素 | 说明 |
| ---- | ---- |
| RegionConfiguration | 包含区域配置内容 |
| DataCSShardingType | 对象数据集合空间的生成规则，按照设定的时间生成指定的集合空间，类型为 string，默认值为"year"，有效值包括"year"、"quarter"和"month" |
| DataCLShardingType | 对象数据集合的生成规则，按照设定的时间生成指定的集合，类型为 string，默认值为"quarter"，有效值包括"year"、"quarter"和"month" |
| DataCSRange | 对象数据集合的生成规则，在设定时间段内能够生成的集合空间数量，类型为 int32 |
| DataDomain | 对象数据集合空间所属域，类型为 string，域必须已在 SequoiaDB 中定义，如果不填写域名称，则对象数据集合空间建立在系统域上 |
| DataLobPageSize | 对象数据集合空间的 LobPageSize，默认值为 262144，有效值包括 0、4096、8192、16384、32768、65536、131072、262144、524288 之一，0 表示选择默认值|
| DataReplSize | 对象集合的ReplSize，写操作同步的副本数，默认值为 -1，有效值包括 -1、0、1~7 |
| MetaDomain | 元数据集合空间所属域，类型为 string，域必须已在 SequoiaDB 中定义，若不填写则元数据集合空间建在系统域上 |
| DataLocation | 指定模式为对象数据的集合空间.集合名称，类型为 string，如 CS.CL |
| MetaLocation | 指定模式为元数据的集合空间.集合名称，类型为 string，如 CS.CL |
| MetaHisLocation | 指定模式为历史元数据的集合空间.集合名称，类型 string，如 CS.CL |

**示例**

创建区域的请求，指定对象数据集合空间和元数据集合空间的域，指定对象数据集合空间和对象数据集合的生成规则

```lang-rest
POST /region/?Action=CreateRegion&RegionName=region1 HTTP/1.1
Host: ip:port
Content-Length: length
Date: date
Authorization: authorization string

<RegionConfiguration>
  <DataCSShardingType>year</DataCSShardingType>
  <DataCLShardingType>month</DataCLShardingType>
  <DataDomain>domain1</DataDomain>
  <MetaDomain>domain2</MetaDomain>
</RegionConfiguration>
```

响应结果如下：

```lang-rest
HTTP/1.1 200 OK
Date: date
Content-Length: 0
```

## GetRegion

获取一个区域的配置

**请求语法**

```lang-rest
POST /region/?Action=GetRegion&RegionName={regionname} HTTP/1.1 
Host: ip:port
Date: date 
Authorization: authorization string 
```

**参数说明**

| 参数名 | 说明 |
| ----   | ---- |
| Action | 固定为 GetRegion，表示该操作为删除一个区域 |
| RegionName | 指定区域名称 |

**示例**

查询一个区域的配置信息

```lang-rest
POST /region/?Action=GetRegion&RegionName=region1 HTTP/1.1
Host: ip:port
Date: Wed, 12 Oct 2009 17:50:00 GMT
Authorization: authorization string
```

响应结果如下：

```lang-xml
<RegionConfiguration>
  <Name>region1</Name>
  <DataCSShardingType>year</DataCSShardingType>
  <DataCLShardingType>month</DataCLShardingType>
  <DataCSRange>1</DataCSRange>
  <DataDomain>domain1</DataDomain>
  <MetaDomain>domain2</MetaDomain>
  <DataLobPageSize>262144</DataLobPageSize>
  <DataReplSize>-1</DataReplSize>
  <DataLocation/>
  <MetaLocation/>
  <MetaHisLocation/>
  <Buckets>
    <Bucket>bucketname1</Bucket>
    <Bucket>bucketname2</Bucket>
  </Buckets>
</RegionConfiguration>

```

## DeleteRegion

删除一个区域

**请求语法**

```lang-rest
POST /region/?Action=DeleteRegion&RegionName={regionname} HTTP/1.1
Host: ip:port
Date: date 
Authorization: authorization string
```

**参数说明**

| 参数名 | 说明 |
| ----   | ---- |
| Action | 固定为 DeleteRegion，表示该操作为删除一个区域 |
| RegionName | 指定区域名称 |

**示例**

删除一个区域的请求

```lang-rest
POST /region/?Action=DeleteRegion&RegionName=region1 HTTP/1.1
Host: ip:port
Date: date 
Authorization: authorization string
```

响应结果如下：

```lang-rest
HTTP/1.1 204 No Content 
Date: date 
```

## ListRegions

查询区域列表，可以查询当前系统中所有区域名称

**请求语法**

```lang-rest
POST /region/?Action=ListRegions HTTP/1.1 
Host: ip:port
Date: date 
Authorization: authorization string 
```

**参数说明**

| 参数名 | 说明 |
| ----   | ---- |
| Action | 固定为 ListRegions，表示该操作为查询区域列表 |

**结果解析**

查询结果以 XML 形式在响应消息头中显示。

| 元素 | 说明 |
| ---- | ---- |
| ListAllRegionsResult | 包含一个到多个 Region | 
| Region | 区域名称 |

**示例**

查询区域列表的响应结果如下：

```lang-rest
<ListAllRegionsResult>
  <Region>region1</Region>
  <Region>region1</Region>
  <Region>region1</Region>
</ListAllRegionsResult>
```

## HeadRegion

查询区域是否存在

**请求语法**

```lang-rest
POST /region/?Action=HeadRegion&RegionName={regionname} HTTP/1.1
Host: ip:port
Date: date 
Authorization: authorization string 
```

**示例**

响应结果如下：

```lang-rest
HTTP/1.1 200 OK
Date: date
```