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

   Source File Name = SiteDataserviceMgr.java

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

import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.schedule.common.BsonUtils;
import com.sequoiadb.schedule.common.crypto.PasswordMgr;
import com.sequoiadb.schedule.common.timer.Timer;
import com.sequoiadb.schedule.common.timer.TimerFactory;
import com.sequoiadb.schedule.common.timer.TimerTask;
import com.sequoiadb.schedule.metasource.config.SequoiadbConfig;
import com.sequoiadb.schedule.model.SiteInfo;
import com.sequoiadb.util.SdbDecrypt;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Component;

import javax.annotation.PostConstruct;
import javax.annotation.PreDestroy;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;

@Component
public class SiteDataserviceMgr {
    private static final Logger logger = LoggerFactory.getLogger(SiteDataserviceMgr.class);
    private static final int CHECK_INTERVAL_MS = 10000; // 10 seconds
    private Map<String, DataServiceWrapper> siteDataServiceMap = new ConcurrentHashMap<>();

    @Autowired
    private SiteMgr siteMgr;

    @Autowired
    private SequoiadbConfig sequoiadbConfig;

    private Timer initTimer;

    private DataServiceWrapper rootSiteDataServiceWrapper;

    @PostConstruct
    public void init() {
        if (initTimer == null) {
            initTimer = TimerFactory.createTimer("RootSiteDataServiceInitTimer");
        }
        initTimer.schedule(new TimerTask() {
            @Override
            public void run() {
                try {
                    initRootSite();
                    logger.info("init root site data service success");
                    cancel();
                }
                catch (Exception e) {
                    logger.warn("do init root site data service failed", e);
                }
            }
        }, 0, CHECK_INTERVAL_MS);
    }

    private void initRootSite() throws Exception {
        rootSiteDataServiceWrapper = createDataServiceWrapper("rootsite");
        siteDataServiceMap.put("rootsite", rootSiteDataServiceWrapper);
    }

    public synchronized DataServiceWrapper getDataService(String siteName) throws Exception {
        DataServiceWrapper dataServiceWrapper = siteDataServiceMap.get(siteName);
        if (dataServiceWrapper == null) {
            dataServiceWrapper = createDataServiceWrapper(siteName);
            siteDataServiceMap.put(siteName, dataServiceWrapper);
            return dataServiceWrapper;
        }

        if (dataServiceWrapper.isOK()) {
            return dataServiceWrapper;
        }

        // 重新创建dataService
        siteDataServiceMap.remove(siteName);
        dataServiceWrapper.clear();

        if (siteName.equals("rootsite")) {
            initRootSite();
            return rootSiteDataServiceWrapper;
        }

        dataServiceWrapper = createDataServiceWrapper(siteName);
        siteDataServiceMap.put(siteName, dataServiceWrapper);
        return dataServiceWrapper;
    }

    private DataServiceWrapper createDataServiceWrapper(String siteName) throws Exception {
        SiteInfo siteInfo = siteMgr.getSite(siteName);
        if (siteInfo == null) {
            throw new Exception("Site [" + siteName + "] not found");
        }

        if (siteInfo.getName().equals("rootsite")) {
            return new DataServiceWrapper(sequoiadbConfig, siteInfo.getUrls(),
                    siteInfo.getUser(), PasswordMgr.getInstance().decrypt(1, siteInfo.getPassword()));
        }

        String datasourceName = siteInfo.getDatasource();

        BSONObject datasourceInfo = null;
        Sequoiadb sdb = null;
        try {
            sdb = rootSiteDataServiceWrapper.getConnection();
            try (DBCursor cursor = sdb.listDataSources(new BasicBSONObject("Name", datasourceName), null, null, null)){
                if (cursor.hasNext()) {
                    datasourceInfo = cursor.getNext();
                }
                else {
                    throw new Exception("Datasourcenot found in rootsite datasource list, datasource=" + datasourceName);
                }
            }
        }
        finally {
            if (sdb != null) {
                rootSiteDataServiceWrapper.releaseConnection(sdb);
            }
        }

        if (datasourceInfo == null) {
            throw new Exception("Datasource info is null, datasource=" + datasourceName);
        }

        String address = BsonUtils.getStringChecked(datasourceInfo, "Address");
        String user = BsonUtils.getStringChecked(datasourceInfo, "User");
        String cipherText = BsonUtils.getStringChecked(datasourceInfo, "CipherText");
        return new DataServiceWrapper(sequoiadbConfig, getSdbUrlList(address),
                user, new SdbDecrypt().decryptPasswd(cipherText));
    }

    private List<String> getSdbUrlList(String sdbUrl) {
        String[] urlArr = sdbUrl.split(",");
        return new ArrayList<>(Arrays.asList(urlArr));
    }

    @PreDestroy
    public void destroy() {
        for (DataServiceWrapper dataServiceWrapper : siteDataServiceMap.values()) {
            dataServiceWrapper.clear();
        }
        siteDataServiceMap.clear();
    }
}
