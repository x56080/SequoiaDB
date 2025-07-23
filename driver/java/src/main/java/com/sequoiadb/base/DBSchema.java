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

   Source File Name = DBSchema.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoiadb.base;

import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.message.request.AdminRequest;
import com.sequoiadb.message.response.SdbReply;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;

import java.util.Objects;

/**
 * Information schema of SequoiaDB.
* */
public class DBSchema {

    private String name;
    private Sequoiadb sequoiadb;

    DBSchema(Sequoiadb sequoiadb, String name) {
        this.name = name;
        this.sequoiadb = sequoiadb;
    }

    /**
     * Add column in schema.
     *
     * @param name The name of schema.
     * @param columnDef The columnDef in schema.
     * @throws BaseException If error happens.
     */
    public void addColumn(String name, BSONObject columnDef) {
        if (name == null || name.equals("")) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "The column name is empty or null");
        }
        if (columnDef == null) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "The column define is null");
        }

        BSONObject rebuildObj = new BasicBSONObject();
        rebuildObj.put(name, columnDef);
        alterInternal(SdbConstants.SCHEMA_ADD_COLUMN, rebuildObj);
    }

    /**
     * Alter column in schema.
     *
     * @param name The name of schema.
     * @param options The alter column option.
     * @throws BaseException If error happens.
     */
    public void alterColumn(String name, BSONObject options) {
        if (name == null || name.equals("")) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "The column name is empty or null");
        }
        if (options == null) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "The options is null");
        }

        BSONObject rebuildObj = new BasicBSONObject();
        rebuildObj.put(name, options);
        alterInternal(SdbConstants.SCHEMA_ALTER_COLUMN, rebuildObj);
    }

    /**
     * Rename column in schema.
     *
     * @param oldName The old column name.
     * @param newName The new column name.
     * @throws BaseException If error happens.
     */
    public void renameColumn(String oldName, String newName) {
        if (oldName == null || oldName.equals("")) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "The column name is empty or null");
        }
        if (newName == null || newName.equals("")) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "The new name of column is empty or null");
        }

        BSONObject rebuildObj = new BasicBSONObject();
        rebuildObj.put(oldName, newName);
        alterInternal(SdbConstants.SCHEMA_RENAME_COLUMN, rebuildObj);
    }

    /**
     * Drop column in schema.
     *
     * @param name The column name.
     * @throws BaseException If error happens.
     */
    public void dropColumn(String name) {
        if (name == null || name.equals("")) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "The column name is empty or null");
        }
        BSONObject rebuildObj = new BasicBSONObject();
        rebuildObj.put(name, 1);
        alterInternal(SdbConstants.SCHEMA_DROP_COLUMN, rebuildObj);
    }

    /**
     * Drop column write default value in schema.
     *
     * @param name The column name.
     * @throws BaseException If error happens.
     */
    public void dropColumnDefault(String name) {
        if (name == null || name.equals("")) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "The column name is empty or null");
        }
        BSONObject rebuildObj = new BasicBSONObject();
        rebuildObj.put(name, 1);
        alterInternal(SdbConstants.SCHEMA_DROP_DEFAULT, rebuildObj);
    }

    /**
     * Alter schema option.
     *
     * @param options The schema option.
     * @throws BaseException If error happens.
     */
    public void alter(BSONObject options) {
        if (options == null) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "The options is null");
        }

        if (options.containsField(SdbConstants.FIELD_NAME_ACTION)) {
            BSONObject rebuildObj = new BasicBSONObject();
            rebuildObj.put(SdbConstants.FIELD_NAME_NAME, name);
            rebuildObj.putAllUnique(options);

            AdminRequest request = new AdminRequest(AdminCommand.ALTER_SCHEMA, rebuildObj);
            SdbReply response = sequoiadb.requestAndResponse(request);
            sequoiadb.throwIfError(response);
        } else {
            alterInternal(SdbConstants.SCHEMA_SET_ATTRIBUTES, options);
        }
    }

    @Override
    public String toString() {
        return "DBSchema{" +
                "name='" + name + '\'' +
                '}';
    }

    @Override
    public boolean equals(Object o) {
        if (this == o) return true;
        if (!(o instanceof DBSchema)) return false;

        DBSchema dbSchema = (DBSchema) o;

        return Objects.equals(name, dbSchema.name);
    }

    @Override
    public int hashCode() {
        return name != null ? name.hashCode() : 0;
    }

    private void alterInternal(String actionName, BSONObject options) {
        BSONObject obj = new BasicBSONObject();
        obj.put(SdbConstants.FIELD_NAME_NAME, name);
        obj.put(SdbConstants.FIELD_NAME_ACTION, actionName);
        if (options != null) {
            obj.put(SdbConstants.FIELD_NAME_OPTIONS, options);
        }

        AdminRequest request = new AdminRequest(AdminCommand.ALTER_SCHEMA, obj);
        SdbReply response = sequoiadb.requestAndResponse(request);
        sequoiadb.throwIfError(response);
    }
}
