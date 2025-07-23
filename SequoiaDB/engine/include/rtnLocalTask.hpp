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

   Source File Name = rtnLocalTask.hpp

   Descriptive Name = Local Task

   When/how to use: this program may be used on binary and text-formatted
   versions of data management component. This file contains structure for
   DMS Temporary Storage Unit Management.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/27/2020  XJH Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef RTN_LOCAL_TASK_HPP__
#define RTN_LOCAL_TASK_HPP__

#include "dms.hpp"
#include "rtnLocalTaskFactory.hpp"
#include "ossUtil.hpp"
#include "../bson/bson.h"

using namespace bson ;

namespace engine
{

   /*
      Field Name define
   */
   #define RTN_LT_FIELD_FROM                       "From"
   #define RTN_LT_FIELD_TO                         "To"
   #define RTN_LT_FIELD_TIME                       "Time"

   /*
   Rename CS Format
   {
      _id: <id>,
      Type: 1,
      TypeDesp: "RENAMECS",
      From: "my",
      To: "test",
      Time: "2020-04-27-10:30:30.000000"
   }

   Rename CL Format
   {
      _id: <id>,
      Type: 2,
      TypeDesp: "RENAMECL",
      Form: "my.my",
      To: "my.test",
      Time: "2020-04-27-10:30:30.000000"
   }
   */
   /*
      _rtnLTRename define
   */
   class _rtnLTRename : public _rtnLocalTaskBase
   {
      public:
         _rtnLTRename() ;
         virtual ~_rtnLTRename() ;

         void           setInfo( const CHAR *from, const CHAR *to ) ;
         const CHAR*    getFrom() const ;
         const CHAR*    getTo() const ;

      public:
         virtual INT32     initFromBson( const BSONObj &obj ) ;
         virtual BOOLEAN   muteXOn ( const _rtnLocalTaskBase *pOther ) const ;

      protected:
         virtual void      _toBson( BSONObjBuilder &builder ) const ;

      private:
         CHAR  _from[ DMS_COLLECTION_FULL_NAME_SZ + 1 ] ;
         CHAR  _to[ DMS_COLLECTION_FULL_NAME_SZ + 1 ] ;
         CHAR  _time[ OSS_TIMESTAMP_STRING_LEN + 1 ] ;

   } ;
   typedef _rtnLTRename rtnLTRename ;

   /*
      _rtnLTRenameCS define
   */
   class _rtnLTRenameCS : public _rtnLTRename
   {
      RTN_DECLARE_LT_AUTO_REGISTER() ;
      public:
         _rtnLTRenameCS() ;
         virtual ~_rtnLTRenameCS() ;

         virtual RTN_LOCAL_TASK_TYPE getTaskType() const ;
   } ;
   typedef _rtnLTRenameCS rtnLTRenameCS ;

   /*
      _rtnLTRenameCL define
   */
   class _rtnLTRenameCL : public _rtnLTRename
   {
      RTN_DECLARE_LT_AUTO_REGISTER() ;
      public:
         _rtnLTRenameCL() ;
         virtual ~_rtnLTRenameCL() ;

         virtual RTN_LOCAL_TASK_TYPE getTaskType() const ;
   } ;
   typedef _rtnLTRenameCL rtnLTRenameCL ;

}

#endif //RTN_LOCAL_TASK_HPP__

