package com.sequoias3.testcommon.s3utils.bean;

import java.util.ArrayList;
import java.util.List;

import com.amazonaws.util.DateUtils;
import com.fasterxml.jackson.annotation.JsonIgnore;
import com.fasterxml.jackson.annotation.JsonProperty;
import com.fasterxml.jackson.dataformat.xml.annotation.JacksonXmlElementWrapper;
import com.fasterxml.jackson.dataformat.xml.annotation.JacksonXmlRootElement;

/**
 * Created by fanyu on 2019/1/21.
 */
@JacksonXmlRootElement(localName = "RegionConfiguration")
public class GetRegionResult {
    public static final String REGION_NAME = "Name";
    public static final String DATA_CS_SHARDINGTYPE = "DataCSShardingType";
    public static final String DATA_CL_SHARDINGTYPE = "DataCLShardingType";
    public static final String DATA_DOMAIN = "DataDomain";
    public static final String DATA_LOBPAGESIZE = "DataLobPageSize";
    public static final String DATA_REPLSIZE = "DataReplSize";
    public static final String META_DOMAIN = "MetaDomain";
    public static final String DATA_LOCATION = "DataLocation";
    public static final String META_LOCATION = "MetaLocation";
    public static final String META_HIS_LOCATION = "MetaHisLocation";
    public static final String DATA_CS_RANGE = "DataCSRange";

    private Region region;

    public GetRegionResult( ) {}

    public GetRegionResult( Region region ) {
        this.region = region;
    }

    @JsonProperty(REGION_NAME)
    private String name;

    @JsonIgnore
    private long createTime;

    @JsonProperty(DATA_CS_SHARDINGTYPE)
    private String dataCSShardingType = "";

    @JsonProperty(DATA_CL_SHARDINGTYPE)
    private String dataCLShardingType = "";

    @JsonProperty(DATA_DOMAIN)
    private String dataDomain = "";

    @JsonProperty(DATA_LOBPAGESIZE)
    private String dataLobPageSize = "";

    @JsonProperty(DATA_REPLSIZE)
    private String dataReplSize = "";

    @JsonProperty(META_DOMAIN)
    private String metaDomain = "";

    @JsonProperty(DATA_LOCATION)
    private String dataLocation = "";

    @JsonProperty(META_LOCATION)
    private String metaLocation = "";

    @JsonProperty(META_HIS_LOCATION)
    private String metaHisLocation = "";

    @JsonProperty(DATA_CS_RANGE)
    private Integer dataCSRange;

    @JacksonXmlElementWrapper(localName = "Buckets")
    @JsonProperty("Bucket")
    private List< Bucket > buckets;

    public Region getRegion() {
        if(dataCLShardingType == null){
            dataCLShardingType = "";
        }
        if(dataCSShardingType == null){
            dataCSShardingType = "";
        }
        if(dataDomain == null){
            dataDomain = "";
        }
        if(dataLobPageSize == null){
            dataLobPageSize = "";
        }
        if(dataLocation == null){
            dataLocation = "";
        }
        if(dataReplSize == null){
            dataReplSize = "";
        }
        if(metaDomain == null){
            metaDomain = "";
        }
        if(metaHisLocation == null){
            metaHisLocation = "";
        }
        if(metaLocation == null){
            metaLocation = "";
        }
        region = new Region(  ).withDataCLShardingType( dataCLShardingType )
                .withDataCSShardingType( dataCSShardingType )
                .withDataDomain( dataDomain )
                .withDataLobPageSize( dataLobPageSize )
                .withDataLocation( dataLocation )
                .withDataReplSize( dataReplSize )
                .withMetaDomain( metaDomain )
                .withMetaHisLocation( metaHisLocation )
                .withMetaLocation( metaLocation )
                .withDataCSRange( dataCSRange )
                .withName( name );
        return this.region;
    }

    public List< com.amazonaws.services.s3.model.Bucket > getBuckets() {
        List< com.amazonaws.services.s3.model.Bucket>  amazonBuckets = new
                ArrayList<>(  );
        if(buckets != null) {
            for ( Bucket bucket : buckets ) {
                com.amazonaws.services.s3.model.Bucket amazonBucket = new com
                        .amazonaws.services.s3.model.Bucket();
                amazonBucket.setCreationDate( DateUtils.parseISO8601Date
                        ( bucket.getFormatDate() ) );
                amazonBucket.setName( bucket.getBucketName() );
                amazonBuckets.add( amazonBucket );
            }
        }
        return amazonBuckets;
    }

    public void setBuckets( List< Bucket > buckets ) {
        this.buckets = buckets;
    }
}

class Bucket {
    public static final String BUCKET_NAME = "Name";
    public static final String BUCKET_CREATETIME = "CreationDate";
    @JsonProperty(BUCKET_NAME)
    private String bucketName;

    @JsonProperty(BUCKET_CREATETIME)
    private String formatDate;


    public void setFormatDate(String creationDate){
        this.formatDate = creationDate;
    }

    public String getFormatDate(){
        return this.formatDate;
    }


    public void setBucketName(String bucketName){
        this.bucketName = bucketName;
    }

    public String getBucketName(){
        return this.bucketName;
    }
}
