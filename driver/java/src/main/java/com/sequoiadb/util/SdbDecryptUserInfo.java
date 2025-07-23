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

   Source File Name = SdbDecryptUserInfo.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoiadb.util;

public class SdbDecryptUserInfo {
    private String userName;
    private String clusterName;
    private String passwd;

    /**
     * Get the cluster name
     *
     * @return cluster name
     */
    public String getClusterName() {
        return clusterName;
    }

    void setClusterName(String clusterName) {
        this.clusterName = clusterName;
    }

    /**
     * Get the user name
     *
     * @return user name
     */
    public String getUserName() {
        return userName;
    }

    void setUserName(String userName) {
        this.userName = userName;
    }

    /**
     * Get the password
     *
     * @return password
     */
    public String getPasswd() {
        return passwd;
    }

    void setPasswd(String passwd) {
        this.passwd = passwd;
    }

    @Override
    public String toString() {
        StringBuilder sb = new StringBuilder();
        sb.append("user:").append(userName).append(",cluster:").append(clusterName)
                .append(",passwd:").append(passwd);

        return sb.toString();
    }
}
