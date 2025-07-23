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

   Source File Name = SequoiadbDataDao.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoias3.dao.sequoiadb;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBLob;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoias3.common.DBParamDefine;
import com.sequoias3.config.SequoiadbConfig;
import com.sequoias3.core.DataAttr;
import com.sequoias3.core.Region;
import com.sequoias3.dao.*;
import com.sequoias3.exception.S3Error;
import com.sequoias3.exception.S3ServerException;
import org.apache.commons.codec.binary.Hex;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.ObjectId;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Repository;

import java.io.*;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;

@Repository("DataDao")
public class SequoiadbDataDao implements DataDao {
    private static final Logger logger = LoggerFactory.getLogger(SequoiadbDataDao.class);

    private static int ONCE_WRITE_BYTES  = 2 * 1024 * 1024;       //2MB

    @Autowired
    SdbDataSourceWrapper sdbDatasourceWrapper;

    @Autowired
    SequoiadbConfig sdbConfig;

    @Autowired
    SequoiadbRegionSpaceDao sequoiadbRegionSpaceDao;

    @Autowired
    SdbBaseOperation sdbBaseOperation;

    @Override
    public DataAttr insertObjectData(String csName, String clName, InputStream data, Region region)
            throws S3ServerException {
        Sequoiadb sdb = null;
        try {
            sdb               = sdbDatasourceWrapper.getSequoiadb();
            DBLob dbLob       = createLobWithCsCl(sdb, csName, clName, region);
            MessageDigest MD5 = MessageDigest.getInstance("MD5");

            byte[] buffer    = new byte[ONCE_WRITE_BYTES];

            int size = 0;
            while (true){
                if ((size = readAsMuchAsPossible(data, buffer, 0, buffer.length)) > 0) {
                    //md5
                    MD5.update(buffer, 0, size);

                    //write lob
                    dbLob.write(buffer, 0, size);
                }else {
                    break;
                }
            }

            //record md5 lobId size
            DataAttr result = new DataAttr();
            result.seteTag(new String(Hex.encodeHex(MD5.digest())));
            result.setSize(dbLob.getSize());
            result.setLobId(dbLob.getID());

            //close lob
            dbLob.close();
            return result;
        } catch (S3ServerException e) {
            throw e;
        } catch (IOException e){
            throw new S3ServerException(S3Error.UNKNOWN_ERROR, "IOException", e);
        } catch (NoSuchAlgorithmException e){
            throw new S3ServerException(S3Error.UNKNOWN_ERROR, "NoSuchAlgorithmException", e);
        } catch (BaseException e){
            throw e;
        }catch (Exception e) {
            logger.error("create lob failed");
            throw e;
        } finally {
            sdbDatasourceWrapper.releaseSequoiadb(sdb);
        }
    }

    private int readAsMuchAsPossible(InputStream is, byte buf[], int offset, int length)
            throws IOException{
        if(offset < 0 || offset + length > buf.length){
            throw new RuntimeException("bufferLength = "+buf.length
                    +", offset=" + ", length=" + length);
        }

        if (length <= 0){
            return 0;
        }

        int maxLength  = offset + length;
        int realOffset = offset;
        int tempLength = 0;
        while (realOffset < maxLength && tempLength > -1){
            tempLength = is.read(buf, realOffset, maxLength - realOffset);
            if (tempLength > 0){
                realOffset += tempLength;
            }
        }

        if (realOffset > offset) {
            return realOffset - offset;
        }

        return -1;
    }

    private DBLob createLobWithCsCl(Sequoiadb sdb, String csName, String clName, Region region)
            throws S3ServerException{
        try {
            return createLob(sdb, csName, clName);
        } catch (BaseException e) {
            if (e.getErrorCode() == SDBError.SDB_DMS_CS_NOTEXIST.getErrorCode()
                || e.getErrorCode() == SDBError.SDB_DMS_NOTEXIST.getErrorCode()) {
                if (region != null && region.getDataLocation() != null) {
                    throw new S3ServerException(S3Error.REGION_LOCATION_NOT_EXIST,
                            "location not exist. csName="+csName+", clName="+clName);
                }

                BSONObject option   = null;
                String regionName   = null;
                String  domain      = null;
                Integer lobPageSize = null;
                Integer replSize    = null;
                if (region != null){
                    regionName  = region.getName();
                    domain      = region.getDataDomain();
                    lobPageSize = region.getDataLobPageSize();
                    replSize    = region.getDataReplSize();
                }else {
                    domain      = sdbConfig.getDataDomain();
                    lobPageSize = sdbConfig.getDataLobPageSize();
                    replSize    = sdbConfig.getDataReplSize();
                }

                if (!sdb.isCollectionSpaceExist(csName)) {
                    option = new BasicBSONObject();
                    if (domain != null) {
                        option.put("Domain", domain);
                    }
                    if (lobPageSize != null) {
                        option.put("LobPageSize", lobPageSize);
                    }
                    if(DBParamDefine.CREATE_OK == sdbBaseOperation.createCS(sdb, csName, option)){
                        sequoiadbRegionSpaceDao.insertRegionCSList(csName, regionName);
                    }
                }

                BSONObject clOption = generateDataCLOption(replSize);
                sdbBaseOperation.createCL(sdb, csName, clName, clOption);
                return createLob(sdb, csName, clName);
            } else {
                logger.error("create lob failed. error:"+e);
                throw e;
            }
        }
    }

