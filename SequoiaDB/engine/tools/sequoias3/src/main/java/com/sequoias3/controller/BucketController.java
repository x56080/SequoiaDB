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

   Source File Name = BucketController.java

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

import com.fasterxml.jackson.databind.ObjectMapper;
import com.fasterxml.jackson.dataformat.xml.XmlMapper;
import com.sequoias3.common.RestParamDefine;
import com.sequoias3.core.Bucket;
import com.sequoias3.model.*;
import com.sequoias3.core.User;
import com.sequoias3.exception.S3Error;
import com.sequoias3.exception.S3ServerException;
import com.sequoias3.service.BucketService;
import com.sequoias3.utils.RestUtils;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.http.HttpHeaders;
import org.springframework.http.HttpStatus;
import org.springframework.http.MediaType;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.*;

import javax.servlet.ServletInputStream;
import javax.servlet.http.HttpServletRequest;

@RestController
@RequestMapping(RestParamDefine.REST_S3)
public class BucketController {
    private static final Logger logger = LoggerFactory.getLogger(BucketController.class);

    @Autowired
    RestUtils restUtils;

    @Autowired
    BucketService bucketService;

    @PutMapping(value = "/{bucketname:.+}", produces = MediaType.APPLICATION_XML_VALUE)
    public ResponseEntity putBucket(@PathVariable("bucketname") String bucketName,
                                    @RequestHeader(RestParamDefine.AUTHORIZATION) String authorization,
                                    HttpServletRequest httpServletRequest)
            throws S3ServerException {
        User operator = restUtils.getOperatorByAuthorization(authorization);

        String location = getLocation(httpServletRequest);
        logger.info("Create bucket. bucketName ={}, operator={}, location={} ",
                bucketName, operator.getUserName(), location);
        bucketService.createBucket(operator.getUserId(),bucketName, location);
        return ResponseEntity.ok()
                .header(RestParamDefine.LOCATION, RestParamDefine.REST_DELIMITER+bucketName)
                .build();
    }

    @GetMapping(value="", produces = MediaType.APPLICATION_XML_VALUE)
    public ResponseEntity listBuckets( @RequestHeader(RestParamDefine.AUTHORIZATION) String authorization)
            throws S3ServerException {
        User operator = restUtils.getOperatorByAuthorization(authorization);

        logger.info("list buckets. operator={}", operator.getUserName());
        return ResponseEntity.ok()
                .body(bucketService.getService(operator));
    }

    @DeleteMapping(value = "/{bucketname:.+}", produces = MediaType.APPLICATION_XML_VALUE)
    public ResponseEntity deleteBucket(@PathVariable("bucketname") String bucketName,
                               @RequestHeader(RestParamDefine.AUTHORIZATION) String authorization)
            throws S3ServerException {
        User operator = restUtils.getOperatorByAuthorization(authorization);

        logger.info("delete bucket. bucketName={}, operator={}", bucketName, operator.getUserName());
        bucketService.deleteBucket(operator.getUserId(), bucketName);
        return ResponseEntity.noContent().build();
    }

    @RequestMapping(method = RequestMethod.HEAD, value = "/{bucketName:.+}")
    public ResponseEntity headBucket(@PathVariable("bucketName") String bucketName,
                               @RequestHeader(RestParamDefine.AUTHORIZATION) String authorization)
            throws S3ServerException {
        User operator = restUtils.getOperatorByAuthorization(authorization);
        logger.info("head bucket. bucketName={}, operator={}", bucketName, operator.getUserName());
        Bucket bucket = bucketService.getBucket(operator.getUserId(), bucketName);
        HttpHeaders headers = new HttpHeaders();
        if (bucket.getRegion() != null){
            headers.add(RestParamDefine.HeadBucketResultHeader.REGION, bucket.getRegion());
        }
        return ResponseEntity.ok()
                .headers(headers)
                .build();
    }

    @RequestMapping(method = RequestMethod.HEAD, value = "")
    public ResponseEntity headNone(@RequestHeader(RestParamDefine.AUTHORIZATION) String authorization)
            throws S3ServerException {
        restUtils.getOperatorByAuthorization(authorization);

        logger.error("Method not allowed. head none bucket.");
        return ResponseEntity.status(HttpStatus.METHOD_NOT_ALLOWED)
                .build();
    }

    private String getLocation(HttpServletRequest httpServletRequest)
            throws S3ServerException{
        int ONCE_READ_BYTES  = 1024;
        try {
            ServletInputStream inputStream = httpServletRequest.getInputStream();
            StringBuilder stringBuilder = new StringBuilder();
            byte[] b = new byte[ONCE_READ_BYTES];
            int len = inputStream.read(b , 0, ONCE_READ_BYTES);
            while(len > 0){
                stringBuilder.append(new String(b,0, len));
                len = inputStream.read(b , 0, ONCE_READ_BYTES);
            }
            String content = stringBuilder.toString();
            if (content.length() > 0) {
                ObjectMapper objectMapper = new XmlMapper();
                return objectMapper.readValue(content, CreateBucketConfiguration.class).getLocationConstraint();
            }else {
                return null;
            }
        }catch (Exception e){
            throw new S3ServerException(S3Error.BUCKET_INVALID_LOCATION, "get location failed", e);
        }
    }
}
