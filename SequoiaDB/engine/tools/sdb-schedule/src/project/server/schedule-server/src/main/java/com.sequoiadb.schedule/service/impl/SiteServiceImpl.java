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

   Source File Name = SiteServiceImpl.java

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

import com.sequoiadb.schedule.common.FieldName;
import com.sequoiadb.schedule.dao.SiteDao;
import com.sequoiadb.schedule.metasource.MetaCursor;
import com.sequoiadb.schedule.service.SiteService;
import org.bson.BSONObject;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Service;

import java.util.ArrayList;
import java.util.List;

@Service
public class SiteServiceImpl implements SiteService {

    @Autowired
    private SiteDao siteDao;

    @Override
    public List<BSONObject> getSiteList(BSONObject condition, BSONObject orderby, long skip,
            long limit) throws Exception {
        List<BSONObject> result = new ArrayList<>();
        MetaCursor cursor = null;
        try {
            cursor = siteDao.listSite(condition, orderby, skip, limit);
            while (cursor.hasNext()) {
                BSONObject obj = cursor.getNext();
                obj.removeField(FieldName.Site.PASSWORD);
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
    public long getSiteCount(BSONObject filter) throws Exception {
        return siteDao.countSite(filter);
    }
}
