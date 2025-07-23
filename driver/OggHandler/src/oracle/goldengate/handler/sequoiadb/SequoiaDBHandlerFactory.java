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

   Source File Name = SequoiaDBHandlerFactory.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
package oracle.goldengate.handler.sequoiadb;

/**
 * Created by chen on 2017/09/20.
 */
import oracle.goldengate.datasource.DataSourceListener;
import oracle.goldengate.datasource.HandlerFactory;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class SequoiaDBHandlerFactory
        extends HandlerFactory
{
    private static final Logger logger = LoggerFactory.getLogger(SequoiaDBHandlerFactory.class);

    public DataSourceListener instantiateHandler()
    {
        return new SequoiaDBHandler();
    }
}
