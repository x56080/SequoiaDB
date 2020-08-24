package com.sequoias3.testcommon.s3utils.bean;

import com.fasterxml.jackson.annotation.JsonProperty;
import com.fasterxml.jackson.dataformat.xml.annotation.JacksonXmlRootElement;

@JacksonXmlRootElement(localName = "RegionConfiguration")
public class Region {
    public static final String REGION_NAME = "Name";
    public static final String REGION_CREATERTIME = "CreateTime";
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

    @JsonProperty(REGION_NAME)
    private String name;

    @JsonProperty(DATA_CS_SHARDINGTYPE)
    private String dataCSShardingType;

    @JsonProperty(DATA_CL_SHARDINGTYPE)
    private String dataCLShardingType;

    @JsonProperty(DATA_DOMAIN)
    private String dataDomain;

    @JsonProperty(DATA_LOBPAGESIZE)
    private String dataLobPageSize;

    @JsonProperty(META_DOMAIN)
    private String metaDomain;

    @JsonProperty(DATA_LOCATION)
    private String dataLocation;

    @JsonProperty(DATA_REPLSIZE)
    private String dataReplSize;

    @JsonProperty(META_LOCATION)
    private String metaLocation;

    @JsonProperty(META_HIS_LOCATION)
    private String metaHisLocation;

    @JsonProperty(DATA_CS_RANGE)
    private Integer dataCSRange = 1;

    public Region() {
    }

    public Region( Region region ) {
        this.name = region.getName();
        this.dataLocation = region.getDataLocation();
        this.metaLocation = region.getMetaLocation();
        this.metaHisLocation = region.getMetaHisLocation();
    }

    public Region withName( String name ) {
        this.name = name;
        return this;
    }

    public String getName() {
        return name;
    }

    public Region withDataCSShardingType( String dataCSShardingType ) {
        this.dataCSShardingType = dataCSShardingType;
        return this;
    }

    public Region withDataCLShardingType( String dataCLShardingType ) {
        this.dataCLShardingType = dataCLShardingType;
        return this;
    }

    public Region withDataDomain( String dataDomain ) {
        this.dataDomain = dataDomain;
        return this;
    }

    public Region withDataLobPageSize( String dataLobPageSize ) {
        this.dataLobPageSize = dataLobPageSize;
        return this;
    }

    public Region withDataReplSize( String dataReplSize ) {
        this.dataReplSize = dataReplSize;
        return this;
    }

    public Region withMetaDomain( String metaDomain ) {
        this.metaDomain = metaDomain;
        return this;
    }

    public Region withDataLocation( String dataLocation ) {
        this.dataLocation = dataLocation;
        return this;
    }

    public String getDataLocation() {
        return dataLocation;
    }

    public Region withMetaLocation( String metaLocation ) {
        this.metaLocation = metaLocation;
        return this;
    }

    public String getMetaLocation() {
        return metaLocation;
    }

    public Region withMetaHisLocation( String metaHisLocation ) {
        this.metaHisLocation = metaHisLocation;
        return this;
    }

    public String getMetaHisLocation() {
        return metaHisLocation;
    }

    public Integer getDataCSRange() {
        return dataCSRange;
    }

    public Region withDataCSRange( Integer dataCSRange ) {
        this.dataCSRange = dataCSRange;
        return this;
    }
}
