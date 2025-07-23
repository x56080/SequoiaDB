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

   Source File Name = RegionDao.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoias3.dao;

import com.sequoiadb.base.Sequoiadb;
import com.sequoias3.core.Region;
import com.sequoias3.exception.S3ServerException;

import java.util.Date;
import java.util.List;

public interface RegionDao {
    void insertRegion(ConnectionDao connection, Region regionCon) throws S3ServerException;
    void updateRegion(ConnectionDao connection, Region regionCon) throws S3ServerException;
    void deleteRegion(ConnectionDao connection, String regionName) throws S3ServerException;
    Region queryForUpdateRegion(ConnectionDao connection, String regionName) throws S3ServerException;

    Region queryRegion(String regionName) throws S3ServerException;
    List<String> queryRegionList() throws S3ServerException;

    void detectDomain(ConnectionDao connection, String domain) throws S3ServerException;
    void detectLocation(ConnectionDao connection, String CSName, String CLName, int locationType)
            throws S3ServerException;

    String getMetaCurCSName(Region region);

    String getMetaCurCLName(Region region);

    String getMetaHisCSName(Region region);

    String getMetaHisCLName(Region region);

    String getDataCSName(Region region, Date date);

    String getDataClName(Region region, Date date);

    void createMetaCSCL(Region region, String csMetaName,
                        String clMetaName, Boolean isHistory)
            throws S3ServerException;

    void createDirCSCL(Region region, String metaCsName) throws S3ServerException;
}
