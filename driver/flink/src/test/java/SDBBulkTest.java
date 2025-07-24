<<<<<<< HEAD
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

   Source File Name = SDBBulkTest.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
=======
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2
import java.util.ArrayList;

import com.sequoiadb.flink.common.exception.SDBException;
import com.sequoiadb.flink.sink.state.SDBBulk;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.junit.Assert;
import org.junit.Test;

public class SDBBulkTest {

    @Test
    public void testBulk1(){
        int bulkSize = 10;
        SDBBulk bulk = new SDBBulk(bulkSize);
        BSONObject bsonObject = new BasicBSONObject("test", 1);
        int size = bulk.add(bsonObject);

        Assert.assertEquals(1, size);
        Assert.assertEquals(1, bulk.size());
        Assert.assertEquals(bsonObject, bulk.getBsonObjects().get(0)); 
    }

    @Test
    public void testBulk2(){
        int bulkSize = 10;
        SDBBulk bulk = new SDBBulk(bulkSize);
        BSONObject bsonObject = new BasicBSONObject("test", 1);
        bulk.add(bsonObject);
        bulk.clear();

        Assert.assertEquals(0, bulk.size());
        Assert.assertEquals(false, bulk.isFull());
        Assert.assertEquals(new ArrayList<>(bulkSize), bulk.getBsonObjects());

        for(int i =0; i < 10; i++) {
            bulk.add(bsonObject);
        }

        Assert.assertEquals(10, bulk.size());
        Assert.assertEquals(true, bulk.isFull());
    }

    @Test
    public void testBulk3(){
        int bulkSize = 10;
        SDBBulk bulk = new SDBBulk(bulkSize);
        BSONObject bsonObject = new BasicBSONObject("test", 1);
     
        for(int i =0; i < 10; i++) {
            bulk.add(bsonObject);
        }

        Assert.assertEquals(10, bulk.size());
        Assert.assertEquals(true, bulk.isFull());
        
    }
   
    @Test(expected = SDBException.class )
    public void testBulk4(){
        int bulkSize = 10;
        SDBBulk bulk = new SDBBulk(bulkSize);
        BSONObject bsonObject = new BasicBSONObject("test", 1);
        for(int i =0; i < 100; i++) {
            bulk.add(bsonObject);
        }
    }

}
