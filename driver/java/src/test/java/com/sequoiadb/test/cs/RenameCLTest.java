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

   Source File Name = RenameCLTest.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoiadb.test.cs;

import static org.junit.Assert.*;

import org.junit.After;
import org.junit.AfterClass;
import org.junit.Before;
import org.junit.BeforeClass;
import org.junit.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.test.common.Constants;

public class RenameCLTest {
    private static Sequoiadb sdb;
    private static CollectionSpace cs;
    public final static String TEST_CL_OLDNAME = "oldCL";
    public final static String TEST_CL_NEWNAME = "newCL";
    
    @BeforeClass
    public static void setUpBeforeClass() throws Exception {
    }

    @AfterClass
    public static void tearDownAfterClass() throws Exception {
    }

    @Before
    public void setUp() throws Exception {
        sdb = new Sequoiadb(Constants.COOR_NODE_CONN, "", "");
        if (!sdb.isCollectionSpaceExist(Constants.TEST_CS_NAME_1))
            cs=sdb.createCollectionSpace(Constants.TEST_CS_NAME_1);
        else
            cs=sdb.getCollectionSpace(Constants.TEST_CS_NAME_1);
        /*Create OLDCL and delete NEWCL */
        if(!cs.isCollectionExist(TEST_CL_OLDNAME))
            cs.createCollection(TEST_CL_OLDNAME);
        if(cs.isCollectionExist(TEST_CL_NEWNAME))
            cs.dropCollection(TEST_CL_NEWNAME);
    }

    @After
    public void tearDown() throws Exception {
        /*Create OLDCL and delete NEWCL */
        try {
            if(sdb.isCollectionSpaceExist(Constants.TEST_CS_NAME_1))
                sdb.dropCollectionSpace(Constants.TEST_CS_NAME_1);
            if(cs.isCollectionExist(TEST_CL_NEWNAME))
                cs.dropCollection(TEST_CL_NEWNAME);
            if(cs.isCollectionExist(TEST_CL_OLDNAME))
                cs.dropCollection(TEST_CL_OLDNAME);
        }finally {
            sdb.close();
        }

    }

    @Test
    public void test() {
        String oldName=TEST_CL_OLDNAME;
        String newName=TEST_CL_NEWNAME;
        cs.renameCollection(oldName, newName);
        assertEquals(false, cs.isCollectionExist(oldName));
        assertEquals(true, cs.isCollectionExist(newName));
    }

}
