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

   Source File Name = ServerNodeEntity.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          26/01/2026  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoiadb.schedule.model;

import com.sequoiadb.schedule.common.BsonUtils;
import com.sequoiadb.schedule.common.FieldName;
import lombok.Data;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;

@Data
public class ServerNodeEntity {
    private String hostName;
    private int port;
    private String ipAddress;
    private String status;
    private long lastHeartTime;
    private int leaseNum;

    public BSONObject toBSONObject() {
        BSONObject obj = new BasicBSONObject();
        obj.put(FieldName.ServerNode.FIELD_HOSTNAME, hostName);
        obj.put(FieldName.ServerNode.FIELD_PORT, port);
        obj.put(FieldName.ServerNode.FIELD_IPADDR, ipAddress);
        obj.put(FieldName.ServerNode.FIELD_STATUS, status);
        obj.put(FieldName.ServerNode.FIELD_LAST_HEART_TIME, lastHeartTime);
        obj.put(FieldName.ServerNode.FIELD_LEASE_NUM, leaseNum);
        return obj;
    }

    public static ServerNodeEntity fromBSONObject(BSONObject bson) {
        ServerNodeEntity entity = new ServerNodeEntity();
        entity.setHostName(BsonUtils.getStringChecked(bson, FieldName.ServerNode.FIELD_HOSTNAME));
        entity.setPort(
                BsonUtils.getNumberChecked(bson, FieldName.ServerNode.FIELD_PORT).intValue());
        entity.setIpAddress(BsonUtils.getStringChecked(bson, FieldName.ServerNode.FIELD_IPADDR));
        entity.setStatus(BsonUtils.getStringChecked(bson, FieldName.ServerNode.FIELD_STATUS));
        entity.setLastHeartTime(BsonUtils
                .getNumberChecked(bson, FieldName.ServerNode.FIELD_LAST_HEART_TIME).longValue());
        entity.setLeaseNum(
                BsonUtils.getNumberChecked(bson, FieldName.ServerNode.FIELD_LEASE_NUM).intValue());
        return entity;
    }

    public String getUrl() {
        return hostName + ":" + port;
    }

    public void incLeaseNum() {
        this.leaseNum += 1;
    }

}
