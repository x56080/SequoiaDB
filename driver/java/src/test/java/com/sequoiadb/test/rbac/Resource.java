package com.sequoiadb.test.rbac;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;

import java.util.Objects;

public class Resource {
    private String cs;
    private String cl;

    public Resource() {
    }

    public Resource(String cs, String cl) {
        this.cs = cs;
        this.cl = cl;
    }

    public BSONObject toBson() {
        BasicBSONObject bson = new BasicBSONObject();
        bson.put("cs", cs);
        bson.put("cl", cl);
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

    @Override
    public boolean equals(Object o) {
        if (this == o) return true;
        if (o == null || getClass() != o.getClass()) return false;
        Resource resource = (Resource) o;
        return Objects.equals(cs, resource.cs) && Objects.equals(cl, resource.cl);
    }

    @Override
    public int hashCode() {
        return Objects.hash(cs, cl);
    }

    @Override
    public String toString() {
        return "Resource{" +
                "cs='" + cs + '\'' +
                ", cl='" + cl + '\'' +
                '}';
    }
}
