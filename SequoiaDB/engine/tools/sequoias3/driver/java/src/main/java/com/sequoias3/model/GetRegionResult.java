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

   Source File Name = GetRegionResult.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoias3.model;

import java.util.List;

public class GetRegionResult {
    private Region region;

    private List<String> Buckets;

    public GetRegionResult(){}

    /**
     *
     * @param region set region configuration
     */
    public void setRegion(Region region) {
        this.region = region;
    }

    /**
     *
     * @return get region configuration
     */
    public Region getRegion() {
        return region;
    }

    /**
     * Get the summary of buckets in the region.
     *
     * @return
     *   A list of buckets.
     */
    public List<String> getBuckets() {
        return Buckets;
    }

    /**
     *
     * @param buckets Set bucket list
     */
    public void setBuckets(List<String> buckets) {
        Buckets = buckets;
    }
}
