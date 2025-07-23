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

   Source File Name = CountInfoTest.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoiadb.datasource;

import org.junit.*;

import java.util.TreeSet;

/**
 * Created by tanzhaobo on 2017/12/27.
 */
public class CountInfoTest {

    @BeforeClass
    public static void setConnBeforeClass() {
    }

    @AfterClass
    public static void DropConnAfterClass() {
    }

    @Before
    public void setUp() {
    }

    @After
    public void tearDown() {
    }

    @Test
    public void countInfoSortTest() {
        TreeSet<CountInfo> set = new TreeSet<CountInfo>();
        CountInfo dump = new CountInfo("1", 0, false);
        set.add(dump);
        set.add(new CountInfo("", 0, false));
        set.add(new CountInfo("192.168.20.166:11810", 10, true));
        set.add(new CountInfo("192.168.20.166:11810", 0, true));
        set.add(new CountInfo("192.168.20.166:11810", 6, true));
        set.add(new CountInfo("192.168.20.166:11810", 6, false));
        set.add(new CountInfo("192.168.20.166:11810", 5, false));
        set.add(new CountInfo("192.168.20.166:50000", 0, false));
        set.add(new CountInfo("192.168.20.166:51000", 0, false));
        set.add(new CountInfo("192.168.20.166:52000", 0, false));

        System.out.println("ceiling: " + set.ceiling(dump));
        System.out.println("floor: " + set.floor(dump));
        System.out.println("first: " + set.first());
        System.out.println("last: " + set.last());
        System.out.println("higher: " + set.higher(dump));
        System.out.println("lower: " + set.lower(dump));

        System.out.println("---------------------------");
        while(set.size() != 0) {
            System.out.println(set.pollFirst());
        }
        System.out.println("---------------------------");

    }
}
