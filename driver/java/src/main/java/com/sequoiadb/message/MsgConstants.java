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

package com.sequoiadb.message;

public final class MsgConstants {
    /* 
     * Internal flag, not exposed to users.
     * This flag identifies whether the inserted record (or batch of records) contains
     * the '_id' field, which can be used to skip the '_id' field check.
    */
    public final static int FLG_INSERT_HAS_ID_FIELD = 0x00000010;
    public final static String AUTH_OPTIONS = "Options";
}
