package com.mongodb.dds_ctl.common;

/**
 * @Descreption
 * @Author wangxingming
 * @CreateDate 2023/10/17
 * @UpdateUser wangxingming
 * @UpdateDate 2023/10/17
 */
public class CTLTestBase {
    // SSH工具连接信息
    public static String remoteHost = "192.168.17.85";
    public static String remoteHost1 = "192.168.18.46";
    public static String remoteUser = "root";
    public static String remotePwd = "sequoiadb";
    public static String sdbUser = "sdbadmin";
    public static String sdbPwd = "Admin@1024";

    // 集群端口信息
    public static Integer shardingPort = 16000;
    public static Integer replicaSetPort = 10000;
    public static Integer routePort = 20000;

    // sdb_dds_ctl工具路径
    private static String ddsPath = "/opt/sequoiadds/";
    public static String ctlPath = ddsPath + "bin/sdb_dds_ctl";
    public static String mongoshPath = ddsPath + "bin/mongosh";
    public static String configFilePath = ddsPath + "configFile/";  // 目录需要手工创建
    public static String confPath = ddsPath + "conf/local/";
    public static String dataBasePath = ddsPath + "database/";
}
