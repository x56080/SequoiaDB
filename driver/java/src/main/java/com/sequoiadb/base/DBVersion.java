/*
 * Copyright 2023 SequoiaDB Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.sequoiadb.base;

import java.util.Objects;

/**
 * Version information of SequoiaDB.
 */

public class DBVersion implements Comparable<DBVersion> {
    private final int version;
    private final int subVersion;
    private final int fixVersion;

    DBVersion(int version, int subVersion, int fixVersion) {
        this.version = version;
        this.subVersion = subVersion;
        this.fixVersion = fixVersion;
    }

    /**
     * Get framework version number.
     *
     * @return The number of framework version.
     */
    public int getVersion() {
        return version;
    }

    /**
     * Get function version number.
     *
     * @return The number of function version.
     */
    public int getSubVersion() {
        return subVersion;
    }

    /**
     * Get fix version number.
     *
     * @return The number of fix version.
     */
    public int getFixVersion() {
        return fixVersion;
    }

    /**
     * Get the full version number in string format, for example: <code> "x.x.x" </code>
     *
     * @return The string of full version number.
     */
    @Override
    public String toString() {
        StringBuilder strBuilder = new StringBuilder();
        return strBuilder.append(version).append(".")
                         .append(subVersion).append(".")
                         .append(fixVersion).toString();
    }

    @Override
    public boolean equals(Object anObject) {
        if (this == anObject) return true;
        if (!(anObject instanceof DBVersion)) return false;

        DBVersion dbVersion = (DBVersion) anObject;
        return version == dbVersion.version
               && subVersion == dbVersion.subVersion
               && fixVersion == dbVersion.fixVersion;
    }

    @Override
    public int hashCode() {
        return Objects.hash(version, subVersion, fixVersion);
    }

    @Override
    public int compareTo(DBVersion dbVersion) {
        int versionDiff = this.version - dbVersion.version;
        if(versionDiff != 0) return versionDiff;

        int subVersionDiff = this.subVersion - dbVersion.subVersion;
        if(subVersionDiff != 0) return subVersionDiff;

        return this.fixVersion - dbVersion.fixVersion;
    }
}
