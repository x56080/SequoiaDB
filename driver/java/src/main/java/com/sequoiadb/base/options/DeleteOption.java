/*
 * Copyright 2022 SequoiaDB Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except
 * in compliance with the License. You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed under the License
 * is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express
 * or implied. See the License for the specific language governing permissions and limitations under
 * the License.
 */

package com.sequoiadb.base.options;
import com.sequoiadb.util.Helper;
import org.bson.BSONObject;

/**
 * The optional parameter of the delete operation.
 *
 * @since 3.4.5/5.0.3
 */
public class DeleteOption extends BaseOption {

    private BSONObject hint = null;
    private int flag = 0;

    /**
     * Gets the hint option.
     *
     * @return A BSONObject of the hint option.
     */
    public BSONObject getHint() {
        return hint;
    }

    /**
     * Sets the hint option, default is null.
     *
     * @param hint Specified the index used to scan data. e.g. {"":"ageIndex"} means using index
     *             "ageIndex" to scan data(index scan); {"":null} means table scan. when hint is
     *             null, database automatically match the optimal index to scan data.
     * @return this
     */
    public DeleteOption setHint( BSONObject hint ) {
        this.hint = hint;
        return this;
    }

    @Override
    public String toString() {
        return "DeleteOption{" +
                "hint=" + hint +
                ", flag=" + flag +
                '}';
    }

    @Override
    public Object clone() throws CloneNotSupportedException {
        return super.clone();
    }
}
