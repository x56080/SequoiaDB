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

   Source File Name = SdbBase64Test.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoiadb.test.misc;

import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.util.SdbBase64;
import org.junit.*;

import java.nio.charset.StandardCharsets;
import java.util.Base64;
import java.util.Random;

public class SdbBase64Test {

    private static SdbBase64 sdbBase64;
    private static Base64.Encoder javaEncoder;

    private static Random random;

    @BeforeClass
    public static void setConnBeforeClass() throws Exception {
        sdbBase64 = new SdbBase64();
        javaEncoder = Base64.getEncoder();
        random = new Random();
    }

    @AfterClass
    public static void DropConnAfterClass() throws Exception {
        // do nothing
    }

    @Before
    public void setUp() throws Exception {
        // do nothing
    }

    @After
    public void tearDown() throws Exception {
        // do nothing
    }

    @Test
    public void errorTest() {
        // case 1: null
        String data = null;
        try {
            sdbBase64.encode(data);
        } catch (BaseException e) {
            Assert.assertEquals(SDBError.SDB_INVALIDARG.getErrorCode(), e.getErrorCode());
        }

        try {
            sdbBase64.decode(data);
        } catch (BaseException e) {
            Assert.assertEquals(SDBError.SDB_INVALIDARG.getErrorCode(), e.getErrorCode());
        }

        // case 2: empty string
        data = "";
        String encodeData = sdbBase64.encode(data);
        Assert.assertEquals("", encodeData);

        String decodeData = sdbBase64.decode(data);
        Assert.assertEquals("", decodeData);
    }

    @Test
    public void encodeWithJdkTest() {
        int testCount = 10_0000;

        for(int i = 0; i < testCount; i++) {
            String originData = generateData();

            String sdbEncodeData = sdbBase64.encode(originData);
            byte[] javaEncodeBytes = javaEncoder.encode(originData.getBytes(StandardCharsets.UTF_8));
            String javaEncodeData = new String(javaEncodeBytes, StandardCharsets.UTF_8);

            Assert.assertEquals(sdbEncodeData, javaEncodeData);
        }
    }

    @Test
    public void encodeAndDecodeTest() {
        int testCount = 10_0000;
        SdbBase64 sdbBase64 = new SdbBase64();

        for(int i = 0; i < testCount; i++) {
            String originData = generateData();

            String encodeData = sdbBase64.encode(originData);
            String decodeData = sdbBase64.decode(encodeData);

            String errorMsg = "encodeData: " + encodeData;
            Assert.assertEquals(errorMsg, originData, decodeData);
        }
    }

    private String generateData() {
        String text = "abcdefghijklmnopqrstuvwxyz" +
                "ABCDEFGHIJKLMNOPQRSTUVWXYZ" +
                "1234567890" +
                "`~!@#$%^&*()-_=+[{]}\\|;:'\",<.>/? " +
                "你好" +
                "·！￥……（）——【】；’，。《》？、";
        StringBuilder password = new StringBuilder();

        int len = random.nextInt( text.length() );
        for ( int i = 0; i < len; i++ ) {
            int post = random.nextInt( text.length() );
            password.append( text.charAt( post ) );
        }
        return password.toString();
    }

}
