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

   Source File Name = SequoiaStorageTest.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoia.pig.test;

import static org.junit.Assert.assertEquals;

import java.util.HashMap;
import java.util.Map;
import java.util.Set;

import org.apache.pig.ResourceSchema;
import org.apache.pig.impl.util.Utils;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.junit.Test;

import com.sequoia.pig.SequoiaWriter;

@SuppressWarnings( {"rawtypes", "unchecked"} )
public class SequoiaStorageTest {

    @Test
    public void testWriteField_map() throws Exception {
        SequoiaWriter ms = new SequoiaWriter();
        BSONObject builder = new BasicBSONObject();
        ResourceSchema schema = new ResourceSchema(Utils.getSchemaFromString("m:map[]"));

        Map val = new HashMap();
        val.put("f1", 1);
        val.put("f2", "2");
        
//        ms.writeField(builder, schema.getFields()[0], val);
        
        Set<String> outKeySet = builder.keySet();
        
        assertEquals(2, outKeySet.size());
        assertEquals(1, builder.get("f1"));
        assertEquals("2", builder.get("f2"));
    }
    
    
}
