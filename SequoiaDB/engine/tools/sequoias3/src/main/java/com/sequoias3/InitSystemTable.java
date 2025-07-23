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

   Source File Name = InitSystemTable.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoias3;

import com.sequoias3.dao.RegionDao;
import com.sequoias3.dao.sequoiadb.SdbInitSystem;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.ApplicationArguments;
import org.springframework.boot.ApplicationRunner;
import org.springframework.core.annotation.Order;
import org.springframework.stereotype.Component;

@Component
@Order(1)
public class InitSystemTable implements ApplicationRunner {
    @Autowired
    SdbInitSystem sdbInitSystem;

    @Autowired
    RegionDao regionDao;

    @Override
    public void run(ApplicationArguments applicationArguments) throws Exception {
        // is cs exist and create cs
        sdbInitSystem.createSystemCS();

        // is user exist and create user
        sdbInitSystem.createUserCL();

        // is bucketlist exist and create bucketlist
        sdbInitSystem.createBucketCL();

        // is regionlist exist and create regionlist
        sdbInitSystem.createRegionCL();

        // is regionspacelist exist and create regionlist
        sdbInitSystem.createRegionSpaceCL();

        sdbInitSystem.createIDGeneratorCL();

        sdbInitSystem.createTaskCL();

        sdbInitSystem.createUploadCL();

        sdbInitSystem.createPartCL();

        sdbInitSystem.createACLTable();

        regionDao.createMetaCSCL(null, regionDao.getMetaCurCSName(null), regionDao.getMetaCurCLName(null), false);
        regionDao.createMetaCSCL(null, regionDao.getMetaHisCSName(null), regionDao.getMetaHisCLName(null), true);
        regionDao.createDirCSCL(null, regionDao.getMetaCurCSName(null));
    }
}
