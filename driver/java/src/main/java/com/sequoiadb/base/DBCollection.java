/*
 * Copyright 2018 SequoiaDB Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except
 * in compliance with the License. You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed under the License
 * is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express
 * or implied. See the License for the specific language governing permissions and limitations under
 * the License.
 */

package com.sequoiadb.base;

import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.HashSet;
import java.util.Iterator;
import java.util.List;
import java.util.Set;

import org.bson.BSON;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;
import org.bson.types.ObjectId;
import org.bson.util.JSON;

import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.message.ResultSet;
import com.sequoiadb.message.request.AdminRequest;
import com.sequoiadb.message.request.AggregateRequest;
import com.sequoiadb.message.request.DeleteRequest;
import com.sequoiadb.message.request.InsertRequest;
import com.sequoiadb.message.request.LobCreateIDRequest;
import com.sequoiadb.message.request.LobRemoveRequest;
import com.sequoiadb.message.request.LobTruncateRequest;
import com.sequoiadb.message.request.QueryRequest;
import com.sequoiadb.message.request.UpdateRequest;
import com.sequoiadb.message.response.SdbReply;

/**
 * Collection of SequoiaDB.
 */
public class DBCollection {
    private String name;
    private Sequoiadb sequoiadb;
    private CollectionSpace collectionSpace;
    private String csName;
    private String collectionFullName;
    private Set<String> mainKeys;
    private boolean ensureOID;

    private boolean isOldLobServer = true;

    /**
     * The flag represent whether insert continue(no errors were reported) when hitting index key
     * duplicate error
     */
    public static final int FLG_INSERT_CONTONDUP = 0x00000001;

    /**
     * The flag represent whether insert return the "_id" field of the record for user
     */
    public static final int FLG_INSERT_RETURN_OID = 0x10000000;

    /**
     * The flag represent whether insert becomes update when hitting index key duplicate error.
     */
    public static final int FLG_INSERT_REPLACEONDUP = 0x00000004;

    /**
     * The sharding key in update rule is not filtered, when executing update or upsert.
     */
    public static final int FLG_UPDATE_KEEP_SHARDINGKEY = 0x00008000;

    /**
     * The flag represent whether to update only one matched record or all matched records.
     */
    public static final int FLG_UPDATE_ONE = 0x00000002;

    /**
     * The flag represent whether to delete only one matched record or all matched records
     */
    public static final int FLG_DELETE_ONE = 0x00000002;

    /**
     * Get the name of current collection.
     *
     * @return The collection name
     */
    public String getName() {
        return name;
    }

    /**
     * Get the full name of specified collection in current collection space.
     *
     * @return The full name of specified collection
     */
    public String getFullName() {
        return collectionFullName;
    }

    /**
     * Get the full name of specified collection in current collection space.
     *
     * @return The full name of specified collection.
     */
    public String getCSName() {
        return csName;
    }

    /**
     * Get the Sequoiadb instance of current collection.
     *
     * @return Sequoiadb instance
     */
    public Sequoiadb getSequoiadb() {
        return sequoiadb;
    }

    /**
     * Get the CollectionSpace instance of current collection.
     *
     * @return CollectionSpace instance
     */
    public CollectionSpace getCollectionSpace() {
        return collectionSpace;
    }

