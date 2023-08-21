package com.mongodb.m2s.testcommon;

/**
 * @Descreption m2s测试参数配置类
 * @Author
 * @CreateDate
 * @UpdateUser
 * @UpdateDate 2023/8/17
 * @UpdateRemark
 * @Version
 */
public class M2STestBase {

    // mongodb服务地址
    public static String mongodbUri = "mongodb://192.168.17.196:27500";
    // 工具根路径
    public static String toolRootPath = "/opt/test/m2s/build/m2s_1.0.0_linux_x86_64/";
    // 输出报告根路径
    public static String m2sTestPath = "/tmp/";

    // collector收集工具
    // 工具路径
    public static String collectorPath = toolRootPath
            + "m2s-collector/m2s-collector";
    // 收集文件输出路径
    public static String collectorOutputPath = m2sTestPath
            + "collector_output/";
    // 采样数量
    public static int collectSample = 100;

    // sniffer捕获工具
    // 工具路径
    public static String snifferPath = toolRootPath
            + "m2s-sniffer/bin/sniffer.sh";
    // 监听端口(对应工具启动的-l参数)
    public static String snifferListenPort = "30000";
    // sniffer监听地址(对应工具启动的-m参数)
    public static String snifferAddr = "192.168.17.19:27500";
    // sniffer报告输出路径
    public static String snifferOutputPath = m2sTestPath + "sniffer_output/";

    // analyzer分析工具
    // 工具路径
    public static String analyzerPath = toolRootPath
            + "m2s-analyzer/m2s-analyzer";
    // 分析文件输出路径
    public static String analyzerOutputPath = m2sTestPath + "analyzer_output/";
    // 分析兼容性的sequoiadb版本
    public static String sdbVersion = "7.0.0";

    // SSH工具连接信息
    public static String remoteHost = "192.168.17.196";
    public static String remoteUser = "root";
    public static String remotePwd = "sequoiadb";

}
