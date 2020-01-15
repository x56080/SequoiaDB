package com.sequoias3.testcommon.s3utils.bean;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;

import com.fasterxml.jackson.annotation.JsonIgnore;
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

    @JsonIgnore
    private long createTime;

    @JsonProperty(DATA_CS_SHARDINGTYPE)
    private String dataCSShardingType;

    @JsonProperty(DATA_CL_SHARDINGTYPE)
    private String dataCLShardingType;

    @JsonProperty(DATA_DOMAIN)
    private String dataDomain;

    @JsonProperty(DATA_LOBPAGESIZE)
    private String dataLobPageSize;

    @JsonProperty(DATA_REPLSIZE)
    private String dataReplSize;

    @JsonProperty(META_DOMAIN)
    private String metaDomain;

    @JsonProperty(DATA_LOCATION)
    private String dataLocation;

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
        this.createTime = region.getCreateTime();
        this.dataCSShardingType = region.getDataCSShardingType();
        this.dataCLShardingType = region.getDataCLShardingType();
        this.dataDomain = region.getDataDomain();
        this.dataLobPageSize = region.getDataLobPageSize();
        this.dataReplSize = region.getDataReplSize();
        this.metaDomain = region.getMetaDomain();
        this.dataLocation = region.getDataLocation();
        this.metaLocation = region.getMetaLocation();
        this.metaHisLocation = region.getMetaHisLocation();
        this.dataCSRange = region.getDataCSRange();
    }

    public Region withName( String name ) {
        this.name = name;
        return this;
    }

    public String getName() {
        return name;
    }

    public long getCreateTime() {
        return createTime;
    }

    public void setCreateTime( long createTime ) {
        this.createTime = createTime;
    }

    public Region withDataCSShardingType( String dataCSShardingType ) {
        this.dataCSShardingType = dataCSShardingType;
        return this;
    }

    public String getDataCSShardingType() {
        return dataCSShardingType;
    }

    public Region withDataCLShardingType( String dataCLShardingType ) {
        this.dataCLShardingType = dataCLShardingType;
        return this;
    }

    public String getDataCLShardingType() {
        return dataCLShardingType;
    }

    public Region withDataDomain( String dataDomain ) {
        this.dataDomain = dataDomain;
        return this;
    }

    public String getDataDomain() {
        return dataDomain;
    }

    public Region withDataLobPageSize( String dataLobPageSize ) {
        this.dataLobPageSize = dataLobPageSize;
        return this;
    }

    public String getDataLobPageSize() {
        return dataLobPageSize;
    }

    public Region withDataReplSize( String dataReplSize ) {
        this.dataReplSize = dataReplSize;
        return this;
    }

    public String getDataReplSize() {
        return dataReplSize;
    }

    public Region withMetaDomain( String metaDomain ) {
        this.metaDomain = metaDomain;
        return this;
    }

    public String getMetaDomain() {
        return metaDomain;
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

    public BSONObject toBson() {
        BSONObject newRegion = new BasicBSONObject();
        newRegion.put( Region.REGION_NAME, this.name );
        newRegion.put( Region.REGION_CREATERTIME, this.createTime );
        newRegion.put( Region.DATA_CS_SHARDINGTYPE, this.dataCSShardingType );
        newRegion.put( Region.DATA_CL_SHARDINGTYPE, this.dataCLShardingType );
        newRegion.put( Region.DATA_DOMAIN, this.dataDomain );
        newRegion.put( Region.DATA_LOBPAGESIZE, this.dataLobPageSize );
        newRegion.put( Region.DATA_REPLSIZE, this.dataReplSize );
        newRegion.put( Region.META_DOMAIN, this.metaDomain );
        newRegion.put( Region.DATA_LOCATION, this.dataLocation );
        newRegion.put( Region.META_LOCATION, this.metaLocation );
        newRegion.put( Region.META_HIS_LOCATION, this.metaHisLocation );
        newRegion.put( Region.DATA_CS_RANGE, this.dataCSRange );
        return newRegion;
    }

    public String toString() {
        StringBuffer buffer = new StringBuffer();
        buffer.append( "[" + Region.REGION_NAME + "=" + this.name );
        buffer.append( "," + Region.DATA_CS_SHARDINGTYPE + "=" +
                this.dataCSShardingType );
        buffer.append( "," + Region.DATA_CL_SHARDINGTYPE + "=" +
                this.dataCLShardingType );
        buffer.append( "," + Region.DATA_DOMAIN + "=" + this.dataDomain );
        buffer.append(
                "," + Region.DATA_LOBPAGESIZE + "=" + this.dataLobPageSize );
        buffer.append( "," + Region.DATA_REPLSIZE + "=" + this.dataReplSize );
        buffer.append( "," + Region.META_DOMAIN + "=" + this.metaDomain );
        buffer.append( "," + Region.DATA_LOCATION + "=" + this.dataLocation );
        buffer.append( "," + Region.DATA_CS_RANGE + "=" + this.dataCSRange );
        buffer.append( "," + Region.META_LOCATION + "=" + this.metaLocation );
        buffer.append(
                "," + Region.META_HIS_LOCATION + "=" + this.metaHisLocation +
                        "]" );
        return buffer.toString();
    }
}
