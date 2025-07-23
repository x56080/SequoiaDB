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

   Source File Name = DBOperationFactory.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
package oracle.goldengate.handler.sequoiadb.util;

/**
 * Created by chen on 2017/09/20.
 */
import oracle.goldengate.handler.sequoiadb.HandlerProperties;
import oracle.goldengate.datasource.DsOperation;
import oracle.goldengate.handler.sequoiadb.operations.*;
import oracle.goldengate.util.ConfigException;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class DBOperationFactory
{
    private static final Logger logger = LoggerFactory.getLogger(DBOperationFactory.class);
    public OperationHandler insertOperationHandler;
    public OperationHandler updateOperationHandler;
    public OperationHandler deleteOperationHandler;
    public OperationHandler pkUpdateOperationHandler;
    public OperationHandler truncateOperationHandler;

    public void init(HandlerProperties handlerProperties)
    {
        this.insertOperationHandler = new InsertOperationHandler(handlerProperties);
        this.updateOperationHandler = new UpdateOperationHandler(handlerProperties);
        this.deleteOperationHandler = new DeleteOperationHandler(handlerProperties);
        this.pkUpdateOperationHandler = new PkUpdateOperationHandler(handlerProperties);
        this.truncateOperationHandler = new TruncateOperationHandler(handlerProperties);
    }

    public OperationHandler getInstance(DsOperation.OpType opType)
    {
        if (opType.isInsert()) {
            return this.insertOperationHandler;
        }
        if (opType.isPkUpdate()) {
            return this.pkUpdateOperationHandler;
        }
        if (opType.isUpdate()) {
            return this.updateOperationHandler;
        }
        if (opType.isDelete()) {
            return this.deleteOperationHandler;
        }
        if (opType.isTruncate())
        {
            return this.truncateOperationHandler;
        }
        return null;
    }
}
