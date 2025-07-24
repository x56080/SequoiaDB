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

   Source File Name = TestUser.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoiadb.base;

import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.test.SingleCSCLTestCase;
import org.junit.Test;

import java.io.UnsupportedEncodingException;

import static org.junit.Assert.fail;

public class TestUser extends SingleCSCLTestCase {
    @Test
    public void testCreateAndRemoveUser() {
        String user = "admin";
        String password = "admin";
        try {
            sdb.createUser(user, password);
            sdb.removeUser(user, password);
        } catch (Exception e) {
            fail(e.toString());
        }
    }

    @Test
    public void testUserWithChinese(){

        // case 1: UTF-8
        try {
            String user = "用户";
            String password = new String("密码".getBytes("UTF-8"));
            sdb.createUser(user, password);
            sdb.removeUser(user, password);
        }catch (Exception e){
            throw new BaseException(SDBError.SDB_INVALIDARG, e);
        }

        // case 2: GBK
        try {
            String user = "用户";
            String password = new String("密码".getBytes("GBK"));
            sdb.createUser(user, password);
            sdb.removeUser(user, password);
        }catch (Exception e){
            throw new BaseException(SDBError.SDB_INVALIDARG, e);
        }
    }

}
