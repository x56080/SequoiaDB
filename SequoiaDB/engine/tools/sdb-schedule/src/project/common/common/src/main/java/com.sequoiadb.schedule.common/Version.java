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

   Source File Name = Version.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          26/01/2026  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoiadb.schedule.common;

import java.util.Objects;

public class Version {
    private final int version;
    private final int subVersion;
    private final int fixVersion;

    public Version(int version, int subVersion, int fixVersion) {
        this.version = version;
        this.subVersion = subVersion;
        this.fixVersion = fixVersion;
    }

    public int getVersion() {
        return version;
    }

    public int getSubVersion() {
        return subVersion;
    }

    public int getFixVersion() {
        return fixVersion;
    }

    public String toString() {
        StringBuilder strBuilder = new StringBuilder();
        return strBuilder.append(this.version).append(".").append(this.subVersion).append(".").append(this.fixVersion).toString();
    }

    public boolean equals(Object anObject) {
        if (this == anObject) {
            return true;
        } else if (!(anObject instanceof Version)) {
            return false;
        } else {
            Version version = (Version)anObject;
            return this.version == version.version && this.subVersion == version.subVersion && this.fixVersion == version.fixVersion;
        }
    }

    public int hashCode() {
        return Objects.hash(new Object[]{this.version, this.subVersion, this.fixVersion});
    }

    public int compareTo(Version version) {
        int versionDiff = this.version - version.version;
        if (versionDiff != 0) {
            return versionDiff;
        } else {
            int subVersionDiff = this.subVersion - version.subVersion;
            return subVersionDiff != 0 ? subVersionDiff : this.fixVersion - version.fixVersion;
        }
    }

}
