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

   Source File Name = ScheduleNodeDao.java

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

import com.sequoiadb.schedule.model.ServerNodeEntity;
import org.bson.BSONObject;

import java.util.List;

public interface ScheduleNodeDao {

    void upsert(ServerNodeEntity info) throws Exception;

    void renew(ServerNodeEntity info) throws Exception;

    void down(ServerNodeEntity info) throws Exception;

    void useTempConnection();

    List<ServerNodeEntity> list() throws Exception;

    long countNode(BSONObject condition) throws Exception;

    List<ServerNodeEntity> listNode(BSONObject condition, BSONObject orderBy, long skip, long limit) throws Exception;
}
