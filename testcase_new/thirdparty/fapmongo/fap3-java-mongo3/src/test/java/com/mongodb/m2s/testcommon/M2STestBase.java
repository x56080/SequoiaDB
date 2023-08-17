package com.mongodb.m2s.testcommon;

/**
 * @Descreption
 * @Author
 * @CreateDate
 * @UpdateUser
 * @UpdateDate 2023/8/17
 * @UpdateRemark
 * @Version
 */
public class M2STestBase {
    public static String mongodbUri = "mongodb://192.168.17.196:27017";
    public static String collectorUri = "/opt/test/m2s/build/m2s_1.0.0_linux_x86_64/m2s-collector/m2s-collector";
    public static String collectorOutputPath = "/tmp/collector_output/";
    public static String analyzerUri = "/opt/test/m2s/build/m2s_1.0.0_linux_x86_64/m2s-analyzer/m2s-analyzer";
    public static String analyzerOutputPath = "/tmp/analyzer_output/";
    public static String snifferUri = "/opt/test/m2s/build/m2s_1.0.0_linux_x86_64/m2s-sniffer/m2s-sniffer";
    public static String snifferOutputPath = "/tmp/sniffer_output/";
    public static String sdbVersion = "7.0.0";
    public static String remoteHost = "192.168.17.196";
    public static String remoteUser = "root";
    public static String remotePwd = "sequoiadb";

    public static String getMongodbUri() {
        return mongodbUri;
    }

    public static void setMongodbUri(String mongodbUri) {
        M2STestBase.mongodbUri = mongodbUri;
    }

    public static String getCollectorUri() {
        return collectorUri;
    }

    public static void setCollectorUri(String collectorUri) {
        M2STestBase.collectorUri = collectorUri;
    }

    public static String getCollectorOutputPath() {
        return collectorOutputPath;
    }

    public static void setCollectorOutputPath(String collectorOutputPath) {
        M2STestBase.collectorOutputPath = collectorOutputPath;
    }

    public static String getAnalyzerUri() {
        return analyzerUri;
    }

    public static void setAnalyzerUri(String analyzerUri) {
        M2STestBase.analyzerUri = analyzerUri;
    }

    public static String getAnalyzerOutputPath() {
        return analyzerOutputPath;
    }

    public static void setAnalyzerOutputPath(String analyzerOutputPath) {
        M2STestBase.analyzerOutputPath = analyzerOutputPath;
    }

    public static String getSnifferUri() {
        return snifferUri;
    }

    public static void setSnifferUri(String snifferUri) {
        M2STestBase.snifferUri = snifferUri;
    }

    public static String getSnifferOutputPath() {
        return snifferOutputPath;
    }

    public static void setSnifferOutputPath(String snifferOutputPath) {
        M2STestBase.snifferOutputPath = snifferOutputPath;
    }

    public static String getSdbVersion() {
        return sdbVersion;
    }

    public static void setSdbVersion(String sdbVersion) {
        M2STestBase.sdbVersion = sdbVersion;
    }

    public static String getRemoteHost() {
        return remoteHost;
    }

    public static void setRemoteHost(String remoteHost) {
        M2STestBase.remoteHost = remoteHost;
    }

    public static String getRemoteUser() {
        return remoteUser;
    }

    public static void setRemoteUser(String remoteUser) {
        M2STestBase.remoteUser = remoteUser;
    }

    public static String getRemotePwd() {
        return remotePwd;
    }

    public static void setRemotePwd(String remotePwd) {
        M2STestBase.remotePwd = remotePwd;
    }
}
