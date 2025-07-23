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

   Source File Name = SingleCSCLTestCase.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoiadb.test;

import com.sequoiadb.base.DBCollection;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.ObjectId;
import org.junit.AfterClass;
import org.junit.BeforeClass;

/*
 * Super class of single test that use only one collectionspace and one collection
 * */
public abstract class SingleCSCLTestCase extends SingleCSTestCase {
    protected static String clName;
    protected static DBCollection cl;

    @BeforeClass
    public static void setUpTestCase() {
        SingleCSTestCase.setUpTestCase();

        clName = "SingleCSCLTestCase_" + new ObjectId().toString();
        if (cs.isCollectionExist(clName)) {
            cs.dropCollection(clName);
        }

        BSONObject options = new BasicBSONObject();
        String groupName = TestConfig.getDataGroupName();
        if (groupName != null) {
            options.put("Group", groupName);
        }
        cl = cs.createCollection(clName, options);
    }

    @AfterClass
    public static void tearDownTestCase() {
        if (sdb != null && cs != null) {
            if (cs.isCollectionExist(clName)) {
                cs.dropCollection(clName);
            }
            cl = null;
            clName = null;
        }
        SingleCSTestCase.tearDownTestCase();
    }
}
