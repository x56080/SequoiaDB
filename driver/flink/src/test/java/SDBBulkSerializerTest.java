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

   Source File Name = SDBBulkSerializerTest.java

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
import java.io.IOException;

import com.sequoiadb.flink.sink.state.SDBBulk;
import com.sequoiadb.flink.sink.state.SDBBulkSerializer;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.junit.Assert;
import org.junit.Test;


public class SDBBulkSerializerTest {
    @Test
    public void testSerializeVersion(){
        int version = 1;
        SDBBulkSerializer bulkSerializer = new SDBBulkSerializer();
        Assert.assertEquals(version, bulkSerializer.getVersion());
    }

    @Test
    public void testSerialization1() throws IOException {
        int version = 1;
        BSONObject bsonObject = new BasicBSONObject("test", 1);
        SDBBulk bulk = new SDBBulk(1);
        bulk.add(bsonObject);
        SDBBulkSerializer bulkSerializer = new SDBBulkSerializer();
        byte [] out = bulkSerializer.serialize(bulk);

        Assert.assertEquals(bulk, bulkSerializer.deserialize(version, out));

    }

    @Test
    public void testSerialization2() throws IOException {
        int version = 1;
        SDBBulk bulk = new SDBBulk(0);
        SDBBulkSerializer bulkSerializer = new SDBBulkSerializer();
        byte [] out = bulkSerializer.serialize(bulk);

        Assert.assertEquals(bulk, bulkSerializer.deserialize(version, out));

    }

}
