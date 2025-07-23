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

   Source File Name = ObjectServiceImpl.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoias3.service.impl;

import com.sequoias3.common.DBParamDefine;
import com.sequoias3.common.DelimiterStatus;
import com.sequoias3.common.RestParamDefine;
import com.sequoias3.common.VersioningStatusType;
import com.sequoias3.context.Context;
import com.sequoias3.context.ContextManager;
import com.sequoias3.core.*;
import com.sequoias3.model.*;
import com.sequoias3.model.Content;
import com.sequoias3.model.ListObjectsResult;
import com.sequoias3.model.Owner;
import com.sequoias3.model.PutDeleteResult;
import com.sequoias3.dao.*;
import com.sequoias3.exception.S3Error;
import com.sequoias3.exception.S3ServerException;
import com.sequoias3.service.BucketService;
import com.sequoias3.service.ObjectService;
import com.sequoias3.utils.DirUtils;
import org.apache.commons.codec.binary.Hex;
import org.bson.BSONObject;
import org.bson.types.ObjectId;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Service;
import sun.misc.BASE64Decoder;

import javax.servlet.ServletOutputStream;
import java.io.InputStream;
import java.util.*;

import static com.sequoias3.utils.DataFormatUtils.parseDate;

@Service
public class ObjectServiceImpl implements ObjectService {
    private static final Logger logger = LoggerFactory.getLogger(ObjectServiceImpl.class);

    @Autowired
    BucketDao bucketDao;

    @Autowired
    BucketService bucketService;

    @Autowired
    UserDao userDao;

    @Autowired
    DataDao dataDao;

    @Autowired
    MetaDao metaDao;

    @Autowired
    ContextManager contextManager;

    @Autowired
    DaoMgr daoMgr;

    @Autowired
    Transaction transaction;

    @Autowired
    RegionDao regionDao;

    @Autowired
    DirDao dirDao;

    @Autowired
    IDGeneratorDao idGenerator;

    @Override
    public PutDeleteResult putObject(long ownerID, String bucketName, String objectName,
                                     String contentMD5, Map<String, String> headers,
                                     Map<String, String> xMeta, InputStream inputStream)
            throws S3ServerException {
        //check key length
        if (objectName.length() > RestParamDefine.KEY_LENGTH){
            throw new S3ServerException(S3Error.OBJECT_KEY_TOO_LONG,
                    "ObjectName is too long. objectName:"+objectName);
        }

        //check meta length
        if (getXMetaLength(xMeta) > RestParamDefine.X_AMZ_META_LENGTH){
            throw new S3ServerException(S3Error.OBJECT_METADATA_TOO_LARGE,
                    "metadata headers exceed the maximum. xMeta:"+xMeta.toString());
        }

        //get and check bucket
        Bucket bucket = bucketService.getBucket(ownerID, bucketName);

        Region region = null;
        if (bucket.getRegion() != null) {
            region = regionDao.queryRegion(bucket.getRegion());
        }

        //get cs and cl
        Date createDate      = new Date();
        String dataCsName    = regionDao.getDataCSName(region, createDate);
        String dataClName    = regionDao.getDataClName(region, createDate);

        DataAttr insertResult;
        try {//insert lob
            insertResult = dataDao.insertObjectData(dataCsName, dataClName, inputStream, region);
        }catch (S3ServerException e){
            throw e;
        }catch (Exception e){
            throw new S3ServerException(S3Error.OBJECT_PUT_fAILED, "put object failed.", e);
        }

        try {
            //check md5
            if (null != contentMD5) {
                if (!isMd5EqualWithETag(contentMD5, insertResult.geteTag())) {
                    throw new S3ServerException(S3Error.OBJECT_BAD_DIGEST,
                            "The Content-MD5 you specified does not match what we received.");
                }
            }

            VersioningStatusType versioningStatusType = VersioningStatusType.getVersioningStatus(bucket.getVersioningStatus());

            //build meta
            ObjectMeta objectMeta = buildObjectMeta(objectName, bucket.getBucketId(),
                    headers, xMeta, dataCsName, dataClName, false,
                    generateNoVersionFlag(versioningStatusType));
            objectMeta.seteTag(insertResult.geteTag());
            objectMeta.setSize(insertResult.getSize());
            objectMeta.setLobId(insertResult.getLobId());

            writeObjectMeta(objectMeta, objectName, bucket.getBucketId(),
                    versioningStatusType, region);

            //build response
            PutDeleteResult response = new PutDeleteResult();
            response.seteTag(insertResult.geteTag());
            if (VersioningStatusType.ENABLED == versioningStatusType){
                response.setVersionId(String.valueOf(objectMeta.getVersionId()));
            }else if(VersioningStatusType.SUSPENDED == versioningStatusType){
                response.setVersionId(ObjectMeta.NULL_VERSION_ID);
            }
            return response;
        }catch (S3ServerException e){
            dataDao.deleteObjectDataByLobId(null, dataCsName,
                    dataClName, insertResult.getLobId());
            if (e.getError().getErrIndex() == S3Error.DAO_DUPLICATE_KEY.getErrIndex()) {
                throw new S3ServerException(S3Error.OBJECT_PUT_fAILED,
                        "bucket+key duplicate too times. bucket:"+bucketName+" key:"+objectName, e);
            } else {
                throw e;
            }
        }catch (Exception e){
            dataDao.deleteObjectDataByLobId(null, dataCsName,
                    dataClName, insertResult.getLobId());
            throw new S3ServerException(S3Error.OBJECT_PUT_fAILED, "put object failed.", e);
        }
    }

