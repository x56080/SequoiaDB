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

   Source File Name = BsonUtils.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          26/01/2026  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/

package com.sequoiadb.schedule.common;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;

import java.util.ArrayList;
import java.util.Date;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

public final class BsonUtils {
    private BsonUtils() {
    }

    @SuppressWarnings("unchecked")
    private static <F> F get(BSONObject object, String field) {
        try {
            return (F) object.get(field);
        }
        catch (Exception e) {
            throw new IllegalArgumentException(
                    "getting field value failed: field= " + field + ", obj= " + object, e);
        }
    }

    private static <F> F getChecked(BSONObject object, String field) {
        F f = get(object, field);
        if (f == null) {
            throw new IllegalArgumentException("missing field:field= " + field + ", obj=" + object);
        }
        return f;
    }

    private static <F> F getOrElse(BSONObject object, String field, F defaultValue) {
        F f = get(object, field);
        if (f == null) {
            f = defaultValue;
        }
        return f;
    }

    public static Object getObject(BSONObject object, String field) {
        return get(object, field);
    }

    public static Object getObjectChecked(BSONObject object, String field) {
        return getChecked(object, field);
    }

    public static Object getObjectOrElse(BSONObject object, String field, Object defaultValue) {
        return getOrElse(object, field, defaultValue);
    }

    public static String getString(BSONObject object, String field) {
        try {
            return get(object, field);
        }
        catch (Exception e) {
            throw new IllegalArgumentException(
                    "convert Object to String failed: field= " + field + ", obj= " + object, e);
        }
    }

    public static String getStringChecked(BSONObject object, String field) {
        try {
            return getChecked(object, field);
        }
        catch (Exception e) {
            throw new IllegalArgumentException(
                    "convert Object to String failed: field= " + field + ", obj= " + object, e);
        }
    }

    public static String getStringCheckedNotEmpty(BSONObject object, String field) {
        String value = getStringChecked(object, field);
        if (value.isEmpty()) {
            throw new IllegalArgumentException("field " + field + " is empty");
        }
        return value;
    }

    public static String getStringOrElse(BSONObject object, String field, String defaultValue) {
        try {
            return getOrElse(object, field, defaultValue);
        }
        catch (Exception e) {
            throw new IllegalArgumentException(
                    "convert Object to String failed: field= " + field + ", obj= " + object, e);
        }
    }

    public static Number getNumber(BSONObject object, String field) {
        try {
            return get(object, field);
        }
        catch (Exception e) {
            throw new IllegalArgumentException(
                    "convert Object to Number failed: field= " + field + ", obj= " + object, e);
        }
    }

    public static Number getNumberChecked(BSONObject object, String field) {
        try {
            return getChecked(object, field);
        }
        catch (Exception e) {
            throw new IllegalArgumentException(
                    "convert Object to Number failed: field= " + field + ", obj= " + object, e);
        }
    }

    public static Number getNumberOrElse(BSONObject object, String field, Number defaultValue) {
        try {
            return getOrElse(object, field, defaultValue);
        }
        catch (Exception e) {
            throw new IllegalArgumentException(
                    "convert Object to Number failed: field= " + field + ", obj= " + object, e);
        }
    }

    public static Long getLong(BSONObject object, String field) {
        try {
            Long resultNum = null;
            Number num = getNumber(object, field);
            if (num != null) {
                resultNum = num.longValue();
            }
            return resultNum;
        }
        catch (Exception e) {
            throw new IllegalArgumentException(
                    "convert Number to Long failed: field= " + field + ", obj= " + object, e);
        }
    }

    public static Long getLongChecked(BSONObject object, String field) {
        try {
            return getNumberChecked(object, field).longValue();
        }
        catch (Exception e) {
            throw new IllegalArgumentException(
                    "convert Number to Long failed: field= " + field + ", obj= " + object, e);
        }
    }

    public static Long getLongOrElse(BSONObject object, String field, long defaultValue) {
        try {
            return getNumberOrElse(object, field, defaultValue).longValue();
        }
        catch (Exception e) {
            throw new IllegalArgumentException(
                    "convert Number to Long failed: field= " + field + ", obj= " + object, e);
        }
    }

