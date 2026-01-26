
# sdb-schedule

sdb-schedule 是一个基于微服务架构实现的 SequoiaDB 数据迁移与切换工具。

## 1 项目目录结构

```shell
|--src          # sdb-schedule 源码
  |--project    
    |--assembly    # 项目打包相关配置
    |--common      
      |--common   # 通用模块
      |--tools  # 工具模块
    |--server     
      |--schedule-server  # 调度服务模块
        |--schedule-ui    # 调度服务 Web 前端源码
        |--src  # 调度服务后端源码
|--README.md    # sdb-schedule 项目说明文档
|--build.py    # sdb-schedule 编译打包脚本
```

## 2 打包

### 2.1 环境要求

| 软件环境 | 版本        |
| -------- |-----------|
| jdk      | 1.8及以上    |
| python   | 2.x/3.x版本 |
| maven    | 3.6 及以上   |
| node.js    | 16.0及以上   |

```xml
<!-- maven需要在settings.xml配置私服 -->
<mirrors>
    <mirror>
      <id>central</id>
      <mirrorOf>*</mirrorOf>
      <name>Human Readable Name for this Mirror.</name>
      <url>http://192.168.20.204:8082/repository/maven-public/</url>
    </mirror>
</mirrors>
```

### 2.2 git 配置

如果是在 Windows 下操作，在 clone 项目之前还需要设置 ：

```shell
# 设置禁止 git 自动转换换行符
git config --global core.autocrlf false
git config --global core.safecrlf true
```

> 注：若是不设置禁用，git 换行符在 Windows 和 Linux 上将会不一致，这会导致 Windows 上编译的 jar 无法在 Linux 上使用。

### 2.3 打包项目

```shell
# 进入源码根目录，编译、打包项目,例如打包版本号为 5.8.6 的版本
python build.py 5.8.6
```

编译打包之后的产物在 `源码根目录/src/project/assembly/target/ 目录下 `sdb-schedule-5.8.6-release.tar.gz` 。

### 2.4 解压目录

```shell
tar -zxvf sdb-schedule-5.8.6-release.tar.gz
```

产物解压后的目录结构：

```shell
|-sdb-schedule/
  |--bin         # 工具脚本
  |--jars        # 依赖的 jar 包
  |--script      # 快速部署脚本存放目录
```

### 2.5 调度服务节点目录

- 如下是调度服务节点的 log 目录说明

```shell
|--admin
    |--admin.log                    # 执行 schadmin.sh 脚本输出的日志
|--schedule-server
    |--9000                        # 根据该服务设置的端口命名
        |--scheduleserver.log       # 该服务节点运行过程输出的日志
        |--error.out                # Linux 标准错误流输出日志
|--start
    |--start.log                    # 工具启动节点的输出日志
```

- 如下是调度服务节点的 conf 目录说明

```shell
|--schedule-server
    |--9000                        # 根据该服务设置的端口命名
        |--application.properties       # 该服务节点配置文件
        |--logback.xml                # 该服务节点日志配置文件
```

## 3. 前端开发指南

1. 修改 `src\project\server\schedule-server\src\main\java\com\sequoiadb\schedule\config\WebMvcConfig.java`，加上如下内容，以支持跨域访问：

```java
    @Override
    public void addCorsMappings(CorsRegistry registry) {
        registry.addMapping("/**").allowedOrigins("*").allowedMethods("*")
                .exposedHeaders("x-record-count").allowedHeaders("*");

    }
```

2. 编译打包，将编译好的 jar 包替换掉调度服务的 jar 包，然后启动调度服务

3. 修改 `src\project\server\schedule-server\schedule-ui\src\utils\request.js`，修改 baseURL 为调度服务节点地址：

```javascript
const service = axios.create({
  //baseURL: process.env.VUE_APP_BASE_API,
  baseURL: 'http://192.168.31.15:9000',
  timeout: 1800000, // 30分钟
  headers: {
    'Content-Type': 'application/json'
  }
})
```

4. 启动前端工程。进入到前端源码根目录 `src\project\server\schedule-server\schedule-ui`，执行命令：

```shell
npm run dev
```

5. 最后开发调试完成后，将 WebMvcConfig.java 和 request.js 文件还原回去即可。
