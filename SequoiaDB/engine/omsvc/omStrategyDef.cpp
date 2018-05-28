/*******************************************************************************


   Copyright (C) 2011-2018 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

   Source File Name = omStrategyDef.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          01/18/2016  Li Jianhua  Initial Draft

   Last Changed =

*******************************************************************************/
#include "omStrategyDef.hpp"
#include "omDef.hpp"
#include "pd.hpp"

using namespace bson ;
namespace engine
{
   void _omTaskStrategyInfo::setTaskID( INT64 newTaskID )
   {
      taskID = newTaskID ;
   }

   INT32 _omTaskStrategyInfo::toBSON( BSONObj &obj )
   {
      BSONArrayBuilder arrBuilder ;
      std::set<std::string>::iterator iter = ips.begin() ;
      while( iter != ips.end() )
      {
         if ( !iter->empty() )
         {
            arrBuilder.append( *iter ) ;
         }
         ++iter ;
      }

      // userName and ips maybe empty,
      // we keep the empty field in the record to uniform query interface
      BSONObjBuilder objBuilder ;
      objBuilder.append( OM_REST_FIELD_RULE_ID, _id ) ;
      objBuilder.append( OM_REST_FIELD_TASK_ID, taskID ) ;
      objBuilder.append( OM_REST_FIELD_TASK_NAME, taskName ) ;
      objBuilder.append( OM_REST_FIELD_NICE, nice ) ;
      objBuilder.append( OM_REST_FIELD_USER_NAME, userName ) ;
      objBuilder.appendArray( OM_REST_FIELD_IPS, arrBuilder.arr() ) ;

      obj = objBuilder.obj() ;
      return SDB_OK ;
   }

   INT32 _omTaskStrategyInfo::fromBSON( const BSONObj &obj )
   {
      INT32 rc = SDB_OK ;
      BSONElement beField ;
      BSONObj ipsObj ;

      beField = obj.getField( OM_REST_FIELD_RULE_ID ) ;
      PD_CHECK( beField.isNumber(), SDB_INVALIDARG, error, PDERROR,
                "Failed to parse the field(%s)1", OM_REST_FIELD_RULE_ID ) ;
      _id = beField.numberLong() ;

      beField = obj.getField( OM_REST_FIELD_TASK_ID ) ;
      PD_CHECK( beField.isNumber(), SDB_INVALIDARG, error, PDERROR,
                "Failed to parse the field(%s)1", OM_REST_FIELD_TASK_ID ) ;
      taskID = beField.numberLong() ;

      beField = obj.getField( OM_REST_FIELD_TASK_NAME ) ;
      PD_CHECK( beField.type() == String, SDB_INVALIDARG, error, PDERROR,
                "Failed to parse the field(%s)1", OM_REST_FIELD_TASK_NAME ) ;
      taskName = beField.str();

      beField = obj.getField( OM_REST_FIELD_NICE ) ;
      PD_CHECK( beField.isNumber(), SDB_INVALIDARG, error, PDERROR,
                "Failed to parse the field(%s)1", OM_REST_FIELD_NICE ) ;
      taskID = beField.numberInt() ;

      beField = obj.getField( OM_REST_FIELD_USER_NAME ) ;
      PD_CHECK( beField.type() == String, SDB_INVALIDARG, error, PDERROR,
                "Failed to parse the field(%s)1", OM_REST_FIELD_USER_NAME ) ;
      userName = beField.str();

      beField = obj.getField( OM_REST_FIELD_IPS ) ;
      PD_CHECK( beField.type() == Array, SDB_INVALIDARG, error, PDERROR,
                "Failed to parse the field(%s)1", OM_REST_FIELD_IPS ) ;
      {
      BSONObjIterator iter( beField.embeddedObject() ) ;
      BSONElement beTmp ;
      while( iter.more() )
      {
         std::string strTmp ;
         beTmp = iter.next() ;
         PD_CHECK( beField.type() == String, SDB_INVALIDARG, error, PDERROR,
                  "Failed to parse the field(%s)1", OM_REST_FIELD_IPS ) ;
         strTmp = beTmp.str() ;
         if ( !strTmp.empty() )
         {
            ips.insert( strTmp ) ;
         }
      }
      }

   done:
      return rc ;
   error:
      goto done ;
   }
}