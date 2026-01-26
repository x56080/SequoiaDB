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

   Source File Name = SiteMgr.java

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

import com.sequoiadb.schedule.common.timer.Timer;
import com.sequoiadb.schedule.common.timer.TimerFactory;
import com.sequoiadb.schedule.common.timer.TimerTask;
import com.sequoiadb.schedule.dao.SiteDao;
import com.sequoiadb.schedule.model.SiteInfo;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Component;

import javax.annotation.PostConstruct;
import javax.annotation.PreDestroy;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.concurrent.ConcurrentHashMap;

@Component
public class SiteMgr {

    private static final Logger logger = LoggerFactory.getLogger(SiteMgr.class);

    private Timer refreshTimer;

    private Map<String, SiteInfo> siteMap = new ConcurrentHashMap<>();

    @Autowired
    private SiteDao siteDao;

    @PostConstruct
    public void init() {
        if (refreshTimer == null) {
            refreshTimer = TimerFactory.createTimer("SiteRefreshTimer");
        }
        long refreshInterval = 60 * 1000; // 10 minutes
        refreshTimer.schedule(new TimerTask() {
            @Override
            public void run() {
                try {
                    refresh();
                }
                catch (Exception e) {
                    logger.warn("refresh site info failed", e);
                }
            }
        }, 0, refreshInterval);
    }

    private void refresh() throws Exception {
        List<SiteInfo> siteInfoList = siteDao.findAll();
        Set<String> existingKeys = new HashSet<>(siteMap.keySet());
        Set<String> newKeys = new HashSet<>();
        for (SiteInfo siteInfo : siteInfoList) {
            String name = siteInfo.getName();
            siteMap.put(name, siteInfo);
            newKeys.add(name);
        }
        // Remove sites that no longer exist
        for (String key : existingKeys) {
            if (!newKeys.contains(key)) {
                siteMap.remove(key);
            }
        }
    }

    public SiteInfo getSite(String name) throws Exception {
        SiteInfo siteInfo = siteMap.get(name);
        if (siteInfo != null) {
            return siteInfo;
        }
        SiteInfo info = siteDao.findByName(name);
        if (info == null) {
            return null;
        }
        siteMap.put(name, info);
        return info;
    }

    @PreDestroy
    public void destroy() {
        if (refreshTimer != null) {
            refreshTimer.cancel();
            refreshTimer = null;
        }
        siteMap.clear();
    }
}
