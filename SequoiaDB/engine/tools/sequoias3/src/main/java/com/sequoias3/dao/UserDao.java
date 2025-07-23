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

   Source File Name = UserDao.java

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

import com.sequoias3.model.Owner;
import com.sequoias3.core.User;
import com.sequoias3.exception.S3ServerException;

public interface UserDao {
    void insertUser(User user) throws S3ServerException;

    void deleteUser(String userName) throws S3ServerException;

    void updateUserKeys(String userName, String accessKeyId, String secretAccessKey)
            throws S3ServerException;

    User getUserByName(String userName) throws S3ServerException;

    User getUserByAccessKeyID(String accessKeyID) throws S3ServerException;

    Owner getOwnerByUserID(long userId) throws S3ServerException;
}