    public static Integer getInteger(BSONObject object, String field) {
        try {
            Integer resultNum = null;
            Number num = getNumber(object, field);
            if (num != null) {
                resultNum = num.intValue();
            }
            return resultNum;
        }
        catch (Exception e) {
            throw new IllegalArgumentException(
                    "convert Number to Integer failed: field= " + field + ", obj= " + object, e);
        }
    }

    public static Integer getIntegerChecked(BSONObject object, String field) {
        try {
            return getNumberChecked(object, field).intValue();
        }
        catch (Exception e) {
            throw new IllegalArgumentException(
                    "convert Number to Integer failed: field= " + field + ", obj= " + object, e);
        }
    }

    public static Integer getIntegerOrElse(BSONObject object, String field, int defaultValue) {
        try {
            return getNumberOrElse(object, field, defaultValue).intValue();
        }
        catch (Exception e) {
            throw new IllegalArgumentException(
                    "convert Number to Integer failed: field= " + field + ", obj= " + object, e);
        }
    }

    public static Boolean getBoolean(BSONObject object, String field) {
        try {
            return get(object, field);
        }
        catch (Exception e) {
            throw new IllegalArgumentException(
                    "convert Object to Boolean failed: field= " + field + ", obj= " + object, e);
        }
    }

    public static Boolean getBooleanChecked(BSONObject object, String field) {

        try {
            return getChecked(object, field);
        }
        catch (Exception e) {
            throw new IllegalArgumentException(
                    "convert Object to Boolean failed: field= " + field + ", obj= " + object, e);
        }
    }

    public static Boolean getBooleanOrElse(BSONObject object, String field, boolean defaultValue) {
        try {
            return getOrElse(object, field, defaultValue);
        }
        catch (Exception e) {
            throw new IllegalArgumentException(
                    "convert Object to Boolean failed: field= " + field + ", obj= " + object, e);
        }
    }

    public static BasicBSONList getArray(BSONObject object, String field) {
        try {
            return get(object, field);
        }
        catch (Exception e) {
            throw new IllegalArgumentException(
                    "convert Object to BasicBSONList failed: field= " + field + ", obj= " + object,
                    e);
        }
    }

    public static List<Long> getLongArray(BSONObject object, String field) {
        BasicBSONList bsonList = getArray(object, field);
        if (bsonList == null) {
            return null;
        }

        List<Long> ret = new ArrayList<Long>();
        for (Object idObj : bsonList) {
            if (!(idObj instanceof Number)) {
                throw new IllegalArgumentException(field + " is not number type: " + object);
            }
            ret.add(((Number) idObj).longValue());
        }
        return ret;
    }

    public static List<String> getStringArray(BSONObject object, String field) {
        BasicBSONList bsonList = getArray(object, field);
        if (bsonList == null) {
            return null;
        }

        List<String> ret = new ArrayList<String>();
        for (Object idObj : bsonList) {
            if (!(idObj instanceof String)) {
                throw new IllegalArgumentException(field + " is not String type: " + object);
            }
            ret.add(((String) idObj));
        }
        return ret;
    }

    public static List<Integer> getIntegerArray(BSONObject object, String field) {
        BasicBSONList bsonList = getArray(object, field);
        if (bsonList == null) {
            return null;
        }

        List<Integer> ret = new ArrayList<Integer>();
        for (Object idObj : bsonList) {
            if (!(idObj instanceof Number)) {
                throw new IllegalArgumentException(field + " is not number type: " + object);
            }
            ret.add(((Number) idObj).intValue());
        }
        return ret;
    }

    public static BasicBSONList getArrayChecked(BSONObject object, String field) {
        try {
            return getChecked(object, field);
        }
        catch (Exception e) {
            throw new IllegalArgumentException(
                    "convert Object to BasicBSONList failed: field= " + field + ", obj= " + object,
                    e);
        }
    }

    public static BasicBSONList getArrayOrElse(BSONObject object, String field,
            BasicBSONList defaultValue) {
        try {
            return getOrElse(object, field, defaultValue);
        }
        catch (Exception e) {
            throw new IllegalArgumentException(
                    "convert Object to BasicBSONList failed: field= " + field + ", obj= " + object,
                    e);
        }
    }

    public static BSONObject getBSON(BSONObject object, String field) {
        try {
            return get(object, field);
        }
        catch (Exception e) {
            throw new IllegalArgumentException(
                    "convert Object to BSONObject failed: field= " + field + ", obj= " + object, e);
        }
    }

