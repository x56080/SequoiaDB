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

   Source File Name = TempSequoiadbTemplate.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          26/01/2026  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoiadb.schedule.dao.sdb;

import com.sequoiadb.base.ConfigOptions;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.base.UserConfig;
import com.sequoiadb.schedule.common.crypto.PasswordMgr;
import com.sequoiadb.schedule.metasource.config.SequoiadbConfig;
import com.sequoiadb.schedule.metasource.template.DataSourceWrapper;
import com.sequoiadb.schedule.metasource.template.SequoiadbTemplate;
import com.sequoiadb.schedule.metasource.template.SequoiadbTransaction;

public class TempSequoiadbTemplate extends SequoiadbTemplate {

    private SequoiadbConfig configuration;

    public TempSequoiadbTemplate(DataSourceWrapper dataSourceWrapper,
            SequoiadbConfig configuration) {
        super(dataSourceWrapper);
        this.configuration = configuration;
    }

    @Override
    protected Sequoiadb getSequoiadb(SequoiadbTransaction context) {
        if (context != null) {
            return context.getSequoiadb();
        }
        return createSequoiadb();
    }

    @Override
    protected void releaseSequoiadb(Sequoiadb sdb, SequoiadbTransaction context) {
        if (context == null) {
            sdb.close();
        }
    }

    private Sequoiadb createSequoiadb() {
        ConfigOptions nwOpt = new ConfigOptions();
        nwOpt.setConnectTimeout(configuration.getConnectTimeout());
        nwOpt.setMaxAutoConnectRetryTime(configuration.getMaxAutoConnectRetryTime());
        nwOpt.setSocketKeepAlive(true);
        nwOpt.setSocketTimeout(configuration.getSocketTimeout());
        nwOpt.setUseNagle(configuration.getUseNagle());
        nwOpt.setUseSSL(configuration.getUseSSL());
        return Sequoiadb.builder().serverAddress(configuration.getUrls())
                .userConfig(new UserConfig(configuration.getUsername(),
                        PasswordMgr.getInstance().decrypt(1, configuration.getPassword())))
                .configOptions(nwOpt).build();
    }

}
