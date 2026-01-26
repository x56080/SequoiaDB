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

   Source File Name = DataServiceWrapper.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          26/01/2026  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoiadb.schedule.core;

import com.sequoiadb.base.ConfigOptions;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.base.UserConfig;
import com.sequoiadb.datasource.DatasourceOptions;
import com.sequoiadb.datasource.SequoiadbDatasource;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.schedule.metasource.config.SequoiadbConfig;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.ArrayList;
import java.util.List;

public class DataServiceWrapper {
    private static final Logger logger = LoggerFactory.getLogger(DataServiceWrapper.class);
    private SequoiadbDatasource dataSource = null;
    // 获取连接时，如果权限错误，则标记为不可用
    private volatile boolean isOK = false;

    public DataServiceWrapper(SequoiadbConfig sequoiadbConfig, List<String> urls, String user,
            String password) throws Exception {
        try {
            ConfigOptions nwOpt = new ConfigOptions();
            nwOpt.setConnectTimeout(sequoiadbConfig.getConnectTimeout());
            nwOpt.setMaxAutoConnectRetryTime(sequoiadbConfig.getMaxAutoConnectRetryTime());
            nwOpt.setSocketKeepAlive(true);
            nwOpt.setSocketTimeout(sequoiadbConfig.getSocketTimeout());
            nwOpt.setUseNagle(sequoiadbConfig.getUseNagle());
            nwOpt.setUseSSL(sequoiadbConfig.getUseSSL());

            DatasourceOptions dsOpt = new DatasourceOptions();
            dsOpt.setMaxCount(sequoiadbConfig.getMaxConnectionNum());
            dsOpt.setDeltaIncCount(sequoiadbConfig.getDeltaIncCount());
            dsOpt.setMaxIdleCount(sequoiadbConfig.getMaxIdleNum());
            dsOpt.setKeepAliveTimeout(sequoiadbConfig.getKeepAliveTime());
            dsOpt.setCheckInterval(sequoiadbConfig.getRecheckCyclePeriod());
            dsOpt.setValidateConnection(sequoiadbConfig.getValidateConnection());
            List<String> preferedInstance = new ArrayList<>();
            preferedInstance.add("M");
            dsOpt.setPreferredInstance(preferedInstance);
            dsOpt.setConnectStrategy(sequoiadbConfig.getConnectStrategy());
            dsOpt.setMinIdleCount(sequoiadbConfig.getMinIdleNum());
            dsOpt.setSyncCoordInterval(sequoiadbConfig.getSyncCoordInterval());
            dsOpt.setSessionTimeout(sequoiadbConfig.getSessionTimeout());
            dsOpt.setNetworkBlockTimeout(sequoiadbConfig.getNetworkBlockTimeout());
            dsOpt.setCacheLimit(sequoiadbConfig.getCacheLimit());

            this.dataSource = SequoiadbDatasource.builder().serverAddress(urls)
                    .userConfig(new UserConfig(user, password)).configOptions(nwOpt)
                    .datasourceOptions(dsOpt).build();

            releaseConnection(dataSource.getConnection());
        }
        catch (Exception e) {
            throw new Exception("initialize SequoiadbDatasource failed, " + e.getMessage(), e);
        }

        this.isOK = true;
    }

    public Sequoiadb getConnection() throws Exception {
        if (!isOK) {
            throw new Exception("get sdb connection failed, dataService is not ready");
        }
        try {
            return dataSource.getConnection();
        }
        catch (BaseException e) {
            if (e.getErrorCode() == SDBError.SDB_AUTH_AUTHORITY_FORBIDDEN.getErrorCode()) {
                logger.error("datasource error, authority is forbidden", e);
                isOK = false;
            }
            throw e;
        }
        catch (Exception e) {
            throw new BaseException(SDBError.SDB_INTERRUPT, "datasource error", e);
        }
    }

    public void releaseConnection(Sequoiadb sdb) {
        try {
            if (null != sdb) {
                dataSource.releaseConnection(sdb);
            }
        }
        catch (Exception e) {
            logger.warn("release connection failed", e);
            try {
                sdb.close();
            }
            catch (Exception e1) {
                logger.warn("disconnect sequoiadb failed", e1);
            }
        }
    }

    public boolean isOK() {
        return isOK;
    }

    public void clear() {
        try {
            if (null != dataSource) {
                dataSource.close();
                dataSource = null;
            }
        }
        catch (Exception e) {
            logger.warn("close sequoiadb data source failed", e);
        }
    }
}
