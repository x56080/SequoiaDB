/*
 * Copyright 2022 SequoiaDB Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
*/

package org.apache.spark.sql.sequoiadb

import org.apache.spark.sql.AnalysisException
import org.apache.spark.sql.connector.catalog.Table
import org.apache.spark.sql.sequoiadb.catalog.SupportsUpdate

object SdbDataSourceV2Implicits {

    implicit class TableHelper(table: Table) {
        def asUpdatable: SupportsUpdate = {
            table match {
                case support: SupportsUpdate =>
                    support
                case _ =>
                    throw new AnalysisException(s"Table does not support updates: ${table.name}")
            }
        }
    }

}
