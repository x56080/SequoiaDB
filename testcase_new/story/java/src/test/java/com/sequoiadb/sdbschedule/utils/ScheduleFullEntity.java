package com.sequoiadb.sdbschedule.utils;

import org.bson.BSONObject;

public class ScheduleFullEntity extends ScheduleUserEntity {
    private String id;
    private long createTime;
    private long updateTime;

    public String getId() {
        return id;
    }

    public void setId( String id ) {
        this.id = id;
    }

    public long getCreateTime() {
        return createTime;
    }

    public void setCreateTime( long createTime ) {
        this.createTime = createTime;
    }

    public long getUpdateTime() {
        return updateTime;
    }

    public void setUpdateTime( long updateTime ) {
        this.updateTime = updateTime;
    }

    public static ScheduleFullEntity fromBSONObject( BSONObject obj ) {
        ScheduleFullEntity entity = new ScheduleFullEntity();
        entity.setId( ( String ) obj.get( "id" ) );
        entity.setName( ( String ) obj.get( "name" ) );
        entity.setDesc( ( String ) obj.get( "desc" ) );
        entity.setType( ( String ) obj.get( "type" ) );
        entity.setContent( ( BSONObject ) obj.get( "content" ) );
        entity.setCron( ( String ) obj.get( "cron" ) );
        entity.setEnable( ( Boolean ) obj.get( "enable" ) );
        entity.setCreateTime( ( Long ) obj.get( "createTime" ) );
        entity.setUpdateTime( ( Long ) obj.get( "updateTime" ) );
        return entity;
    }

}
