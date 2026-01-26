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

   Source File Name = SiteDao.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          26/01/2026  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoiadb.schedule.dao;


import com.sequoiadb.schedule.metasource.MetaCursor;
import com.sequoiadb.schedule.model.SiteInfo;
import org.bson.BSONObject;

import java.util.List;

public interface SiteDao {

    List<SiteInfo> findAll() throws Exception;

    SiteInfo findByName(String name) throws Exception;

    MetaCursor listSite(BSONObject condition, BSONObject orderBy, long skip, long limit) throws Exception;

    long countSite(BSONObject condition) throws Exception;
}
