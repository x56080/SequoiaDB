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

   Source File Name = DataSourceWrapper.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          26/01/2026  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoiadb.schedule.metasource.template;

import com.sequoiadb.base.ConfigOptions;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.base.UserConfig;
import com.sequoiadb.datasource.DatasourceOptions;
import com.sequoiadb.datasource.SequoiadbDatasource;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.schedule.common.crypto.PasswordMgr;
import com.sequoiadb.schedule.metasource.config.SequoiadbConfig;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.stereotype.Repository;

import javax.annotation.PreDestroy;
import java.util.ArrayList;
import java.util.List;

@Repository
public class DataSourceWrapper {
    private static final Logger logger = LoggerFactory.getLogger(DataSourceWrapper.class);

    private SequoiadbDatasource dataSource = null;

    @Value("${system.metasource.collectionSpace:SDB_SCHEDULE_SYSTEM}")
    private String systemCSName;

    @Autowired
    public DataSourceWrapper(SequoiadbConfig sequoiadbConfig) {
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

        this.dataSource = SequoiadbDatasource.builder().serverAddress(sequoiadbConfig.getUrls())
                .userConfig(new UserConfig(sequoiadbConfig.getUsername(),
                        PasswordMgr.getInstance().decrypt(1, sequoiadbConfig.getPassword())))
                .configOptions(nwOpt).datasourceOptions(dsOpt).build();
    }

    public String getSystemCSName() {
        return systemCSName;
    }

    public SequoiadbDatasource getSdbDataSource() throws BaseException {
        if (dataSource != null) {
            return dataSource;
        }
        throw new BaseException(SDBError.SDB_INTERRUPT, "datasource is null");
    }

    public Sequoiadb getConnection() throws BaseException {
        Sequoiadb sdb = null;
        try {
            return dataSource.getConnection();
        }
        catch (Exception e) {
            releaseConnection(sdb);
            sdb = null;
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

    @PreDestroy
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
