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

package org.bson.types;

import java.util.Date;

/**
 * The class BSONDate represents a specific instant in time, with millisecond precision. This type
 * corresponds to the Date type of SequoiaDB database. When accessing SequoiaDB database, you need
 * to convert the date type in your own business code to BSONDate.
 *
 */
public class BSONDate extends Date {

    // BSONDate is a bridge between SequoiaDB date type and java date type,
    // so do not provide a construct with no arguments.

    /**
     * Construct a BSONDate.
     * @param value The milliseconds since January 1, 1970, 00:00:00 GMT.
     */
    public BSONDate( long value ) {
        super( value );
    }
}
