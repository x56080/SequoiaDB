配置文件相对路径
nodejs-12.14.1/demo/src/config/adapter.js

修改访问地址：
exports.model = {
  type: 'mongo',
  common: {
      logConnect: isDev,
      logger: msg => think.logger.info(msg)
  },
  mongo: {
      host: 'localhost',
      database: 'cs_think_mongo',  // 自己创建的数据库名字
      port: 11817,
      user: '',
      password: ''
  }
};


测试用例相对路径：
nodejs-12.14.1/demo/src/controller/index.js

执行测试用例：
启动npm：npm start
web页面加载：http://192.168.20.55:8360/


查看当前mongodb版本：
vi nodejs-12.14.1/demo/package-lock.json

// think-mongo驱动mongodb的版本

    "think-mongo": {
      ......
      "requires": {
        ......
        "mongodb": "^2.2.30",
        ......


注意事项：
必须在 demo 目录下执行用例


【FAQ】
问题1：执行 npm start 失败
susesp1-2:/opt/nodejs-12.14.1/demo # ../bin/npm start
/usr/bin/env: node: No such file or directory
解决办法：
对 /usr/bin/node 文件添加软连接，软连接地址为实际的 node 执行文件地址，如：
/usr/bin # ln -s /opt/nodejs-12.14.1/bin/node node
/usr/bin # ll node
lrwxrwxrwx 1 root root 28 Sep 30 17:30 node -> /opt/nodejs-12.14.1/bin/node


