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

   Source File Name = ScheduleHelpGenerator.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          26/01/2026  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoiadb.schedule.tools.common;

import com.sequoiadb.schedule.tools.exception.ScheduleBaseExitCode;
import com.sequoiadb.schedule.tools.exception.ScheduleToolsException;
import org.apache.commons.cli.Option;
import org.apache.commons.cli.Option.Builder;

import java.util.ArrayList;
import java.util.List;

public class ScheduleHelpGenerator {
    List<String> optionList = new ArrayList<>();
    List<String> descslist = new ArrayList<>();
    List<Boolean> isHideList = new ArrayList<>();
    private int maxOptsLen = 0;

    public ScheduleHelpGenerator() {

    }

    public ScheduleHelpGenerator addOptHelp(String opt, String desc) {
        return addOptHelp(opt, desc, false);
    }

    public ScheduleHelpGenerator addOptHelp(String opt, String desc, boolean isHide) {
        opt = " " + opt;
        String[] descsArr = desc.split("\n");
        optionList.add(opt);
        if (opt.length() >= maxOptsLen) {
            maxOptsLen = opt.length() + 2;
        }
        descslist.add(descsArr[0]);
        isHideList.add(isHide);

        for (int i = 1; i < descsArr.length; i++) {
            optionList.add("");
            descslist.add(descsArr[i]);
            isHideList.add(isHide);
        }
        return this;
    }

    public void printHelp(boolean isPrintHideOpt) {
        System.out.println("Command options:");
        for (int i = 0; i < optionList.size(); i++) {
            if (isPrintHideOpt == false && isHideList.get(i) == true) {
                continue;
            }
            System.out.print(optionList.get(i));
            ScheduleCommon.printSpace(maxOptsLen - optionList.get(i).length());
            System.out.println(descslist.get(i));
        }
        System.out.println();
    }

    public Option createDOption(String shortOpt, String desc) throws ScheduleToolsException {
        return createOpt(shortOpt, null, desc, false, true, false, true);
    }

    public Option createOpt(String shortOpt, String longOpt, String desc, boolean isRequire,
            boolean hasArg, boolean isHide) throws ScheduleToolsException {
        return createOpt(shortOpt, longOpt, desc, isRequire, hasArg, false, isHide, false);
    }

    public Option createOpt(String shortOpt, String longOpt, String desc, boolean isRequire,
            boolean hasArg, boolean isHide, boolean hasArgs) throws ScheduleToolsException {
        return createOpt(shortOpt, longOpt, desc, isRequire, hasArg, false, isHide, hasArgs);
    }

    public Option createOpt(String shortOpt, String longOpt, String desc, boolean isRequire,
            boolean hasArg, boolean optionalArg, boolean isHide, boolean hasArgs)
            throws ScheduleToolsException {
        Builder opb;
        String opt;
        if (longOpt != null && shortOpt != null) {
            opb = Option.builder(shortOpt).longOpt(longOpt).desc(desc);
            opt = "-" + shortOpt + " [ --" + longOpt + " ]";
        }
        else if (longOpt == null) {
            opb = Option.builder(shortOpt).desc(desc);
            opt = "-" + shortOpt;
        }
        else if (shortOpt == null) {
            opb = Option.builder().longOpt(longOpt).desc(desc);
            opt = "--" + longOpt;
        }
        else {
            throw new ScheduleToolsException(
                    "Inner Error,failed to generate help msg,longOpt is null,shortOpt is null",
                    ScheduleBaseExitCode.SYSTEM_ERROR);
        }

        if (hasArgs) {
            opb.hasArgs();
            opt = opt + "<key>=<value>";
        }
        else if (hasArg) {
            opb.hasArg(true);
            opb.optionalArg(optionalArg);
            if (!optionalArg) {
                opt = opt + " arg";
            }
        }

        if (isRequire) {
            opb.required(true);
        }
        addOptHelp(opt, desc, isHide);
        return opb.build();

    }
}
