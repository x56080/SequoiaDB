/*
 * Copyright 2022 SequoiaDB Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
*/

package com.sequoiadb.flink.config;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Optional;
import java.util.Set;

import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.flink.client.SDBSinkClient;

import org.apache.flink.configuration.ReadableConfig;
import org.apache.flink.table.catalog.UniqueConstraint;

public class SDBSinkOptions extends SDBClientOptions {
    private final int sinkParallelism;
    private final int bulkSize;
    private final Boolean ignoreNullField;
    private final int pageSize;
    private final String domain;
    private final String shardingKey;
    private final String shardingType;
    private final int replSize;
    private final String compressionType;
    private final Boolean autoSplit;
    private final String group;
    private final long maxBulkFillTime;
    private final Boolean overwrite;
    private Boolean idempotent;
    private HashSet<String> primaryKey = null;
    
    public SDBSinkOptions(ReadableConfig options) {
        super(options);

        this.sinkParallelism = options.get(SDBOptions.SINK_PARALLELISM);
        this.bulkSize = options.get(SDBOptions.BULK_SIZE);
        this.ignoreNullField = options.get(SDBOptions.IGNORE_NULL_FIELD);
        this.pageSize = options.get(SDBOptions.PAGE_SIZE);
        this.domain = options.get(SDBOptions.DOMAIN);
        this.shardingKey = options.get(SDBOptions.SHARDING_KEY);
        this.shardingType = options.get(SDBOptions.SHARDING_TYPE);
        this.replSize = options.get(SDBOptions.REPL_SIZE);
        this.compressionType = options.get(SDBOptions.COMPRESSION_TYPE);
        this.autoSplit = options.get(SDBOptions.AUTO_SPLIT);
        this.group = options.get(SDBOptions.GROUP);
        this.maxBulkFillTime = options.get(SDBOptions.MAX_BULK_FILL_TIME);
        this.overwrite = options.get(SDBOptions.OVERWRITE);

      
    }

    public Boolean getIdempotent() {
        return idempotent;
    }

    public int getSinkParallelism() {
        return sinkParallelism;
    }
    public int getBulkSize() {
        return bulkSize;
    }
    public Boolean getIgnoreNullField() {
        return ignoreNullField;
    }
    public int getPageSize() {
        return pageSize;
    }
    public String getDomain() {
        return domain;
    }
    public String getShardingKey() {
        return shardingKey;
    }
    public String getShardingType() {
        return shardingType;
    }
    public int getReplSize() {
        return replSize;
    }
    public String getCompressionType() {
        return compressionType;
    }
    public Boolean getAutoSplit() {
        return autoSplit;
    }
    public String getGroup() {
        return group;
    }
    public long getMaxBulkFillTime() {
        return maxBulkFillTime;
    }

    public Boolean getOverwrite() {
        return overwrite;
    }

    public  HashSet<String> getPrimaryKeys() {
        return primaryKey;
    }

    @Override
    public String toString() {
        return "SDBSinkOptions [" 
        + "hosts= " + getHosts() 
        + ", collectionSpace=" + getCollectionSpace() 
        + ", collection=" + getCollection() 
        + ", username=" + getUsername() 
        + " autoSplit=" + autoSplit 
        + ", bulkSize=" + bulkSize 
        + ", compressionType=" + compressionType 
        + ", domain=" + domain 
        + ", group=" + group 
        + ", ignoreNullField=" + ignoreNullField
        + ", idempotent=" + idempotent 
        + ", maxBulkFillTime=" + maxBulkFillTime 
        + ", pageSize=" + pageSize
        + ", replSize=" + replSize 
        + ", shardingKey=" + shardingKey 
        + ", shardingType=" + shardingType
        + ", sinkParallelism=" + sinkParallelism 
        + ", overwrite=" + overwrite + "]";
    }

    public void computeIdempotentWriteOptimization(Optional<UniqueConstraint> primarykey) {

        if (primarykey.isPresent()){
            HashSet<String> pks = new HashSet<>(primarykey.get().getColumns());
            List<HashSet<String>> unique_indexes = new ArrayList<>();
            try {
                unique_indexes = SDBSinkClient.checkUniqueIndex(
                    super.getHosts(), 
                    super.getCollectionSpace(),
                    super.getCollection(), 
                    super.getUsername(), 
                    super.getPassword());
            } catch (BaseException e) {
                if (e.getErrorCode() == SDBError.SDB_DMS_CS_NOTEXIST.getErrorCode() 
                 || e.getErrorCode() == SDBError.SDB_DMS_NOTEXIST.getErrorCode()) {
                    primaryKey = pks;
                } else {
                    throw e;
                }
            }
            
            boolean hasIndex = false;
            for (HashSet<String> uniquekeyset : unique_indexes){
                if (uniquekeyset.equals(pks)) {
                    hasIndex = true;
                    break;
                }
            }

            this.idempotent = !unique_indexes.isEmpty() && hasIndex || primarykey != null;
        } else {
            this.idempotent = false;
        }
       
    }
}