    @Override
    public GetResult getObject(long ownerID, String bucketName, String objectName,
                               Long versionId, Boolean isNoVersion, Map headers,
                               Range range)
            throws S3ServerException {
        try {
            Bucket bucket = bucketService.getBucket(ownerID, bucketName);

            Region region = null;
            if (bucket.getRegion() != null) {
                region = regionDao.queryRegion(bucket.getRegion());
            }

            String metaCsName    = regionDao.getMetaCurCSName(region);
            String metaClName    = regionDao.getMetaCurCLName(region);
            String metaHisCsName = regionDao.getMetaHisCSName(region);
            String metaHisClName = regionDao.getMetaHisCLName(region);

            int tryTime = DBParamDefine.DB_DUPLICATE_MAX_TIME;
            while (tryTime > 0) {
                tryTime--;
                try {
                    ObjectMeta versionIdMeta;
                    ObjectMeta objectMeta = metaDao.queryMetaByObjectName(metaCsName, metaClName,
                            bucket.getBucketId(), objectName, null, null);
                    if (null == objectMeta) {
                        if (versionId != null || isNoVersion != null){
                            throw new S3ServerException(S3Error.OBJECT_NO_SUCH_VERSION,
                                    "no such version. object:" + objectName + ",version:" + versionId);
                        }else {
                            throw new S3ServerException(S3Error.OBJECT_NO_SUCH_KEY, "no object. object:" + objectName);
                        }
                    }

                    if (versionId != null) {
                        if (versionId == objectMeta.getVersionId() && !objectMeta.getNoVersionFlag()){
                            versionIdMeta = objectMeta;
                        }else {
                            ObjectMeta objectMetaHis = metaDao.queryMetaByObjectName(metaHisCsName,
                                    metaHisClName, bucket.getBucketId(), objectName, versionId, false);
                            if (null == objectMetaHis) {
                                throw new S3ServerException(S3Error.OBJECT_NO_SUCH_VERSION,
                                        "no such version. object:" + objectName + ",version:" + versionId);
                            }
                            versionIdMeta = objectMetaHis;
                        }
                    }else if(isNoVersion != null) {
                        if (objectMeta.getNoVersionFlag()) {
                            versionIdMeta = objectMeta;
                        } else {
                            ObjectMeta objectMetaHis = metaDao.queryMetaByObjectName(metaHisCsName,
                                    metaHisClName, bucket.getBucketId(), objectName, null, true);
                            if (null == objectMetaHis) {
                                throw new S3ServerException(S3Error.OBJECT_NO_SUCH_VERSION,
                                        "no such version. object:" + objectName + ",version:" + versionId);
                            }
                            versionIdMeta = objectMetaHis;
                        }
                    }else{
                        versionIdMeta = objectMeta;
                    }

                    DataLob dataLob = null;
                    if (!versionIdMeta.getDeleteMarker()){
                        checkMatchModify(headers, versionIdMeta);
                        dataLob = dataDao.getDataLobForRead(versionIdMeta.getCsName(), versionIdMeta.getClName(),
                                versionIdMeta.getLobId());
                        try {
                            analyseRangeWithLob(range, dataLob);
                        }catch (Exception e){
                            dataDao.releaseDataLob(dataLob);
                            throw e;
                        }
                    }
                    return new GetResult(versionIdMeta, dataLob);
                }catch (S3ServerException e){
                    if (e.getError() == S3Error.DAO_LOB_FNE){
                        continue;
                    }else{
                        throw e;
                    }
                } catch (Exception e){
                    throw e;
                }
            }
            throw new S3ServerException(S3Error.OBJECT_NO_SUCH_KEY,
                    "Lob is not find");
        }catch (S3ServerException e) {
            throw e;
        } catch (Exception e) {
            throw new S3ServerException(S3Error.OBJECT_GET_FAILED,
                    "get object failed. bucket:" + bucketName + ", object=" + objectName, e);
        }
    }

    @Override
    public void readObjectData(DataLob data, ServletOutputStream outputStream, Range range)
            throws S3ServerException{
        if (data == null || outputStream == null){
            throw new S3ServerException(S3Error.OBJECT_GET_FAILED,
                    "get object data failed. ");
        }
        try{
            data.read(outputStream, range);
        }catch (S3ServerException e){
            throw e;
        } catch (Exception e) {
            throw new S3ServerException(S3Error.OBJECT_GET_FAILED,
                    "get object data failed. ", e);
        }
    }

    @Override
    public void releaseGetResult(GetResult result){
        dataDao.releaseDataLob(result.getData());
    }

    @Override
    public PutDeleteResult deleteObject(long ownerID, String bucketName, String objectName)
            throws S3ServerException {
        try {
            Bucket bucket = bucketService.getBucket(ownerID, bucketName);

            Region region = null;
            if (bucket.getRegion() != null) {
                region = regionDao.queryRegion(bucket.getRegion());
            }

            VersioningStatusType versioningStatusType = VersioningStatusType.getVersioningStatus(bucket.getVersioningStatus());
            Boolean noVersionFlag = generateNoVersionFlag(versioningStatusType);

            PutDeleteResult response = null;
            switch (versioningStatusType) {
                case NONE:
                    String metaCsName    = regionDao.getMetaCurCSName(region);
                    String metaClName    = regionDao.getMetaCurCLName(region);
                    ObjectMeta objectMeta = removeMetaForNoneVersioning(metaCsName, metaClName, bucket, objectName);
                    deleteObjectLob(objectMeta);
                    return null;
                case SUSPENDED:
                case ENABLED:
                    ObjectMeta deleteMarker = buildObjectMeta(objectName,
                            bucket.getBucketId(), null, null,
                            null, null, true,
                            noVersionFlag);
                    writeObjectMeta(deleteMarker, objectName,
                            bucket.getBucketId(), versioningStatusType, region);

                    response = new PutDeleteResult();
                    if (deleteMarker.getNoVersionFlag()){
                        response.setVersionId(ObjectMeta.NULL_VERSION_ID);
                    }else {
                        response.setVersionId(String.valueOf(deleteMarker.getVersionId()));
                    }
                    response.setDeleteMarker(true);
                    break;
                default:
                    break;
            }
            return response;
        }catch (S3ServerException e) {
            if (e.getError().getErrIndex() == S3Error.DAO_DUPLICATE_KEY.getErrIndex()) {
                throw new S3ServerException(S3Error.OBJECT_DELETE_FAILED,
                        "bucket+key duplicate too times. bucket:"+bucketName+" key:"+objectName);
            } else {
                throw e;
            }
        } catch (Exception e) {
            throw new S3ServerException(S3Error.OBJECT_DELETE_FAILED,
                    "delete object failed. bucket:" + bucketName + ", object=" + objectName, e);
        }
    }

