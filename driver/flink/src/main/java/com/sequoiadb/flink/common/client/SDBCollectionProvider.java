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

package com.sequoiadb.flink.common.client;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.ConfigOptions;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.flink.common.constant.SDBConstant;
import com.sequoiadb.flink.common.exception.SDBException;
import com.sequoiadb.flink.config.SDBSinkOptions;
import org.apache.flink.util.Preconditions;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.util.JSON;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.IOException;
import java.util.List;

public class SDBCollectionProvider implements SDBClientProvider {

    private static final Logger LOG = LoggerFactory.getLogger(SDBCollectionProvider.class);

    private static final String PRIMARY_KEY_NAME = "primary_key";

    // sequoiadb connection config
    private final List<String> hosts;
    private final String collectionSpaceStr;
    private final String collectionStr;
    private final String username;
    private final String password;

    private final SDBSinkOptions sinkOptions;

    // sequoiadb handler
    private transient Sequoiadb sequoiadb;
    private transient CollectionSpace collectionSpace;
    private transient DBCollection collection;

    public SDBCollectionProvider(
            List<String> hosts,
            String collectionSpaceStr,
            String collectionStr,
            String username,
            String password,
            SDBSinkOptions sinkOptions) {
        Preconditions.checkNotNull(hosts);
        Preconditions.checkNotNull(collectionSpaceStr);
        Preconditions.checkNotNull(collectionStr);
        Preconditions.checkNotNull(username);
        Preconditions.checkNotNull(password);

        this.hosts = hosts;
        this.collectionSpaceStr = collectionSpaceStr;
        this.collectionStr = collectionStr;
        this.username = username;
        this.password = password;
        this.sinkOptions = sinkOptions;
    }

    @Override
    public Sequoiadb getClient() {
        try {
            if (sequoiadb == null) {
                sequoiadb = new Sequoiadb(hosts, username, password, new ConfigOptions());
            }
        } catch (Exception ex) {
            throw new SDBException("cannot connect to SequoiaDB", ex);
        }
        return sequoiadb;
    }

    @Override
    public CollectionSpace getCollectionSpace() {
        if (collectionSpace == null) {
            try {
                collectionSpace = getClient().getCollectionSpace(collectionSpaceStr);
            } catch (BaseException ex) {
                if (ex.getErrorCode() == SDBError.SDB_DMS_CS_NOTEXIST.getErrorCode()) {
                    collectionSpace = ensureCollectionSpace(sinkOptions);
                } else {
                    throw new SDBException("cannot get collection space from Sequoiadb.", ex);
                }
            } catch (Exception ex) {
                throw new SDBException("cannot get collection space from Sequoiadb.", ex);
            }
        }
        return collectionSpace;
    }

    @Override
    public DBCollection getCollection() {
        if (collection == null) {
            try {
                collection = getCollectionSpace().getCollection(collectionStr);
            } catch (BaseException ex) {
                if (ex.getErrorCode() == SDBError.SDB_DMS_NOTEXIST.getErrorCode()) {
                    collection = ensureCollection(sinkOptions);
                } else {
                    throw new SDBException("cannot get collection from Sequoiadb.", ex);
                }
            } catch (Exception ex) {
                throw new SDBException("cannot get collection from Sequoiadb.", ex);
            }
        }
        return collection;
    }

    @Override
    public Sequoiadb recreateClient() throws IOException {
        close();
        return getClient();
    }

    public CollectionSpace ensureCollectionSpace(SDBSinkOptions sinkOptions) {
        BSONObject options = new BasicBSONObject();
        options.put(SDBConstant.PAGE_SIZE, sinkOptions.getPageSize());

        String domain = sinkOptions.getDomain();
        if (domain != null) {
            options.put(SDBConstant.DOMAIN, domain);
        }
        return getClient().createCollectionSpace(collectionSpaceStr, options);
    }

    public DBCollection ensureCollection(SDBSinkOptions sinkOptions) {
        BSONObject options = new BasicBSONObject();
        String ShardingKey = sinkOptions.getShardingKey();
        String[] upsertKeys = sinkOptions.getUpsertKey();

        BSONObject indexBson = new BasicBSONObject();
        if (upsertKeys != null && upsertKeys.length != 0) {
            for (String upsertKey : upsertKeys) {
                indexBson.put(upsertKey, 1);
            }
        }

        if (ShardingKey != null) {
            options.put(SDBConstant.SHARDING_KEY, JSON.parse(ShardingKey));
            options.put(SDBConstant.SHARDING_TYPE, sinkOptions.getShardingType());
        } else {
            if (indexBson.isEmpty()) {
                throw new SDBException("can't create SequoiaDB collection without defining " +
                        "primary keys or sharding key.");
            }

            options.put(SDBConstant.SHARDING_KEY, indexBson);
            options.put(SDBConstant.SHARDING_TYPE, indexBson);
        }

        options.put(SDBConstant.REPL_SIZE, sinkOptions.getReplSize());
        options.put(SDBConstant.COMPRESSION_TYPE, sinkOptions.getCompressionType());
        options.put(SDBConstant.AUTO_SPLIT, sinkOptions.getAutoSplit());

        String Group = sinkOptions.getGroup();
        if (Group != null) {
            options.put(SDBConstant.GROUP, Group);
        }

        DBCollection cl = getCollectionSpace().createCollection(collectionStr, options);
        if (!indexBson.isEmpty()) {
            cl.createIndex(PRIMARY_KEY_NAME, indexBson, true, false);
        }

        return cl;
    }

    @Override
    public void close() throws IOException {
        try {
            if (sequoiadb != null) {
                sequoiadb.close();
            }
        } catch (Exception ex) {
            throw new SDBException("close sequoiadb connection failed.",
                    ex);
        } finally {
            sequoiadb = null;
            collectionSpace = null;
            collection = null;
        }
    }

}
