package com.sequoiadb.datasource;

import com.sequoiadb.exception.BaseException;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.ObjectInputStream;
import java.io.ObjectOutputStream;

/**
 * Created by tanzhaobo on 2018/1/22.
 */
class Helper {
    public static BaseException copyBaseException(final BaseException exp) {
        BaseException exception = null;
        if (exp == null) {
            return null;
        }
        try {
            // write object
            ByteArrayOutputStream baos = new ByteArrayOutputStream();
            ObjectOutputStream  oos = new ObjectOutputStream(baos);
            oos.writeObject(exp);
            // read object
            ByteArrayInputStream bais = new ByteArrayInputStream(baos.toByteArray());
            ObjectInputStream ois = new ObjectInputStream(bais);
            exception = (BaseException) ois.readObject();
        } catch (Exception e) {
            return null;
        }
        return exception;
    }
}
