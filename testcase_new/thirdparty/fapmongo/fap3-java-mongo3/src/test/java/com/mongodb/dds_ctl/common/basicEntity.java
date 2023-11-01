package com.mongodb.dds_ctl.common;

/**
 * @Descreption
 * @Author wangxingming
 * @CreateDate 2023/10/17
 * @UpdateUser wangxingming
 * @UpdateDate 2023/10/17
 */
public class basicEntity {
    private Net net;
    private Replication replication;
    private Sharding sharding;
    private Storage storage;
    private SystemLog systemLog;

    public SystemLog getSystemLog() {
        return systemLog;
    }

    public void setSystemLog( SystemLog systemLog ) {
        this.systemLog = systemLog;
    }

    public Storage getStorage() {
        return storage;
    }

    public void setStorage( Storage storage ) {
        this.storage = storage;
    }

    public Net getNet() {
        return net;
    }

    public void setNet( Net net ) {
        this.net = net;
    }

    public Replication getReplication() {
        return replication;
    }

    public void setReplication( Replication replication ) {
        this.replication = replication;
    }

    public Sharding getSharding() {
        return sharding;
    }

    public void setSharding( Sharding sharding ) {
        this.sharding = sharding;
    }

    public static class SystemLog {
        private String destination;
        private String path;
        private Boolean logAppend;

        public String getDestination() {
            return destination;
        }

        public void setDestination( String destination ) {
            this.destination = destination;
        }

        public String getPath() {
            return path;
        }

        public void setPath( String path ) {
            this.path = path;
        }

        public Boolean getLogAppend() {
            return logAppend;
        }

        public void setLogAppend( Boolean logAppend ) {
            this.logAppend = logAppend;
        }
    }

    public static class Storage {
        private String dbPath;

        public String getDbPath() {
            return dbPath;
        }

        public void setDbPath( String dbPath ) {
            this.dbPath = dbPath;
        }
    }

    public static class Net {
        private String bindIp;
        private String port;

        public String getBindIp() {
            return bindIp;
        }

        public void setBindIp( String bindIp ) {
            this.bindIp = bindIp;
        }

        public String getPort() {
            return port;
        }

        public void setPort( String port ) {
            this.port = port;
        }
    }

    public static class Replication {
        private String replSetName;

        public String getReplSetName() {
            return replSetName;
        }

        public void setReplSetName( String replSetName ) {
            this.replSetName = replSetName;
        }
    }

    public static class Sharding {
        private String clusterRole;
        private String configDB;

        public String getConfigDB() {
            return configDB;
        }

        public void setConfigDB( String configDB ) {
            this.configDB = configDB;
        }

        public String getClusterRole() {
            return clusterRole;
        }

        public void setClusterRole( String clusterRole ) {
            this.clusterRole = clusterRole;
        }
    }
}
