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

   Source File Name = SiteInfo.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          26/01/2026  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoiadb.schedule.model;

import com.sequoiadb.schedule.common.BsonUtils;
import com.sequoiadb.schedule.common.FieldName;
import lombok.Data;
import org.bson.BSONObject;

import java.util.List;

@Data
public class SiteInfo {
    private String name;
    private List<String> urls;
    private String user;
    private String password;
    private String datasource;

    public static SiteInfo fromBSONObject(BSONObject obj) {
        SiteInfo info = new SiteInfo();
        info.setName(BsonUtils.getStringChecked(obj, FieldName.Site.NAME));
        info.setUrls(BsonUtils.getStringArray(obj, FieldName.Site.URLS));
        info.setUser(BsonUtils.getString(obj, FieldName.Site.USER));
        info.setPassword(BsonUtils.getString(obj, FieldName.Site.PASSWORD));
        info.setDatasource(BsonUtils.getString(obj, FieldName.Site.DATASOURCE));
        return info;
    }
}
