/* 3.2 以前（不包含 3.2）版本使用本工具时要求所有机器 SequoiaDB 的安装路径相同 */
/* 机器登入用户名定义, 3.2 及以上版本无须填写 */
var USERNAME = "sdbadmin";
/* 机器登入密码定义，3.2 及以上版本无须填写 */
var PASSWD = "xxxxxx";
/* 数据库登入用户名定义 */
var SDBUSERNAME = "sdbadmin";
/* 数据库登入密码定义 */
var SDBPASSWD = "xxxxxx";
/* 子网1机器定义，必须为字符串数组 */
var SUB1HOSTS = ["sdbserver1", "sdbserver2"];
/* 子网2机器定义，必须为字符串数组 */
var SUB2HOSTS = ["sdbserver3"];
/* 协调节点定义，如果协调节点已经在 Catalog的编目组信息中，则此处填写一个可用Coord即可 */
var COORDADDR = ["sdbserver1:11810"];
/* 剔除故障组节点后剩余的最小副本数, 若剔除后剩余副本数小于最小副本数，将不会执行剔除操作 */
var MINREPLICANUM = 2;
/* 执行init时是否重新选举 */
var NEEDREELECT = false;
/* 是否将init阶段生成的集群信息文件分发到集群的所有主机上 */
var NEEDBROADCASTINITINFO = true;

/*
注意：重要参数配置！！！
*/
/* 当前脚本所处的是子网 1 还是子网 2, 1 表示子网 1，2 表示子网 2，其它取值非法 */
var CURSUB = 1;
/* 当前子网是否为激活状态，如果取 false，则在集群执行切换后，当前子网的集群为只读状态 */
var ACTIVE = false;