    @Override
    public PutDeleteResult deleteObject(long ownerID, String bucketName, String objectName,
                                        Long versionId, Boolean isNoVersion)
            throws S3ServerException {
        try {
            Bucket bucket = bucketService.getBucket(ownerID, bucketName);

            Region region = null;
            if (bucket.getRegion() != null) {
                region = regionDao.queryRegion(bucket.getRegion());
            }

            String metaCsName    = regionDao.getMetaCurCSName(region);
            String metaClName    = regionDao.getMetaCurCLName(region);
            String metaHisCsName = regionDao.getMetaHisCSName(region);
            String metaHisClName = regionDao.getMetaHisCLName(region);

            VersioningStatusType versioningStatusType = VersioningStatusType.getVersioningStatus(bucket.getVersioningStatus());
            ObjectMeta deleteObject = null;
            switch (versioningStatusType){
                case NONE:
                    if (isNoVersion != null) {
                        ObjectMeta objectMeta = removeMetaForNoneVersioning(metaCsName, metaClName, bucket, objectName);
                        deleteObject = objectMeta;
                    }
                    break;
                case SUSPENDED:
                case ENABLED:
                    ConnectionDao connection = daoMgr.getConnectionDao();
                    transaction.begin(connection);
                    try{
                        ObjectMeta objectMeta = null;
                        if (isNoVersion != null){
                            objectMeta = metaDao.queryForUpdate(connection, metaCsName,
                                    metaClName, bucket.getBucketId(), objectName, null, true);
                        }else if (versionId != null){
                            objectMeta = metaDao.queryForUpdate(connection, metaCsName,
                                    metaClName, bucket.getBucketId(), objectName, versionId, false);
                        }
                        if (objectMeta != null){
                            deleteObject = objectMeta;
                            ObjectMeta objectMeta1 = metaDao.queryForUpdate(connection, metaHisCsName,
                                    metaHisClName, bucket.getBucketId(), objectName, null, null);
                            if (objectMeta1 != null){
                                objectMeta1.setParentId1(objectMeta.getParentId1());
                                objectMeta1.setParentId2(objectMeta.getParentId2());
                                metaDao.updateMeta(connection, metaCsName, metaClName, bucket.getBucketId(),
                                        objectName, versionId, objectMeta1);
                                metaDao.removeMeta(connection, metaHisCsName, metaHisClName, bucket.getBucketId(),
                                        objectName, objectMeta1.getVersionId(), null);
                            }else {
                                deleteDirForObject(connection, metaCsName, metaClName, bucket, objectName);
                                metaDao.removeMeta(connection, metaCsName, metaClName, bucket.getBucketId(),
                                        objectName, objectMeta.getVersionId(), null);
                                Bucket newBucket = bucketDao.getBucketById(bucket.getBucketId());
                                if (newBucket != null && newBucket.getDelimiter() != bucket.getDelimiter()){
                                    deleteDirForObject(connection, metaCsName, metaClName, newBucket, objectName);
                                }
                            }
                        }else{
                            ObjectMeta objectMeta2 = null;
                            if (isNoVersion != null){
                                objectMeta2 = metaDao.queryForUpdate(connection, metaHisCsName,
                                        metaHisClName, bucket.getBucketId(), objectName, null, true);
                            }else if (versionId != null){
                                objectMeta2 = metaDao.queryForUpdate(connection, metaHisCsName,
                                        metaHisClName, bucket.getBucketId(), objectName, versionId, false);
                            }

                            if (objectMeta2 != null){
                                deleteObject = objectMeta2;
                                metaDao.removeMeta(connection, metaHisCsName, metaHisClName, bucket.getBucketId(),
                                        objectName, objectMeta2.getVersionId(), null);
                            }
                        }
                        transaction.commit(connection);
                    }catch(Exception e){
                        transaction.rollback(connection);
                        throw e;
                    } finally {
                        daoMgr.releaseConnectionDao(connection);
                    }
                    break;
                default:
                    break;
            }

            deleteObjectLob(deleteObject);
            return null;
        }catch (S3ServerException e) {
            throw e;
        } catch (Exception e) {
            throw new S3ServerException(S3Error.OBJECT_DELETE_FAILED,
                    "delete object failed. bucket:" + bucketName + ", object=" + objectName, e);
        }
    }

    @Override
    public ListObjectsResult listObjects(long ownerID, String bucketName, String prefix,
                                         String delimiter, String startAfter, Integer maxKeys,
                                         String continueToken, String encodingType, Boolean fetchOwner)
            throws S3ServerException {
        Context queryContext = null;
        QueryDbCursor dbCursorKeys = null;
        QueryDbCursor dbCursorDir = null;
        try {
            Bucket bucket = bucketService.getBucket(ownerID, bucketName);
            Region region = null;
            if (bucket.getRegion() != null) {
                region = regionDao.queryRegion(bucket.getRegion());
            }
            ListObjectsResult listObjectsResult = new ListObjectsResult(bucketName, maxKeys,
                    encodingType, prefix, startAfter, delimiter, continueToken);

            String metaCsName = regionDao.getMetaCurCSName(region);
            String metaClName = regionDao.getMetaCurCLName(region);

            String newStartAfter = startAfter;
            if (null != continueToken) {
                queryContext = contextManager.get(continueToken);
                if (null == queryContext) {
                    throw new S3ServerException(S3Error.OBJECT_INVALID_TOKEN,
                            "The continuation token provided is incorrect.token:"+continueToken);
                }
                if (!IsContextMatch(queryContext, prefix, startAfter, delimiter)){
                    queryContext = null;
                }else{
                    newStartAfter = queryContext.getLastKey();
                }
            }

            Owner owner = null;
            if (fetchOwner){
                owner = userDao.getOwnerByUserID(ownerID);
            }
            int count = 0;
            int maxNumber = Math.min(maxKeys, RestParamDefine.MAX_KEYS_DEFAULT);

            String parentIdName = getParentName(delimiter, bucket);
            ObjectsFilter filter;
            if (parentIdName != null){
                Long parentId = getParentId(metaCsName, bucket.getBucketId(), prefix, delimiter);
                dbCursorDir = dirDao.queryDirList(metaCsName, bucket.getBucketId(),
                        delimiter, prefix, newStartAfter);
                if (parentId != null){
                    dbCursorKeys = metaDao.queryMetaListByParentId(metaCsName,
                            metaClName, bucket.getBucketId(), parentIdName, parentId,
                            prefix, newStartAfter, null, false);
                }

                filter = new ObjectsFilter(dbCursorDir, dbCursorKeys,
                        prefix, delimiter, newStartAfter, encodingType, FilterRecord.FILTER_DIR);
            }else {
                dbCursorKeys = metaDao.queryMetaByBucket(metaCsName, metaClName,
                        bucket.getBucketId(), prefix, newStartAfter,
                        null, false);

                if (null == delimiter){
                    filter = new ObjectsFilter(null, dbCursorKeys,
                            prefix, delimiter, newStartAfter, encodingType, FilterRecord.FILTER_NO_DELIMITER);
                }else {
                    filter = new ObjectsFilter(null, dbCursorKeys,
                            prefix, delimiter, newStartAfter, encodingType, FilterRecord.FILTER_DELIMITER);
                }
            }

            if (queryContext != null){
                filter.setLastCommonPrefix(queryContext.getLastCommonPrefix());
            }

            LinkedHashSet<Content> contentList = listObjectsResult.getContentList();
            LinkedHashSet<CommonPrefix> commonPrefixesList = listObjectsResult.getCommonPrefixList();
            FilterRecord matcher;
            while ((matcher =  filter.getNextRecord()) != null){
                if (matcher.getRecordType() == FilterRecord.COMMONPREFIX){
                    if (!commonPrefixesList.contains(matcher.getCommonPrefix())) {
                        commonPrefixesList.add(matcher.getCommonPrefix());
                        count++;
                    }
                }else if (matcher.getRecordType() == FilterRecord.CONTENT){
                    matcher.getContent().setOwner(owner);
                    contentList.add(matcher.getContent());
                    count++;
                }

                if (count >= maxNumber){
                    break;
                }
            }

            listObjectsResult.setKeyCount(count);
            if (filter.hasNext()){
                if (null == queryContext) {
                    queryContext = contextManager.create(bucket.getBucketId());
                    queryContext.setPrefix(prefix);
                    queryContext.setStartAfter(startAfter);
                    queryContext.setDelimiter(delimiter);
                }

                queryContext.setLastKey(filter.getLastKey());
                queryContext.setLastCommonPrefix(filter.getLastCommonPrefix());
                listObjectsResult.setIsTruncated(true);
                listObjectsResult.setNextContinueToken(queryContext.getToken());
            }else {
                contextManager.release(queryContext);
            }

            return listObjectsResult;
        } catch (S3ServerException e){
            contextManager.release(queryContext);
            throw e;
        } catch (Exception e){
            contextManager.release(queryContext);
            throw new S3ServerException(S3Error.OBJECT_LIST_FAILED, "error message:"+e.getMessage(), e);
        }finally {
            metaDao.releaseQueryDbCursor(dbCursorDir);
            metaDao.releaseQueryDbCursor(dbCursorKeys);
        }
    }

