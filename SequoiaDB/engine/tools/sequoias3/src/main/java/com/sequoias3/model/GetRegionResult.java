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

import com.fasterxml.jackson.annotation.JsonProperty;
import com.fasterxml.jackson.dataformat.xml.annotation.JacksonXmlElementWrapper;
import com.fasterxml.jackson.dataformat.xml.annotation.JacksonXmlRootElement;
import com.sequoias3.core.Bucket;
import com.sequoias3.core.Region;

import java.util.List;

@JacksonXmlRootElement(localName = "RegionConfiguration")
public class GetRegionResult extends Region{
    public GetRegionResult(Region region){
        super(region);
    }

//    public GetRegionResult(){}

    @JacksonXmlElementWrapper(localName = "Buckets")
    @JsonProperty("Bucket")
    List<Bucket> Buckets;


    public void setBuckets(List<Bucket> buckets) {
        Buckets = buckets;
    }
}
