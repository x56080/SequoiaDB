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

   Source File Name = BucketVersioningServiceImpl.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoias3.service.impl;

import com.sequoias3.common.VersioningStatusType;
import com.sequoias3.core.Bucket;
import com.sequoias3.model.*;
import com.sequoias3.dao.BucketDao;
import com.sequoias3.dao.UserDao;
import com.sequoias3.exception.S3Error;
import com.sequoias3.exception.S3ServerException;
import com.sequoias3.service.BucketService;
import com.sequoias3.service.BucketVersioningService;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Service;

@Service
public class BucketVersioningServiceImpl implements BucketVersioningService {
    private static final Logger logger = LoggerFactory.getLogger(BucketVersioningServiceImpl.class);

    @Autowired
    BucketDao bucketDao;

    @Autowired
    BucketService bucketService;

    @Autowired
    UserDao userDao;

    @Override
    public void putBucketVersioning(long ownerID, String bucketName, String status) throws S3ServerException {
        try {
            bucketService.getBucket(ownerID, bucketName);
            bucketDao.updateBucketVersioning(bucketName, status);
        }catch (S3ServerException e){
            throw e;
        }catch (Exception e){
            throw new S3ServerException(S3Error.BUCKET_VERSIONING_SET_FAILED,
                    "put bucket versioning failed. bucketname="+bucketName+",status="+status, e);
        }
    }

    @Override
    public VersioningConfigurationNull getBucketVersioning(long ownerID, String bucketName)
            throws S3ServerException{
        try{
            Bucket bucket = bucketService.getBucket(ownerID, bucketName);
            if (!bucket.getVersioningStatus().equals(VersioningStatusType.NONE.getName())){
                VersioningConfiguration versioningCfg = new VersioningConfiguration();
                versioningCfg.setStatus(bucket.getVersioningStatus());
                return versioningCfg;
            }else {
                return new VersioningConfigurationNull();
            }
        }catch (S3ServerException e){
            throw e;
        }catch (Exception e){
            throw new S3ServerException(S3Error.BUCKET_VERSIONING_GET_FAILED,
                    "get bucket versioning failed. bucketname="+bucketName, e);
        }
    }
}
