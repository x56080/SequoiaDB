/*******************************************************************************

   Copyright (C) 2011-Present SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   Source File Name = SequoiadbConfig.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          26/01/2026  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoiadb.schedule.metasource.config;

import com.sequoiadb.base.ConfigOptions;
import com.sequoiadb.datasource.ConnectStrategy;
import com.sequoiadb.datasource.DatasourceOptions;
import org.springframework.boot.context.properties.ConfigurationProperties;
import org.springframework.context.annotation.Configuration;

import java.util.List;

@Configuration
@ConfigurationProperties(prefix = "system.store.sequoiadb")
public class SequoiadbConfig {
    private List<String> urls;
    private String username;
    private String password;

    private final ConfigOptions connConf = new ConfigOptions();
    private final DatasourceOptions dsConf = new DatasourceOptions();
    private int connectTimeout = connConf.getConnectTimeout();
    private int socketTimeout = connConf.getSocketTimeout();
    private long maxAutoConnectRetryTime = connConf.getMaxAutoConnectRetryTime();
    private boolean useNagle = connConf.getUseNagle();
    private boolean useSSL = connConf.getUseSSL();
    private int keepAliveTime = 60 * 1000;
    private int maxConnectionNum = dsConf.getMaxCount();
    private boolean validateConnection = true;
    private int deltaIncCount = dsConf.getDeltaIncCount();
    private int maxIdleNum = dsConf.getMaxIdleCount();
    private int recheckCyclePeriod = 30 * 1000;
    private String location;
    private ConnectStrategy connectStrategy = dsConf.getConnectStrategy();
    private int minIdleNum = dsConf.getMinIdleCount();
    private int syncCoordInterval = dsConf.getSyncCoordInterval();
    private int sessionTimeout = dsConf.getSessionTimeout();
    private int networkBlockTimeout = dsConf.getNetworkBlockTimeout();
    private int cacheLimit = dsConf.getCacheLimit();

    public int getConnectTimeout() {
        return connectTimeout;
    }

    public void setConnectTimeout(int connectTimeout) {
        this.connectTimeout = connectTimeout;
    }

    public int getSocketTimeout() {
        return socketTimeout;
    }

    public void setSocketTimeout(int socketTimeout) {
        this.socketTimeout = socketTimeout;
    }

    public long getMaxAutoConnectRetryTime() {
        return maxAutoConnectRetryTime;
    }

    public void setMaxAutoConnectRetryTime(long maxAutoConnectRetryTime) {
        this.maxAutoConnectRetryTime = maxAutoConnectRetryTime;
    }

    public boolean getUseNagle() {
        return useNagle;
    }

    public void setUseNagle(boolean useNagle) {
        this.useNagle = useNagle;
    }

    public boolean getUseSSL() {
        return useSSL;
    }

    public void setUseSSL(boolean useSSL) {
        this.useSSL = useSSL;
    }

    public int getKeepAliveTime() {
        return keepAliveTime;
    }

    public void setKeepAliveTime(int keepAliveTime) {
        this.keepAliveTime = keepAliveTime;
    }

    public int getMaxConnectionNum() {
        return maxConnectionNum;
    }

    public void setMaxConnectionNum(int maxConnectionNum) {
        this.maxConnectionNum = maxConnectionNum;
    }

    public boolean getValidateConnection() {
        return validateConnection;
    }

    public void setValidateConnection(boolean validateConnection) {
        this.validateConnection = validateConnection;
    }

    public int getDeltaIncCount() {
        return deltaIncCount;
    }

    public void setDeltaIncCount(int deltaIncCount) {
        this.deltaIncCount = deltaIncCount;
    }

    public int getMaxIdleNum() {
        return maxIdleNum;
    }

    public void setMaxIdleNum(int maxIdleNum) {
        this.maxIdleNum = maxIdleNum;
    }

    public int getRecheckCyclePeriod() {
        return recheckCyclePeriod;
    }

    public void setRecheckCyclePeriod(int recheckCyclePeriod) {
        this.recheckCyclePeriod = recheckCyclePeriod;
    }

    public List<String> getUrls() {
        return urls;
    }

    public void setUrls(List<String> urls) {
        this.urls = urls;
    }

    public String getUsername() {
        return username;
    }

    public void setUsername(String username) {
        this.username = username;
    }

    public String getPassword() {
        return password;
    }

    public void setPassword(String password) {
        this.password = password;
    }

    public String getLocation() {
        return location;
    }

    public void setLocation(String location) {
        this.location = location;
    }

    public ConnectStrategy getConnectStrategy() {
        return connectStrategy;
    }

    public void setConnectStrategy(ConnectStrategy connectStrategy) {
        this.connectStrategy = connectStrategy;
    }

    public int getMinIdleNum() {
        return minIdleNum;
    }

    public void setMinIdleNum(int minIdleNum) {
        this.minIdleNum = minIdleNum;
    }

    public int getSyncCoordInterval() {
        return syncCoordInterval;
    }

    public void setSyncCoordInterval(int syncCoordInterval) {
        this.syncCoordInterval = syncCoordInterval;
    }

    public int getSessionTimeout() {
        return sessionTimeout;
    }

    public void setSessionTimeout(int sessionTimeout) {
        this.sessionTimeout = sessionTimeout;
    }

    public int getNetworkBlockTimeout() {
        return networkBlockTimeout;
    }

    public void setNetworkBlockTimeout(int networkBlockTimeout) {
        this.networkBlockTimeout = networkBlockTimeout;
    }

    public int getCacheLimit() {
        return cacheLimit;
    }

    public void setCacheLimit(int cacheLimit) {
        this.cacheLimit = cacheLimit;
    }

    @Override
    public String toString() {
        return "SequoiadbConfig{" + "urls=" + urls + ", username='" + username + '\'' + ", password='" + password +
                '\'' + ", connConf=" + connConf + ", dsConf=" + dsConf + ", connectTimeout=" + connectTimeout +
                ", socketTimeout=" + socketTimeout + ", maxAutoConnectRetryTime=" + maxAutoConnectRetryTime +
                ", useNagle=" + useNagle + ", useSSL=" + useSSL + ", keepAliveTime=" + keepAliveTime +
                ", maxConnectionNum=" + maxConnectionNum + ", validateConnection=" + validateConnection +
                ", deltaIncCount=" + deltaIncCount + ", maxIdleNum=" + maxIdleNum + ", recheckCyclePeriod=" +
                recheckCyclePeriod + ", location='" + location + '\'' + ", connectStrategy=" + connectStrategy +
                ", minIdleNum=" + minIdleNum + ", syncCoordInterval=" + syncCoordInterval + ", sessionTimeout=" +
                sessionTimeout + ", networkBlockTimeout=" + networkBlockTimeout + ", cacheLimit=" + cacheLimit + '}';
    }
}
