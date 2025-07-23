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

   Source File Name = PartDao.java

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

import com.sequoias3.core.Part;
import com.sequoias3.exception.S3ServerException;

public interface PartDao {
    void insertPart(ConnectionDao connection, long uploadId, long partNumber, Part part) throws S3ServerException;

    void updatePart(ConnectionDao connection, long uploadId, long partNumber, Part part) throws S3ServerException;

    Part queryPartByPartnumber(ConnectionDao connection, long uploadId, long partNumber) throws S3ServerException;

    Part queryPartBySize(ConnectionDao connection, long uploadId, Long size) throws S3ServerException;

    void deletePart(ConnectionDao connection, long uploadId, Long partNumber) throws S3ServerException;

    QueryDbCursor queryPartList(long uploadId, Boolean onlyPositiveNo,
                                Integer marker, Integer maxSize) throws S3ServerException;

    QueryDbCursor queryPartListForUpdate(ConnectionDao connection, long uploadId) throws S3ServerException;
}
