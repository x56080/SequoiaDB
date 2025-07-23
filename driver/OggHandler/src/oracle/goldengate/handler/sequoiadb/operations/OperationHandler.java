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

   Source File Name = OperationHandler.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
package oracle.goldengate.handler.sequoiadb.operations;

/**
 * Created by chen on 2017/09/20.
 */

import oracle.goldengate.datasource.adapt.Op;
import oracle.goldengate.datasource.meta.TableMetaData;
import oracle.goldengate.handler.sequoiadb.DB;
import oracle.goldengate.handler.sequoiadb.HandlerProperties;
import org.bson.BSONObject;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public abstract class OperationHandler
{
    private static final Logger logger = LoggerFactory.getLogger(OperationHandler.class);
    protected HandlerProperties handlerProperties = null;

    public OperationHandler(HandlerProperties handlerProperties)
    {
        this.handlerProperties = handlerProperties;
    }

    public abstract void process(TableMetaData paramTableMetaData, Op paramOp, DB paramDB)
            throws Exception;

    public abstract BSONObject getFormattedData(TableMetaData paramTableMetaData, Op paramOp, DB paramDB)
            throws Exception; 


    public String getPrimaryKey(TableMetaData tableMetaData, Op op, boolean isBefore)
    {
        return this.handlerProperties.getOggPrimaryKey().getKey(tableMetaData, op, isBefore);
    }
}