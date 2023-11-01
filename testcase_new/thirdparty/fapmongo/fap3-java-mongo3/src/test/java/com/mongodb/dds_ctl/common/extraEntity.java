package com.mongodb.dds_ctl.common;

/**
 * @Descreption
 * @Author wangxingming
 * @CreateDate 2023/10/19
 * @UpdateUser wangxingming
 * @UpdateDate 2023/10/19
 */
public class extraEntity {
    private ProcessManagement processManagement;

    public ProcessManagement getProcessManagement() {
        return processManagement;
    }

    public void setProcessManagement( ProcessManagement processManagement ) {
        this.processManagement = processManagement;
    }

    public static class ProcessManagement {
        private Boolean fork;
        private String pidFilePath;

        public Boolean getFork() {
            return fork;
        }

        public void setFork( Boolean fork ) {
            this.fork = fork;
        }

        public String getPidFilePath() {
            return pidFilePath;
        }

        public void setPidFilePath( String pidFilePath ) {
            this.pidFilePath = pidFilePath;
        }
    }
}
