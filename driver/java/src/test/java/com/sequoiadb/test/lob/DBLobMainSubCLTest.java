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

   Source File Name = DBLobMainSubCLTest.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoiadb.test.lob;

import com.sequoiadb.base.*;
import com.sequoiadb.test.common.Constants;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.ObjectId;
import org.junit.*;

import java.text.SimpleDateFormat;

public class DBLobMainSubCLTest {
    private static Sequoiadb sdb;
    private static DBCollection mainCL;
    private static DBCollection subCL1;
    private static DBCollection subCL2;
    private final static String mainCLName = "lobMainCL";
    private final static String subCLName1 = "lobSubCL1";
    private final static String subCLName2 = "lobSubCL2";

    @Before
    public void setUp() throws Exception {
        sdb = new Sequoiadb( Constants.COOR_NODE_CONN, "", "" );
        if ( sdb.isCollectionSpaceExist( Constants.TEST_CS_NAME_1 ) ) {
            sdb.dropCollectionSpace( Constants.TEST_CS_NAME_1 );
        }
        CollectionSpace cs = sdb.createCollectionSpace( Constants.TEST_CS_NAME_1 );

        // main cl
        BSONObject option = new BasicBSONObject();
        option.put( "LobShardingKeyFormat", "YYYY" );
        option.put( "ShardingKey", new BasicBSONObject("date", 1) );
        option.put( "ShardingType", "range" );
        option.put( "IsMainCL", true );
        mainCL = cs.createCollection( mainCLName, option );

        // sub cl
        subCL1 = cs.createCollection( subCLName1 );
        subCL2 = cs.createCollection( subCLName2 );

        // attach sub cl
        BSONObject bound1 = new BasicBSONObject();
        bound1.put( "LowBound", new BasicBSONObject( "date", "2018" ) );
        bound1.put( "UpBound", new BasicBSONObject( "date", "2019" ) );
        mainCL.attachCollection( subCL1.getFullName(), bound1 );

        BSONObject bound2 = new BasicBSONObject();
        bound2.put( "LowBound", new BasicBSONObject( "date", "2019" ) );
        bound2.put( "UpBound", new BasicBSONObject( "date", "2020" ) );
        mainCL.attachCollection( subCL2.getFullName(), bound2 );

    }

    @After
    public void tearDown() throws Exception {
        try {
            sdb.dropCollectionSpace( Constants.TEST_CS_NAME_1 );
        }finally {
            sdb.close();
        }
    }

    @Test
    public void test() throws Exception {
        SimpleDateFormat format = new SimpleDateFormat( "yyyy-MM-dd" );
        ObjectId id = mainCL.createLobID( format.parse( "2018-12-31" ));
        DBLob lob = mainCL.createLob( id );
        lob.write( "lob test".getBytes() );
        lob.close();

        DBCursor cursor1 = subCL1.listLobs();
        DBCursor cursor2 = subCL2.listLobs();
        try {
            Assert.assertTrue( cursor1.hasNext() );
            Assert.assertFalse( cursor2.hasNext() );
        } finally {
            cursor1.close();
            cursor2.close();
        }
    }
}
