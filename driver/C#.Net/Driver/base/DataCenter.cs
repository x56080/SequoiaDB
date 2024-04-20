/*
 * Copyright 2018 SequoiaDB Inc.
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

using System.Collections.Generic;
using SequoiaDB.Bson;

/** \namespace SequoiaDB
 *  \brief SequoiaDB Driver for C#.Net
 *  \author Zhaobo Tan
 */
namespace SequoiaDB
{
    /** \class DataCenter
     *  \brief Database operation interfaces of data center
     */
	public class DataCenter
	{
        internal string name;
        private Sequoiadb sdb;
        internal bool isBigEndian = false;

        /** \property Name
         *  \brief Return the name of current data center.
         *  \return The data center's name
         */
        public string Name
        {
            get { return name; }
        }

        /** \property SequoiaDB
         *  \brief Return the Sequoiadb handle of current data center.
         *  \return Sequoiadb object
         */
        public Sequoiadb SequoiaDB
        {
            get { return sdb; }
        }

        internal DataCenter(Sequoiadb sdb)
        {
            this.sdb = sdb;
            this.isBigEndian = sdb.isBigEndian;
        }

        /** \fn BsonDocument GetDetail()
         *  \brief Get the detail of data center.
         *  \return void
         *  \exception SequoiaDB.BaseException
         *  \exception System.Exception
         */
        public BsonDocument GetDetail()
        {
            string command = SequoiadbConstants.ADMIN_PROMPT + SequoiadbConstants.CMD_NAME_GET_DCINFO ;
            DBCursor cursor = RunCommand.RunGeneralCmd(sdb, command,
                                                       null, null, null, null, 0, 0, -1, -1);
            if (null == cursor)
            {
                throw new BaseException("SDB_SYS");
            }
            return cursor.Next();
        }

        /** \fn void ActivateDC()
         *  \brief Activate data center.
         *  \return void
         *  \exception SequoiaDB.BaseException
         *  \exception System.Exception
         */
        public void ActivateDC()
        {
            _DCCommon(SequoiadbConstants.CMD_VALUE_NAME_ACTIVATE, null);
        }

        /** \fn void DeactivateDC()
         *  \brief Deactivate data center.
         *  \return void
         *  \exception SequoiaDB.BaseException
         *  \exception System.Exception
         */
        public void DeactivateDC()
        {
            _DCCommon(SequoiadbConstants.CMD_VALUE_NAME_DEACTIVATE, null);
        }

        /** \fn void EnableReadOnly( bool isReadOnly )
         *  \brief Set data center to be read-only or not
         *  \param [in] isReadOnly To set data center to be read-only or not
         *  \return void         
         *  \exception SequoiaDB.BaseException
         *  \exception System.Exception
         */
        public void EnableReadOnly( bool isReadOnly )
        {
            if ( true == isReadOnly )
            {
                _DCCommon(SequoiadbConstants.CMD_VALUE_NAME_ENABLE_READONLY, null);
            }
            else
            {
                _DCCommon(SequoiadbConstants.CMD_VALUE_NAME_DISABLE_READONLY, null);
            }
        }

        private void _DCCommon(string str, BsonDocument info)
        {
            string command = SequoiadbConstants.ADMIN_PROMPT + SequoiadbConstants.CMD_NAME_ALTER_DC;
            // check
            if (null == str || "" == str)
            {
                throw new BaseException("SDB_INVALID");
            }
            // build obj
            BsonDocument newObj = new BsonDocument();
            newObj.Add(SequoiadbConstants.FIELD_NAME_ACTION, str);
            if (null != info)
            {
                newObj.Add(SequoiadbConstants.FIELD_OPTIONS, info);
            }
            // run command
            RunCommand.RunGeneralCmd(sdb, command,
                                     newObj, null, null, null, 0, 0, -1, -1 );
        }

	}
}
