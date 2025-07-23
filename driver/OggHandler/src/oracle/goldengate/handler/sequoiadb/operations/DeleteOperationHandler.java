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

   Source File Name = DeleteOperationHandler.java

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

import java.util.HashMap;
import java.util.ListIterator;
import java.util.Map;
import oracle.goldengate.datasource.adapt.Col;
import oracle.goldengate.datasource.adapt.Op;
import oracle.goldengate.datasource.meta.TableMetaData;
import oracle.goldengate.handler.sequoiadb.DB;
import oracle.goldengate.handler.sequoiadb.HandlerProperties;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class DeleteOperationHandler
        extends OperationHandler
{
    private static final Logger logger = LoggerFactory.getLogger(DeleteOperationHandler.class);
    private String tableName;

    public DeleteOperationHandler(HandlerProperties handlerProperties)
    {
        super(handlerProperties);
    }

    public void process(TableMetaData tableMetaData, Op op, DB db)
            throws Exception
    {
        if(this.handlerProperties.getIsPrintInfo()) {
            logger.info("DeleteOperationHandler's process");
        }
        this.tableName = tableMetaData.getTableName().getOriginalShortName();
        BSONObject obj = getFormattedData(tableMetaData, op, db);
        if (handlerProperties.getCheckMaxRowSizeLimit()) {
            db.checkMaxRowSizeLimit(obj);
        }
        db.deleteOne(tableMetaData, obj);
    }

    public BSONObject getFormattedData(TableMetaData tableMetaData, Op op, DB db)
    {
        Map<String, Object> map = new HashMap();
        BSONObject obj = new BasicBSONObject();
        for (ListIterator localListIterator = op.iterator(); localListIterator.hasNext();)
        {
            Col c = (Col)localListIterator.next();
            if (c.getMeta().isKeyCol())
            {
                Object columnValue = DB.getColumnValue(c, true);
//                keyDocument.append(c.getOriginalName(), columnValue);
                if (this.handlerProperties.getChangeFieldToLowCase())
                    obj.put (c.getOriginalName().toLowerCase(), columnValue);
                else
                    obj.put (c.getOriginalName(), columnValue);
                    
            }
        }
        return obj;
    }
}
