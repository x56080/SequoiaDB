package com.sequoiadb.base;

import com.sequoiadb.test.SingleCSCLTestCase;
import org.junit.Test;

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
}
