本章介绍如何使用 Java 接口向 SequoiaS3 发送请求及接收响应。

SequoiaS3 安装路径下的 sample 目录中的压缩包是一个 maven 类型的 Java 工程样例。
解压后，使用 IDEA 打开该工程(File->Open->选中解压后文件夹中的Pom.xml->Open as Project->Open Existing Project->New Window)，将 Test.java 中的 endPoint 修改为提供 S3 服务的 IP 和端口，开始使用 sample 中的样例对存储桶和对象及区域进行操作。

初始化客户端
----
生成一个 AmazonS3 连接和一个 SequoiaS3 连接，此处需要修改 endPoint 的地址和端口，指向 SequoiaS3 的地址和端口。

```lang-java
String accessKey="ABCDEFGHIJKLMNOPQRST";
String secretKey="abcdefghijklmnopqrstuvwxyz0123456789ABCD";
String endPoint = "http://192.168.10.71:8002";

sequoiaS3 = SequoiaS3ClientBuilder.standard()
            .withEndpoint(endPoint)
            .withAccessKeys(accessKey, secretKey)
            .build();

AWSCredentials credentials = new BasicAWSCredentials(accessKey,secretKey);
AwsClientBuilder.EndpointConfiguration endpointConfiguration = new AwsClientBuilder.EndpointConfiguration(endPoint, null);
AmazonS3 s3 = AmazonS3ClientBuilder.standard()
            .withEndpointConfiguration(endpointConfiguration)
            .withCredentials(new AWSStaticCredentialsProvider(credentials))
            .build();
```

参数定义
----

```lang-java
String regionName = "region-example";
String bucketName = "bucketname";
String objectName = "objectname";
File file = new File("example.png");
```

创建区域
----

创建一个名为 region-example 的区域，该区域设置为每年创建一个集合空间，在该集合空间中每月创建一个新的集合，用于存放对象数据。

```lang-java
CreateRegionRequest request = new CreateRegionRequest(regionName)
            .withDataCLShardingType(DataShardingType.MONTH)
            .withDataCSShardingType(DataShardingType.YEAR);
sequoiaS3.createRegion(request);
```

获取区域列表
----

该接口可以查询当前系统中的区域列表。

```lang-java
ListRegionsResult listRegionsResult = sequoiaS3.listRegions();
```

查询区域配置
----

该样例查询区域配置及区域内的存储桶列表。

```lang-java
GetRegionResult regionResult = sequoiaS3.getRegion(regionName);
Region region = regionResult.getRegion();
List<String> buckets = regionResult.getBuckets();
System.out.println("region:" + regionResult.getRegion().toString());
for (int i=0; i < buckets.size(); i++) {
    System.out.println("Name:" + buckets.get(i));
}
```

创建存储桶
----

在 region-example 区域中创建一个名为 bucketname 的桶。

```lang-java
s3.createBucket(bucketName, regionName );
```

上传对象
----

该样例从本地上传一个名为 example.png 的文件到存储桶中，并命名为 objectname。

```lang-java
PutObjectRequest request = new PutObjectRequest(bucketName, objectName, file);
s3.putObject(request);
```

获取对象
----

该样例从存储桶中获得对象内容，并将对象内容存储在本地文件中。

```lang-java
GetObjectRequest request = new GetObjectRequest(bucketName, objectName);
S3Object result = s3.getObject(request);

S3ObjectInputStream s3is = result.getObjectContent();
FileOutputStream fos = new FileOutputStream(new File(object));
byte[] read_buf = new byte[1024];
int read_len = 0;
while ((read_len = s3is.read(read_buf)) > 0) {
    fos.write(read_buf, 0, read_len);
}
s3is.close();
fos.close();
```

查询桶内对象列表
----

该接口用于查询存储桶中所有对象。

```lang-java
ListObjectsV2Result result = s3.listObjectsV2(bucketName);
```

删除对象
----

该接口用于删除指定对象。

```lang-java
s3.deleteObject(bucketName, objectName);
```

删除桶
----

该接口用于删除指定存储桶。

```lang-java
s3.deleteBucket(bucketName);
```

删除区域
----

该接口用于删除指定区域。

```lang-java
sequoiaS3.deleteRegion(regionName);
```

查询区域是否存在
----

该接口用于判断指定区域是否存在。

```lang-java
Boolean isRegionExist = sequoiaS3.headRegion(regionName);
System.out.println("Region("+ regionName +") exist: " + isRegionExist);
```





