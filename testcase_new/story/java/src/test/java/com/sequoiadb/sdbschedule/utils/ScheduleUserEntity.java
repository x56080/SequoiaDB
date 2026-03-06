package com.sequoiadb.sdbschedule.utils;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;

public class ScheduleUserEntity {
    protected String name;
    protected String desc;
    protected String type;
    protected BSONObject content;
    protected String cron;
    protected boolean enable = true;

    public String getName() {
        return name;
    }

    public void setName( String name ) {
        this.name = name;
    }

    public String getDesc() {
        return desc;
    }

    public void setDesc( String desc ) {
        this.desc = desc;
    }

    public String getType() {
        return type;
    }

    public void setType( String type ) {
        this.type = type;
    }

    public BSONObject getContent() {
        return content;
    }

    public void setContent( BSONObject content ) {
        this.content = content;
    }

    public String getCron() {
        return cron;
    }

    public void setCron( String cron ) {
        this.cron = cron;
    }

    public boolean isEnable() {
        return enable;
    }

    public void setEnable( boolean enable ) {
        this.enable = enable;
    }

    public BSONObject toBSONObject() {
        BSONObject obj = new BasicBSONObject();
        obj.put( "name", name );
        obj.put( "desc", desc );
        obj.put( "type", type );
        obj.put( "content", content );
        obj.put( "cron", cron );
        obj.put( "enable", enable );
        return obj;
    }
}
