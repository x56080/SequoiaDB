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

   Source File Name = DecimalCommonTest.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoiadb.test.decimal;

import com.sequoiadb.test.common.DecimalCommon;
import org.bson.types.BSONDecimal;
import org.junit.*;

/**
 * @author tanzhaobo
 * @brief 测试Decimal公共的函数
 */
public class DecimalCommonTest {

    @BeforeClass
    public static void beforeClass() throws Exception {
    }

    @AfterClass
    public static void afterClass() throws Exception {
    }

    @Before
    public void setUp() throws Exception {
    }

    @After
    public void tearDown() throws Exception {
    }

    /*
     * test DecimalCommon::genBSONDecimal()
     * */
    @Test
    @Ignore
    public void genBSONDecimalTest() {
        BSONDecimal decimal = DecimalCommon.genBSONDecimal();
        System.out.println("decimal is: " + decimal);
    }

    /*
     * test DecimalCommon::genBSONDecimal(...)
     * */
    @Test
    @Ignore
    public void genBSONDecimalTest2() {
        BSONDecimal decimal1 = DecimalCommon.genBSONDecimal(true, true, true, 10);
        System.out.println("decimal1 is: " + decimal1);

        BSONDecimal decimal2 = DecimalCommon.genBSONDecimal(true, true, false, 0);
        System.out.println("decimal2 is: " + decimal2);

        BSONDecimal decimal3 = DecimalCommon.genBSONDecimal(true, false, false, 0);
        System.out.println("decimal3 is: " + decimal3);

        BSONDecimal decimal4 = DecimalCommon.genBSONDecimal(false, false, false, 0);
        System.out.println("decimal4 is: " + decimal4);
    }

    /*
     * test DecimalCommon::genIntegerBSONDecimal()
     * */
    @Test
    @Ignore
    public void genIntegerBSONDecimalTest() {
        BSONDecimal decimal1 = DecimalCommon.genIntegerBSONDecimal(true, true);
        System.out.println("decimal1 is: " + decimal1);

        BSONDecimal decimal2 = DecimalCommon.genIntegerBSONDecimal(true, false);
        System.out.println("decimal2 is: " + decimal2);

        BSONDecimal decimal3 = DecimalCommon.genIntegerBSONDecimal(false, false);
        System.out.println("decimal3 is: " + decimal3);
    }


}
