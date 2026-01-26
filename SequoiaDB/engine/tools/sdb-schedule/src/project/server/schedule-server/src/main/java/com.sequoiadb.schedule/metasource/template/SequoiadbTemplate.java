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

   Source File Name = SequoiadbTemplate.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          26/01/2026  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoiadb.schedule.metasource.template;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.DBQuery;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.base.result.UpdateResult;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.schedule.metasource.MetaCursor;
import com.sequoiadb.schedule.metasource.SdbMetaCursor;
import org.bson.BSONObject;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class SequoiadbTemplate {
    private static final Logger logger = LoggerFactory.getLogger(SequoiadbTemplate.class);

    private DataSourceWrapper datasourceWrapper;

    public SequoiadbTemplate(DataSourceWrapper datasourceWrapper) {
        this.datasourceWrapper = datasourceWrapper;
    }

    public SequoiadbCollectionSpaceTemplate collectionSpace() {
        return collectionSpace(datasourceWrapper.getSystemCSName());
    }

    private SequoiadbCollectionSpaceTemplate collectionSpace(String collectionSpace) {
        return new SequoiadbCollectionSpaceTemplate(collectionSpace);
    }

    public SequoiadbCollectionTemplate collection(String collection) {
        return collection(datasourceWrapper.getSystemCSName(), collection);
    }

    public String getSystemCSName() {
        return datasourceWrapper.getSystemCSName();
    }

    private SequoiadbCollectionTemplate collection(String collectionSpace, String collection) {
        return new SequoiadbCollectionTemplate(collectionSpace, collection);
    }

    protected Sequoiadb getSequoiadb(SequoiadbTransaction context) {
        if (null == context) {
            Sequoiadb db = datasourceWrapper.getConnection();
            if (logger.isDebugEnabled()) {
                logger.debug("acquired connection from pool to sequoiadb, nodeName: {}.",
                        db.getNodeName());
            }
            return db;
        }
        else {
            return context.getSequoiadb();
        }
    }

    protected void releaseSequoiadb(Sequoiadb sdb, SequoiadbTransaction context) {
        if (null == context) {
            datasourceWrapper.releaseConnection(sdb);
        }
        else {
            // do nothing
        }
    }

    public class SequoiadbCollectionSpaceTemplate {
        private final String collectionSpace;

        public SequoiadbCollectionSpaceTemplate(String collectionSpace) {
            this.collectionSpace = collectionSpace;
        }

        public SequoiadbCollectionTemplate createCollection(String collectionName) {
            Sequoiadb sdb = null;
            try {
                sdb = getSequoiadb(null);
                CollectionSpace cs = sdb.getCollectionSpace(collectionSpace);
                cs.createCollection(collectionName, null);
                return collection(collectionSpace, collectionName);
            }
            finally {
                releaseSequoiadb(sdb, null);
            }
        }
    }

    public class SequoiadbCollectionTemplate {
        private final String collectionSpace;
        private final String collection;

        public SequoiadbCollectionTemplate(String collectionSpace, String collection) {
            this.collectionSpace = collectionSpace;
            this.collection = collection;
        }

        public void insert(BSONObject obj, SequoiadbTransaction context) {
            Sequoiadb sdb = getSequoiadb(context);
            try {
                sdb.getCollectionSpace(collectionSpace).getCollection(collection).insertRecord(obj);
            }
            finally {
                releaseSequoiadb(sdb, context);
            }
        }

        public void insert(BSONObject obj) {
            insert(obj, null);
        }

        public void update(BSONObject matcher, BSONObject modifier, SequoiadbTransaction context) {
            Sequoiadb sdb = getSequoiadb(context);
            try {
                sdb.getCollectionSpace(collectionSpace).getCollection(collection)
                        .updateRecords(matcher, modifier, null);
            }
            finally {
                releaseSequoiadb(sdb, context);
            }
        }

        public void update(BSONObject matcher, BSONObject modifier) {
            update(matcher, modifier, null);
        }

        public void upsert(BSONObject matcher, BSONObject modifier) {
            Sequoiadb sdb = getSequoiadb(null);
            try {
                sdb.getCollectionSpace(collectionSpace).getCollection(collection)
                        .upsertRecords(matcher, modifier, null);
            }
            finally {
                releaseSequoiadb(sdb, null);
            }
        }

        public UpdateResult upsert(BSONObject matcher, BSONObject modifier,
                SequoiadbTransaction context) {
            Sequoiadb sdb = getSequoiadb(context);
            try {
                return sdb.getCollectionSpace(collectionSpace).getCollection(collection)
                        .upsertRecords(matcher, modifier);
            }
            finally {
                releaseSequoiadb(sdb, context);
            }
        }

        // return an matching record, and update all matching records.
        public BSONObject queryAndUpdate(BSONObject matcher, BSONObject selector,
                BSONObject modifier, boolean returnNew, SequoiadbTransaction context) {
            Sequoiadb sdb = getSequoiadb(context);
            DBCursor cursor = null;
            try {
                cursor = sdb.getCollectionSpace(collectionSpace).getCollection(collection)
                        .queryAndUpdate(matcher, selector, null, null, modifier, 0, -1,
                                DBQuery.FLG_QUERY_WITH_RETURNDATA, returnNew);
                BSONObject ret = null;
                while (cursor.hasNext()) {
                    ret = cursor.getNext();
                }
                return ret;
            }
            finally {
                closeCursor(cursor);
                releaseSequoiadb(sdb, context);
            }
        }

        public BSONObject queryAndUpdate(BSONObject matcher, BSONObject selector,
                BSONObject modifier, boolean returnNew) {
            return queryAndUpdate(matcher, selector, modifier, returnNew, null);
        }

        public void delete(BSONObject matcher, SequoiadbTransaction context) {
            Sequoiadb sdb = getSequoiadb(context);
            try {
                sdb.getCollectionSpace(collectionSpace).getCollection(collection)
                        .deleteRecords(matcher);
            }
            finally {
                releaseSequoiadb(sdb, context);
            }
        }

        public void delete(BSONObject matcher) {
            delete(matcher, null);
        }

        // return an matching record (old), and delete all matching records.
        public BSONObject queryAndDelete(BSONObject matcher, BSONObject selector,
                SequoiadbTransaction context) {
            Sequoiadb sdb = getSequoiadb(context);
            DBCursor cursor = null;
            try {
                cursor = sdb.getCollectionSpace(collectionSpace).getCollection(collection)
                        .queryAndRemove(matcher, selector, null, null, 0, -1,
                                DBQuery.FLG_QUERY_WITH_RETURNDATA);
                BSONObject ret = null;
                while (cursor.hasNext()) {
                    ret = cursor.getNext();
                }
                return ret;
            }
            finally {
                closeCursor(cursor);
                releaseSequoiadb(sdb, context);
            }
        }

        public BSONObject queryAndDelete(BSONObject matcher, BSONObject selector) {
            return queryAndDelete(matcher, selector, null);
        }

        public BSONObject findOne(BSONObject matcher) {
            return findOne(matcher, null);
        }

        public BSONObject findOne(BSONObject matcher, BSONObject selector) {
            Sequoiadb sdb = getSequoiadb(null);
            try {
                return sdb.getCollectionSpace(collectionSpace).getCollection(collection)
                        .queryOne(matcher, selector, null, null, 0);
            }
            finally {
                releaseSequoiadb(sdb, null);
            }
        }

        public BSONObject findOne(BSONObject matcher, BSONObject selector, BSONObject orderBy) {
            Sequoiadb sdb = getSequoiadb(null);
            try {
                return sdb.getCollectionSpace(collectionSpace).getCollection(collection)
                        .queryOne(matcher, selector, orderBy, null, 0);
            }
            finally {
                releaseSequoiadb(sdb, null);
            }
        }

        public MetaCursor find(BSONObject matcher) throws Exception {
            return find(matcher, null, null, 0, -1);
        }

        public MetaCursor find(BSONObject matcher, BSONObject selector, BSONObject orderBy,
                long skip, long limit) throws Exception {
            Sequoiadb sdb = getSequoiadb(null);
            MetaCursor meatCursor = null;
            DBCursor cursor = null;
            try {
                cursor = sdb.getCollectionSpace(collectionSpace).getCollection(collection)
                        .query(matcher, selector, orderBy, null, skip, limit);
                meatCursor = new SdbMetaCursor(new SdbMetaCursor.CloseCallback() {
                    @Override
                    public void close(Sequoiadb sdb) {
                        releaseSequoiadb(sdb, null);
                    }
                }, sdb, cursor);
            }
            catch (Exception e) {
                closeCursor(cursor);
                releaseSequoiadb(sdb, null);
                throw e;
            }
            return meatCursor;
        }

        private void closeCursor(DBCursor cursor) {
            if (null != cursor) {
                try {
                    cursor.close();
                }
                catch (Exception e) {
                    logger.warn("close cursor failed", e);
                }
            }
        }

        public void ensureIndex(String indexName, BSONObject indexDefinition, boolean isUnique) {
            Sequoiadb sdb = getSequoiadb(null);
            DBCursor cursor = null;
            try {
                DBCollection cl = sdb.getCollectionSpace(collectionSpace).getCollection(collection);
                if (cl.isIndexExist(indexName)) {
                    return;
                }

                try {
                    cl.createIndex(indexName, indexDefinition, isUnique, false);
                }
                catch (BaseException e) {
                    if (e.getErrorCode() != SDBError.SDB_IXM_EXIST.getErrorCode()
                            && e.getErrorCode() != SDBError.SDB_IXM_REDEF.getErrorCode()
                            && e.getErrorCode() != SDBError.SDB_IXM_EXIST_COVERD_ONE
                                    .getErrorCode()) {
                        throw e;
                    }
                }
            }
            finally {
                closeCursor(cursor);
                releaseSequoiadb(sdb, null);
            }
        }

        public long count(BSONObject filter) {
            Sequoiadb sdb = getSequoiadb(null);
            try {
                return sdb.getCollectionSpace(collectionSpace).getCollection(collection)
                        .getCount(filter);
            }
            finally {
                releaseSequoiadb(sdb, null);
            }
        }
    }
}
