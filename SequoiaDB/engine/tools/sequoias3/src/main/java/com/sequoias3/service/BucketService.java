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

   Source File Name = BucketService.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoias3.service;

import com.sequoias3.core.Bucket;
import com.sequoias3.model.GetServiceResult;
import com.sequoias3.core.User;
import com.sequoias3.exception.S3ServerException;
import com.sequoias3.model.LocationConstraint;

public interface BucketService {
    void createBucket(long ownerID, String bucketName, String region) throws S3ServerException;

    void deleteBucket(long ownerID, String bucketName) throws S3ServerException;

    GetServiceResult getService(User owner) throws S3ServerException;

    Bucket getBucket(long ownerID, String bucketName) throws S3ServerException;

    void deleteBucketForce(Bucket bucket) throws S3ServerException;

    LocationConstraint getBucketLocation(long ownerID, String bucketName) throws S3ServerException;
}
