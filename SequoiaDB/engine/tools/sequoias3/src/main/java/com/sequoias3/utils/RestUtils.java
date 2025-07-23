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

   Source File Name = RestUtils.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoias3.utils;

import com.sequoias3.common.RestParamDefine;
import com.sequoias3.core.Range;
import com.sequoias3.core.User;
import com.sequoias3.dao.UserDao;
import com.sequoias3.exception.S3Error;
import com.sequoias3.exception.S3ServerException;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Component;

import java.io.UnsupportedEncodingException;
import java.net.URLDecoder;

@Component
public class RestUtils {
    private static final Logger logger = LoggerFactory.getLogger(RestUtils.class);

    @Autowired
    UserDao userDao;

    public User getOperatorByAuthorization(String authorization) throws S3ServerException {
        //       1.get access key id
        String accessKeyId = null;
        int beginIndex = authorization.indexOf(RestParamDefine.REST_CREDENTIAL);
        if (-1 == beginIndex) {
            throw new S3ServerException(S3Error.NO_CREDENTIALS, "no credentials. authorization = " + authorization);
        }

        int endIndex = authorization.indexOf(RestParamDefine.REST_DELIMITER, beginIndex);
        if (endIndex != -1) {
            accessKeyId = authorization.substring(beginIndex + RestParamDefine.REST_CREDENTIAL.length(), endIndex);
        } else {
            accessKeyId = authorization.substring(beginIndex + RestParamDefine.REST_CREDENTIAL.length());
        }

        //       2.check access key
        User user = userDao.getUserByAccessKeyID(accessKeyId);
        if (null == user) {
            throw new S3ServerException(S3Error.INVALID_ACCESSKEYID,
                    "Invalid accessKeyId. accessKeyId = " + accessKeyId);
        }

        //       3.check signature

        return user;
    }

    public String getObjectNameByURI(String url) throws S3ServerException {
        String decodeUrl;
        try {
            decodeUrl = URLDecoder.decode(url, "UTF-8");
        }catch (UnsupportedEncodingException e){
            throw new S3ServerException(S3Error.OBJECT_INVALID_KEY, "Invalid key. url = " + url);
        }

        int beginS3 = decodeUrl.indexOf(RestParamDefine.REST_S3, 0);
        if (beginS3 == -1) {
            throw new S3ServerException(S3Error.OBJECT_INVALID_KEY, "Invalid key. url = " + url);
        }

        int beginObject = decodeUrl.indexOf(RestParamDefine.REST_DELIMITER, beginS3+RestParamDefine.REST_S3.length()+1);
        if (beginObject == -1) {
            throw new S3ServerException(S3Error.OBJECT_INVALID_KEY, "Invalid key. url = " + url);
        }

        return decodeUrl.substring(beginObject+1);
    }

    public Range getRange(String rangeHeader){
        try {
            return new Range(rangeHeader);
        }catch (S3ServerException e){
            return null;
        }
    }

}
