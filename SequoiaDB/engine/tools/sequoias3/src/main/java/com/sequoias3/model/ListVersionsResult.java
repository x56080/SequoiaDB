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

   Source File Name = ListVersionsResult.java

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

import com.fasterxml.jackson.annotation.JsonInclude;
import com.fasterxml.jackson.annotation.JsonProperty;
import com.fasterxml.jackson.dataformat.xml.annotation.JacksonXmlElementWrapper;
import com.fasterxml.jackson.dataformat.xml.annotation.JacksonXmlRootElement;
import com.sequoias3.exception.S3Error;
import com.sequoias3.exception.S3ServerException;

import java.io.UnsupportedEncodingException;
import java.net.URLEncoder;
import java.util.LinkedHashSet;

@JacksonXmlRootElement(localName = "ListVersionsResult")
@JsonInclude(value = JsonInclude.Include.NON_NULL)
public class ListVersionsResult {
    @JsonProperty("Name")
    private String name;

    @JsonProperty("Prefix")
    private String prefix;

    @JsonProperty("Delimiter")
    private String delimiter;

    @JsonProperty("KeyMarker")
    private String keyMarker;

    @JsonProperty("VersionIdMarker")
    private String versionIdMarker;

    @JsonProperty("MaxKeys")
    private int    maxKeys;

    @JsonProperty("EncodingType")
    private String encodingType;

    @JsonProperty("IsTruncated")
    private Boolean isTruncated = false;

    @JsonProperty("NextKeyMarker")
    private String nextKeyMarker;

    @JsonProperty("NextVersionIdMarker")
    private String nextVersionIdMarker;

    @JacksonXmlElementWrapper(localName = "Version", useWrapping = false)
    @JsonProperty("Version")
    private LinkedHashSet<Version> versionList;

    @JacksonXmlElementWrapper(localName = "DeleteMarker", useWrapping = false)
    @JsonProperty("DeleteMarker")
    private LinkedHashSet<RawVersion> deleteMarkerList;

    @JacksonXmlElementWrapper(localName = "CommonPrefixes", useWrapping = false)
    @JsonProperty("CommonPrefixes")
    private LinkedHashSet<CommonPrefix> commonPrefixList;

    public ListVersionsResult(String bucketName, Integer maxKeys, String encodingType,
                              String prefix, String delimiter, String keyMarker,
                              String versionIdMarker)throws S3ServerException {
        try {
            this.name = bucketName;
            this.maxKeys = maxKeys;
            this.encodingType = encodingType;
            this.versionIdMarker = versionIdMarker;
            this.versionList = new LinkedHashSet<>();
            this.deleteMarkerList = new LinkedHashSet<>();
            this.commonPrefixList = new LinkedHashSet<>();
            if (null != encodingType) {
                if (null != prefix) {
                    this.prefix = URLEncoder.encode(prefix, "UTF-8");
                }
                if (null != delimiter) {
                    this.delimiter = URLEncoder.encode(delimiter, "UTF-8");
                }
                if (null != keyMarker) {
                    this.keyMarker = URLEncoder.encode(keyMarker, "UTF-8");
                }
            } else {
                this.prefix = prefix;
                this.delimiter = delimiter;
                this.keyMarker = keyMarker;
            }
        } catch (UnsupportedEncodingException e) {
            throw new S3ServerException(S3Error.UNKNOWN_ERROR, "URL encode failed", e);
        }
    }

    public void setCommonPrefixList(LinkedHashSet<CommonPrefix> commonPrefixList) {
        this.commonPrefixList = commonPrefixList;
    }

    public LinkedHashSet<CommonPrefix> getCommonPrefixList() {
        return commonPrefixList;
    }

    public void setDeleteMarkerList(LinkedHashSet<RawVersion> deleteMarkerList) {
        this.deleteMarkerList = deleteMarkerList;
    }

    public LinkedHashSet<RawVersion> getDeleteMarkerList() {
        return deleteMarkerList;
    }

    public void setVersionList(LinkedHashSet<Version> versionList) {
        this.versionList = versionList;
    }

    public LinkedHashSet<Version> getVersionList() {
        return versionList;
    }

    public void setIsTruncated(Boolean isTruncated) {
        this.isTruncated = isTruncated;
    }

    public Boolean getIsTruncated() {
        return isTruncated;
    }

    public void setNextKeyMarker(String nextKeyMarker) {
        this.nextKeyMarker = nextKeyMarker;
    }

    public String getNextKeyMarker() {
        return nextKeyMarker;
    }

    public void setNextVersionIdMarker(String nextVersionIdMarker) {
        this.nextVersionIdMarker = nextVersionIdMarker;
    }

    public String getNextVersionIdMarker() {
        return nextVersionIdMarker;
    }
}
