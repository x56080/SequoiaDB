package com.sequoias3.model;

import com.amazonaws.services.s3.model.Bucket;

import java.util.List;

public class GetRegionResult {
    private Region region;

    private List<Bucket> Buckets;

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
    public List<Bucket> getBuckets() {
        return Buckets;
    }

    /**
     *
     * @param buckets Set bucket list
     */
    public void setBuckets(List<Bucket> buckets) {
        Buckets = buckets;
    }
}