    @Override
    public ListVersionsResult listVersions(long ownerID, String bucketName, String prefix,
                                           String delimiter, String keyMarker, String versionIdMarker,
                                           Integer maxKeys, String encodingType)
            throws S3ServerException {
        QueryDbCursor queryDbCursorCur = null;
        QueryDbCursor queryDbCursorHis = null;
        QueryDbCursor queryDbCursorDir = null;
        try {
            Bucket bucket = bucketService.getBucket(ownerID, bucketName);
            Region region = null;
            if (bucket.getRegion() != null) {
                region = regionDao.queryRegion(bucket.getRegion());
            }
            ListVersionsResult listVersionsResult = new ListVersionsResult(bucketName, maxKeys,
                    encodingType, prefix, delimiter, keyMarker, versionIdMarker);

            String metaCsName    = regionDao.getMetaCurCSName(region);
            String metaClName    = regionDao.getMetaCurCLName(region);
            String metaHisCsName = regionDao.getMetaHisCSName(region);
            String metaHisClName = regionDao.getMetaHisCLName(region);
            //get sdb and cursor
            Long specifiedVersionId = null;
            if (versionIdMarker != null && versionIdMarker.length() > 0){
                specifiedVersionId = getInnerVersionId(metaCsName, metaClName, metaHisCsName, metaHisClName,
                        keyMarker, versionIdMarker,bucket);
            }

            int count = 0;
            int maxNumber = Math.min(maxKeys, RestParamDefine.MAX_KEYS_DEFAULT);
            Owner owner = userDao.getOwnerByUserID(ownerID);

            VersionsFilter filter;
            String parentIdName = getParentName(delimiter, bucket);
            if (parentIdName == null) {
                queryDbCursorCur = metaDao.queryMetaByBucket(metaCsName, metaClName,
                        bucket.getBucketId(), prefix, keyMarker, specifiedVersionId, true);
                if (queryDbCursorCur == null) {
                    return listVersionsResult;
                }

                queryDbCursorHis = metaDao.queryMetaByBucket(metaHisCsName, metaHisClName,
                        bucket.getBucketId(), prefix, keyMarker, specifiedVersionId, true);

                if (delimiter != null) {
                    filter = new VersionsFilter(queryDbCursorCur,
                            queryDbCursorDir, queryDbCursorHis, prefix, delimiter,
                            keyMarker, specifiedVersionId, encodingType, owner,
                            FilterRecord.FILTER_DELIMITER);
                }else {
                    filter = new VersionsFilter(queryDbCursorCur,
                            queryDbCursorDir, queryDbCursorHis, prefix, delimiter,
                            keyMarker, specifiedVersionId, encodingType, owner,
                            FilterRecord.FILTER_NO_DELIMITER);
                }
            }else {
                Long parentId = getParentId(metaCsName, bucket.getBucketId(), prefix, delimiter);
                queryDbCursorDir = dirDao.queryDirList(metaCsName, bucket.getBucketId(),
                        delimiter, prefix, keyMarker);
                if (parentId != null){
                    queryDbCursorCur = metaDao.queryMetaListByParentId(metaCsName,
                            metaClName, bucket.getBucketId(), parentIdName, parentId,
                            prefix, keyMarker, specifiedVersionId, false);
                }

                filter = new VersionsFilter(queryDbCursorCur, queryDbCursorDir,
                        null, prefix, delimiter, keyMarker, specifiedVersionId,
                        encodingType, owner, FilterRecord.FILTER_DIR);
            }

            FilterRecord matcher = null;
            LinkedHashSet<CommonPrefix> commonPrefixesList = listVersionsResult.getCommonPrefixList();
            while (count < maxNumber){
                if (!filter.isFilterReady()){
                    metaDao.releaseQueryDbCursor(queryDbCursorHis);
                    List<String> keys = filter.getCurKeys();
                    if (keys.size() > 0){
                        queryDbCursorHis = metaDao.queryMetaByBucketInKeys(metaHisCsName, metaHisClName, bucket.getBucketId(), keys);
                        filter.setHisCursor(queryDbCursorHis);
                    }
                }

                if ((matcher = filter.getNextRecord()) != null){
                    if (matcher.getRecordType() == FilterRecord.COMMONPREFIX){
                        if (!commonPrefixesList.contains(matcher.getCommonPrefix())){
                            commonPrefixesList.add(matcher.getCommonPrefix());
                            count++;
                        }
                    }else if (matcher.getRecordType() == FilterRecord.DELETEMARKER){
                        listVersionsResult.getDeleteMarkerList().add(matcher.getDeleteMarker());
                        count++;
                    }else if (matcher.getRecordType() == FilterRecord.VERSION){
                        listVersionsResult.getVersionList().add(matcher.getVersion());
                        count++;
                    }
                }else {
                    break;
                }
            }

            if (filter.hasNext()){
                listVersionsResult.setIsTruncated(true);
                if (matcher.getRecordType() == FilterRecord.COMMONPREFIX){
                    listVersionsResult.setNextKeyMarker(matcher.getCommonPrefix().getPrefix());
                }else if (matcher.getRecordType() == FilterRecord.DELETEMARKER){
                    listVersionsResult.setNextKeyMarker(matcher.getDeleteMarker().getKey());
                    listVersionsResult.setNextVersionIdMarker(matcher.getDeleteMarker().getVersionId());
                }else if (matcher.getRecordType() == FilterRecord.VERSION){
                    listVersionsResult.setNextKeyMarker(matcher.getVersion().getKey());
                    listVersionsResult.setNextVersionIdMarker(matcher.getVersion().getVersionId());
                }
            }

            return listVersionsResult;
        }catch (S3ServerException e){
            throw e;
        }catch (Exception e){
            throw new S3ServerException(S3Error.OBJECT_LIST_VERSIONS_FAILED,
                    "List versions failed. bucket:"+bucketName, e);
        }finally {
            metaDao.releaseQueryDbCursor(queryDbCursorCur);
            metaDao.releaseQueryDbCursor(queryDbCursorHis);
            metaDao.releaseQueryDbCursor(queryDbCursorDir);
        }
    }