    public static BSONObject getBSONOrElse(BSONObject object, String field,
            BSONObject defaultValue) {
        try {
            return getOrElse(object, field, defaultValue);
        }
        catch (Exception e) {
            throw new IllegalArgumentException(
                    "convert Object to BSONObject failed: field= " + field + ", obj= " + object, e);
        }
    }

    public static BSONObject getBSONChecked(BSONObject object, String field) {
        try {
            return getChecked(object, field);
        }
        catch (Exception e) {
            throw new IllegalArgumentException(
                    "convert Object to BSONObject failed: field= " + field + ", obj= " + object, e);
        }
    }

    public static BSONObject getBSONObject(BSONObject object, String field) {
        try {
            return get(object, field);
        }
        catch (Exception e) {
            throw new IllegalArgumentException(
                    "convert Object to BSONObject failed: field= " + field + ", obj= " + object, e);
        }
    }

    public static BSONObject getBSONObjectChecked(BSONObject object, String field) {
        try {
            return getChecked(object, field);
        }
        catch (Exception e) {
            throw new IllegalArgumentException(
                    "convert Object to BSONObject failed: field= " + field + ", obj= " + object, e);
        }
    }

    public static Date getDateChecked(BSONObject object, String field) {
        try {
            return getChecked(object, field);
        }
        catch (Exception e) {
            throw new IllegalArgumentException(
                    "convert Object to Date failed: field= " + field + ", obj= " + object, e);
        }
    }

    public static BSONObject deepCopyRecordBSON(BSONObject obj) {
        if (obj == null) {
            return null;
        }
        try {
            if (obj instanceof BasicBSONObject) {
                return deepCopyBasicBSON((BasicBSONObject) obj);
            }
            if (obj instanceof BasicBSONList) {
                return deepCopyBasicBSONList((BasicBSONList) obj);
            }
        }
        catch (Exception e) {
            throw new IllegalArgumentException(
                    "deep copy failed:" + obj.getClass().getName() + ", " + obj.toString(), e);
        }
        throw new IllegalArgumentException("deep copy failed: unknown type:"
                + obj.getClass().getName() + ", " + obj.toString());
    }

    private static BasicBSONObject deepCopyBasicBSON(BasicBSONObject bson) {
        if (bson == null) {
            return null;
        }
        BasicBSONObject ret = new BasicBSONObject();
        for (Map.Entry<String, Object> e : bson.entrySet()) {
            ret.put(e.getKey(), deepCopyRecordObject(e.getValue()));
        }
        return ret;
    }

    public static BasicBSONList deepCopyBasicBSONList(BasicBSONList bson) {
        if (bson == null) {
            return null;
        }
        BasicBSONList ret = new BasicBSONList();
        for (Object e : bson) {
            ret.add(deepCopyRecordObject(e));
        }
        return ret;
    }

    public static Map<String, Object> deepCopyMap(Map<String, Object> map) {
        if (map == null) {
            return null;
        }
        Map<String, Object> ret = new HashMap<String, Object>();
        for (Map.Entry<String, Object> e : map.entrySet()) {
            ret.put(e.getKey(), deepCopyRecordObject(e.getValue()));
        }
        return ret;
    }

    private static Object deepCopyRecordObject(Object obj) {
        if (noNeedCopy(obj)) {
            // 基本类型(数值、布尔）、字符串、null、无需拷贝
            return obj;
        }
        if (obj instanceof BasicBSONObject) {
            return deepCopyBasicBSON((BasicBSONObject) obj);
        }
        if (obj instanceof BasicBSONList) {
            return deepCopyBasicBSONList((BasicBSONList) obj);
        }
        throw new IllegalArgumentException("deep copy failed: unknown type:"
                + obj.getClass().getName() + ", " + obj.toString());
    }

    private static boolean noNeedCopy(Object o) {
        if (o == null) {
            return true;
        }
        if (o instanceof Boolean) {
            return true;
        }
        if (o instanceof Character) {
            return true;
        }
        if (o instanceof Byte) {
            return true;
        }
        if (o instanceof Short) {
            return true;
        }
        if (o instanceof Integer) {
            return true;
        }
        if (o instanceof Long) {
            return true;
        }
        if (o instanceof Float) {
            return true;
        }
        if (o instanceof Double) {
            return true;
        }
        if (o instanceof String) {
            return true;
        }
        return false;
    }
}
