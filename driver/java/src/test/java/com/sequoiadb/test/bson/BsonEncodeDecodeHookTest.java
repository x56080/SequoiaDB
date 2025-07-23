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

   Source File Name = BsonEncodeDecodeHookTest.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoiadb.test.bson;

import com.sequoiadb.base.*;
import com.sequoiadb.test.common.Constants;
import org.bson.BSON;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.Transformer;
import org.junit.*;

import java.util.Calendar;
import java.util.Date;
import java.util.GregorianCalendar;

public class BsonEncodeDecodeHookTest {

    private static Sequoiadb sdb;
    private static CollectionSpace cs;
    private static DBCollection cl;

    @BeforeClass
    public static void setConnBeforeClass() throws Exception {
        // sdb
        sdb = new Sequoiadb(Constants.COOR_NODE_CONN, "", "");
    }

    @AfterClass
    public static void DropConnAfterClass() throws Exception {
        sdb.disconnect();
    }

    @Before
    public void setUp() throws Exception {
        // cs
        if (sdb.isCollectionSpaceExist(Constants.TEST_CS_NAME_1)) {
            sdb.dropCollectionSpace(Constants.TEST_CS_NAME_1);
        }
        cs = sdb.createCollectionSpace(Constants.TEST_CS_NAME_1);
        // cl
        BSONObject conf = new BasicBSONObject();
        conf.put("ReplSize", 0);
        cl = cs.createCollection(Constants.TEST_CL_NAME_1, conf);
    }

    @After
    public void tearDown() throws Exception {
        sdb.dropCollectionSpace(Constants.TEST_CS_NAME_1);
    }

    @Test
    public void testEncodeDecodeHook() {
        Transformer encodeTf = new Transformer() {
            @Override
            public Object transform(Object o) {
                return ((GregorianCalendar)o).getTime();
            }
        };
        Transformer decodeTf = new Transformer() {
            @Override
            public Object transform(Object o) {
                Calendar calendar = new GregorianCalendar();
                calendar.setTime((Date)o);
                return calendar;
            }
        };

        BSON.addEncodingHook(GregorianCalendar.class, encodeTf);
        BSON.addDecodingHook(Date.class, decodeTf);

        Calendar calendar = new GregorianCalendar();
        calendar.set(2018, 0,1);
        BSONObject object = new BasicBSONObject();
        object.put("calendar", calendar);
        try {
            cl.insert(object);
            DBCursor cursor = cl.query();
            while(cursor.hasNext()) {
                BSONObject record = cursor.getNext();
                System.out.println("record is: " + record);
                Object resultObject = record.get("calendar");
                calendar.equals(resultObject);
                Assert.assertTrue(resultObject instanceof GregorianCalendar);
            }
        }finally {
            BSON.removeEncodingHook(GregorianCalendar.class, encodeTf);
            BSON.removeDecodingHook(Date.class, decodeTf);
        }
    }
}
