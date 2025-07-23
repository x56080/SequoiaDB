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

   Source File Name = SDBDomain.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoiadb.test.domain;

import com.sequoiadb.base.*;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.test.common.Constants;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;
import org.junit.*;

import static org.junit.Assert.assertTrue;


public class SDBDomain {
    private static Sequoiadb sdb;
    private static Domain dm;
    private static CollectionSpace cs;
    private static DBCollection cl;
    private static DBCursor cursor;
    private static long i = 0;
    private static boolean isCluster = true;

    @BeforeClass
    public static void setConnBeforeClass() throws Exception {
        // sdb
        sdb = new Sequoiadb(Constants.COOR_NODE_CONN, "", "");
        isCluster = Constants.isCluster();
    }

    @AfterClass
    public static void DropConnAfterClass() throws Exception {
        sdb.disconnect();
    }

    @Before
    public void setUp() throws Exception {
        if (!isCluster) {
            return;
        }
        // domain
        BSONObject options = new BasicBSONObject();
        BSONObject arr = new BasicBSONList();
        arr.put("0", Constants.GROUPNAME);
        options.put("Groups", arr);
        if (sdb.isDomainExist(Constants.TEST_DOMAIN_NAME)) {
            sdb.dropDomain(Constants.TEST_DOMAIN_NAME);
            dm = sdb.createDomain(Constants.TEST_DOMAIN_NAME, options);
        } else {
            dm = sdb.createDomain(Constants.TEST_DOMAIN_NAME, options);
        }
    }

    @After
    public void tearDown() throws Exception {
        if (!isCluster)
            return;
        sdb.dropDomain(Constants.TEST_DOMAIN_NAME);
    }

    @Test
    public void Sdb_DomainGlobal() {
        if (!isCluster)
            return;
        String dmName = "test_domain_name";
        // create domain
        Domain domain1 = sdb.createDomain(dmName, null);
        // get domain
        Domain domain2 = sdb.getDomain(dmName);
        String name = domain2.getName();
        // check
        assertTrue(name.equals(dmName));
        // dropDomain
        try {
            sdb.dropDomain(dmName);
        } catch (BaseException e) {
            assertTrue(false);
        }
    }

}
