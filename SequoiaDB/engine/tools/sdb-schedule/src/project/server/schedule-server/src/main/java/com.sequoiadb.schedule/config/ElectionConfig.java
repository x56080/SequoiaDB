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

   Source File Name = ElectionConfig.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          26/01/2026  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoiadb.schedule.config;

import org.springframework.context.annotation.Configuration;

import javax.annotation.PostConstruct;

@Configuration
public class ElectionConfig {
    private int leaderDuration = 60; // seconds

    private int renewInterval = 15; // seconds

    @PostConstruct
    public void check() {
        if (leaderDuration <= 0) {
            throw new IllegalArgumentException(
                    "leaderDuration must be greater than 0, leaderDuration:" + leaderDuration);
        }
        if (renewInterval <= 0) {
            throw new IllegalArgumentException(
                    "renewInterval must be greater than 0, renewInterval:" + renewInterval);
        }
        if (renewInterval >= leaderDuration) {
            throw new IllegalArgumentException(
                    "renewInterval must be less than leaderDuration, renewInterval:" + renewInterval
                            + ", leaderDuration:" + leaderDuration);
        }
    }

    public int getLeaderDuration() {
        return leaderDuration;
    }

    public void setLeaderDuration(int leaderDuration) {
        this.leaderDuration = leaderDuration;
    }

    public int getRenewInterval() {
        return renewInterval;
    }

    public void setRenewInterval(int renewInterval) {
        this.renewInterval = renewInterval;
    }

    @Override
    public String toString() {
        return "ElectionConfig{" + "leaderDuration=" + leaderDuration + ", renewInterval="
                + renewInterval + "}";
    }
}
