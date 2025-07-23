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

   Source File Name = SdbDataSourceWrapper.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoias3.dao.sequoiadb;

import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.datasource.ConnectStrategy;
import com.sequoiadb.datasource.DatasourceOptions;
import com.sequoiadb.datasource.SequoiadbDatasource;
import com.sequoiadb.base.ConfigOptions;
import com.sequoias3.config.SequoiadbConfig;
import com.sequoias3.exception.S3DaoGetConnException;
import com.sequoias3.exception.S3ServerException;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Repository;

import java.util.List;

@Repository
public class SdbDataSourceWrapper {
    private static final Logger logger = LoggerFactory.getLogger(SdbDataSourceWrapper.class);

    private SequoiadbDatasource sdbDatasource;

    @Autowired
    public SdbDataSourceWrapper(SequoiadbConfig config) {
        List<String> addrs = config.getUrlList();
        ConfigOptions nwOpt = new ConfigOptions();
        DatasourceOptions dsOpt = new DatasourceOptions();

        nwOpt.setConnectTimeout(1000);
        nwOpt.setMaxAutoConnectRetryTime(0);
		nwOpt.setSocketKeepAlive(true);

        dsOpt.setMaxCount(config.getMaxConnectionNum());
        dsOpt.setDeltaIncCount(config.getDeltaIncCount());
        dsOpt.setMaxIdleCount(config.getMaxIdleNum());
        dsOpt.setKeepAliveTimeout(config.getKeepAliveTime());
        dsOpt.setCheckInterval(config.getCheckInterval());
        dsOpt.setSyncCoordInterval(0);
        dsOpt.setValidateConnection(config.getValidateConnection());
        dsOpt.setConnectStrategy(ConnectStrategy.BALANCE);

        sdbDatasource = new SequoiadbDatasource(addrs, config.getUsername(), config.getPassword(), nwOpt, dsOpt);
    }

    public Sequoiadb getSequoiadb() throws S3ServerException {
        try {
            return sdbDatasource.getConnection();
        } catch (Exception e) {
            throw new S3DaoGetConnException("get connection from Sequoiadb failed", e);
        }
    }

    public void releaseSequoiadb(Sequoiadb sdb) {
        if (null == sdb) {
            return;
        }

        try {
            sdbDatasource.releaseConnection(sdb);
        } catch (Exception e) {
            logger.warn("release connection failed:sdb={}" + sdb, e);
            try {
                sdb.close();
            } catch (Exception e1) {
                logger.warn("disconnect sequoiadb failed", e1);
            }
        }
    }

}
