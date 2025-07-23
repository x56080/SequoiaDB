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

   Source File Name = Resource.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoiadb.test.rbac;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;

import java.util.Objects;

public class Resource {
    private String cs;
    private String cl;
    private Boolean cluster;

    public Resource() {
    }

    public Resource(String cs, String cl, Boolean cluster) {
        this.cs = cs;
        this.cl = cl;
        this.cluster = cluster;
    }

    public BSONObject toBson() {
        BasicBSONObject bson = new BasicBSONObject();
        if (cs != null) {
            bson.put("cs", cs);
        }
        if (cl != null) {
            bson.put("cl", cl);
        }
        if (cluster != null) {
            bson.put("Cluster", cluster);
        }
        return bson;
    }

    public String getCs() {
        return cs;
    }

    public void setCs(String cs) {
        this.cs = cs;
    }

    public String getCl() {
        return cl;
    }

    public void setCl(String cl) {
        this.cl = cl;
    }

    public Boolean getCluster() {
        return cluster;
    }

    public void setCluster(Boolean cluster) {
        this.cluster = cluster;
    }

    @Override
    public boolean equals(Object o) {
        if (this == o) return true;
        if (o == null || getClass() != o.getClass()) return false;
        Resource resource = (Resource) o;
        return Objects.equals(cs, resource.cs)
                && Objects.equals(cl, resource.cl)
                && Objects.equals(cluster, resource.cluster);
    }

    @Override
    public int hashCode() {
        return Objects.hash(cs, cl, cluster);
    }

    @Override
    public String toString() {
        return "Resource{" +
                "cs='" + cs + '\'' +
                ", cl='" + cl + '\'' +
                ", cluster=" + cluster +
                '}';
    }
}