    @Override
    public Boolean isEmptyBucket(ConnectionDao connection, Bucket bucket) throws S3ServerException{
        try {
            Region region = null;
            if (bucket.getRegion() != null) {
                region = regionDao.queryRegion(bucket.getRegion());
            }
            String metaCsName    = regionDao.getMetaCurCSName(region);
            String metaClName    = regionDao.getMetaCurCLName(region);
            String metaHisCsName = regionDao.getMetaHisCSName(region);
            String metaHisClName = regionDao.getMetaHisCLName(region);

            if (null != metaDao.queryMetaByBucketId(connection, metaCsName, metaClName, bucket.getBucketId())) {
                return false;
            }
            if (null != metaDao.queryMetaByBucketId(connection, metaHisCsName, metaHisClName, bucket.getBucketId())){
                return false;
            }

            return true;
        }catch (S3ServerException e){
            throw e;
        }catch (Exception e){
            throw new S3ServerException(S3Error.DAO_DB_ERROR, "unknown error", e);
        }
    }

    @Override
    public void deleteObjectByBucket(Bucket bucket) throws S3ServerException {
        try {
            Region region = null;
            if (bucket.getRegion() != null) {
                region = regionDao.queryRegion(bucket.getRegion());
            }
            String metaCsName    = regionDao.getMetaCurCSName(region);
            String metaClName    = regionDao.getMetaCurCLName(region);
            String metaHisCsName = regionDao.getMetaHisCSName(region);
            String metaHisClName = regionDao.getMetaHisCLName(region);
            long bucketId = bucket.getBucketId();

            deleteObjectByClBucket(metaHisCsName, metaHisClName, bucketId);
            deleteObjectByClBucket(metaCsName, metaClName, bucketId);
            dirDao.delete(null, metaCsName, bucketId, null, null);
        }catch (S3ServerException e){
            throw e;
        }catch (Exception e){
            throw new S3ServerException(S3Error.DAO_DB_ERROR, "unknown error", e);
        }
    }

    private String getParentName(String delimiter, Bucket bucket){
        if (delimiter != null) {
            if (bucket.getDelimiter() == 1) {
                if (delimiter.equals(bucket.getDelimiter1())
                        && DelimiterStatus.getDelimiterStatus(bucket.getDelimiter1Status()) == DelimiterStatus.NORMAL) {
                    return ObjectMeta.META_PARENTID1;
                }
            } else {
                if (delimiter.equals(bucket.getDelimiter2())
                        && DelimiterStatus.getDelimiterStatus(bucket.getDelimiter2Status()) == DelimiterStatus.NORMAL) {
                    return ObjectMeta.META_PARENTID2;
                }
            }
        }
        return null;
    }

    private Long getParentId(String metaCsName, long bucketId, String prefix, String delimiter)
            throws S3ServerException{
        Long parentId = null;
        if (prefix != null){
            String dirName = DirUtils.getDir(prefix, delimiter);
            if (dirName != null){
                Dir dir = dirDao.queryDir(null, metaCsName, bucketId,
                        delimiter, dirName, false);
                if (dir != null){
                    parentId = dir.getID();
                }
            }else {
                parentId = 0L;
            }
        }else {
            parentId = 0L;
        }
        return parentId;
    }

    private ObjectMeta removeMetaForNoneVersioning(String metaCsName, String metaClName, Bucket bucket, String objectName)
            throws S3ServerException{
        ConnectionDao connection = daoMgr.getConnectionDao();
        transaction.begin(connection);
//        logger.info("transaction begin. Key:{}", objectName);
        try {
            ObjectMeta objectMeta = metaDao.queryForUpdate(connection, metaCsName,
                    metaClName, bucket.getBucketId(), objectName, null, null);
            if (objectMeta != null){
                deleteDirForObject(connection, metaCsName, metaClName, bucket, objectName);
                metaDao.removeMeta(connection, metaCsName, metaClName, bucket.getBucketId(),
                        objectName, null, null);
                Bucket newBucket = bucketDao.getBucketById(bucket.getBucketId());
                if (newBucket != null && newBucket.getDelimiter() != bucket.getDelimiter()){
                    deleteDirForObject(connection, metaCsName, metaClName, newBucket, objectName);
                }
            }
//            logger.info("transaction commit. Key:{}", objectName);
            transaction.commit(connection);
            return objectMeta;
        } catch (S3ServerException e){
            transaction.rollback(connection);
            throw e;
        } catch(Exception e){
//            logger.info("transaction rollback. Key:{}", objectName);
            transaction.rollback(connection);
            throw e;
        } finally {
            daoMgr.releaseConnectionDao(connection);
        }
    }

