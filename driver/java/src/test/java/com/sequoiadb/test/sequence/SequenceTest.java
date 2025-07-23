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

   Source File Name = SequenceTest.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoiadb.test.sequence;

import com.sequoiadb.base.*;
import com.sequoiadb.test.common.Constants;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.junit.*;

import static org.junit.Assert.assertEquals;

public class SequenceTest {

    private static Sequoiadb sdb;
    private static String seqName = "test_seq";
    private static String seqNewName = "test_new_seq";

    @BeforeClass
    public static void setConnBeforeClass() throws Exception {
        sdb = new Sequoiadb(Constants.COOR_NODE_CONN, "", "");
    }

    @AfterClass
    public static void DropConnAfterClass() throws Exception {
        sdb.disconnect();
    }

    @Before
    public void setUp() throws Exception {
    }

    @After
    public void tearDown() throws Exception {
        clearSeq(seqName);
        clearSeq(seqNewName);
    }

    private void clearSeq(String seqName){
        DBCursor cursor = sdb.getSnapshot(Sequoiadb.SDB_SNAP_SEQUENCES, new BasicBSONObject("Name",seqName),
                null,null);
        if (cursor.hasNext()){
            sdb.dropSequence(seqName);
        }
    }

    @Test
    public void testSequoiadbSeqAPI() {
        BSONObject options = new BasicBSONObject("StartValue",100);

        sdb.createSequence(seqName, options);
        BSONObject obj = sdb.getSnapshot(Sequoiadb.SDB_SNAP_SEQUENCES,new BasicBSONObject("Name",seqName),null,null).getNext();
        assertEquals(100, (long)obj.get("StartValue"));
        sdb.renameSequence(seqName, seqNewName);
        DBSequence s2 = sdb.getSequence(seqNewName);
        assertEquals(seqNewName, s2.getName());
        sdb.dropSequence(seqNewName);
    }

    @Test
    public void testSequenceAPI() {
        BSONObject options = new BasicBSONObject("StartValue",100);

        DBSequence sequence = sdb.createSequence(seqName, options);
        assertEquals(100,sequence.getNextValue());

        sequence.setCurrentValue(200);
        assertEquals(200,sequence.getCurrentValue());

        sequence.restart(300);
        assertEquals(300,sequence.getNextValue());

        options.put("CurrentValue",400);
        sequence.setAttributes(options);
        BSONObject obj = sequence.fetch(3);
        assertEquals(401L,obj.get("NextValue"));
        assertEquals(3,obj.get("ReturnNum"));
        assertEquals(1,obj.get("Increment"));

        sdb.dropSequence(seqName);
    }

}
