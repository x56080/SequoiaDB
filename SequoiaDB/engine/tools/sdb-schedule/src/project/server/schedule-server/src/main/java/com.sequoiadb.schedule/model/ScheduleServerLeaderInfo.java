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

   Source File Name = ScheduleServerLeaderInfo.java

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
import org.bson.BSONObject;
import org.bson.BasicBSONObject;

import java.util.Objects;

public class ScheduleServerLeaderInfo {
    private String serverType;
    private String nodeUrl;
    private long leaseNum;
    private long updateTime;

    public ScheduleServerLeaderInfo(BSONObject res) {
        this.serverType = BsonUtils.getStringChecked(res,
                FieldName.ScheduleServerElection.SERVER_TYPE);
        this.nodeUrl = BsonUtils.getStringChecked(res, FieldName.ScheduleServerElection.NODE_URL);
        this.leaseNum = BsonUtils.getNumberChecked(res, FieldName.ScheduleServerElection.LEASE_NUM)
                .longValue();
        this.updateTime = BsonUtils
                .getNumberChecked(res, FieldName.ScheduleServerElection.UPDATE_TIME).longValue();
    }

    public ScheduleServerLeaderInfo(String nodeUrl, int leaseNum, long updateTime) {
        this.serverType = "schedule-server";
        this.nodeUrl = nodeUrl;
        this.leaseNum = leaseNum;
        this.updateTime = updateTime;
    }

    public ScheduleServerLeaderInfo() {
        this.serverType = "schedule-server";
    }

    public String getServerType() {
        return serverType;
    }

    public void setServerType(String serverType) {
        this.serverType = serverType;
    }

    public String getNodeUrl() {
        return nodeUrl;
    }

    public void setNodeUrl(String nodeUrl) {
        this.nodeUrl = nodeUrl;
    }

    public long getLeaseNum() {
        return leaseNum;
    }

    public void setLeaseNum(long leaseNum) {
        this.leaseNum = leaseNum;
    }

    public long getUpdateTime() {
        return updateTime;
    }

    public void setUpdateTime(long updateTime) {
        this.updateTime = updateTime;
    }

    @Override
    public String toString() {
        return "LockServerLeaderInfo{" + "serverType='" + serverType + '\'' + ", nodeUrl='"
                + nodeUrl + '\'' + ", leaseNum=" + leaseNum + ", updateTime=" + updateTime + '}';
    }

    public BSONObject toBson() {
        BSONObject bson = new BasicBSONObject();
        bson.put(FieldName.ScheduleServerElection.SERVER_TYPE, serverType);
        bson.put(FieldName.ScheduleServerElection.NODE_URL, nodeUrl);
        bson.put(FieldName.ScheduleServerElection.LEASE_NUM, leaseNum);
        bson.put(FieldName.ScheduleServerElection.UPDATE_TIME, updateTime);
        return bson;
    }

    @Override
    public boolean equals(Object o) {
        if (this == o)
            return true;
        if (o == null || getClass() != o.getClass())
            return false;
        ScheduleServerLeaderInfo that = (ScheduleServerLeaderInfo) o;
        return leaseNum == that.leaseNum && updateTime == that.updateTime
                && Objects.equals(serverType, that.serverType)
                && Objects.equals(nodeUrl, that.nodeUrl);
    }

    @Override
    public int hashCode() {
        return Objects.hash(serverType, nodeUrl, leaseNum, updateTime);
    }
}