    private void deleteObjectByClBucket(String metaCsName, String metaClName, long bucketId)
            throws S3ServerException{
        QueryDbCursor queryDbCursor = metaDao.queryMetaByBucket(metaCsName, metaClName,
                bucketId, null, null, null, true);
        if (queryDbCursor == null){
            return;
        }
        ConnectionDao connection = daoMgr.getConnectionDao();
        try {
            while (queryDbCursor.hasNext()) {
                BSONObject record = queryDbCursor.getNext();
                String key = record.get(ObjectMeta.META_KEY_NAME).toString();
                Long versionId = (Long)record.get(ObjectMeta.META_VERSION_ID);
                metaDao.removeMeta(connection, metaCsName, metaClName, bucketId,
                        key, versionId, null);
                if (record.get(ObjectMeta.META_CS_NAME) != null
                        && record.get(ObjectMeta.META_CL_NAME) != null
                        && record.get(ObjectMeta.META_LOB_ID) != null) {
                    String dataCsName = record.get(ObjectMeta.META_CS_NAME).toString();
                    String dataClName = record.get(ObjectMeta.META_CL_NAME).toString();
                    ObjectId lobId = (ObjectId)record.get(ObjectMeta.META_LOB_ID);
                    dataDao.deleteObjectDataByLobId(connection, dataCsName, dataClName, lobId);
                }
            }
        } catch (S3ServerException e) {
            throw e;
        }catch (Exception e) {
            throw e;
        }finally {
            daoMgr.releaseConnectionDao(connection);
        }
    }

    private ObjectMeta buildObjectMeta(String objectName, long bucketId, Map headers,
                                       Map xMeta, String dataCsName, String dataClName,
                                       Boolean isDeleteMarker, Boolean noVersionFlag) {
        ObjectMeta objectMeta = new ObjectMeta();
        objectMeta.setKey(objectName);
        objectMeta.setBucketId(bucketId);
        objectMeta.setCsName(dataCsName);
        objectMeta.setClName(dataClName);
        objectMeta.setLastModified(System.currentTimeMillis());
        objectMeta.setMetaList(xMeta);
        objectMeta.setDeleteMarker(isDeleteMarker);
        objectMeta.setNoVersionFlag(noVersionFlag);

        if (headers != null) {
            if (headers.containsKey(RestParamDefine.PutObjectHeader.CACHE_CONTROL)) {
                objectMeta.setCacheControl(headers.get(RestParamDefine.PutObjectHeader.CACHE_CONTROL).toString());
            }

            if (headers.containsKey(RestParamDefine.PutObjectHeader.CONTENT_DISPOSITION)) {
                objectMeta.setContentDisposition(headers.get(RestParamDefine.PutObjectHeader.CONTENT_DISPOSITION).toString());
            }

            if (headers.containsKey(RestParamDefine.PutObjectHeader.CONTENT_ENCODING)) {
                objectMeta.setContentEncoding(headers.get(RestParamDefine.PutObjectHeader.CONTENT_ENCODING).toString());
            }

            if (headers.containsKey(RestParamDefine.PutObjectHeader.CONTENT_TYPE)) {
                objectMeta.setContentType(headers.get(RestParamDefine.PutObjectHeader.CONTENT_TYPE).toString());
            }

            if (headers.containsKey(RestParamDefine.PutObjectHeader.EXPIRES)) {
                objectMeta.setExpires(headers.get(RestParamDefine.PutObjectHeader.EXPIRES).toString());
            }

            if (headers.containsKey(RestParamDefine.PutObjectHeader.CONTENT_LANGUAGE)) {
                objectMeta.setContentLanguage(headers.get(RestParamDefine.PutObjectHeader.CONTENT_LANGUAGE).toString());
            }
        }

        return objectMeta;
    }

    private Boolean isMd5EqualWithETag(String contentMd5, String eTag) throws S3ServerException{
        try {
            if(contentMd5.length() % 4 != 0){
                throw new S3ServerException(S3Error.OBJECT_INVALID_DIGEST,
                        "decode md5 failed, contentMd5:"+contentMd5);
            }
            BASE64Decoder decoder = new BASE64Decoder();
            String textMD5 = new String(Hex.encodeHex(decoder.decodeBuffer(contentMd5)));
            if (textMD5.equals(eTag)){
                return true;
            }else {
                return false;
            }
        }catch (Exception e){
            throw new S3ServerException(S3Error.OBJECT_INVALID_DIGEST,
                    "decode md5 failed, contentMd5:"+contentMd5, e);
        }
    }

    private Boolean checkMatchModify(Map headers, ObjectMeta objectMeta) throws S3ServerException{
        String eTag           = objectMeta.geteTag();
        long lastModifiedTime = objectMeta.getLastModified();
        boolean isMatch     = false;
        boolean isNoneMatch = false;

        Object matchEtag = headers.get(RestParamDefine.GetObjectReqHeader.REQ_IF_MATCH);
        if (null != matchEtag){
            if (!matchEtag.toString().equals(eTag)){
                throw new S3ServerException(S3Error.OBJECT_IF_MATCH_FAILED,
                        "if-match failed: if-match value:" + matchEtag.toString() +
                                ", current object eTag:" + eTag);
            }else{
                isMatch = true;
            }
        }

        Object noneMatchEtag = headers.get(RestParamDefine.GetObjectReqHeader.REQ_IF_NONE_MATCH);
        if (null != noneMatchEtag){
            if (noneMatchEtag.toString().equals(eTag)){
                throw new S3ServerException(S3Error.OBJECT_IF_NONE_MATCH_FAILED,
                        "if-none-match failed: if-none-match value:" + noneMatchEtag.toString() +
                                ", current object eTag:" + eTag);
            }else{
                isNoneMatch = true;
            }
        }

        Object unModifiedSince = headers.get(RestParamDefine.GetObjectReqHeader.REQ_IF_UNMODIFIED_SINCE);
        if (null != unModifiedSince){
            Date date = parseDate(unModifiedSince.toString());
            if (getSecondTime(date.getTime()) < getSecondTime(lastModifiedTime)) {
                if (!isMatch) {
                    throw new S3ServerException(S3Error.OBJECT_IF_UNMODIFIED_SINCE_FAILED,
                            "if-unmodified-since failed: if-unmodified-since value:" + unModifiedSince.toString() +
                                    ", current object lastModifiedTime:" + new Date(lastModifiedTime));
                }
            }
        }

        Object modifiedSince = headers.get(RestParamDefine.GetObjectReqHeader.REQ_IF_MODIFIED_SINCE);
        if (null != modifiedSince){
            Date date = parseDate(modifiedSince.toString());
            if (getSecondTime(date.getTime()) >= getSecondTime(lastModifiedTime)) {
                if (!isNoneMatch) {
                    throw new S3ServerException(S3Error.OBJECT_IF_MODIFIED_SINCE_FAILED,
                            "if-modified-since failed: if-modified-since value:" + modifiedSince.toString() +
                                    ", current object lastModifiedTime:" + new Date(lastModifiedTime));
                }
            }
        }

        return true;
    }

