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

   Source File Name = NodeController.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          26/01/2026  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoiadb.schedule.controller;

import com.sequoiadb.schedule.service.NodeService;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RequestParam;
import org.springframework.web.bind.annotation.RestController;

import javax.servlet.http.HttpServletResponse;
import java.util.Collections;
import java.util.List;

@RestController
@RequestMapping("/api")
public class NodeController {

    @Autowired
    private NodeService nodeService;

    @GetMapping("/v1/nodes")
    public List<BSONObject> listNode(
            @RequestParam(value = "skip", required = false, defaultValue = "0") long skip,
            @RequestParam(value = "limit", required = false, defaultValue = "1000") long limit,
            @RequestParam(value = "filter", required = false) BSONObject filter,
            @RequestParam(value = "orderby", required = false) BSONObject orderBy,
            HttpServletResponse response) throws Exception {
        if (null == filter) {
            filter = new BasicBSONObject();
        }
        long scheduleCount = nodeService.getNodeCount(filter);
        response.setHeader("x-record-count", String.valueOf(scheduleCount));
        if (scheduleCount <= 0) {
            return Collections.emptyList();
        }
        return nodeService.getNodeList(filter, orderBy, skip, limit);
    }
}
