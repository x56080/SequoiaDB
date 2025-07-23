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

   Source File Name = AssertUtil.java

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

import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;

import static org.junit.Assert.fail;

public class AssertUtil {
    public static void assertNotThrows(Executable executable) {
        if (executable == null) {
            throw new RuntimeException("executable can not be null");
        }

        try {
            executable.execute();
        } catch (Throwable e) {
            fail(String.format("Expected no exception, but an exception was thrown: %s", e.getMessage()));
        }
    }

    public static void assertSDBError(SDBError expected, Executable executable) {
        if (expected == null) {
            throw new RuntimeException("expected can not be null");
        }
        if (executable == null) {
            throw new RuntimeException("executable can not be null");
        }

        try {
            executable.execute();
            fail("Expected to raise a BaseException, but it did not.");
        } catch (Throwable e) {
            if (!(e instanceof BaseException)) {
                fail(String.format("Expected SDBError to be %s, but it did not: %s",
                        expected.getErrorCode(), e.getMessage()));
            }
        }
    }

    public static void assertNotSDBError(SDBError expected, Executable executable) {
        if (expected == null) {
            throw new RuntimeException("expected can not be null");
        }
        if (executable == null) {
            throw new RuntimeException("executable can not be null");
        }

        try {
            executable.execute();
        } catch (Throwable e) {
            if (e instanceof BaseException && ((BaseException) e).getErrorCode() == expected.getErrorCode()) {
                fail(String.format("Expected SDBError not %s, but it is.", expected.getErrorCode()));
            }
        }
    }

    public interface Executable {
        void execute() throws Throwable;
    }
}
