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

   Source File Name = SequoiadbDataSourceTest.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoiadb.datasource;

import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.test.common.Constants;
import org.junit.*;

import java.util.ArrayList;
import java.util.List;

public class SequoiadbDataSourceTest {
    private static SequoiadbDatasource ds;
    private static DatasourceOptions options;
    private static int checkTime = 5 * 1000; // 5s

    @Before
    public void setUp() {
        options = new DatasourceOptions();
        options.setCheckInterval(checkTime);
        options.setMaxIdleCount(20);
        options.setMinIdleCount(10);
        ds = new SequoiadbDatasource(Constants.COOR_NODE_CONN, "", "", options);
    }

    @After
    public void tearDown() {
        ds.close();
    }

    @Test
    public void createNumTest(){
        try {
            List<Sequoiadb> dbList = new ArrayList<>();
            for (int i = 0; i < options.getMinIdleCount(); i++){
                Sequoiadb db = ds.getConnection();
                dbList.add(db);
            }
            Thread.sleep(checkTime);
            Assert.assertEquals(options.getMinIdleCount(), ds.getIdleConnNum());
            for (int i = 0; i < options.getMinIdleCount(); i++){
                ds.releaseConnection(dbList.get(i));
            }
        }catch (Exception e){
            e.printStackTrace();
        }
    }
}