    private Boolean IsContextMatch(Context queryContext, String prefix, String startAfter,
                                 String delimiter){
        if(queryContext.getDelimiter() != null){
            if (!(queryContext.getDelimiter().equals(delimiter))) {
                return false;
            }
        }else if (delimiter != null){
            return false;
        }

        if (queryContext.getPrefix() != null) {
            if (!(queryContext.getPrefix().equals(prefix))) {
                return false;
            }
        }else if (prefix != null){
            return false;
        }

        if (queryContext.getStartAfter() != null) {
            if (!(queryContext.getStartAfter().equals(startAfter))) {
                return false;
            }
        }else if (startAfter != null){
            return false;
        }

        return true;
    }

    private Boolean generateNoVersionFlag(VersioningStatusType status){
        if (null == status){
            return false;
        }
        switch(status){
            case ENABLED:
                return false;
            default:
                return true;
        }
    }

    private void deleteObjectLob(ObjectMeta deleteObject) throws S3ServerException{
        if (deleteObject != null && !deleteObject.getDeleteMarker()) {
            int tryTime = DBParamDefine.DB_DUPLICATE_MAX_TIME;
            while (tryTime > 0) {
                tryTime--;
                try {
                    dataDao.deleteObjectDataByLobId(null, deleteObject.getCsName(),
                            deleteObject.getClName(), deleteObject.getLobId());
                    break;
                }catch (S3ServerException e){
                    if (e.getError() == S3Error.OBJECT_IS_IN_USE && tryTime > 0){
                        if (tryTime == DBParamDefine.DB_DUPLICATE_MAX_TIME-1){
                            logger.error("lob is in use.lob is:{}",deleteObject);
                        }
                        continue;
                    }else {
                        throw e;
                    }
                }
            }
        }
    }

    private void buildDirForObject(ConnectionDao connection, String metaCsName, Bucket bucket,
                                   String objectName, ObjectMeta objectMeta, Region region)
            throws S3ServerException{
        long bucketId = bucket.getBucketId();
        if (bucket.getDelimiter1() != null) {
            DelimiterStatus status = DelimiterStatus.getDelimiterStatus(bucket.getDelimiter1Status());
            if (status == DelimiterStatus.NORMAL
                    || status == DelimiterStatus.CREATING
                    || status == DelimiterStatus.SUSPENDED) {
                String dirName = DirUtils.getDir(objectName, bucket.getDelimiter1());
                if (dirName != null) {
                    Dir dir = dirDao.queryDir(connection, metaCsName, bucketId, bucket.getDelimiter1(), dirName, true);
                    if (dir != null) {
                        objectMeta.setParentId1(dir.getID());
                    } else {
                        long newId = idGenerator.getNewId(IDGenerator.TYPE_PARENTID);
                        Dir newDir = new Dir(bucketId, bucket.getDelimiter1(), dirName, newId);
                        dirDao.insertDir(connection, metaCsName, newDir, region);
                        objectMeta.setParentId1(newId);
                    }
                }else {
                    objectMeta.setParentId1(0L);
                }
            }
        }

        if (bucket.getDelimiter2() != null) {
            DelimiterStatus status = DelimiterStatus.getDelimiterStatus(bucket.getDelimiter2Status());
            if (status == DelimiterStatus.NORMAL
                    || status == DelimiterStatus.CREATING
                    || status == DelimiterStatus.SUSPENDED) {
                String dirName = DirUtils.getDir(objectName, bucket.getDelimiter2());
                if (dirName != null) {
                    Dir dir = dirDao.queryDir(connection, metaCsName, bucketId, bucket.getDelimiter2(), dirName, true);
                    if (dir != null) {
                        objectMeta.setParentId2(dir.getID());
                    } else {
                        long newId = idGenerator.getNewId(IDGenerator.TYPE_PARENTID);
                        Dir newDir = new Dir(bucketId, bucket.getDelimiter2(), dirName, newId);
                        dirDao.insertDir(connection, metaCsName, newDir, region);
                        objectMeta.setParentId2(newId);
                    }
                }else {
                    objectMeta.setParentId2(0L);
                }
            }
        }
    }

    private void deleteDirForObject(ConnectionDao connection, String metaCsName, String metaClName,
                                    Bucket bucket, String objectName)
            throws S3ServerException{
        if (bucket.getDelimiter1() != null) {
            DelimiterStatus status = DelimiterStatus.getDelimiterStatus(bucket.getDelimiter1Status());
            if (status == DelimiterStatus.NORMAL
                    || status == DelimiterStatus.CREATING
                    || status == DelimiterStatus.SUSPENDED) {
                String dirName = DirUtils.getDir(objectName, bucket.getDelimiter1());
                if (dirName != null) {
                    Dir dir = dirDao.queryDir(connection, metaCsName, bucket.getBucketId(),
                            bucket.getDelimiter1(), dirName, true);
                    if (dir != null) {
                        String parentIdName = ObjectMeta.META_PARENTID1;
                        if(!metaDao.queryOneOtherMetaByParentId(null, metaCsName, metaClName,
                                bucket.getBucketId(), objectName, parentIdName, dir.getID())){
                            dirDao.delete(connection, metaCsName, dir.getBucketId(),
                                    bucket.getDelimiter1(), dirName);
                        }
                    }
                }
            }
        }

        if (bucket.getDelimiter2() != null) {
            DelimiterStatus status = DelimiterStatus.getDelimiterStatus(bucket.getDelimiter2Status());
            if (status == DelimiterStatus.NORMAL
                    || status == DelimiterStatus.CREATING
                    || status == DelimiterStatus.SUSPENDED) {
                String dirName = DirUtils.getDir(objectName, bucket.getDelimiter2());
                if (dirName != null) {
                    Dir dir = dirDao.queryDir(connection, metaCsName, bucket.getBucketId(),
                            bucket.getDelimiter2(), dirName, true);
                    if (dir != null) {
                        String parentIdName = ObjectMeta.META_PARENTID2;
                        if(!metaDao.queryOneOtherMetaByParentId(null, metaCsName, metaClName,
                                bucket.getBucketId(), objectName, parentIdName, dir.getID())){
                            dirDao.delete(connection, metaCsName, bucket.getBucketId(),
                                    bucket.getDelimiter2(), dirName);
                        }
                    }
                }
            }
        }
    }

