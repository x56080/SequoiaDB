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

   Source File Name = GlobalConfServiceImpl.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          26/01/2026  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoiadb.schedule.service.impl;

import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.schedule.common.BsonUtils;
import com.sequoiadb.schedule.common.FieldName;
import com.sequoiadb.schedule.common.timer.Timer;
import com.sequoiadb.schedule.common.timer.TimerFactory;
import com.sequoiadb.schedule.common.timer.TimerTask;
import com.sequoiadb.schedule.dao.GlobalConfDao;
import com.sequoiadb.schedule.exception.ScheduleServerError;
import com.sequoiadb.schedule.exception.ScheduleServerException;
import com.sequoiadb.schedule.exception.ScheduleSystemException;
import com.sequoiadb.schedule.metasource.MetaCursor;
import com.sequoiadb.schedule.model.GlobalConfEnum;
import com.sequoiadb.schedule.model.GlobalConfValidator;
import com.sequoiadb.schedule.service.GlobalConfService;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Service;

import javax.annotation.PostConstruct;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;

@Service
public class GlobalConfServiceImpl implements GlobalConfService {
    private static final Logger logger = LoggerFactory.getLogger(GlobalConfServiceImpl.class);
    @Autowired
    private GlobalConfDao globalConfDao;
    private Timer retryTimer;
    private final static int RETRY_INTERVAL = 1000 * 3;

    @PostConstruct
    public void init() {
        try {
            ensureDefaultGlobalConf();
        }
        catch (Exception e) {
            logger.error("failed to ensure default global conf", e);
            asyncRetryEnsureDefaultGlobalConf();
        }
    }

    private void asyncRetryEnsureDefaultGlobalConf() {
        if (retryTimer == null) {
            retryTimer = TimerFactory.createTimer("retryEnsureDefaultGlobalConfTimer");
        }
        retryTimer.schedule(new TimerTask() {
            @Override
            public void run() {
                try {
                    logger.info("retry to ensure default Global conf");
                    ensureDefaultGlobalConf();
                    cancel();
                }
                catch (Exception e) {
                    logger.error("failed to ensure default Global conf", e);
                }
            }
        }, RETRY_INTERVAL, RETRY_INTERVAL);
    }

    private void ensureDefaultGlobalConf() throws Exception {
        Set<String> keys = GlobalConfEnum.getKeys();
        MetaCursor cursor = null;
        try {
            cursor = globalConfDao.listGlobalConf(new BasicBSONObject(FieldName.GlobalConf.FIELD_GLOBAL_CONF_KEY,
                    new BasicBSONObject("$in", keys)), null, 0, -1);
            while (cursor.hasNext()) {
                BSONObject conf = cursor.getNext();
                String key = BsonUtils.getStringChecked(conf, FieldName.GlobalConf.FIELD_GLOBAL_CONF_KEY);
                keys.remove(key);
            }
        }
        finally {
            if (cursor != null) {
                cursor.close();
            }
        }

        if (keys.isEmpty()) {
            return;
        }

        for (String key : keys) {
            GlobalConfEnum type = GlobalConfEnum.getType(key);
            try {
                globalConfDao.insert(type.getKeyName(), type.getDefaultValue(), type.getDesc());
            }
            catch (BaseException e) {
                if (e.getErrorCode() != SDBError.SDB_IXM_DUP_KEY.getErrorCode()) {
                    throw e;
                }
            }
        }
    }


    @Override
    public List<BSONObject> getGlobalConfList(BSONObject condition, BSONObject orderby, long skip,
            long limit) throws Exception {
        List<BSONObject> result = new ArrayList<>();
        MetaCursor cursor = null;
        try {
            cursor = globalConfDao.listGlobalConf(condition, orderby, skip, limit);
            while (cursor.hasNext()) {
                BSONObject obj = cursor.getNext();
                result.add(obj);
            }
            return result;
        }
        finally {
            if (null != cursor) {
                cursor.close();
            }
        }
    }

    @Override
    public long getGlobalConfCount(BSONObject filter) throws Exception {
        return globalConfDao.countGlobalConf(filter);
    }

    @Override
    public void updateGlobalConf(String key, String value) throws ScheduleServerException {
        GlobalConfEnum type = GlobalConfEnum.getType(key);
        GlobalConfValidator validator = type.getValidator();
        try {
            validator.validate(value);
        }
        catch (IllegalArgumentException e) {
            throw new ScheduleServerException(ScheduleServerError.INVALID_ARG, "invalid global conf value: " + value, e);
        }
        try {
            globalConfDao.updateGlobalConf(key, value);
        }
        catch (Exception e) {
            throw new ScheduleSystemException("failed to update global conf", e);
        }
    }
}
