/**
 * Copyright (C) 2012 SequoiaDB Inc.
 * <p>
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * <p>
 * http://www.apache.org/licenses/LICENSE-2.0
 * <p>
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */
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
