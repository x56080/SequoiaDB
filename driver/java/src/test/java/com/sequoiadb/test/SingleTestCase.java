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

   Source File Name = SingleTestCase.java

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

import com.sequoiadb.base.ConfigOptions;
import com.sequoiadb.base.Sequoiadb;
import org.junit.AfterClass;
import org.junit.BeforeClass;

/*
 * Super class of single test class
 * */
public abstract class SingleTestCase extends TestCase {
    protected static Sequoiadb sdb;

    @BeforeClass
    public static void setUpTestCase() {
        TestCase.setUpTestCase();
        ConfigOptions options = new ConfigOptions();
        options.setConnectTimeout(3000);
        sdb = new Sequoiadb(
            TestConfig.getSingleHost(),
            Integer.valueOf(TestConfig.getSinglePort()),
            TestConfig.getSingleUsername(),
            TestConfig.getSinglePassword(),
            options);
    }

    @AfterClass
    public static void tearDownTestCase() {
        if (sdb != null) {
            sdb.disconnect();
            sdb = null;
        }
        TestCase.tearDownTestCase();
    }
}
