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

   Source File Name = UploadDao.java

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

import com.sequoias3.core.UploadMeta;
import com.sequoias3.exception.S3ServerException;

public interface UploadDao {
    void insertUploadMeta(ConnectionDao connectionDao, long bucketId, String objectName, Long uploadId, UploadMeta upload)
            throws S3ServerException;

    void updateUploadMeta(ConnectionDao connection, long bucketId,
                          String objectName, Long uploadId, UploadMeta upload)
            throws S3ServerException;

    UploadMeta queryUploadByUploadId(ConnectionDao connection, Long bucketId,
                                     String objectName, long uploadId, Boolean forUpdate)
            throws S3ServerException;

    void deleteUploadByUploadId(ConnectionDao connection, long bucketId,
                                String objectName, long uploadId)
            throws S3ServerException;

    QueryDbCursor queryInvalidUploads() throws S3ServerException;

    QueryDbCursor queryExceedUploads(long exceedTime) throws S3ServerException;

    QueryDbCursor queryUploadsByBucket(long bucketId, String prefix, String keyMarker,
                                       Long uploadMarker, Integer status) throws S3ServerException;

    void setUploadsStatus(long bucketId, Long uploadId, int status) throws S3ServerException;
}
