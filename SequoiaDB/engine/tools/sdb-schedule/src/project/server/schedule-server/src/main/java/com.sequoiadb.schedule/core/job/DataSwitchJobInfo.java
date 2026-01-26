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

   Source File Name = DataSwitchJobInfo.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          26/01/2026  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoiadb.schedule.core.job;

import com.sequoiadb.schedule.core.ScheduleServer;
import org.bson.BSONObject;

public class DataSwitchJobInfo extends ScheduleJobInfo {
    private BSONObject content;

    public DataSwitchJobInfo(String id, String type, String cron, BSONObject content) throws Exception {
        super(id, type, cron);
        this.content = content;
        checkAndParse(ScheduleServer.getInstance());
    }

    private void checkAndParse(ScheduleServer server) throws Exception {
        commonCheckArg(server, content);

    }

    public BSONObject getContent() {
        return content;
    }
}
