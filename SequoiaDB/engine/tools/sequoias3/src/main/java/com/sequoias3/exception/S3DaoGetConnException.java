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

   Source File Name = S3DaoGetConnException.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoias3.exception;

public class S3DaoGetConnException extends S3ServerException {
    private static final long serialVersionUID = -5238230425202542393L;

    public S3DaoGetConnException(String message) {
        super(S3Error.DAO_GETCONN_ERROR, message);
    }

    public S3DaoGetConnException(String message, Throwable e) {
        super(S3Error.DAO_GETCONN_ERROR, message, e);
    }
}