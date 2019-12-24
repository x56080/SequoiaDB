package com.sequoias3.region;

import com.amazonaws.services.s3.model.Bucket;
import com.fasterxml.jackson.annotation.JsonProperty;
import com.fasterxml.jackson.dataformat.xml.annotation.JacksonXmlElementWrapper;
import com.fasterxml.jackson.dataformat.xml.annotation.JacksonXmlRootElement;

import java.util.List;

/**
 * Created by fanyu on 2019/1/21.
 */
@JacksonXmlRootElement(localName = "RegionConfiguration") public class GetRegionResult {
    private Region region;
    @JacksonXmlElementWrapper(localName = "Buckets") @JsonProperty("Bucket") private List<Bucket> buckets;

    public GetRegionResult( Region region ) {
        this.region = region;
    }

    public Region getRegion() {
        return region;
    }

    public void setRegion( Region region ) {
        this.region = region;
    }

    public List<Bucket> getBuckets() {
        return buckets;
    }

    public void setBuckets( List<Bucket> buckets ) {
        this.buckets = buckets;
    }
}