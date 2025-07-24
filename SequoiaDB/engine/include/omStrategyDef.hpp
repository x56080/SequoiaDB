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

   Source File Name = omStrategyDef.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          01/18/2016  Li Jianhua  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef OM_STRATEGY_DEF_HPP_
#define OM_STRATEGY_DEF_HPP_


#include "ossTypes.h"
#include "../bson/bson.h"
#include <boost/shared_ptr.hpp>


namespace engine
{
#define OM_TASK_STRATEGY_NICE_MAX                  19
#define OM_TASK_STRATEGY_NICE_MIN                  -20
#define OM_TASK_STRATEGY_NICE_DEF                  0
#define OM_TASK_STRATEGY_INVALID_VER               -1

   typedef struct _omTaskStrategyInfo
   {
   private:
      INT64                      taskID ;
   public:
      INT64                      _id  ;
      INT32                      nice ;
      std::string                taskName ;
      std::string                userName ;
      std::set<std::string>      ips ;

   public:
      friend class omStrategyMgr ;
      _omTaskStrategyInfo()
      {
         _id = 0 ;
         nice = 0 ;
         taskID = 0 ;
      }

      INT32 toBSON( bson::BSONObj &obj ) ;
      INT32 fromBSON( const bson::BSONObj &obj ) ;

      BOOLEAN isMatch( const std::string &userName, const std::string &ip ) ;

   protected:
      void setTaskID( INT64 newTaskID ) ;

   }omTaskStrategyInfo ;

   typedef boost::shared_ptr< omTaskStrategyInfo >          taskStrategyInfoPtr ;
}
#endif
