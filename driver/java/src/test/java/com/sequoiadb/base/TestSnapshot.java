package com.sequoiadb.base;

import com.sequoiadb.test.SingleCSCLTestCase;
import org.junit.Test;

public class TestSnapshot extends SingleCSCLTestCase {
    @Test
    public void testResetSnapshot() {
        sdb.resetSnapshot();
    }
}
