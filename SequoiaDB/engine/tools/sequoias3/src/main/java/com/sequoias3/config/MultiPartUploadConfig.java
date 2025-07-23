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

   Source File Name = MultiPartUploadConfig.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoias3.config;

import org.springframework.boot.context.properties.ConfigurationProperties;
import org.springframework.stereotype.Component;

@Component
@ConfigurationProperties(prefix = "sdbs3.multipartupload")
public class MultiPartUploadConfig {
    private boolean partlistinuse = true;
    private boolean partsizelimit = true;
    private int     incompletelifecycle = 3;
    private int     completereservetime = 24 * 60;

    public void setPartlistinuse(boolean partlistinuse) {
        this.partlistinuse = partlistinuse;
    }

    public boolean isPartlistinuse() {
        return partlistinuse;
    }

    public void setPartsizelimit(boolean partsizelimit) {
        this.partsizelimit = partsizelimit;
    }

    public boolean isPartSizeLimit(){
        return this.partsizelimit;
    }

    public void setIncompletelifecycle(int incompletelifecycle) {
        this.incompletelifecycle = incompletelifecycle;
    }

    public int getIncompletelifecycle() {
        return incompletelifecycle;
    }

    public void setCompletereservetime(int completereservetime) {
        this.completereservetime = completereservetime;
    }

    public int getCompletereservetime() {
        return completereservetime;
    }
}
