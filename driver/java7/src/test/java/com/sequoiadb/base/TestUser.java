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
        } catch (BaseException e) {
            if (e.getErrorCode() != SDBError.SDB_RTN_COORD_ONLY.getErrorCode()) {
                fail(e.toString());
            }
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
        }catch (BaseException | UnsupportedEncodingException e){
            throw new BaseException(SDBError.SDB_INVALIDARG, e);
        }

        // case 2: GBK
        try {
            String user = "用户";
            String password = new String("密码".getBytes("GBK"));
            sdb.createUser(user, password);
            sdb.removeUser(user, password);
        }catch (BaseException | UnsupportedEncodingException e){
            throw new BaseException(SDBError.SDB_INVALIDARG, e);
        }
    }
}
