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

   Source File Name = DirDao.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoias3.dao;

import com.sequoiadb.base.DBCursor;
import com.sequoias3.core.Dir;
import com.sequoias3.core.Region;
import com.sequoias3.exception.S3ServerException;

public interface DirDao {
    void insertDir(ConnectionDao connection, String metaCsName, Dir dir, Region region) throws S3ServerException;

    Dir queryDir(ConnectionDao connection, String metaCsName, Long bucketId,
                          String delimiter, String dirName, Boolean forUpdate)
            throws S3ServerException;

    QueryDbCursor queryDirList(String metaCsName, Long bucketId,
                          String delimiter, String dirPrefix, String startAfter)
            throws S3ServerException;

    void delete(ConnectionDao connectionDao, String metaCsName, Long bucketId,
                String delimiter, String dirName)
            throws S3ServerException;
}
