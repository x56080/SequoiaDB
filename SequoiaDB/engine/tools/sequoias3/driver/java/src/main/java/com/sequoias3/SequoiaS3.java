package com.sequoias3;

import com.amazonaws.services.s3.model.AmazonS3Exception;
import com.sequoias3.model.CreateRegionRequest;
import com.sequoias3.model.GetRegionResult;
import com.sequoias3.model.ListRegionsResult;

public interface SequoiaS3 {

    /**
     * Create a region with regionName, all parameters use
     *      *  default values.
     *
     * @param regionName Set the region name
     * @throws AmazonS3Exception
     *   If any errors occurred  while processing the request.
     */
    void createRegion(String regionName) throws AmazonS3Exception;

    /**
     * Create a region with CreateRegionRequest.
     *
     * @param request A request with region configuration
     * @throws AmazonS3Exception
     *   If any errors occurred  while processing the request.
     */
    void createRegion(CreateRegionRequest request) throws AmazonS3Exception;

    /**
     * Delete a region and the region must be empty before it can be deleted.
     *
     * @param regionName Region name to be delete.
     * @throws AmazonS3Exception
     *   If any errors occurred  while processing the request.
     */
    void deleteRegion(String regionName) throws AmazonS3Exception;

    /**
     * Return the region configuration and a list of buckets that
     * stored in the region.
     *
     * @param regionName Region name
     * @return Region configuration and bucket list
     * @throws AmazonS3Exception
     *   If any errors occurred  while processing the request.
     */
    GetRegionResult getRegion(String regionName) throws AmazonS3Exception;

    /**
     *  Check whether the region exists.
     *
     * @param regionName Region name
     * @return true or false
     * @throws AmazonS3Exception
     *   If any errors occurred  while processing the request.
     */
    boolean headRegion(String regionName) throws AmazonS3Exception;

    /**
     * Returns a list of all regions.
     *
     * @return A list of buckets.
     * @throws AmazonS3Exception
     *   If any errors occurred  while processing the request.
     */
    ListRegionsResult listRegions() throws AmazonS3Exception;

    /**
     * Close the client.
     * @throws AmazonS3Exception
     */
    void shutdown() throws AmazonS3Exception;
}
