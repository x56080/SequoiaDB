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

   Source File Name = Helper.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoiadb.test.common;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import org.bson.BSONObject;

/**
 * Created by tanzhaobo on 2017/9/26.
 */
public class Helper {
    public static CollectionSpace getOrCreateCollectionSpace(Sequoiadb db, String csName, BSONObject options) {
        CollectionSpace cs;
        if (db.isCollectionSpaceExist(csName)) {
            cs = db.getCollectionSpace(csName);
        } else {
            cs = db.createCollectionSpace(csName, options);
        }
        return cs;
    }

    public static DBCollection getOrCreateCollection(CollectionSpace cs, String clName, BSONObject options) {
        DBCollection cl;
        if (cs.isCollectionExist(clName)) {
            cl = cs.getCollection(clName);
        } else {
            cl = cs.createCollection(clName, options);
        }
        return cl;
    }

    public static void sleepSec(int sec) {
        try {
            Thread.sleep(sec * 1000);
        } catch (InterruptedException e) {
            e.printStackTrace();
        }
    }

    public static void sleepMS(long millisecond) {
        try {
            Thread.sleep(millisecond);
        } catch (InterruptedException e) {
            e.printStackTrace();
        }
    }

    public static void print(String message) {
        System.out.println(message);
    }
}