    private void writeObjectMeta(ObjectMeta objectMeta, String objectName, long bucketId,
                                 VersioningStatusType versioningStatusType, Region region)
            throws S3ServerException{
        String metaCsName    = regionDao.getMetaCurCSName(region);
        String metaClName    = regionDao.getMetaCurCLName(region);
        String metaHisCSName = regionDao.getMetaHisCSName(region);
        String metaHisClName = regionDao.getMetaHisCLName(region);

        int tryTime = DBParamDefine.DB_DUPLICATE_MAX_TIME;
        while (tryTime > 0) {
            tryTime--;
            ObjectMeta deleteObject = null;
            ConnectionDao connection = daoMgr.getConnectionDao();
            transaction.begin(connection);
            try {
                ObjectMeta metaResult = metaDao.queryForUpdate(connection, metaCsName, metaClName,
                        bucketId, objectName, null, null);
                if (null == metaResult) {
                    objectMeta.setVersionId(0);
                    Bucket bucket = bucketDao.getBucketById(bucketId);
                    if (bucket == null){
                        throw new S3ServerException(S3Error.BUCKET_NOT_EXIST, "bucket is deleting before insert meta.");
                    }
                    buildDirForObject(connection, metaCsName, bucket, objectName, objectMeta, region);
                    metaDao.insertMeta(connection, metaCsName, metaClName, objectMeta,
                            false, region);
                    Bucket newBucket = bucketDao.getBucketById(bucketId);
                    if (newBucket == null){
                        throw new S3ServerException(S3Error.BUCKET_NOT_EXIST, "bucket is deleting after insert meta.");
                    }
                    if (bucket.getDelimiter() != newBucket.getDelimiter()) {
                        transaction.rollback(connection);
                        continue;
                    }
                    transaction.commit(connection);
                } else {
                    objectMeta.setVersionId(metaResult.getVersionId() + 1);
                    objectMeta.setParentId1(metaResult.getParentId1());
                    objectMeta.setParentId2(metaResult.getParentId2());
                    if (VersioningStatusType.NONE == versioningStatusType
                            || (VersioningStatusType.SUSPENDED == versioningStatusType
                            && metaResult.getNoVersionFlag())) {
                        metaDao.updateMeta(connection, metaCsName, metaClName, bucketId,
                                objectName, null, objectMeta);
                        transaction.commit(connection);
                        deleteObject = metaResult;
                    } else {
                        metaDao.insertMeta(connection, metaHisCSName, metaHisClName,
                                metaResult, true, region);
                        metaDao.updateMeta(connection, metaCsName, metaClName, bucketId,
                                objectName, null, objectMeta);
                        ObjectMeta nullMeta = null;
                        if (VersioningStatusType.SUSPENDED == versioningStatusType) {
                            nullMeta = metaDao.queryForUpdate(connection, metaHisCSName, metaHisClName,
                                    bucketId, objectName, null, true);
                            if (null != nullMeta) {
                                metaDao.removeMeta(connection, metaHisCSName, metaHisClName, bucketId,
                                        objectName, nullMeta.getVersionId(), null);
                            }
                            deleteObject = nullMeta;
                        }
                        transaction.commit(connection);
                    }
                }
            } catch (S3ServerException e) {
                transaction.rollback(connection);
                if (e.getError().getErrIndex() == S3Error.DAO_DUPLICATE_KEY.getErrIndex() && tryTime > 0) {
                    continue;
                } else {
                    throw e;
                }
            } catch (Exception e) {
                transaction.rollback(connection);
                throw e;
            } finally {
                daoMgr.releaseConnectionDao(connection);
            }
            deleteObjectLob(deleteObject);
            return;
        }
    }

    private Long getInnerVersionId(String metaCsName, String metaClName,
                                   String metaHisCsName, String metaHisClName, String keyMarker, String versionIdMarker, Bucket bucket)
            throws S3ServerException{
        try {
            Long versionId = null;
            if (versionIdMarker.equals(ObjectMeta.NULL_VERSION_ID)) {
                ObjectMeta metaCur = metaDao.queryMetaByObjectName(metaCsName, metaClName, bucket.getBucketId(), keyMarker, null, true);
                if (metaCur != null){
                    versionId = metaCur.getVersionId();
                }else {
                    ObjectMeta metaHis = metaDao.queryMetaByObjectName(metaHisCsName, metaHisClName, bucket.getBucketId(), keyMarker, null, true);
                    if (metaHis != null) {
                        versionId = metaHis.getVersionId();
                    }
                }
            } else {
                versionId = Long.parseLong(versionIdMarker);
            }
            return versionId;
        }catch (NumberFormatException e){
            logger.error("Parse versionIdMarker failed, versionIdMarker:{}",
                    versionIdMarker);
            throw new S3ServerException(S3Error.OBJECT_INVALID_VERSION,
                    "Parse versionIdMarker failed, versionIdMarker:"+ versionIdMarker, e);
        }catch (Exception e){
            logger.error("isExistKeyVersion failed, versionIdMarker:{}",
                    versionIdMarker);
            throw new S3ServerException(S3Error.OBJECT_INVALID_VERSION,
                    "isExistKeyVersion failed, versionIdMarker:"+ versionIdMarker, e);
        }
    }

    private void analyseRangeWithLob(Range range, DataLob dataLob) throws S3ServerException{
        if (null == range){
            return;
        }

        long contentLength = dataLob.getSize();
        if (range.getStart() >= contentLength){
            throw new S3ServerException(S3Error.OBJECT_RANGE_NOT_SATISFIABLE,
                    "start > contentlength. start:" + range.getStart() +
                            ", contentlength:" + contentLength);
        }

        //final bytes
        if (range.getStart() == -1){
            if(range.getEnd() < contentLength) {
                range.setStart(contentLength - range.getEnd());
                range.setEnd(contentLength-1);
            }else {
                range.setStart(0);
                range.setEnd(contentLength-1);
            }
        }

        //from start to the final of Lob
        if (range.getEnd() == -1 || range.getEnd() >= contentLength){
            range.setEnd(contentLength - 1);
        }

        //from 0 - final of Lob
        if (range.getStart() == 0 && range.getEnd() == contentLength - 1){
            range.setContentLength(contentLength);
            return;
        }

        long readLength  = range.getEnd() - range.getStart() + 1;
        range.setContentLength(readLength);
    }

    private int getXMetaLength(Map<String, String> xMeta){
        if (xMeta == null){
            return 0;
        }

        int length = 0;
        int prefixLength = RestParamDefine.PutObjectHeader.X_AMZ_META_PREFIX.length();
        for (Map.Entry<String, String> entry : xMeta.entrySet()){
            String headerName = entry.getKey();
            String headerValue = entry.getValue();
            if(headerName.startsWith(RestParamDefine.PutObjectHeader.X_AMZ_META_PREFIX)){
                length += (headerName.length()-prefixLength);
                length += headerValue.length();
            }
        }

        return length;
    }

    private long getSecondTime(long millionSecond){
        return millionSecond/1000;
    }
}
