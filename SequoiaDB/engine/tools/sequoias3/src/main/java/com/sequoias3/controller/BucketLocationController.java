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

   Source File Name = BucketLocationController.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoias3.controller;

import com.sequoias3.common.RestParamDefine;
import com.sequoias3.core.User;
import com.sequoias3.exception.S3ServerException;
import com.sequoias3.service.BucketService;
import com.sequoias3.utils.RestUtils;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.http.MediaType;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.*;

@RestController
public class BucketLocationController {
    private static final Logger logger = LoggerFactory.getLogger(BucketLocationController.class);

    @Autowired
    RestUtils restUtils;

    @Autowired
    BucketService bucketService;

    @GetMapping(value = "/{bucketname:.+}", params = RestParamDefine.LOCATION, produces = MediaType.APPLICATION_XML_VALUE)
    public ResponseEntity getBucketLocation(@PathVariable("bucketname") String bucketName,
                                            @RequestHeader(value = RestParamDefine.AUTHORIZATION, required = false) String authorization)
            throws S3ServerException {
        try {
            User operator = restUtils.getOperatorByAuthorization(authorization);

            logger.debug("get bucket location. bucket={}, operator={}", bucketName, operator.getUserName());

            return ResponseEntity.ok()
                    .body(bucketService.getBucketLocation(operator.getUserId(), bucketName));
        }catch (Exception e){
            logger.error("get bucket location failed. bucketName={}", bucketName);
            throw e;
        }
    }
}