    private BSONObject generateDataCLOption(Integer replSize){
        BSONObject clOption = new BasicBSONObject();

        BSONObject shardingKey = new BasicBSONObject("_id", 1);
        clOption.put("AutoIndexId", false);
        clOption.put("ShardingKey", shardingKey);
        clOption.put("ShardingType", "hash");
        if (replSize != null){
            clOption.put("ReplSize", replSize);
        }else {
            clOption.put("ReplSize", -1);
        }
        clOption.put("AutoSplit", true);

        return clOption;
    }

    private DBLob createLob(Sequoiadb sdb, String csName, String clName){
        try{
            CollectionSpace cs = sdb.getCollectionSpace(csName);
            DBCollection cl = cs.getCollection(clName);

            return cl.createLob();
        }catch (Exception e){
            logger.error("create lob failed. error:"+e);
            throw e;
        }
    }

    @Override
    public DataLob getDataLobForRead(String csName, String clName, ObjectId lobId)throws S3ServerException {
        Sequoiadb sdb = null;
        DBLob dbLob = null;
        try {
            sdb = sdbDatasourceWrapper.getSequoiadb();
            CollectionSpace cs = sdb.getCollectionSpace(csName);
            DBCollection cl = cs.getCollection(clName);

            dbLob = cl.openLob(lobId);
            return new SdbDataLob(sdb, dbLob);
        } catch (BaseException e) {
            closeLob(dbLob);
            sdbDatasourceWrapper.releaseSequoiadb(sdb);
            if (e.getErrorCode() == SDBError.SDB_DMS_CS_NOTEXIST.getErrorCode()
                    || e.getErrorCode() == SDBError.SDB_DMS_NOTEXIST.getErrorCode()) {
                //no cs or no cl
                throw new S3ServerException(S3Error.OBJECT_NO_SUCH_KEY,
                        "cs:" + csName + ", cl:" + clName + ", error message:" + e.toString());
            } else if (e.getErrorCode() == SDBError.SDB_FNE.getErrorCode()) {
                //no lob
                throw new S3ServerException(S3Error.DAO_LOB_FNE,
                        "cs:" + csName + ", cl:" + clName + ", error message:" + e.toString());
            } else {
                logger.error("get lob failed. error:");
                throw e;
            }
        } catch (Exception e) {
            closeLob(dbLob);
            sdbDatasourceWrapper.releaseSequoiadb(sdb);
            logger.error("get lob failed.");
            throw e;
        }
    }

    @Override
    public void releaseDataLob(DataLob dataLob){
        if (null != dataLob){
            SdbDataLob sdbDataLob = (SdbDataLob)dataLob;
            closeLob(sdbDataLob.getDbLob());
            sdbDatasourceWrapper.releaseSequoiadb(sdbDataLob.getSdb());
            sdbDataLob.setDbLob(null);
            sdbDataLob.setSdb(null);
        }
    }

    @Override
    public void deleteObjectDataByLobId(ConnectionDao connection, String csName, String clName, ObjectId lobId)
            throws S3ServerException {
        Sequoiadb sdb = null;
        try {
            if (connection != null){
                sdb = ((SdbConnectionDao)connection).getConnection();
            }else {
                sdb = sdbDatasourceWrapper.getSequoiadb();
            }
            CollectionSpace cs = sdb.getCollectionSpace(csName);
            DBCollection cl = cs.getCollection(clName);

            cl.removeLob(lobId);
        }catch (BaseException e){
            if (e.getErrorCode() == SDBError.SDB_DMS_CS_NOTEXIST.getErrorCode()
                    || e.getErrorCode() == SDBError.SDB_DMS_NOTEXIST.getErrorCode()
                    || e.getErrorCode() == SDBError.SDB_FNE.getErrorCode()) {
                //no cs or no cl or no lob ,return null
                return;
            }else if (e.getErrorCode() == SDBError.SDB_LOB_IS_IN_USE.getErrorCode()){
                throw new S3ServerException(S3Error.OBJECT_IS_IN_USE,
                        "lob is in use. cs" + csName +", cl:" + clName +", LobId:" +lobId.toString());
            }else {
                logger.error("delete lob failed. error:",e);
                throw e;
            }
        } catch (Exception e){
            logger.error("delete lob failed.",e);
            throw e;
        } finally {
            if (null == connection) {
                sdbDatasourceWrapper.releaseSequoiadb(sdb);
            }
        }
    }

    private void closeLob(DBLob lob){
        if (null == lob) {
            return;
        }
        try {
            lob.close();
        }catch (Exception e){
            logger.error("lob close failed.", e);
        }
    }

}