    /**
     * Set the main keys used in save(). if no main keys are set, use the default main key "_id".
     * Every time invokes this method, it will remove the main keys set in the last time.
     *
     * @param keys the main keys specified by user. the main key should exist in the object
     * @throws BaseException when keys is null
     */
    public void setMainKeys(String[] keys) throws BaseException {
        if (keys == null) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "keys is null");
        }
        // remove the main keys set in the last time
        mainKeys.clear();

        // add the new main keys
        if (keys.length == 0) {
            return;
        }

        for (String k : keys) {
            mainKeys.add(k);
        }
    }

    /**
     * @param sequoiadb Sequoiadb object
     * @param cs        CollectionSpace object
     * @param name      Collection name
     */
    DBCollection(Sequoiadb sequoiadb, CollectionSpace cs, String name) {
        this.name = name;
        this.sequoiadb = sequoiadb;
        this.collectionSpace = cs;
        this.csName = cs.getName();
        this.collectionFullName = csName + "." + name;
        this.mainKeys = new HashSet<String>();
        this.ensureOID = true;
    }

    /**
     * Insert a document into current collection.
     *
     * @param insertor The bson object to be inserted, can't be null.
     * @param flags    The flag to control the behavior of inserting. The value of flags default to be 0,
     *                 and it can choose the follow values:
     *                 <ul>
     *                 <li>0: default value.</li>
     *                 <li>FLG_INSERT_CONTONDUP: if the record hit index key duplicate error, database
     *                 will skip them and go on inserting.</li>
     *                 <li>FLG_INSERT_RETURN_OID: return the value of "_id" field in the record.</li>
     *                 <li>FLG_INSERT_REPLACEONDUP: if the record hit index key duplicate error, database
     *                 will replace the existing record by the inserting new record.</li>
     *                 </ul>
     * @return The result of inserting, can be the follow values:
     * <ul>
     * <li>null: when there is no result to return.</li>
     * <li>bson which contains the "_id" field: when flag "FLG_INSERT_RETURN_OID" is set,
     * return the value of "_id" field of the inserted record. e.g.: { "_id": { "$oid":
     * "5c456e8eb17ab30cfbf1d5d1" } }</li>
     * </ul>
     * @throws BaseException If error happens.
     */
    public BSONObject insert(BSONObject insertor, int flags) throws BaseException {
        BSONObject result = null;
        if (insertor == null) {
            throw new BaseException(SDBError.SDB_INVALIDARG);
        }
        // send to engine
        InsertRequest request = new InsertRequest(collectionFullName, insertor, flags);
        SdbReply response = sequoiadb.requestAndResponse(request);
        sequoiadb.throwIfError(response, insertor);
        sequoiadb.upsertCache(collectionFullName);
        // return result
        if ((flags & FLG_INSERT_RETURN_OID) != 0) {
            Object oid = request.getOIDValue();
            if (oid != null) {
                result = new BasicBSONObject();
                result.put(SdbConstants.OID, oid);
            }
        }
        return result;
    }

    /**
     * Insert a document into current collection, if the document does not contain field "_id", it
     * will be added.
     *
     * @param insertor The insertor.
     * @return the value of the filed "_id"
     * @throws BaseException If error happens.
     */
    public Object insert(BSONObject insertor) throws BaseException {
        BSONObject result = insert(insertor, FLG_INSERT_RETURN_OID);
        return result.get(SdbConstants.OID);
    }

    /**
     * Insert a document into current collection, if the document does not contain field "_id", it
     * will be added.
     *
     * @param insertor The string of insertor
     * @return the value of the filed "_id"
     * @throws BaseException If error happens.
     */
    public Object insert(String insertor) throws BaseException {
        BSONObject in = null;
        if (insertor != null) {
            in = (BSONObject) JSON.parse(insertor);
        }
        return insert(in);
    }

    /**
     * Insert a bulk of bson objects into current collection.
     *
     * @param insertor The Bson object of insertor list, can't be null
     * @param flags    The flag to control the behavior of inserting. The value of flags default to be 0,
     *                 and it can choose the follow values:
     *                 <ul>
     *                 <li>0: default value.</li>
     *                 <li>FLG_INSERT_CONTONDUP: if the record hit index key duplicate error, database
     *                 will skip them and go on inserting.</li>
     *                 <li>FLG_INSERT_RETURN_OID: return the value of "_id" field in the record. When set
     *                 this flag, ensureOID() will be set to true.</li>
     *                 <li>FLG_INSERT_REPLACEONDUP: if the record hit index key duplicate error, database
     *                 will replace the existing record by the inserting new record and them go on
     *                 inserting.</li>
     *                 </ul>
     * @return The result of inserting, can be the follow values:
     * <ul>
     * <li>null: when there is no result to return.</li>
     * <li>bson which contains the "_id" field: when flag "FLG_INSERT_RETURN_OID" is set,
     * return all the values of "_id" field in a bson array. e.g.: { "_id": [ { "$oid":
     * "5c456e8eb17ab30cfbf1d5d1" }, { "$oid": "5c456e8eb17ab30cfbf1d5d2" } ] }</li>
     * </ul>
     * @throws BaseException If error happens.
     * @since 3.0.2
     */
    public BSONObject insertRecords(List<BSONObject> insertor, int flags) throws BaseException {
        BSONObject result = null;
        if (insertor == null || insertor.size() == 0) {
            throw new BaseException(SDBError.SDB_INVALIDARG);
        }
        // try to ensure oid
        if ((flags & FLG_INSERT_RETURN_OID) != 0) {
            if (!isOIDEnsured()) {
                ensureOID(true);
            }
        }
        // build and send message
        InsertRequest request = new InsertRequest(collectionFullName, insertor, flags, ensureOID);
        SdbReply response = sequoiadb.requestAndResponse(request);
        sequoiadb.throwIfError(response);
        sequoiadb.upsertCache(collectionFullName);
        // return result
        if ((flags & FLG_INSERT_RETURN_OID) != 0) {
            Object oid = request.getOIDValue();
            if (oid != null) {
                result = new BasicBSONObject();
                result.put(SdbConstants.OID, oid);
            }
        }
        sequoiadb.cleanRequestBuff();
        return result;
    }

    /**
     * Insert a bulk of bson objects into current collection.
     *
     * @param insertor The Bson object of insertor list, can't be null
     * @param flags    The flag to control the behavior of inserting. The value of flags default to be 0,
     *                 and it can choose the follow values:
     *                 <ul>
     *                 <li>0: default value</li>
     *                 <li>FLG_INSERT_CONTONDUP: if the record hit index key duplicate error, database
     *                 will skip them and go on inserting.</li>
     *                 <li>FLG_INSERT_REPLACEONDUP: if the record hit index key duplicate error, database
     *                 will replace the existing record by the inserting new record and them go on
     *                 inserting.</li>
     *                 </ul>
     * @throws BaseException If error happens.
     * @since 3.0.2
     */
    public void insert(List<BSONObject> insertor, int flags) throws BaseException {
        if (flags != 0) {
            flags = DBQuery.eraseSingleFlag(flags, FLG_INSERT_RETURN_OID);
        }
        insertRecords(insertor, flags);
    }

    /**
     * Insert a bulk of bson objects into current collection.
     *
     * @param insertor The Bson object of insertor list, can't be null. insert will interrupt when
     *                 Duplicate key exist.
     * @throws BaseException If error happens.
     * @since 2.9
     */
    public void insert(List<BSONObject> insertor) throws BaseException {
        insert(insertor, 0);
    }

    /**
     * Insert an object into current collection. When flag is set to 0, it won't work to update the
     * ShardingKey field, but the other fields take effect.
     *
     * @param type            The object of insertor, can't be null
     * @param ignoreNullValue true:if type's inner value is null, it will not save to collection;
     * @param flag            the update flag, default to be 0. Please see the definition of follow flags for
     *                        more detail.
     *                        <ul>
     *                        <li>DBCollection.FLG_UPDATE_KEEP_SHARDINGKEY
     *                        </ul>
     * @throws BaseException 1.when the type is not support, throw BaseException with the type
     *                       "SDB_INVALIDARG" 2.when offer main keys by setMainKeys(), and try to update "_id"
     *                       field, it may get a BaseException with the type of "SDB_IXM_DUP_KEY"
     * @see com.sequoiadb.base.DBCollection#setMainKeys(String[])
     */
    public /* ! @cond x */ <T> /* ! @endcond */ void save(T type, Boolean ignoreNullValue, int flag)
            throws BaseException {
        // transform java object to bson object
        BSONObject obj;
        try {
            obj = BasicBSONObject.typeToBson(type, ignoreNullValue);
        } catch (Exception e) {
            throw new BaseException(SDBError.SDB_INVALIDARG, type.toString(), e);
        }
        BSONObject matcher = new BasicBSONObject();
        BSONObject modifer = new BasicBSONObject();
        // if user don't specify main keys, use default one "_id"
        if (mainKeys.isEmpty()) {
            Object id = obj.get(SdbConstants.OID);
            if (id == null || (id instanceof ObjectId && ((ObjectId) id).isNew())) {
                if (id != null && id instanceof ObjectId) {
                    ((ObjectId) id).notNew();
                }
                insert(obj);
            } else {
                // build condtion
                matcher.put(SdbConstants.OID, id);
                // build rule
                modifer.put("$set", obj);
                upsert(matcher, modifer, null, null, flag);
            }
        } else { // if user specify main keys, use these main keys
            Iterator<String> it = mainKeys.iterator();
            // build condition
            while (it.hasNext()) {
                String key = it.next();
                if (obj.containsField(key)) {
                    matcher.put(key, obj.get(key));
                }
            }
            // build rule
            if (!matcher.isEmpty()) {
                modifer.put("$set", obj);
                upsert(matcher, modifer, null, null, flag);
            } else {
                insert(obj);
            }
        }
    }

    /**
     * Insert an object into current collection when save include update shardingKey field, the
     * shardingKey modify action is not take effect, but the other field update is take effect.
     *
     * @param type            The object of insertor, can't be null
     * @param ignoreNullValue true:if type's inner value is null, it will not save to collection; false:if
     *                        type's inner value is null, it will save to collection too.
     * @throws BaseException 1.when the type is not support, throw BaseException with the type
     *                       "SDB_INVALIDARG" 2.when offer main keys by setMainKeys(), and try to update "_id"
     *                       field, it may get a BaseException with the type of "SDB_IXM_DUP_KEY"
     * @see com.sequoiadb.base.DBCollection#setMainKeys(String[])
     */
    public /* ! @cond x */ <T> /* ! @endcond */ void save(T type, Boolean ignoreNullValue)
            throws BaseException {
        save(type, ignoreNullValue, 0);
    }

    /**
     * Insert an object into current collection. when save include update shardingKey field, the
     * shardingKey modify action is not take effect, but the other field update is take effect.
     *
     * @param type The object of insertor, can't be null
     * @throws BaseException 1.when the type is not support, throw BaseException with the type
     *                       "SDB_INVALIDARG" 2.when offer main keys by setMainKeys(), and try to update "_id"
     *                       field, it may get a BaseException with the type of "SDB_IXM_DUP_KEY"
     * @see com.sequoiadb.base.DBCollection#setMainKeys(String[])
     */
    public /* ! @cond x */ <T> /* ! @endcond */ void save(T type) throws BaseException {
        save(type, false);
    }

    /**
     * Insert an object into current collection. When flag is set to 0, it won't work to update the
     * ShardingKey field, but the other fields take effect.
     *
     * @param type            The List instance of insertor, can't be null or empty
     * @param ignoreNullValue true:if type's inner value is null, it will not save to collection;
     * @param flag            the update flag, default to be 0. Please see the definition of follow flags for
     *                        more detail.
     *                        <ul>
     *                        <li>DBCollection.FLG_UPDATE_KEEP_SHARDINGKEY
     *                        </ul>
     * @throws BaseException 1.while the input argument is null or the List instance is empty 2.while the type
     *                       is not support, throw BaseException with the type "SDB_INVALIDARG" 3.while offer
     *                       main keys by setMainKeys(), and try to update "_id" field, it may get a
     *                       BaseException with the type of "SDB_IXM_DUP_KEY" when the "_id" field you want to
     *                       update to had been existing in database
     * @see com.sequoiadb.base.DBCollection#setMainKeys(String[])
     */
    public /* ! @cond x */ <T> /* ! @endcond */ void save(List<T> type, Boolean ignoreNullValue,
                                                          int flag) throws BaseException {
        if (type == null || type.size() == 0) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "type is empty or null");
        }
        // transform java object to bson object
        List<BSONObject> objs = new ArrayList<BSONObject>();
        try {
            Iterator<T> it = type.iterator();
            while (it != null && it.hasNext()) {
                objs.add(BasicBSONObject.typeToBson(it.next(), ignoreNullValue));
            }
        } catch (Exception e) {
            throw new BaseException(SDBError.SDB_INVALIDARG, type.toString(), e);
        }
        BSONObject matcher = new BasicBSONObject();
        BSONObject modifer = new BasicBSONObject();
        BSONObject obj = null;
        Iterator<BSONObject> ite = objs.iterator();
        // if user don't specify main keys, use default one "_id"
        if (mainKeys.isEmpty()) {
            while (ite != null && ite.hasNext()) {
                obj = ite.next();
                Object id = obj.get(SdbConstants.OID);
                if (id == null || (id instanceof ObjectId && ((ObjectId) id).isNew())) {
                    if (id != null && id instanceof ObjectId) {
                        ((ObjectId) id).notNew();
                    }
                    insert(obj);
                } else {
                    // build condtion
                    matcher.put(SdbConstants.OID, id);
                    // build rule
                    modifer.put("$set", obj);
                    upsert(matcher, modifer, null, null, flag);
                }
            }
        } else { // if user specify main keys, use these main keys
            while (ite != null && ite.hasNext()) {
                obj = ite.next();
                Iterator<String> i = mainKeys.iterator();
                // build condition
                while (i.hasNext()) {
                    String key = i.next();
                    if (obj.containsField(key)) {
                        matcher.put(key, obj.get(key));
                    }
                }
                if (!matcher.isEmpty()) {
                    // build rule
                    modifer.put("$set", obj);
                    upsert(matcher, modifer, null, null, flag);
                } else {
                    insert(obj);
                }
            }
        }
    }

    /**
     * Insert an object into current collection. when save include update shardingKey field, the
     * shardingKey modify action is not take effect, but the other field update is take effect.
     *
     * @param type            The List instance of insertor, can't be null or empty
     * @param ignoreNullValue true:if type's inner value is null, it will not save to collection; false:if
     *                        type's inner value is null, it will save to collection too.
     * @throws BaseException 1.while the input argument is null or the List instance is empty 2.while the type
     *                       is not support, throw BaseException with the type "SDB_INVALIDARG" 3.while offer
     *                       main keys by setMainKeys(), and try to update "_id" field, it may get a
     *                       BaseException with the type of "SDB_IXM_DUP_KEY" when the "_id" field you want to
     *                       update to had been existing in database
     * @see com.sequoiadb.base.DBCollection#setMainKeys(String[])
     */
    public /* ! @cond x */ <T> /* ! @endcond */ void save(List<T> type, Boolean ignoreNullValue)
            throws BaseException {
        save(type, ignoreNullValue, 0);
    }

    /**
     * Insert an object into current collection. when save include update shardingKey field, the
     * shardingKey modify action is not take effect, but the other field update is take effect.
     *
     * @param type The List instance of insertor, can't be null or empty
     * @throws BaseException 1.while the input argument is null or the List instance is empty 2.while the type
     *                       is not support, throw BaseException with the type "SDB_INVALIDARG" 3.while offer
     *                       main keys by setMainKeys(), and try to update "_id" field, it may get a
     *                       BaseException with the type of "SDB_IXM_DUP_KEY" when the "_id" field you want to
     *                       update to had been existing in database
     * @see com.sequoiadb.base.DBCollection#setMainKeys(String[])
     */
    public /* ! @cond x */ <T> /* ! @endcond */ void save(List<T> type) throws BaseException {
        save(type, false);
    }

    /**
     * Set whether ensure OID of record when bulk insert records to SequoiaDB.
     *
     * @param flag whether ensure OID of record
     */
    public void ensureOID(boolean flag) {
        ensureOID = flag;
    }

    /**
     * @return True if ensure OID of record when bulk insert records to SequoiaDB and false if not.
     */
    public boolean isOIDEnsured() {
        return ensureOID;
    }

    /**
     * Insert a bulk of bson objects into current collection.
     *
     * @param insertor The Bson object of insertor list, can't be null
     * @param flags    The flag to control the behavior of inserting. The value of flags default to be 0,
     *                 and it can choose the follow values:
     *                 <ul>
     *                 <li>0: default value</li>
     *                 <li>FLG_INSERT_CONTONDUP: if the record hit index key duplicate error, database
     *                 will skip them and go on inserting.</li>
     *                 <li>FLG_INSERT_REPLACEONDUP: if the record hit index key duplicate error, database
     *                 will replace the existing record by the inserting new record and them go on
     *                 inserting.</li>
     *                 </ul>
     * @throws BaseException If error happens.
     * @deprecated use insert(List<BSONObject> insertor, int flags) instead
     */
    @Deprecated
    public void bulkInsert(List<BSONObject> insertor, int flags) throws BaseException {
        insert(insertor, flags);
    }

    /**
     * Delete the matching records of current collection.
     *
     * @param matcher The matching condition, match all the documents if null
     * @throws BaseException If error happens.
     */
    public void delete(BSONObject matcher) throws BaseException {
        delete(matcher, null);
    }

    /**
     * Delete the matching records of current collection.
     *
     * @param matcher The matching condition, match all the documents if null
     * @throws BaseException If error happens.
     */
    public void delete(String matcher) throws BaseException {
        BSONObject ma = null;
        if (matcher != null) {
            ma = (BSONObject) JSON.parse(matcher);
        }
        delete(ma, null);
    }

    /**
     * Delete the matching records of current collection.
     *
     * @param matcher The matching condition, match all the documents if null
     * @param hint    Specified the index used to scan data. e.g. {"":"ageIndex"} means using index
     *                "ageIndex" to scan data(index scan); {"":null} means table scan. when hint is
     *                null, database automatically match the optimal index to scan data.
     * @throws BaseException If error happens.
     */
    public void delete(String matcher, String hint) throws BaseException {
        BSONObject ma = null;
        BSONObject hi = null;
        if (matcher != null) {
            ma = (BSONObject) JSON.parse(matcher);
        }
        if (hint != null) {
            hi = (BSONObject) JSON.parse(hint);
        }
        delete(ma, hi, 0);
    }
    /**
     * Delete the matching records of current collection.
     *
     * @param matcher The matching condition, match all the documents if null
     * @param hint    Specified the index used to scan data. e.g. {"":"ageIndex"} means using index
     *                "ageIndex" to scan data(index scan); {"":null} means table scan. when hint is
     *                null, database automatically match the optimal index to scan data.
     * @throws BaseException If error happens.
     */
    public void delete(BSONObject matcher, BSONObject hint) throws BaseException {
        delete(matcher, hint, 0);
    }

    /**
     * Delete the matching records of current collection.
     *
     * @param matcher The matching condition, match all the documents if null.
     * @param hint    Specified the index used to scan data. e.g. {"":"ageIndex"} means using index
     *                "ageIndex" to scan data(index scan); {"":null} means table scan. when hint is
     *                 null, database automatically match the optimal index to scan data.
     * @param flag    The delete flag, default to be 0.
     *                 <ul>
     *                 <li>{@link DBCollection#FLG_DELETE_ONE}
     *                 </ul>
     * @throws BaseException If error happens.
     */
    public void delete(BSONObject matcher, BSONObject hint, int flag) throws BaseException {
        DeleteRequest request = new DeleteRequest(collectionFullName, matcher, hint, flag);
        SdbReply response = sequoiadb.requestAndResponse(request);
        if (response.getFlag() != 0) {
            String msg = "matcher = " + matcher + ", hint = " + hint;
            sequoiadb.throwIfError(response, msg);
        }
        sequoiadb.upsertCache(collectionFullName);
    }

    /**
     * Update the matching records of current collection. It won't work to update the ShardingKey field, but
     * the other fields take effect.
     *
     * @param query DBQuery with matching condition, updating rule and hint
     * @throws BaseException If error happens.
     */
    public void update(DBQuery query) throws BaseException {
        _update(query.getFlag(), query.getMatcher(), query.getModifier(), query.getHint());
    }

    /**
     * Update the matching records of current collection. It won't work to update the ShardingKey field,
     * but the other fields take effect.
     *
     * @param matcher  The matching condition, match all the documents if null
     * @param modifier The updating rule, can't be null
     * @param hint     Specified the index used to scan data. e.g. {"":"ageIndex"} means using index
     *                 "ageIndex" to scan data(index scan); {"":null} means table scan. when hint is
     *                 null, database automatically match the optimal index to scan data.
     * @throws BaseException If error happens.
     */
    public void update(BSONObject matcher, BSONObject modifier, BSONObject hint)
            throws BaseException {
        _update(0, matcher, modifier, hint);
    }

    /**
     * Update the matching records of current collection. When flag is set to 0, it won't work to update
     * the ShardingKey field, but the other fields take effect.
     *
     * @param matcher  The matching condition, match all the documents if null
     * @param modifier The updating rule, can't be null
     * @param hint     Specified the index used to scan data. e.g. {"":"ageIndex"} means using index
     *                 "ageIndex" to scan data(index scan); {"":null} means table scan. when hint is
     *                 null, database automatically match the optimal index to scan data.
     * @param flag     the update flag, default to be 0. Please see the definition of follow flags for
     *                 more detail.
     *                 <ul>
     *                 <li>{@link DBCollection#FLG_UPDATE_KEEP_SHARDINGKEY}
     *                 <li>{@link DBCollection#FLG_UPDATE_ONE}
     *                 </ul>
     * @throws BaseException If error happens.
     */
    public void update(BSONObject matcher, BSONObject modifier, BSONObject hint, int flag)
            throws BaseException {
        _update(flag, matcher, modifier, hint);
    }

    /**
     * Update the matching records of current collection. It won't work to update the ShardingKey field,
     * but the other fields take effect.
     *
     * @param matcher  The matching condition, match all the documents if null
     * @param modifier The updating rule, can't be null or empty
     * @param hint     Specified the index used to scan data. e.g. {"":"ageIndex"} means using index
     *                 "ageIndex" to scan data(index scan); {"":null} means table scan. when hint is
     *                 null, database automatically match the optimal index to scan data.
     * @throws BaseException If error happens.
     */
    public void update(String matcher, String modifier, String hint) throws BaseException {
        BSONObject ma = null;
        BSONObject mo = null;
        BSONObject hi = null;
        if (matcher != null) {
            ma = (BSONObject) JSON.parse(matcher);
        }
        if (modifier != null) {
            mo = (BSONObject) JSON.parse(modifier);
        }
        if (hint != null) {
            hi = (BSONObject) JSON.parse(hint);
        }
        _update(0, ma, mo, hi);
    }

    /**
     * Update the matching records of current collection. When flag is set to 0, it won't work to update
     * the ShardingKey field, but the other fields take effect.
     *
     * @param matcher  The matching condition, match all the documents if null
     * @param modifier The updating rule, can't be null or empty
     * @param hint     Specified the index used to scan data. e.g. {"":"ageIndex"} means using index
     *                 "ageIndex" to scan data(index scan); {"":null} means table scan. when hint is
     *                 null, database automatically match the optimal index to scan data.
     * @param flag     the update flag, default to be 0. Please see the definition of follow flags for
     *                 more detail.
     *                 <ul>
     *                 <li>{@link DBCollection#FLG_UPDATE_KEEP_SHARDINGKEY}
     *                 <li>{@link DBCollection#FLG_UPDATE_ONE}
     *                 </ul>
     * @throws BaseException If error happens.
     */
    public void update(String matcher, String modifier, String hint, int flag)
            throws BaseException {
        BSONObject ma = null;
        BSONObject mo = null;
        BSONObject hi = null;
        if (matcher != null) {
            ma = (BSONObject) JSON.parse(matcher);
        }
        if (modifier != null) {
            mo = (BSONObject) JSON.parse(modifier);
        }
        if (hint != null) {
            hi = (BSONObject) JSON.parse(hint);
        }
        _update(flag, ma, mo, hi);
    }

    /**
     * Update the matching records of current collection, insert if no matching. It won't work to update
     * the ShardingKey field, but the other fields take effect.
     *
     * @param matcher  The matching condition, match all the documents if null
     * @param modifier The updating rule, can't be null
     * @param hint     Specified the index used to scan data. e.g. {"":"ageIndex"} means using index
     *                 "ageIndex" to scan data(index scan); {"":null} means table scan. when hint is
     *                 null, database automatically match the optimal index to scan data.
     * @throws BaseException If error happens.
     */
    public void upsert(BSONObject matcher, BSONObject modifier, BSONObject hint)
            throws BaseException {
        _update(SdbConstants.FLG_UPDATE_UPSERT, matcher, modifier, hint);
    }

    /**
     * Update the matching records of current collection, insert if no matching. It won't work to update
     * the ShardingKey field, but the other fields take effect.
     *
     * @param matcher     The matching condition, match all the documents if null
     * @param modifier    The updating rule, can't be null
     * @param hint        Specified the index used to scan data. e.g. {"":"ageIndex"} means using index
     *                    "ageIndex" to scan data(index scan); {"":null} means table scan. when hint is
     *                    null, database automatically match the optimal index to scan data.
     * @param setOnInsert When "setOnInsert" is not a null or an empty object, it assigns the specified
     *                    values to the fields when insert.
     * @throws BaseException If error happens.
     */
    public void upsert(BSONObject matcher, BSONObject modifier, BSONObject hint,
                       BSONObject setOnInsert) throws BaseException {
        upsert(matcher, modifier, hint, setOnInsert, 0);
    }

    /**
     * Update the matching records of current collection, insert if no matching. When flag is set to 0, it
     * won't work to update the ShardingKey field, but the other fields take effect.
     *
     * @param matcher     The matching condition, match all the documents if null
     * @param modifier    The updating rule, can't be null
     * @param hint        Specified the index used to scan data. e.g. {"":"ageIndex"} means using index
     *                    "ageIndex" to scan data(index scan); {"":null} means table scan. when hint is
     *                    null, database automatically match the optimal index to scan data.
     * @param setOnInsert When "setOnInsert" is not a null or an empty object, it assigns the specified
     *                    values to the fields when insert.
     * @param flag        the upsert flag, default to be 0. Please see the definition of follow flags for
     *                    more detail.
     *                    <ul>
     *                    <li>{@link DBCollection#FLG_UPDATE_KEEP_SHARDINGKEY}
     *                    <li>{@link DBCollection#FLG_UPDATE_ONE}
     *                    </ul>
     * @throws BaseException If error happens.
     */
    public void upsert(BSONObject matcher, BSONObject modifier, BSONObject hint,
                       BSONObject setOnInsert, int flag) throws BaseException {
        BSONObject newHint;
        if (setOnInsert != null) {
            newHint = new BasicBSONObject();
            if (hint != null) {
                newHint.putAll(hint);
            }
            newHint.put(SdbConstants.FIELD_NAME_SET_ON_INSERT, setOnInsert);
        } else {
            newHint = hint;
        }
        flag |= SdbConstants.FLG_UPDATE_UPSERT;
        _update(flag, matcher, modifier, newHint);
    }

    /**
     * Explain query of current collection.
     *
     * @param matcher    the matching rule, return all the documents if null
     * @param selector   the selective rule, return the whole document if null
     * @param orderBy    the ordered rule, never sort if null
     * @param hint       Specified the index used to scan data. e.g. {"":"ageIndex"} means using index
     *                   "ageIndex" to scan data(index scan); {"":null} means table scan. when hint is
     *                   null, database automatically match the optimal index to scan data.
     * @param skipRows   skip the first numToSkip documents, never skip if this parameter is 0
     * @param returnRows return the specified amount of documents, when returnRows is 0, return nothing,
     *                   when returnRows is -1, return all the documents
     * @param flag       the query flag, default to be 0. Please see the definition of follow flags for
     *                   more detail. Usage: e.g. set ( DBQuery.FLG_QUERY_FORCE_HINT |
     *                   DBQuery.FLG_QUERY_WITH_RETURNDATA ) to param flag
     *                   <ul>
     *                   <li>DBQuery.FLG_QUERY_STRINGOUT
     *                   <li>DBQuery.FLG_QUERY_FORCE_HINT
     *                   <li>DBQuery.FLG_QUERY_PARALLED
     *                   <li>DBQuery.FLG_QUERY_WITH_RETURNDATA
     *                   </ul>
     * @param options    The rules of query explain, the options are as below:
     *                   <ul>
     *                   <li>Run : Whether execute query explain or not, true for executing query explain
     *                   then get the data and time information; false for not executing query explain but
     *                   get the query explain information only. e.g. {Run:true}
     *                   </ul>
     * @return a DBCursor instance of the result
     * @throws BaseException If error happens.
     */
    public DBCursor explain(BSONObject matcher, BSONObject selector, BSONObject orderBy,
                            BSONObject hint, long skipRows, long returnRows, int flag, BSONObject options)
            throws BaseException {
        BSONObject innerHint = new BasicBSONObject();
        if (hint != null) {
            innerHint.put(SdbConstants.FIELD_NAME_HINT, hint);
        }

        if (options != null) {
            innerHint.put(SdbConstants.FIELD_NAME_OPTIONS, options);
        }
        if (flag != 0) {
            flag = DBQuery.eraseSingleFlag(flag, DBQuery.FLG_QUERY_MODIFY);
        }
        flag |= DBQuery.FLG_QUERY_EXPLAIN;
        return _query(matcher, selector, orderBy, innerHint, skipRows, returnRows, flag);
    }

    /**
     * Get all documents of current collection.
     *
     * @return a DBCursor instance of the result
     * @throws BaseException If error happens.
     */
    public DBCursor query() throws BaseException {
        return query("", "", "", "", 0, -1);
    }

    /**
     * Get the matching documents in current collection.
     *
     * @param matcher the matching rule, return all the documents if null
     * @return a DBCursor instance of the result or null if no any matched document
     * @throws BaseException If error happens.
     * @see com.sequoiadb.base.DBQuery
     */
    public DBCursor query(DBQuery matcher) throws BaseException {
        if (matcher == null) {
            return query();
        }
        return query(matcher.getMatcher(), matcher.getSelector(), matcher.getOrderBy(),
                matcher.getHint(), matcher.getSkipRowsCount(), matcher.getReturnRowsCount(),
                matcher.getFlag());
    }

    /**
     * Get the matching documents in current collection.
     *
     * @param matcher  the matching rule, return all the documents if null
     * @param selector the selective rule, return the whole document if null
     * @param orderBy  the ordered rule, never sort if null
     * @param hint     Specified the index used to scan data. e.g. {"":"ageIndex"} means using index
     *                 "ageIndex" to scan data(index scan); {"":null} means table scan. when hint is
     *                 null, database automatically match the optimal index to scan data.
     * @return a DBCursor instance of the result or null if no any matched document
     * @throws BaseException If error happens.
     */
    public DBCursor query(BSONObject matcher, BSONObject selector, BSONObject orderBy,
                          BSONObject hint) throws BaseException {
        return query(matcher, selector, orderBy, hint, 0, -1, 0);
    }

    /**
     * Get the matching documents in current collection.
     *
     * @param matcher  the matching rule, return all the documents if null
     * @param selector the selective rule, return the whole document if null
     * @param orderBy  the ordered rule, never sort if null
     * @param hint     Specified the index used to scan data. e.g. {"":"ageIndex"} means using index
     *                 "ageIndex" to scan data(index scan); {"":null} means table scan. when hint is
     *                 null, database automatically match the optimal index to scan data.
     * @param flag     the query flag, default to be 0. Please see the definition of follow flags for
     *                 more detail. Usage: e.g. set ( DBQuery.FLG_QUERY_FORCE_HINT |
     *                 DBQuery.FLG_QUERY_WITH_RETURNDATA ) to param flag
     *                 <ul>
     *                 <li>DBQuery.FLG_QUERY_STRINGOUT
     *                 <li>DBQuery.FLG_QUERY_FORCE_HINT
     *                 <li>DBQuery.FLG_QUERY_PARALLED
     *                 <li>DBQuery.FLG_QUERY_WITH_RETURNDATA
     *                 <li>DBQuery.FLG_QUERY_FOR_UPDATE
     *                 </ul>
     * @return a DBCursor instance of the result or null if no any matched document
     * @throws BaseException If error happens.
     */
    public DBCursor query(BSONObject matcher, BSONObject selector, BSONObject orderBy,
                          BSONObject hint, int flag) throws BaseException {
        return query(matcher, selector, orderBy, hint, 0, -1, flag);
    }

    /**
     * Get the matching documents in current collection.
     *
     * @param matcher  the matching rule, return all the documents if null
     * @param selector the selective rule, return the whole document if null
     * @param orderBy  the ordered rule, never sort if null
     * @param hint     Specified the index used to scan data. e.g. {"":"ageIndex"} means using index
     *                 "ageIndex" to scan data(index scan); {"":null} means table scan. when hint is
     *                 null, database automatically match the optimal index to scan data.
     * @return a DBCursor instance of the result or null if no any matched document
     * @throws BaseException If error happens.
     */
    public DBCursor query(String matcher, String selector, String orderBy, String hint)
            throws BaseException {
        return query(matcher, selector, orderBy, hint, 0);
    }

    /**
     * Get the matching documents in current collection.
     *
     * @param matcher  the matching rule, return all the documents if null
     * @param selector the selective rule, return the whole document if null
     * @param orderBy  the ordered rule, never sort if null
     * @param hint     Specified the index used to scan data. e.g. {"":"ageIndex"} means using index
     *                 "ageIndex" to scan data(index scan); {"":null} means table scan. when hint is
     *                 null, database automatically match the optimal index to scan data.
     * @param flag     the query flag, default to be 0. Please see the definition of follow flags for
     *                 more detail. Usage: e.g. set ( DBQuery.FLG_QUERY_FORCE_HINT |
     *                 DBQuery.FLG_QUERY_WITH_RETURNDATA ) to param flag
     *                 <ul>
     *                 <li>DBQuery.FLG_QUERY_STRINGOUT
     *                 <li>DBQuery.FLG_QUERY_FORCE_HINT
     *                 <li>DBQuery.FLG_QUERY_PARALLED
     *                 <li>DBQuery.FLG_QUERY_WITH_RETURNDATA
     *                 <li>DBQuery.FLG_QUERY_FOR_UPDATE
     *                 </ul>
     * @return a DBCursor instance of the result or null if no any matched document
     * @throws BaseException If error happens.
     */
    public DBCursor query(String matcher, String selector, String orderBy, String hint, int flag)
            throws BaseException {
        BSONObject ma = null;
        BSONObject se = null;
        BSONObject or = null;
        BSONObject hi = null;
        if (matcher != null) {
            ma = (BSONObject) JSON.parse(matcher);
        }
        if (selector != null) {
            se = (BSONObject) JSON.parse(selector);
        }
        if (orderBy != null && !orderBy.equals("")) {
            or = (BSONObject) JSON.parse(orderBy);
        }
        if (hint != null) {
            hi = (BSONObject) JSON.parse(hint);
        }
        return query(ma, se, or, hi, 0, -1, flag);
    }

    /**
     * Get the matching documents in current collection.
     *
     * @param matcher    the matching rule, return all the documents if null
     * @param selector   the selective rule, return the whole document if null
     * @param orderBy    the ordered rule, never sort if null
     * @param hint       Specified the index used to scan data. e.g. {"":"ageIndex"} means using index
     *                   "ageIndex" to scan data(index scan); {"":null} means table scan. when hint is
     *                   null, database automatically match the optimal index to scan data.
     * @param skipRows   skip the first numToSkip documents, never skip if this parameter is 0
     * @param returnRows return the specified amount of documents, when returnRows is 0, return nothing,
     *                   when returnRows is -1, return all the documents
     * @return a DBCursor instance of the result or null if no any matched document
     * @throws BaseException If error happens.
     */
    public DBCursor query(String matcher, String selector, String orderBy, String hint,
                          long skipRows, long returnRows) throws BaseException {
        BSONObject ma = null;
        BSONObject se = null;
        BSONObject or = null;
        BSONObject hi = null;
        if (matcher != null) {
            ma = (BSONObject) JSON.parse(matcher);
        }
        if (selector != null) {
            se = (BSONObject) JSON.parse(selector);
        }
        if (orderBy != null) {
            or = (BSONObject) JSON.parse(orderBy);
        }
        if (hint != null) {
            hi = (BSONObject) JSON.parse(hint);
        }
        return query(ma, se, or, hi, skipRows, returnRows, 0);
    }

    /**
     * Get the matching documents in current collection.
     *
     * @param matcher    the matching rule, return all the documents if null
     * @param selector   the selective rule, return the whole document if null
     * @param orderBy    the ordered rule, never sort if null
     * @param hint       Specified the index used to scan data. e.g. {"":"ageIndex"} means using index
     *                   "ageIndex" to scan data(index scan); {"":null} means table scan. when hint is
     *                   null, database automatically match the optimal index to scan data.
     * @param skipRows   skip the first numToSkip documents, never skip if this parameter is 0
     * @param returnRows return the specified amount of documents, when returnRows is 0, return nothing,
     *                   when returnRows is -1, return all the documents
     * @return a DBCursor instance of the result or null if no any matched document
     * @throws BaseException If error happens.
     */
    public DBCursor query(BSONObject matcher, BSONObject selector, BSONObject orderBy,
                          BSONObject hint, long skipRows, long returnRows) throws BaseException {
        return query(matcher, selector, orderBy, hint, skipRows, returnRows, 0);
    }

    /**
     * Get the matching documents in current collection.
     *
     * @param matcher    the matching rule, return all the documents if null
     * @param selector   the selective rule, return the whole document if null
     * @param orderBy    the ordered rule, never sort if null
     * @param hint       Specified the index used to scan data. e.g. {"":"ageIndex"} means using index
     *                   "ageIndex" to scan data(index scan); {"":null} means table scan. when hint is
     *                   null, database automatically match the optimal index to scan data.
     * @param skipRows   skip the first numToSkip documents, never skip if this parameter is 0
     * @param returnRows return the specified amount of documents, when returnRows is 0, return nothing,
     *                   when returnRows is -1, return all the documents
     * @param flags      the query flags, default to be 0. Please see the definition of follow flags for
     *                   more detail. Usage: e.g. set ( DBQuery.FLG_QUERY_FORCE_HINT |
     *                   DBQuery.FLG_QUERY_WITH_RETURNDATA ) to param flag
     *                   <ul>
     *                   <li>DBQuery.FLG_QUERY_STRINGOUT
     *                   <li>DBQuery.FLG_QUERY_FORCE_HINT
     *                   <li>DBQuery.FLG_QUERY_PARALLED
     *                   <li>DBQuery.FLG_QUERY_WITH_RETURNDATA
     *                   <li>DBQuery.FLG_QUERY_FOR_UPDATE
     *                   </ul>
     * @return a DBCursor instance of the result or null if no any matched document
     * @throws BaseException If error happens.
     */
    public DBCursor query(BSONObject matcher, BSONObject selector, BSONObject orderBy,
                          BSONObject hint, long skipRows, long returnRows, int flags) throws BaseException {
        if (flags != 0) {
            flags = DBQuery.eraseSingleFlag(flags, DBQuery.FLG_QUERY_EXPLAIN);
            flags = DBQuery.eraseSingleFlag(flags, DBQuery.FLG_QUERY_MODIFY);
        }
        return _query(matcher, selector, orderBy, hint, skipRows, returnRows, flags);
    }

    private DBCursor _query(BSONObject matcher, BSONObject selector, BSONObject orderBy,
                            BSONObject hint, long skipRows, long returnRows, int flags) throws BaseException {
        int newFlags = DBQuery.regulateFlags(flags);

        if (returnRows < 0) {
            returnRows = -1;
        }
        if (returnRows == 1) {
            newFlags |= DBQuery.FLG_QUERY_WITH_RETURNDATA;
        }
        newFlags |= DBQuery.FLG_QUERY_PREPARE_MORE;

        QueryRequest request = new QueryRequest(collectionFullName, matcher, selector, orderBy,
                hint, skipRows, returnRows, newFlags);
        SdbReply response = sequoiadb.requestAndResponse(request);

        int flag = response.getFlag();
        if (flag != 0) {
            if (flag == SDBError.SDB_DMS_EOC.getErrorCode()) {
                return null;
            } else {
                String msg = "matcher = " + matcher + ", selector = " + selector + ", orderBy = "
                        + orderBy + ", hint = " + hint + ", skipRows = " + skipRows
                        + ", returnRows = " + returnRows + ", flags = " + flags;
                sequoiadb.throwIfError(response, msg);
            }
        }

        sequoiadb.upsertCache(collectionFullName);

        DBCursor cursor = new DBCursor(response, sequoiadb);
        return cursor;
    }

    /**
     * Get one matched document from current collection.
     *
     * @param matcher  the matching rule, return all the documents if null
     * @param selector the selective rule, return the whole document if null
     * @param orderBy  the ordered rule, never sort if null
     * @param hint     Specified the index used to scan data. e.g. {"":"ageIndex"} means using index
     *                 "ageIndex" to scan data(index scan); {"":null} means table scan. when hint is
     *                 null, database automatically match the optimal index to scan data.
     * @param flag     the query flag, default to be 0. Please see the definition of follow flags for
     *                 more detail. Usage: e.g. set ( DBQuery.FLG_QUERY_FORCE_HINT |
     *                 DBQuery.FLG_QUERY_WITH_RETURNDATA ) to param flag
     *                 <ul>
     *                 <li>DBQuery.FLG_QUERY_STRINGOUT
     *                 <li>DBQuery.FLG_QUERY_FORCE_HINT
     *                 <li>DBQuery.FLG_QUERY_PARALLED
     *                 <li>DBQuery.FLG_QUERY_WITH_RETURNDATA
     *                 <li>DBQuery.FLG_QUERY_FOR_UPDATE
     *                 </ul>
     * @return the matched document or null if no such document
     * @throws BaseException If error happens.
     */
    public BSONObject queryOne(BSONObject matcher, BSONObject selector, BSONObject orderBy,
                               BSONObject hint, int flag) throws BaseException {
        flag = flag | DBQuery.FLG_QUERY_WITH_RETURNDATA;
        DBCursor cursor = query(matcher, selector, orderBy, hint, 0, 1, flag);
        BSONObject obj;
        try {
            obj = cursor.getNext();
        } finally {
            cursor.close();
        }
        return obj;
    }

    /**
     * Get one document from current collection.
     *
     * @return the document or null if no any document in current collection
     * @throws BaseException If error happens.
     */
    public BSONObject queryOne() throws BaseException {
        return queryOne(null, null, null, null, 0);
    }

    /**
     * Get all the indexes of current collection
     *
     * @return DBCursor of indexes
     * @throws BaseException If error happens.
     */
    public DBCursor getIndexes() throws BaseException {
        BSONObject obj = new BasicBSONObject();
        obj.put(SdbConstants.FIELD_COLLECTION, collectionFullName);

        AdminRequest request = new AdminRequest(AdminCommand.GET_INDEXES, null, obj);
        SdbReply response = sequoiadb.requestAndResponse(request);

        int flags = response.getFlag();
        if (flags != 0) {
            if (flags == SDBError.SDB_DMS_EOC.getErrorCode()) {
                return null;
            } else {
                sequoiadb.throwIfError(response);
            }
        }

        sequoiadb.upsertCache(collectionFullName);

        DBCursor cursor = new DBCursor(response, sequoiadb);
        return cursor;
    }

    private DBCursor _queryAndModify(BSONObject matcher, BSONObject selector, BSONObject orderBy,
                                     BSONObject hint, BSONObject update, long skipRows, long returnRows, int flag,
                                     boolean isUpdate, boolean returnNew) throws BaseException {
        BSONObject modify = new BasicBSONObject();

        if (isUpdate) {
            if (update == null || update.isEmpty()) {
                throw new BaseException(SDBError.SDB_INVALIDARG, "update can't be empty");
            }

            modify.put(SdbConstants.FIELD_NAME_OP, SdbConstants.FIELD_OP_VALUE_UPDATE);
            modify.put(SdbConstants.FIELD_NAME_OP_UPDATE, update);
            modify.put(SdbConstants.FIELD_NAME_RETURNNEW, returnNew);
        } else {
            modify.put(SdbConstants.FIELD_NAME_OP, SdbConstants.FIELD_OP_VALUE_REMOVE);
            modify.put(SdbConstants.FIELD_NAME_OP_REMOVE, true);
        }

        BSONObject newHint = new BasicBSONObject();
        if (hint != null) {
            newHint.putAll(hint);
        }
        newHint.put(SdbConstants.FIELD_NAME_MODIFY, modify);
        if (flag != 0) {
            flag = DBQuery.eraseSingleFlag(flag, DBQuery.FLG_QUERY_EXPLAIN);
        }
        flag |= DBQuery.FLG_QUERY_MODIFY;
        return _query(matcher, selector, orderBy, newHint, skipRows, returnRows, flag);
    }

    /**
     * Get the matching documents in current collection and update. In order to make the update take
     * effect, user must travel the DBCursor returned by this function.
     *
     * @param matcher    the matching rule, return all the documents if null
     * @param selector   the selective rule, return the whole document if null
     * @param orderBy    the ordered rule, never sort if null
     * @param update     the update rule, can't be null
     * @param hint       Specified the index used to scan data. e.g. {"":"ageIndex"} means using index
     *                   "ageIndex" to scan data(index scan); {"":null} means table scan. when hint is
     *                   null, database automatically match the optimal index to scan data.
     * @param skipRows   skip the first numToSkip documents, never skip if this parameter is 0
     * @param returnRows return the specified amount of documents, when returnRows is 0, return nothing,
     *                   when returnRows is -1, return all the documents
     * @param flag       the query flags, default to be 0. Please see the definition of follow flags for
     *                   more detail. Usage: e.g. set ( DBQuery.FLG_QUERY_FORCE_HINT |
     *                   DBQuery.FLG_QUERY_WITH_RETURNDATA ) to param flag
     *                   <ul>
     *                   <li>DBQuery.FLG_QUERY_STRINGOUT
     *                   <li>DBQuery.FLG_QUERY_FORCE_HINT
     *                   <li>DBQuery.FLG_QUERY_PARALLED
     *                   <li>DBQuery.FLG_QUERY_WITH_RETURNDATA
     *                   <li>DBQuery.FLG_QUERY_KEEP_SHARDINGKEY_IN_UPDATE
     *                   <li>DBQuery.FLG_QUERY_FOR_UPDATE
     *                   </ul>
     * @param returnNew  When true, returns the updated document rather than the original
     * @return a DBCursor instance of the result or null if no any matched document
     * @throws BaseException If error happens.
     */
    public DBCursor queryAndUpdate(BSONObject matcher, BSONObject selector, BSONObject orderBy,
                                   BSONObject hint, BSONObject update, long skipRows, long returnRows, int flag,
                                   boolean returnNew) throws BaseException {
        return _queryAndModify(matcher, selector, orderBy, hint, update, skipRows, returnRows, flag,
                true, returnNew);
    }

    /**
     * Get the matching documents in current collection and remove. In order to make the remove take
     * effect, user must travel the DBCursor returned by this function.
     *
     * @param matcher    the matching rule, return all the documents if null
     * @param selector   the selective rule, return the whole document if null
     * @param orderBy    the ordered rule, never sort if null
     * @param hint       Specified the index used to scan data. e.g. {"":"ageIndex"} means using index
     *                   "ageIndex" to scan data(index scan); {"":null} means table scan. when hint is
     *                   null, database automatically match the optimal index to scan data.
     * @param skipRows   skip the first numToSkip documents, never skip if this parameter is 0
     * @param returnRows return the specified amount of documents, when returnRows is 0, return nothing,
     *                   when returnRows is -1, return all the documents
     * @param flag       the query flag, default to be 0. Please see the definition of follow flags for
     *                   more detail. Usage: e.g. set ( DBQuery.FLG_QUERY_FORCE_HINT |
     *                   DBQuery.FLG_QUERY_WITH_RETURNDATA ) to param flag
     *                   <ul>
     *                   <li>DBQuery.FLG_QUERY_STRINGOUT
     *                   <li>DBQuery.FLG_QUERY_FORCE_HINT
     *                   <li>DBQuery.FLG_QUERY_PARALLED
     *                   <li>DBQuery.FLG_QUERY_WITH_RETURNDATA
     *                   <li>DBQuery.FLG_QUERY_FOR_UPDATE
     *                   </ul>
     * @return a DBCursor instance of the result or null if no any matched document
     * @throws BaseException If error happens.
     */
    public DBCursor queryAndRemove(BSONObject matcher, BSONObject selector, BSONObject orderBy,
                                   BSONObject hint, long skipRows, long returnRows, int flag) throws BaseException {
        return _queryAndModify(matcher, selector, orderBy, hint, null, skipRows, returnRows, flag,
                false, false);
    }

    /**
     * Get all of or one of the indexes in current collection.
     *
     * @param indexName The index indexName, returns all of the indexes if this parameter is null.
     * @return DBCursor of indexes.
     * @throws BaseException If error happens.
     * @deprecated use "getIndexInfo" or "getIndexes" API instead.
     */
    @Deprecated
    public DBCursor getIndex(String indexName) throws BaseException {
        if (indexName == null) {
            return getIndexes();
        }

        BSONObject condition = new BasicBSONObject();
        condition.put(SdbConstants.IXM_INDEXDEF + "." + SdbConstants.IXM_NAME, indexName);

        BSONObject obj = new BasicBSONObject();
        obj.put(SdbConstants.FIELD_COLLECTION, collectionFullName);

        AdminRequest request = new AdminRequest(AdminCommand.GET_INDEXES, condition, obj);
        SdbReply response = sequoiadb.requestAndResponse(request);

        int flags = response.getFlag();
        if (flags != 0) {
            if (flags == SDBError.SDB_DMS_EOC.getErrorCode()) {
                return null;
            } else {
                sequoiadb.throwIfError(response);
            }
        }
        sequoiadb.upsertCache(collectionFullName);
        return new DBCursor(response, sequoiadb);
    }

    /**
     * Get the information of specified index in current collection.
     *
     * @param name The index name.
     * @return The information of the specified index.
     * @throws BaseException If error happens or the specified index does not exist.
     */
    public BSONObject getIndexInfo(String name) throws BaseException {
        BSONObject retObj;
        if (name == null || name.isEmpty()) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "index name can not be null or empty");
        }

        BSONObject condition = new BasicBSONObject();
        condition.put(SdbConstants.IXM_INDEXDEF + "." + SdbConstants.IXM_NAME, name);

        BSONObject obj = new BasicBSONObject();
        obj.put(SdbConstants.FIELD_COLLECTION, collectionFullName);

        AdminRequest request = new AdminRequest(AdminCommand.GET_INDEXES, condition, obj);
        SdbReply response = sequoiadb.requestAndResponse(request);
        if (response.getFlag() != 0) {
            sequoiadb.throwIfError(response);
        }
        sequoiadb.upsertCache(collectionFullName);
        DBCursor cursor = new DBCursor(response, sequoiadb);
        try {
            if (cursor.hasNext()) {
                retObj = cursor.getNext();
                return retObj;
            } else {
                throw new BaseException(SDBError.SDB_IXM_NOTEXIST,
                        "the specified index[" + name + "] does not exist");
            }
        } finally {
            cursor.close();
        }
    }

    /**
     * Get the statistics of the index.
     *
     * @param name The index name.
     * @return The statistics of the specified index.
     * @throws BaseException If error happens.
     */
    public BSONObject getIndexStat(String name) throws BaseException {
        if (name == null || name.isEmpty()) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "index name can not be null or empty");
        }
        BSONObject hint = new BasicBSONObject();
        hint.put(SdbConstants.FIELD_COLLECTION, collectionFullName);
        hint.put(SdbConstants.FIELD_INDEX, name);

        AdminRequest request = new AdminRequest(AdminCommand.GET_INDEX_STAT, null, hint);
        SdbReply response = sequoiadb.requestAndResponse(request);
        if (response.getFlag() != 0) {
            sequoiadb.throwIfError(response);
        }
        sequoiadb.upsertCache(collectionFullName);
        DBCursor cursor = new DBCursor(response, sequoiadb);
        try {
            if (cursor.hasNext()) {
                return cursor.getNext();
            } else {
                throw new BaseException(SDBError.SDB_IXM_STAT_NOTEXIST);
            }
        } finally {
            cursor.close();
        }
    }

    /**
     * Test the specified index exist or not.
     *
     * @param name The index name.
     * @return True for exist while false for not exist..
     */
    public boolean isIndexExist(String name) {
        if (name == null || name.isEmpty()) {
            return false;
        }
        BSONObject indexObj;
        try {
            indexObj = getIndexInfo(name);
        } catch (Exception e) {
            return false;
        }
        if (indexObj != null) {
            return true;
        }
        return false;
    }

    private long _createIndex(String indexName, BSONObject indexKeys, BSONObject indexAttr, BSONObject option, boolean isAsync) throws BaseException {
        BSONObject matcher = new BasicBSONObject();
        BSONObject hint = new BasicBSONObject();
        BSONObject indexObj = new BasicBSONObject();

        // we are going to build the below message:
        // matcher: { Collection: "foo.bar",
        //           Index:{ key: {a:1}, name: 'aIdx', Unique: true,
        //                   Enforced: true, NotNull: true },
        //           SortBufferSize: 1024 }
        // hint: { SortBufferSize: 1024 }
        // For Compatibility with older engine( version <3.4 ), keep sort buffer
        // size in hint. After several versions, we can delete it.

        indexObj.put(SdbConstants.IXM_KEY, indexKeys);
        indexObj.put(SdbConstants.IXM_NAME, indexName);
        if (indexAttr != null){
            indexObj.putAll(indexAttr);
        }

        matcher.put(SdbConstants.FIELD_COLLECTION, collectionFullName);
        matcher.put(SdbConstants.FIELD_INDEX, indexObj);
        if (option != null){
            matcher.putAll(option);
        }
        if (isAsync){
            matcher.put(SdbConstants.FIELD_NAME_ASYNC, true);
        }else {
            matcher.put(SdbConstants.FIELD_NAME_ASYNC, false);
        }

        // In order to be compatible with the old version engine
        // hint needs to contain sortBufferSize
        if (option != null && option.containsField(SdbConstants.IXM_FIELD_NAME_SORT_BUFFER_SIZE)) {
            Object value = option.get(SdbConstants.IXM_FIELD_NAME_SORT_BUFFER_SIZE);
            int sortBufferSize;
            if (value instanceof Integer) {
                sortBufferSize = (int)value;
            } else {
                throw new BaseException(SDBError.SDB_INVALIDARG, "sortBufferSize should be int value");
            }
            hint.put(SdbConstants.IXM_FIELD_NAME_SORT_BUFFER_SIZE, sortBufferSize);
        }

        AdminRequest request = new AdminRequest(AdminCommand.CREATE_INDEX, matcher, hint);
        SdbReply response = sequoiadb.requestAndResponse(request);
        // check return info
        if (response.getFlag() != 0) {
            String msg = "matcher = " + matcher.toString() + ", hint = " + hint.toString();
            sequoiadb.throwIfError(response, msg);
        }
        long taskId = 0;
        if (isAsync){
            Object obj = sequoiadb.getObjectFromResp(response, SdbConstants.FIELD_NAME_TASKID);
            if (obj == null){
                throw new BaseException(SDBError.SDB_UNEXPECTED_RESULT);
            }
            taskId = (Long) obj;
        }
        sequoiadb.upsertCache(collectionFullName);
        return taskId;
    }

    /**
     * Create a index with name and key.
     *
     * @param indexName The index name.
     * @param indexKeys The index keys in JSON format, like: { "a":1, "b":-1 }.
     * @param indexAttr The attributes are as below:
     *                  <ul>
     *                  <li>Unique : Whether the index elements are unique or not
     *                  <li>Enforced : Whether the index is enforced unique, this element is meaningful when Unique
     *                  is true
     *                  <li>NotNull : Any field of index key should exist and cannot be null when NotNull is true
     *                  </ul>
     * @throws BaseException
     */
    public void createIndex(String indexName, BSONObject indexKeys, BSONObject indexAttr) throws BaseException {
        int sortBufferSize ;
        BSONObject attribute = new BasicBSONObject();
        if (indexAttr != null){
            attribute.putAll(indexAttr);
        }
        // Compatible with the old version(3.4.3 before)
        // indexAttr had SortBufferSize field
        if ( attribute.containsField(SdbConstants.IXM_FIELD_NAME_SORT_BUFFER_SIZE)) {
            Object value = attribute.get(SdbConstants.IXM_FIELD_NAME_SORT_BUFFER_SIZE);
            if (value instanceof Integer) {
                sortBufferSize = (int)value;
            } else {
                throw new BaseException(SDBError.SDB_INVALIDARG, "sortBufferSize should be int value");
            }
            attribute.removeField(SdbConstants.IXM_FIELD_NAME_SORT_BUFFER_SIZE);
        } else {
            sortBufferSize = SdbConstants.IXM_SORT_BUFFER_DEFAULT_SIZE;
        }
        BSONObject option = new BasicBSONObject();
        option.put(SdbConstants.IXM_FIELD_NAME_SORT_BUFFER_SIZE, sortBufferSize);

        createIndex(indexName, indexKeys, attribute, option);
    }

    /**
     * Create a index with indexName and indexKeys.
     *
     * @param indexName      The index indexName.
     * @param indexKeys      The index keys in JSON format, like: { "a":1, "b":-1 }.
     * @param isUnique       Whether the index elements are unique or not.
     * @param enforced       Whether the index is enforced unique, this element is meaningful when isUnique is
     *                       set to true.
     * @param sortBufferSize The size(MB) of sort buffer used when creating index, zero means don't use sort
     *                       buffer.
     * @throws BaseException If error happens.
     */
    public void createIndex(String indexName, BSONObject indexKeys, boolean isUnique, boolean enforced,
                            int sortBufferSize) throws BaseException {
        BSONObject option = new BasicBSONObject();
        BSONObject indexAttr = new BasicBSONObject();
        indexAttr.put(SdbConstants.IXM_UNIQUE, isUnique);
        indexAttr.put(SdbConstants.IXM_ENFORCED, enforced);
        option.put(SdbConstants.IXM_FIELD_NAME_SORT_BUFFER_SIZE, sortBufferSize);
        createIndex(indexName, indexKeys, indexAttr, option);
    }

    /**
     * Create a index with indexName and indexKeys.
     *
     * @param indexName      The index indexName.
     * @param indexKeys      The index keys in JSON format, like: "{\"a\":1, \"b\":-1}".
     * @param isUnique       Whether the index elements are unique or not.
     * @param enforced       Whether the index is enforced unique, this element is meaningful when isUnique is
     *                       set to true.
     * @param sortBufferSize The size(MB) of sort buffer used when creating index, zero means don't use sort
     *                       buffer.
     * @throws BaseException If error happens.
     */
    public void createIndex(String indexName, String indexKeys, boolean isUnique, boolean enforced,
                            int sortBufferSize) throws BaseException {
        BSONObject k = null;
        if (indexKeys != null) {
            k = (BSONObject) JSON.parse(indexKeys);
        }
        createIndex(indexName, k, isUnique, enforced, sortBufferSize);
    }

    /**
     * Create a index with indexName and indexKeys
     *
     * @param indexName The index indexName.
     * @param indexKeys The index keys in JSON format, like: { "a":1, "b":-1}.
     * @param isUnique Whether the index elements are unique or not.
     * @param enforced Whether the index is enforced unique, this element is meaningful when isUnique is
     *                  set to true.
     * @throws BaseException If error happens.
     */
    public void createIndex(String indexName, BSONObject indexKeys, boolean isUnique, boolean enforced)
            throws BaseException {
        createIndex(indexName, indexKeys, isUnique, enforced, SdbConstants.IXM_SORT_BUFFER_DEFAULT_SIZE);
    }

    /**
     * Create a index with indexName and indexKeys.
     *
     * @param indexName The index indexName.
     * @param indexKeys The index keys in JSON format, like: "{\"a\":1, \"b\":-1}".
     * @param isUnique Whether the index elements are unique or not.
     * @param enforced Whether the index is enforced unique, this element is meaningful when isUnique is
     *                 set to true.
     * @throws BaseException If error happens.
     */
    public void createIndex(String indexName, String indexKeys, boolean isUnique, boolean enforced)
            throws BaseException {
        BSONObject k = null;
        if (indexKeys != null) {
            k = (BSONObject) JSON.parse(indexKeys);
        }
        createIndex(indexName, k, isUnique, enforced, SdbConstants.IXM_SORT_BUFFER_DEFAULT_SIZE);
    }

    /**
     * Create the index in current collection
     *
     * @param indexName The index indexName.
     * @param indexKeys The index keys in JSON format, like: { "a":1, "b":-1}.
     * @param indexAttr The attributes are as below:
     *                  <ul>
     *                  <li>Unique : Whether the index elements are unique or not
     *                  <li>Enforced : Whether the index is enforced unique, this element is meaningful when Unique
     *                  is true
     *                  <li>NotNull : Any field of index key should exist and cannot be null when NotNull is true
     *                  </ul>
     * @param option The options are as below:
     *                  <ul>
     *                  <li>SortBufferSize : The size(MB) of sort buffer used when creating index, zero means don't
     *                  use sort buffer.
     *                  </ul>
     * @throws BaseException If error happens.
     */
    public void createIndex(String indexName, BSONObject indexKeys, BSONObject indexAttr, BSONObject option) throws BaseException {
        _createIndex(indexName, indexKeys, indexAttr, option, false);
    }

    /**
     * Create the index in current collection
     *
     * @param indexName The index indexName.
     * @param indexKeys The index keys in JSON format, like: { "a":1, "b":-1}.
     * @param indexAttr The attributes are as below:
     *                  <ul>
     *                  <li>Unique : Whether the index elements are unique or not
     *                  <li>Enforced : Whether the index is enforced unique, this element is meaningful when Unique
     *                  is true
     *                  <li>NotNull : Any field of index key should exist and cannot be null when NotNull is true
     *                  </ul>
     * @param option The options are as below:
     *                  <ul>
     *                  <li>SortBufferSize : The size(MB) of sort buffer used when creating index, zero means don't
     *                  use sort buffer.
     *                  </ul>
     * @return The id of current task
     * @throws BaseException If error happens.
     */
    public long createIndexAsync(String indexName, BSONObject indexKeys, BSONObject indexAttr,
                                 BSONObject option) throws BaseException {
        return _createIndex(indexName, indexKeys, indexAttr, option, true);
    }

    private long _dropIndex(String indexName, boolean isAsync) throws BaseException {
        if (indexName == null || indexName.isEmpty()) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "index name can not be null or empty");
        }
        BSONObject index = new BasicBSONObject();
        index.put("", indexName);

        BSONObject dropObj = new BasicBSONObject();
        dropObj.put(SdbConstants.FIELD_COLLECTION, collectionFullName);
        dropObj.put(SdbConstants.FIELD_INDEX, index);
        if (isAsync){
            dropObj.put(SdbConstants.FIELD_NAME_ASYNC, true);
        }else {
            dropObj.put(SdbConstants.FIELD_NAME_ASYNC, false);
        }

        AdminRequest request = new AdminRequest(AdminCommand.DROP_INDEX, dropObj);
        SdbReply response = sequoiadb.requestAndResponse(request);
        sequoiadb.throwIfError(response, indexName);

        long taskId = 0;
        if (isAsync){
            Object obj = sequoiadb.getObjectFromResp(response, SdbConstants.FIELD_NAME_TASKID);
            if (obj == null){
                throw new BaseException(SDBError.SDB_UNEXPECTED_RESULT);
            }
            taskId = (Long) obj;
        }
        sequoiadb.upsertCache(collectionFullName);
        return taskId;
    }

    /**
     * Remove the named index of current collection.
     *
     * @param indexName The index indexName.
     * @throws BaseException If error happens.
     */
    public void dropIndex(String indexName) throws BaseException {
        _dropIndex(indexName, false);
    }

    /**
     * Remove the named index of current collection.
     *
     * @param indexName The index indexName.
     * @return The id of current task.
     * @throws BaseException If error happens.
     */
    public long dropIndexAsync(String indexName) throws BaseException {
        return _dropIndex(indexName, true);
    }

    /**
     * Snapshot all of or one of the indexes in current collection
     *
     * @param matcher    The matching rule, match all the documents if not provided
     * @param selector   The selective rule, return the whole document if not provided
     * @param orderBy    The ordered rule, result set is unordered if not provided
     * @param hint       The hint rule, the options provided for specific snapshot type format:{
     *                   '$Options': { <options> } }
     * @param skipRows   Skip the first numToSkip documents, never skip if this parameter is 0
     * @param returnRows Return the specified amount of documents, default is -1 for returning all results
     * @return DBCursor of current query
     * @throws BaseException If error happens.
     */
    public DBCursor snapshotIndexes(BSONObject matcher, BSONObject selector, BSONObject orderBy, BSONObject hint,
                                    long skipRows, long returnRows) throws BaseException {
        BSONObject hintObj = new BasicBSONObject();
        hintObj.put(SdbConstants.FIELD_COLLECTION, this.collectionFullName);
        if (hint != null){
            hintObj.putAll(hint);
        }
        return sequoiadb.getSnapshot(Sequoiadb.SDB_SNAP_INDEXES, matcher, selector, orderBy,
                                     hintObj, skipRows, returnRows);
    }

    private long _copyIndex(String subClName, String indexName, boolean isAsync) throws BaseException {
        BSONObject copyObj = new BasicBSONObject();
        copyObj.put(SdbConstants.FIELD_NAME_NAME,collectionFullName);
        if (isAsync){
            copyObj.put(SdbConstants.FIELD_NAME_ASYNC, true);
        }else {
            copyObj.put(SdbConstants.FIELD_NAME_ASYNC, false);
        }
        if (subClName != null && !subClName.isEmpty()){
            copyObj.put(SdbConstants.FIELD_NAME_SUBCLNAME, subClName);
        }
        if (indexName != null && !indexName.isEmpty()){
            copyObj.put(SdbConstants.FIELD_NAME_INDEXNAME, indexName);
        }

        AdminRequest request = new AdminRequest(AdminCommand.COPY_INDEX, copyObj);
        SdbReply response = sequoiadb.requestAndResponse(request);
        sequoiadb.throwIfError(response);

        long taskId = 0;
        if (isAsync){
            Object obj = sequoiadb.getObjectFromResp(response, SdbConstants.FIELD_NAME_TASKID);
            if (obj == null){
                throw new BaseException(SDBError.SDB_UNEXPECTED_RESULT);
            }
            taskId = (Long)obj;
        }
        sequoiadb.upsertCache(collectionFullName);
        return taskId;
    }

    /**
     * copy indexes from main-collection to sub-collection
     *
     * @param subClName The sub-collection name, if it is null or an empty string, it means all sub-collections of
     *                   the main-collection
     * @param indexName The index indexName, if it is null or an empty string, it means all indexes of the
     *                   main-collection
     * @throws BaseException If error happens.
     */
    public void copyIndex(String subClName, String indexName) throws BaseException {
        _copyIndex( subClName, indexName, false);
    }

    /**
     * copy indexes from main-collection to sub-collection
     *
     * @param subClName The sub-collection name, if it is null or an empty string, it means all sub-collections of
     *                   the main-collection
     * @param indexName The index indexName, if it is null or an empty string, it means all indexes of the
     *                   main-collection
     * @return The id of current task.
     * @throws BaseException If error happens.
     */
    public long copyIndexAsync(String subClName, String indexName) throws BaseException {
        return _copyIndex( subClName, indexName, true);
    }

    /**
     * Get the amount of documents in current collection.
     *
     * @return the amount of matching documents
     * @throws BaseException If error happens.
     */
    public long getCount() throws BaseException {
        return getCount("");
    }

    /**
     * Get the amount of matching documents in current collection.
     *
     * @param matcher the matching rule
     * @return the amount of matching documents
     * @throws BaseException If error happens.
     */
    public long getCount(String matcher) throws BaseException {
        BSONObject con = null;
        if (matcher != null) {
            con = (BSONObject) JSON.parse(matcher);
        }
        return getCount(con);
    }

    /**
     * Get the amount of matching documents in current collection.
     *
     * @param matcher The matching rule, when condition is null, the return amount contains all the
     *                records.
     * @return the amount of matching documents
     * @throws BaseException If error happens.
     */
    public long getCount(BSONObject matcher) throws BaseException {
        return getCount(matcher, null);
    }

    /**
     * Get the count of matching BSONObject in current collection.
     *
     * @param matcher The matching rule, when condition is null, the return amount contains all the
     *                records.
     * @param hint    Specified the index used to scan data. e.g. {"":"ageIndex"} means using index
     *                "ageIndex" to scan data(index scan); {"":null} means table scan. when hint is
     *                null, database automatically match the optimal index to scan data.
     * @return The count of matching BSONObjects
     * @throws BaseException If error happens.
     */
    public long getCount(BSONObject matcher, BSONObject hint) throws BaseException {
        BSONObject newHint = new BasicBSONObject();
        newHint.put(SdbConstants.FIELD_COLLECTION, collectionFullName);
        if (null != hint) {
            newHint.put(SdbConstants.FIELD_NAME_HINT, hint);
        }

        QueryRequest request = new QueryRequest(AdminCommand.GET_COUNT,
                matcher, null, null, newHint, -1, -1,
                DBQuery.FLG_QUERY_WITH_RETURNDATA );
        SdbReply response = sequoiadb.requestAndResponse(request);

        if (response.getFlag() != 0) {
            String msg = "condition = " + matcher + ", hint = " + hint;
            sequoiadb.throwIfError(response, msg);
        }

        sequoiadb.upsertCache(collectionFullName);

        DBCursor cursor = new DBCursor(response, sequoiadb);
        BSONObject object;
        try {
            object = cursor.getNext();
        } finally {
            cursor.close();
        }
        return (Long) object.get(SdbConstants.FIELD_TOTAL);
    }

    /**
     * Split the specified collection from source group to target group by range.
     *
     * @param sourceGroupName   the source group name
     * @param destGroupName     the destination group name
     * @param splitCondition    the split condition
     * @param splitEndCondition the split end condition or null, only usable when "ShardingType" is "range". eg:If
     *                          we create a collection with the option
     *                          {ShardingKey:{"age":1},ShardingType:"range"}, we can fill {age:30} as the
     *                          splitCondition, and fill {age:60} as the splitEndCondition. when split, the target
     *                          group will get the records whose age's hash value are in [30,60). If
     *                          splitEndCondition is null, they are in [30,max).
     * @throws BaseException If error happens.
     */
    public void split(String sourceGroupName, String destGroupName, BSONObject splitCondition,
                      BSONObject splitEndCondition) throws BaseException {
        if ((null == sourceGroupName || sourceGroupName.equals(""))
                || (null == destGroupName || destGroupName.equals("")) || null == splitCondition) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "null parameter");
        }

        BSONObject obj = new BasicBSONObject();
        obj.put(SdbConstants.FIELD_NAME_NAME, collectionFullName);
        obj.put(SdbConstants.FIELD_NAME_SOURCE, sourceGroupName);
        obj.put(SdbConstants.FIELD_NAME_TARGET, destGroupName);
        obj.put(SdbConstants.FIELD_NAME_SPLITQUERY, splitCondition);
        if (null != splitEndCondition) {
            obj.put(SdbConstants.FIELD_NAME_SPLITENDQUERY, splitEndCondition);
        }

        AdminRequest request = new AdminRequest(AdminCommand.SPLIT, obj);
        SdbReply response = sequoiadb.requestAndResponse(request);

        if (response.getFlag() != 0) {
            String msg = "sourceGroupName = " + sourceGroupName + ", destGroupName = "
                    + destGroupName + ", splitCondition = " + splitCondition
                    + ", splitEndCondition = " + splitEndCondition;
            sequoiadb.throwIfError(response, msg);
        }

        sequoiadb.upsertCache(collectionFullName);
    }

    /**
     * Split the specified collection from source group to target group by percent.
     *
     * @param sourceGroupName the source group name
     * @param destGroupName   the destination group name
     * @param percent         the split percent, Range:(0,100]
     * @throws BaseException If error happens.
     */
    public void split(String sourceGroupName, String destGroupName, double percent)
            throws BaseException {
        if ((null == sourceGroupName || sourceGroupName.equals(""))
                || (null == destGroupName || destGroupName.equals(""))
                || (percent <= 0.0 || percent > 100.0)) {
            throw new BaseException(SDBError.SDB_INVALIDARG);
        }

        BSONObject obj = new BasicBSONObject();
        obj.put(SdbConstants.FIELD_NAME_NAME, collectionFullName);
        obj.put(SdbConstants.FIELD_NAME_SOURCE, sourceGroupName);
        obj.put(SdbConstants.FIELD_NAME_TARGET, destGroupName);
        obj.put(SdbConstants.FIELD_NAME_SPLITPERCENT, percent);

        AdminRequest request = new AdminRequest(AdminCommand.SPLIT, obj);
        SdbReply response = sequoiadb.requestAndResponse(request);

        if (response.getFlag() != 0) {
            String msg = "sourceGroupName = " + sourceGroupName + ", destGroupName = "
                    + destGroupName + ", percent = " + percent;
            sequoiadb.throwIfError(response, msg);
        }

        sequoiadb.upsertCache(collectionFullName);
    }

    /**
     * Split the specified collection from source group to target group by range asynchronously.
     *
     * @param sourceGroupName   the source group name
     * @param destGroupName     the destination group name
     * @param splitCondition    the split condition
     * @param splitEndCondition the split end condition or null, only usable when "ShardingType" is "range". eg:If
     *                          we create a collection with the option
     *                          {ShardingKey:{"age":1},ShardingType:"range"}, we can fill {age:30} as the
     *                          splitCondition, and fill {age:60} as the splitEndCondition. when split, the target
     *                          group will get the records whose age's hash values are in [30,60). If
     *                          splitEndCondition is null, they are in [30,max).
     * @return return the task id, we can use the return id to manage the sharding which is run
     * background.
     * @throws BaseException If error happens.
     * @see Sequoiadb#listTasks(BSONObject, BSONObject, BSONObject, BSONObject)
     * @see Sequoiadb#cancelTask(long, boolean)
     */
    public long splitAsync(String sourceGroupName, String destGroupName, BSONObject splitCondition,
                           BSONObject splitEndCondition) throws BaseException {
        if ((null == sourceGroupName || sourceGroupName.equals(""))
                || (null == destGroupName || destGroupName.equals("")) || null == splitCondition) {
            throw new BaseException(SDBError.SDB_INVALIDARG);
        }

        BSONObject obj = new BasicBSONObject();
        obj.put(SdbConstants.FIELD_NAME_NAME, collectionFullName);
        obj.put(SdbConstants.FIELD_NAME_ASYNC, true);
        obj.put(SdbConstants.FIELD_NAME_SOURCE, sourceGroupName);
        obj.put(SdbConstants.FIELD_NAME_TARGET, destGroupName);
        obj.put(SdbConstants.FIELD_NAME_SPLITQUERY, splitCondition);
        if (null != splitEndCondition) {
            obj.put(SdbConstants.FIELD_NAME_SPLITENDQUERY, splitEndCondition);
        }

        AdminRequest request = new AdminRequest(AdminCommand.SPLIT, obj);
        SdbReply response = sequoiadb.requestAndResponse(request);

        if (response.getFlag() != 0) {
            String msg = "sourceGroupName = " + sourceGroupName + ", destGroupName = "
                    + destGroupName + ", splitCondition = " + splitCondition
                    + ", splitEndCondition = " + splitEndCondition;
            sequoiadb.throwIfError(response, msg);
        }
        BSONObject result;
        DBCursor cursor = new DBCursor(response, sequoiadb);
        try {
            if (!cursor.hasNext()) {
                throw new BaseException(SDBError.SDB_CAT_TASK_NOTFOUND);
            }
            result = cursor.getNext();
        } finally {
            cursor.close();
        }
        boolean flag = result.containsField(SdbConstants.FIELD_NAME_TASKID);
        if (!flag) {
            throw new BaseException(SDBError.SDB_CAT_TASK_NOTFOUND);
        }

        sequoiadb.upsertCache(collectionFullName);
        long taskid = (Long) result.get(SdbConstants.FIELD_NAME_TASKID);
        return taskid;
    }

    /**
     * Split the specified collection from source group to target group by percent asynchronously.
     *
     * @param sourceGroupName the source group name
     * @param destGroupName   the destination group name
     * @param percent         the split percent, Range:(0,100]
     * @return return the task id, we can use the return id to manage the sharding which is run
     * background.
     * @throws BaseException If error happens.
     */
    public long splitAsync(String sourceGroupName, String destGroupName, double percent)
            throws BaseException {
        if ((null == sourceGroupName || sourceGroupName.equals(""))
                || (null == destGroupName || destGroupName.equals(""))
                || (percent <= 0.0 || percent > 100.0)) {
            throw new BaseException(SDBError.SDB_INVALIDARG);
        }

        BSONObject obj = new BasicBSONObject();
        obj.put(SdbConstants.FIELD_NAME_NAME, collectionFullName);
        obj.put(SdbConstants.FIELD_NAME_ASYNC, true);
        obj.put(SdbConstants.FIELD_NAME_SOURCE, sourceGroupName);
        obj.put(SdbConstants.FIELD_NAME_TARGET, destGroupName);
        obj.put(SdbConstants.FIELD_NAME_SPLITPERCENT, percent);

        AdminRequest request = new AdminRequest(AdminCommand.SPLIT, obj);
        SdbReply response = sequoiadb.requestAndResponse(request);

        if (response.getFlag() != 0) {
            String msg = "sourceGroupName = " + sourceGroupName + ", destGroupName = "
                    + destGroupName + ", percent = " + percent;
            sequoiadb.throwIfError(response, msg);
        }
        BSONObject result;
        DBCursor cursor = new DBCursor(response, sequoiadb);
        try {
            if (!cursor.hasNext()) {
                throw new BaseException(SDBError.SDB_CAT_TASK_NOTFOUND);
            }
            result = cursor.getNext();
        } finally {
            cursor.close();
        }
        boolean flag = result.containsField(SdbConstants.FIELD_NAME_TASKID);
        if (!flag) {
            throw new BaseException(SDBError.SDB_CAT_TASK_NOTFOUND);
        }

        sequoiadb.upsertCache(collectionFullName);

        long taskid = (Long) result.get(SdbConstants.FIELD_NAME_TASKID);
        return taskid;
    }

    /**
     * Execute aggregate operation in current collection.
     *
     * @param objs The Bson object of rule list, can't be null
     * @throws BaseException If error happens.
     */
    public DBCursor aggregate(List<BSONObject> objs) throws BaseException {
        if (objs == null || objs.size() == 0) {
            throw new BaseException(SDBError.SDB_INVALIDARG);
        }

        AggregateRequest request = new AggregateRequest(collectionFullName, objs);
        SdbReply response = sequoiadb.requestAndResponse(request);

        int flags = response.getFlag();
        if (flags != 0) {
            if (flags == SDBError.SDB_DMS_EOC.getErrorCode()) {
                return null;
            } else {
                sequoiadb.throwIfError(response, objs);
            }
        }

        sequoiadb.upsertCache(collectionFullName);

        DBCursor cursor = new DBCursor(response, sequoiadb);
        return cursor;
    }

    /**
     * Get index blocks' or data blocks' information for concurrent query.
     *
     * @param matcher    the matching rule, return all the meta information if null
     * @param orderBy    the ordered rule, never sort if null
     * @param hint       Specified the index used to scan data. e.g. {"":"ageIndex"} means using index
     *                   "ageIndex" to scan data(index scan); {"":null} means table scan. when hint is
     *                   null, database automatically match the optimal index to scan data.
     * @param skipRows   The rows to be skipped
     * @param returnRows return the specified amount of documents, when returnRows is 0, return nothing,
     *                   when returnRows is -1, return all the documents
     * @param flag       The flag to use which form for record data 0: bson stream 1: binary data stream,
     *                   form: col1|col2|col3
     * @return DBCursor of data
     * @throws BaseException If error happens.
     */
    public DBCursor getQueryMeta(BSONObject matcher, BSONObject orderBy, BSONObject hint,
                                 long skipRows, long returnRows, int flag) throws BaseException {
        BSONObject newHint = new BasicBSONObject();
        newHint.put("Collection", this.collectionFullName);
        if (null == hint || hint.isEmpty()) {
            BSONObject empty = new BasicBSONObject();
            newHint.put("Hint", empty);
        } else {
            newHint.put("Hint", hint);
        }

        QueryRequest request = new QueryRequest(AdminCommand.GET_QUERYMETA, matcher, null, orderBy,
                newHint, skipRows, returnRows, flag);
        SdbReply response = sequoiadb.requestAndResponse(request);

        int flags = response.getFlag();
        if (flags != 0) {
            if (flags == SDBError.SDB_DMS_EOC.getErrorCode()) {
                return null;
            } else {
                String msg = "query = " + matcher + ", hint = " + hint + ", orderBy = " + orderBy
                        + ", skipRows = " + skipRows + ", returnRows = " + returnRows;
                sequoiadb.throwIfError(response, msg);
            }
        }

        sequoiadb.upsertCache(collectionFullName);

        DBCursor cursor = new DBCursor(response, sequoiadb);
        return cursor;
    }

    /**
     * Attach the specified collection.
     *
     * @param subClFullName The full name of the sub-collection
     * @param options       The low boundary and up boundary eg: {"LowBound":{a:1},"UpBound":{a:100}}
     * @throws BaseException If error happens.
     */
    public void attachCollection(String subClFullName, BSONObject options) throws BaseException {
        if (null == subClFullName || subClFullName.equals("") || null == options
                || null == collectionFullName || collectionFullName.equals("")) {
            throw new BaseException(SDBError.SDB_INVALIDARG,
                    "sub collection name or options is empty or null");
        }

        BSONObject newOptions = new BasicBSONObject();
        newOptions.put(SdbConstants.FIELD_NAME_NAME, collectionFullName);
        newOptions.put(SdbConstants.FIELD_NAME_SUBCLNAME, subClFullName);
        newOptions.putAll(options);

        AdminRequest request = new AdminRequest(AdminCommand.ATTACH_CL, newOptions);
        SdbReply response = sequoiadb.requestAndResponse(request);

        if (response.getFlag() != 0) {
            String msg = "subCollectionName = " + subClFullName + ", options = " + options;
            sequoiadb.throwIfError(response, msg);
        }

        sequoiadb.upsertCache(collectionFullName);
    }

    /**
     * Detach the specified collection.
     *
     * @param subClFullName The full name of the sub-collection
     * @throws BaseException If error happens.
     */
    public void detachCollection(String subClFullName) throws BaseException {
        if (null == subClFullName || subClFullName.equals("") || null == collectionFullName
                || collectionFullName.equals("")) {
            throw new BaseException(SDBError.SDB_INVALIDARG, subClFullName);
        }

        BSONObject options = new BasicBSONObject();
        options.put(SdbConstants.FIELD_NAME_NAME, collectionFullName);
        options.put(SdbConstants.FIELD_NAME_SUBCLNAME, subClFullName);

        AdminRequest request = new AdminRequest(AdminCommand.DETACH_CL, options);
        SdbReply response = sequoiadb.requestAndResponse(request);
        sequoiadb.throwIfError(response, subClFullName);
        sequoiadb.upsertCache(collectionFullName);
    }

    private void alterInternal(String taskName, BSONObject options, boolean allowNullArgs)
            throws BaseException {
        if (null == options && !allowNullArgs) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "options is null");
        }
        BSONObject argumentObj = new BasicBSONObject();
        argumentObj.put(SdbConstants.FIELD_NAME_NAME, taskName);
        argumentObj.put(SdbConstants.FIELD_NAME_ARGS, options);
        BSONObject alterObject = new BasicBSONObject();
        alterObject.put(SdbConstants.FIELD_NAME_ALTER, argumentObj);
        alterCollection(alterObject);
    }

    /**
     * Alter the attributes of current collection. Can't alter attributes about split in partition
     * collection; After altering a collection to be a partition collection, need to split this
     * collection manually.
     *
     * @param options The options for altering current collection are as below:
     *                <ul>
     *                <li>ReplSize : Assign how many replica nodes need to be synchronized when a write
     *                request(insert, update, etc) is executed
     *                <li>ShardingKey : Assign the sharding key
     *                <li>ShardingType : Assign the sharding type
     *                <li>Partition : When the ShardingType is "hash", need to assign Partition, it's
     *                the bucket number for hash, the range is [2^3,2^20].
     *                <li>CompressionType : The compression type of data, could be "snappy" or "lzw"
     *                <li>EnsureShardingIndex : Assign to true to build sharding index
     *                <li>StrictDataMode : Using strict date mode in numeric operations or not e.g.
     *                {RepliSize:0, ShardingKey:{a:1}, ShardingType:"hash", Partition:1024}
     *                <li>AutoIncrement : Assign attributes of an autoincrement field or batch
     *                autoincrement fields. e.g.
     *                {AutoIncrement:{Field:"a",MaxValue:2000}},{AutoIncrement:[{Field:"a",MaxValue:2000},{Field:"a",MaxValue:4000}]}
     *                </ul>
     * @throws BaseException If error happens.
     */
    public void alterCollection(BSONObject options) throws BaseException {
        if (null == options) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "options is null");
        }

        BSONObject newObj = new BasicBSONObject();
        if (!options.containsField(SdbConstants.FIELD_NAME_ALTER)) {
            newObj.put(SdbConstants.FIELD_NAME_NAME, collectionFullName);
            newObj.put(SdbConstants.FIELD_NAME_OPTIONS, options);
        } else {
            Object tmpAlter = options.get(SdbConstants.FIELD_NAME_ALTER);
            if (tmpAlter instanceof BasicBSONObject || tmpAlter instanceof BasicBSONList) {
                newObj.put(SdbConstants.FIELD_NAME_ALTER, tmpAlter);
            } else {
                throw new BaseException(SDBError.SDB_INVALIDARG, options.toString());
            }
            newObj.put(SdbConstants.FIELD_NAME_ALTER_TYPE, SdbConstants.SDB_ALTER_CL);
            newObj.put(SdbConstants.FIELD_NAME_VERSION, SdbConstants.SDB_ALTER_VERSION);
            newObj.put(SdbConstants.FIELD_NAME_NAME, collectionFullName);

            if (options.containsField(SdbConstants.FIELD_NAME_OPTIONS)) {
                Object tmpOptions = options.get(SdbConstants.FIELD_NAME_OPTIONS);
                if (tmpOptions instanceof BasicBSONObject) {
                    newObj.put(SdbConstants.FIELD_NAME_OPTIONS, tmpOptions);
                } else {
                    throw new BaseException(SDBError.SDB_INVALIDARG, options.toString());
                }
            }
        }

        AdminRequest request = new AdminRequest(AdminCommand.ALTER_COLLECTION, newObj);
        SdbReply response = sequoiadb.requestAndResponse(request);
        sequoiadb.throwIfError(response, options);
        sequoiadb.upsertCache(collectionFullName);
    }

    /**
     * Create auto-increment for current collection.
     *
     * @param options The options for creating auto-increment are as below:
     *                <ul>
     *                <li>Field : Auto-increment field name
     *                <li>Increment : The interval between consecutive values
     *                <li>StartValue : The first value for auto-increment
     *                <li>MinValue : The minimum value
     *                <li>MaxValue : The maximum value
     *                <li>CacheSize : The number of values that are cached in catalog node
     *                <li>AcquireSize : The number of values that are acquired by coord node
     *                <li>Cycled : Whether generate the next value after reaching the maximum or minimum
     *                <li>Generated : Whether generate value if the field has already exist. It can be
     *                "default", "always" or "strict".
     *                <li>default : Generate the value by default if field is not exist. It is default
     *                value either.
     *                <li>always : Always Generate the value, ignore the existent field.
     *                <li>strict : Like 'default' behavior, but additionally check the type of field. If
     *                not number, return error. e.g. {Field:"ID", StartValue:100, Generated:"always"}
     *                </ul>
     * @throws BaseException If error happens.
     */
    public void createAutoIncrement(BSONObject options) {
        if (options == null || options.isEmpty()) {
            throw new BaseException(SDBError.SDB_INVALIDARG);
        }
        List<BSONObject> optionsList = new ArrayList<BSONObject>();
        optionsList.add(options);
        createAutoIncrement(optionsList);
    }

    /**
     * Create one or more auto-increment for current collection.
     *
     * @param options The options of the auto-increment(s)
     * @throws BaseException If error happens.
     */
    public void createAutoIncrement(List<BSONObject> options) {
        if (options == null || options.size() == 0) {
            throw new BaseException(SDBError.SDB_INVALIDARG);
        }
        BSONObject obj = new BasicBSONObject(SdbConstants.FIELD_NAME_AUTOINCREMENT, options);
        alterInternal(SdbConstants.SDB_ALTER_CL_CRT_AUTOINC_FLD, obj, false);
    }

    /**
     * Drop auto-increment of current collection.
     *
     * @param fieldName The auto-increment field name
     * @throws BaseException If error happens.
     */
    public void dropAutoIncrement(String fieldName) {
        if (fieldName == null || fieldName.length() == 0) {
            throw new BaseException(SDBError.SDB_INVALIDARG);
        }
        BSONObject obj = new BasicBSONObject(SdbConstants.FIELD_NAME_AUTOINC_FIELD, fieldName);
        alterInternal(SdbConstants.SDB_ALTER_CL_DROP_AUTOINC_FLD, obj, false);
    }

    /**
     * Drop one or more auto-increment of current collection.
     *
     * @param fieldNames The auto-increment field name(s)
     * @throws BaseException If error happens.
     */
    public void dropAutoIncrement(List<String> fieldNames) {
        if (fieldNames == null || fieldNames.size() == 0) {
            throw new BaseException(SDBError.SDB_INVALIDARG);
        }
        BSONObject obj = new BasicBSONObject(SdbConstants.FIELD_NAME_AUTOINC_FIELD, fieldNames);
        alterInternal(SdbConstants.SDB_ALTER_CL_DROP_AUTOINC_FLD, obj, false);
    }

    /**
     * Create the id index.
     *
     * @param options can be empty or specify option. e.g. {SortBufferSize:64}
     * @throws BaseException If error happens.
     */
    public void createIdIndex(BSONObject options) throws BaseException {
        alterInternal(SdbConstants.SDB_ALTER_CRT_ID_INDEX, options, true);
    }

    /**
     * Drop the id index.
     *
     * @throws BaseException If error happens.
     */
    public void dropIdIndex() throws BaseException {
        alterInternal(SdbConstants.SDB_ALTER_DROP_ID_INDEX, null, true);
    }

    /**
     * Alter the attributes of current collection to enable sharding
     *
     * @param options The options for altering current collection are as below:
     *                <ul>
     *                <li>ShardingKey : Assign the sharding key
     *                <li>ShardingType : Assign the sharding type
     *                <li>Partition : When the ShardingType is "hash", need to assign Partition, it's
     *                the bucket number for hash, the range is [2^3,2^20].
     *                <li>EnsureShardingIndex : Assign to true to build sharding index
     *                </ul>
     * @throws BaseException If error happens.
     */
    public void enableSharding(BSONObject options) throws BaseException {
        alterInternal(SdbConstants.SDB_ALTER_ENABLE_SHARDING, options, false);
    }

    /**
     * Alter the attributes of current collection to disable sharding
     *
     * @throws BaseException If error happens.
     */
    public void disableSharding() throws BaseException {
        alterInternal(SdbConstants.SDB_ALTER_DISABLE_SHARDING, null, true);
    }

    /**
     * Alter the attributes of current collection to enable compression
     *
     * @param options The options for altering current collection are as below:
     *                <ul>
     *                <li>CompressionType : The compression type of data, could be "snappy" or "lzw"
     *                </ul>
     * @throws BaseException If error happens.
     */
    public void enableCompression(BSONObject options) throws BaseException {
        alterInternal(SdbConstants.SDB_ALTER_ENABLE_COMPRESSION, options, true);
    }

    /**
     * Alter the attributes of current collection to disable compression
     *
     * @throws BaseException If error happens.
     */
    public void disableCompression() throws BaseException {
        alterInternal(SdbConstants.SDB_ALTER_DISABLE_COMPRESSION, null, true);
    }

    /**
     * Alter the attributes of current collection Can't alter attributes about split in partition
     * collection; After altering a collection to be a partition collection, need to split this
     * collection manually.
     *
     * @param options The options for altering current collection are as below:
     *                <ul>
     *                <li>ReplSize : Assign how many replica nodes need to be synchronized when a write
     *                request(insert, update, etc) is executed
     *                <li>ShardingKey : Assign the sharding key
     *                <li>ShardingType : Assign the sharding type
     *                <li>Partition : When the ShardingType is "hash", need to assign Partition, it's
     *                the bucket number for hash, the range is [2^3,2^20].
     *                <li>CompressionType : The compression type of data, could be "snappy" or "lzw"
     *                <li>EnsureShardingIndex : Assign to true to build sharding index
     *                <li>StrictDataMode : Using strict date mode in numeric operations or not e.g.
     *                {RepliSize:0, ShardingKey:{a:1}, ShardingType:"hash", Partition:1024}
     *                <li>AutoIncrement : Assign attributes of an autoincrement field or batch
     *                autoincrement fields. e.g. {AutoIncrement:{Field:"a",MaxValue:2000}},
     *                {AutoIncrement:[{Field:"a",MaxValue:2000},{Field:"a",MaxValue:4000}]}
     *                </ul>
     * @throws BaseException If error happens.
     */
    public void setAttributes(BSONObject options) throws BaseException {
        alterInternal(SdbConstants.SDB_ALTER_SET_ATTRIBUTES, options, false);
    }

    private void _update(int flag, BSONObject matcher, BSONObject modifier, BSONObject hint)
            throws BaseException {
        UpdateRequest request = new UpdateRequest(collectionFullName, matcher, modifier, hint,
                flag);
        SdbReply response = sequoiadb.requestAndResponse(request);

        if (response.getFlag() != 0) {
            String msg = "matcher = " + matcher + ", modifier = " + modifier + ", hint = " + hint;
            sequoiadb.throwIfError(response, msg);
        }

        sequoiadb.upsertCache(collectionFullName);
    }

    /**
     * Get all of the lobs in current collection.
     *
     * @return DBCursor of lobs
     * @throws BaseException If error happens.
     */
    public DBCursor listLobs() throws BaseException {
        return listLobs(null, null, null, null, 0, -1);
    }

    private boolean isEmptyObj(BSONObject o) {
        if (null == o) {
            return true;
        }

        return o.isEmpty();
    }

    /**
     * Get the lobs in current collection.
     *
     * @param matcher    the matching rule, return all the lobs if null
     * @param selector   the selective rule, return the whole lobs if null
     * @param orderBy    the ordered rule, never sort if null
     * @param hint       Specified options. e.g. {"ListPieces": 1} means get the detail piece info of lobs;
     * @param skipRows   skip the first numToSkip lobs, never skip if this parameter is 0
     * @param returnRows return the specified amount of lobs, when returnRows is 0, return nothing, when
     *                   returnRows is -1, return all the lobs
     * @return DBCursor of lobs
     * @throws BaseException If error happens.
     */
    public DBCursor listLobs(BSONObject matcher, BSONObject selector, BSONObject orderBy,
                             BSONObject hint, long skipRows, long returnRows) throws BaseException {
        BSONObject newHint = new BasicBSONObject();

        if (null != hint) {
            newHint.putAll(hint);
        }
        newHint.put(SdbConstants.FIELD_COLLECTION, collectionFullName);

        if (!sequoiadb.getIsOldVersionLobServer()) {
            BaseException savedError = null;
            try {
                isOldLobServer = false;
                DBCursor cursor = _listLobs(matcher, selector, orderBy, newHint, skipRows,
                        returnRows);
                return cursor;
            } catch (BaseException e) {
                if (!isOldLobServer) {
                    throw e;
                }
                savedError = e;
            }

            // when we come here, we got rc == -6. there are two cases:
            // case 1: having invalid paraments.
            // case 2: the remote engine is older than v3.2.4.

            // case 1: test having invalid paraments or not
            // only when we have input paraments, we need to do this test.
            if (!isEmptyObj(matcher) || !isEmptyObj(selector) || !isEmptyObj(orderBy) || !isEmptyObj(hint)
                    || skipRows != 0 || returnRows != -1) {
                try {
                    isOldLobServer = false;
                    BSONObject tmpHint = new BasicBSONObject();
                    tmpHint.put(SdbConstants.FIELD_COLLECTION, collectionFullName);
                    // make sure no input paraments can affect this test
                    DBCursor tmpCursor = _listLobs(null, null, null, tmpHint, 0, -1);
                    tmpCursor.close();
                    throw savedError;
                } catch (BaseException e) {
                    if (!isOldLobServer) {
                        throw e;
                    }
                }
            }
            // case 2:
            // when we come here, the remote engine must be an old engine.
            DBCursor cursor = _listLobs(newHint, null, null, null, 0, -1);
            sequoiadb.setIsOldVersionLobServer(true);
            return cursor;
        } else {
            // deal with old version engine. clName is in the query field
            return _listLobs(newHint, null, null, null, 0, -1);
        }
    }

    private DBCursor _listLobs(BSONObject matcher, BSONObject selector, BSONObject orderBy,
                               BSONObject hint, long skipRows, long returnRows) throws BaseException {
        AdminRequest request = new AdminRequest(AdminCommand.LIST_LOBS, matcher, selector, orderBy,
                hint, skipRows, returnRows);
        SdbReply response = sequoiadb.requestAndResponse(request);
        int flag = response.getFlag();
        if (flag == SDBError.SDB_INVALIDARG.getErrorCode()) {
            isOldLobServer = true;
        }

        sequoiadb.throwIfError(response);

        sequoiadb.upsertCache(collectionFullName);
        DBCursor cursor = new DBCursor(response, sequoiadb);
        return cursor;
    }

    /**
     * Create a lob.
     *
     * @return DBLob object
     * @throws BaseException If error happens..
     */
    public DBLob createLob() throws BaseException {
        return createLob(null);
    }

    /**
     * Just create a lobID from server.
     *
     * @param d LobID's relative time. if d is NULL the relative time will be server's system time
     * @return ObjectId object
     * @throws BaseException If error happens..
     */
    public ObjectId createLobID(Date d) throws BaseException {
        BSONObject createLobID = null;
        if (null != d) {
            SimpleDateFormat sdf = new SimpleDateFormat("YYYY-MM-dd-HH.mm.ss");
            createLobID = new BasicBSONObject(DBLobImpl.FIELD_NAME_LOB_CREATE_TIME, sdf.format(d));
        }

        LobCreateIDRequest request = new LobCreateIDRequest(createLobID);
        SdbReply response = sequoiadb.requestAndResponse(request, SdbReply.class);
        sequoiadb.throwIfError(response, createLobID);

        ResultSet r = response.getResultSet();
        if (!r.hasNext()) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "Response must have obj");
        }

        BSONObject o = r.getNext();
        if (o == null) {
            throw new BaseException(SDBError.SDB_SYS, "expect a return obj to get oid, but got null");
        }
        return (ObjectId) o.get(DBLobImpl.FIELD_NAME_LOB_OID);
    }

    /**
     * Just create a lobID from server.
     *
     * @return ObjectId object
     * @throws BaseException If error happens..
     */
    public ObjectId createLobID() throws BaseException {
        return createLobID(null);
    }

    /**
     * Create a lob with a given id.
     *
     * @param id the lob's id. if id is null, it will be generated in this function
     * @return DBLob object
     * @throws BaseException If error happens..
     */
    public DBLob createLob(ObjectId id) throws BaseException {
        DBLobImpl lob = new DBLobImpl(this);
        lob.open(id, DBLobImpl.SDB_LOB_CREATEONLY);
        // upsert cache
        sequoiadb.upsertCache(collectionFullName);
        return lob;
    }

    /**
     * Open an existing lob with id.
     *
     * @param id   the lob's id.
     * @param mode open mode: DBLob.SDB_LOB_READ for reading, DBLob.SDB_LOB_SHAREREAD for share reading,
     *             DBLob.SDB_LOB_WRITE for writing, DBLob.SDB_LOB_SHAREREAD|DBLob.SDB_LOB_WRITE for both 
     *             reading and writing. if one lob has been opened by DBLob.SDB_LOB_READ mode, it can't 
     *             be opened by DBLob.SDB_LOB_WRITE mode; However, if one lob has been opened by 
     *             DBLob.SDB_LOB_SHAREREAD mode, it can be opened by DBLob.SDB_LOB_WRITE mode
     * @return DBLob object
     * @throws BaseException If error happens..
     */
    public DBLob openLob(ObjectId id, int mode) throws BaseException {
        if (!DBLobImpl.hasWriteMode(mode) && !DBLobImpl.isReadOnlyMode(mode)) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "mode is unsupported: " + mode);
        }

        DBLobImpl lob = new DBLobImpl(this);
        lob.open(id, mode);
        // upsert cache
        sequoiadb.upsertCache(collectionFullName);
        return lob;
    }

    /**
     * Open an existing lob with id.
     *
     * @param id the lob's id.
     * @return DBLob object
     * @throws BaseException If error happens.
     */
    public DBLob openLob(ObjectId id) throws BaseException {
        return openLob(id, DBLob.SDB_LOB_READ);
    }

    /**
     * Remove an existing lob.
     *
     * @param lobId the lob's id.
     * @throws BaseException If error happens..
     */
    public void removeLob(ObjectId lobId) throws BaseException {
        BSONObject removeObj = new BasicBSONObject();
        removeObj.put(SdbConstants.FIELD_COLLECTION, collectionFullName);
        removeObj.put(DBLobImpl.FIELD_NAME_LOB_OID, lobId);

        LobRemoveRequest request = new LobRemoveRequest(removeObj);
        SdbReply response = sequoiadb.requestAndResponse(request);
        sequoiadb.throwIfError(response, removeObj);
        sequoiadb.upsertCache(collectionFullName);
    }

    /**
     * Truncate an existing lob.
     *
     * @param lobId  the lob's id.
     * @param length the truncate length
     * @throws BaseException If error happens.
     */
    public void truncateLob(ObjectId lobId, long length) throws BaseException {
        if (length < 0) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "Invalid length");
        }

        BSONObject truncateObj = new BasicBSONObject();
        truncateObj.put(SdbConstants.FIELD_COLLECTION, collectionFullName);
        truncateObj.put(DBLobImpl.FIELD_NAME_LOB_OID, lobId);
        truncateObj.put(DBLobImpl.FIELD_NAME_LOB_LENGTH, length);

        LobTruncateRequest request = new LobTruncateRequest(truncateObj);
        SdbReply response = sequoiadb.requestAndResponse(request);
        sequoiadb.throwIfError(response, truncateObj);
        sequoiadb.upsertCache(collectionFullName);
    }

    /**
     * Truncate the collection.
     *
     * @throws BaseException If error happens.
     */
    public void truncate() throws BaseException {
        BSONObject options = new BasicBSONObject();
        options.put(SdbConstants.FIELD_COLLECTION, collectionFullName);

        AdminRequest request = new AdminRequest(AdminCommand.TRUNCATE, options);
        SdbReply response = sequoiadb.requestAndResponse(request);
        sequoiadb.throwIfError(response, options);
        sequoiadb.upsertCache(collectionFullName);
    }

    /**
     * Pop records from the collection.
     *
     * @param options the pop option for the operation, including a record LogicalID, and an optional
     *                Direction: 1 for forward pop and -1 for backward pop
     * @throws BaseException If error happens.
     */
    public void pop(BSONObject options) throws BaseException {
        if (options == null) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "options is null");
        }

        BSONObject newObj = new BasicBSONObject();
        newObj.put(SdbConstants.FIELD_COLLECTION, collectionFullName);
        Object lidObj = options.get(SdbConstants.FIELD_NAME_LOGICALID);
        if (lidObj instanceof Integer || lidObj instanceof Long) {
            newObj.put(SdbConstants.FIELD_NAME_LOGICALID, lidObj);
        } else {
            throw new BaseException(SDBError.SDB_INVALIDARG, options.toString());
        }
        Object directObj = options.get(SdbConstants.FIELD_NAME_DIRECTION);
        if (directObj == null) {
            newObj.put(SdbConstants.FIELD_NAME_DIRECTION, 1);
        } else if (directObj instanceof Integer) {
            newObj.put(SdbConstants.FIELD_NAME_DIRECTION, directObj);
        } else {
            throw new BaseException(SDBError.SDB_INVALIDARG, options.toString());
        }

        AdminRequest request = new AdminRequest(AdminCommand.POP, newObj);
        SdbReply response = sequoiadb.requestAndResponse(request);
        sequoiadb.throwIfError(response, newObj);
        sequoiadb.upsertCache(collectionFullName);
    }
}
