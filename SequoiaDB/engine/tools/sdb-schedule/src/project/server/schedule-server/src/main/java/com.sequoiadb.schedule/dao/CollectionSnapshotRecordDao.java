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

   Source File Name = CollectionSnapshotRecordDao.java

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

import com.sequoiadb.schedule.metasource.template.ITransaction;
import com.sequoiadb.schedule.model.CollectionSnapshotRecord;
import org.bson.BSONObject;

public interface CollectionSnapshotRecordDao {

    CollectionSnapshotRecord findOne(String siteName, String clFullName) throws Exception;

    void upsert(String siteName, String clFullName, BSONObject snapshot, boolean recordSnapshotEffective, boolean lobSnapshotEffective) throws Exception;

    void delete(String siteName, String clFullName, ITransaction t) throws Exception;

    void updateRecordSnapshotEffective(String siteName, String clFullName, boolean recordSnapshotEffective);
    void updateLobSnapshotEffective(String siteName, String clFullName, boolean lobSnapshotEffective);
}
