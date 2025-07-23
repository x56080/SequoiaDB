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

   Source File Name = MetaDao.java

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

import com.sequoias3.core.ObjectMeta;
import com.sequoias3.core.Region;
import com.sequoias3.exception.S3ServerException;
import org.bson.BSONObject;

import java.util.List;

public interface MetaDao {
    void insertMeta(ConnectionDao connectionDao, String metaCsName, String metaClName,
                    ObjectMeta object, Boolean isHistory, Region region)
            throws S3ServerException;

    QueryDbCursor queryMetaByBucket(String metaCsName, String metaClName, long bucketId,
                                    String prefix, String startAfter, Long specifiedVId,
                                    Boolean isIncludeDM)
            throws S3ServerException;

    ObjectMeta queryMetaByObjectName(String metaCsName, String metaClName,
                                     long bucketId, String objectName, Long versionId,
                                     Boolean noVersionFlag)
            throws S3ServerException;

    ObjectMeta queryMetaByBucketId(ConnectionDao connection, String metaCsName,
                                   String metaClName, long bucketId)
            throws S3ServerException;

    QueryDbCursor queryMetaByBucketForUpdate(ConnectionDao connectionDao, String metaCsName,
                                             String metaClName, long bucketId, String prefix,
                                             String startAfter, int limitNum )
            throws S3ServerException;

    QueryDbCursor queryMetaByBucketInKeys(String metaCsName, String metaClName,
                                  long bucketId, List<String> keys)
            throws S3ServerException;

    QueryDbCursor queryMetaListByParentId(String metaCsName, String metaClName, long bucketId,
                                          String parentIdName, long parentId,  String prefix,
                                          String startAfter, Long versionIdMarker, Boolean isIncludeDeleteMarker)
            throws S3ServerException;

    Boolean queryOneOtherMetaByParentId(ConnectionDao connectionDao, String metaCsName, String metaClName, long bucketId,
                                        String objectName, String parentIdName, long parentId)
            throws S3ServerException;

    ObjectMeta queryForUpdate(ConnectionDao connectionDao, String metaCsName, String metaClName,
                              long bucketId, String objectName, Long versionId, Boolean noVersionFlag)
            throws S3ServerException;

    void updateMeta(ConnectionDao connectionDao, String metaCsName, String metaClName, long bucketId,
                    String objectName, Long versionId, ObjectMeta object)
            throws S3ServerException;

    void updateMetaParentId(ConnectionDao connectionDao, String metaCsName, String metaClName, long bucketId,
                            String objectName, String parentIdName, long parentId)
            throws S3ServerException;

    void removeMeta(ConnectionDao connectionDao, String metaCsName, String metaClName, long bucketId,
                    String objectName, Long versionId, Boolean noVersionFlag)
            throws S3ServerException;

    long getObjectNumber(String metaCsName, String metaClName, long bucketId)
            throws S3ServerException;



    void releaseQueryDbCursor(QueryDbCursor queryDbCursor);
}
