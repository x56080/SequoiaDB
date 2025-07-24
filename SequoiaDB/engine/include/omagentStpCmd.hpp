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

   Source File Name = omagentStpCmd.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/01/2019  HGM Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef OMAGENT_STP_CMD_HPP__
#define OMAGENT_STP_CMD_HPP__

#include "omagentCmdBase.hpp"

namespace engine
{

   /*
      _omaCreateStpCMD define
    */
   // _omaCreateStpCMD creates STP node
   class _omaCreateStpCMD : public _omaCommand
   {
      DECLARE_OACMD_AUTO_REGISTER()

   public:
      _omaCreateStpCMD() ;
      virtual ~_omaCreateStpCMD() ;

   public:
      OSS_INLINE virtual BOOLEAN needCheckBusiness() const
      {
         return FALSE ;
      }

      OSS_INLINE virtual const CHAR *name()
      {
         return CMD_NAME_STP_CREATE ;
      }

      virtual INT32 init( const CHAR *information ) ;
      virtual INT32 doit( bson::BSONObj &retObject ) ;

   protected:
      bson::BSONObj _config ;
   } ;

   typedef class _omaCreateStpCMD omaCreateStpCMD ;

   /*
      _omaRemoveStpCMD define
    */
   // _omaRemoveStpCMD removes STP node
   class _omaRemoveStpCMD : public _omaCommand
   {
      DECLARE_OACMD_AUTO_REGISTER()

   public:
      _omaRemoveStpCMD() ;
      virtual ~_omaRemoveStpCMD() ;

   public:
      OSS_INLINE virtual BOOLEAN needCheckBusiness() const
      {
         return FALSE ;
      }

      OSS_INLINE virtual const CHAR *name()
      {
         return CMD_NAME_STP_REMOVE ;
      }

      virtual INT32 init( const CHAR *information ) ;
      virtual INT32 doit( bson::BSONObj &retObject ) ;
   } ;

   typedef class _omaRemoveStpCMD omaRemoveStpCMD ;

   /*
      _omaStartStpCMD define
    */
   // _omaStartStpCMD starts STP node
   class _omaStartStpCMD : public _omaCommand
   {
      DECLARE_OACMD_AUTO_REGISTER()

   public:
      _omaStartStpCMD() ;
      virtual ~_omaStartStpCMD() ;

   public:
      OSS_INLINE virtual BOOLEAN needCheckBusiness() const
      {
         return FALSE ;
      }

      OSS_INLINE virtual const CHAR *name()
      {
         return CMD_NAME_STP_START ;
      }

      virtual INT32 init( const CHAR *information ) ;
      virtual INT32 doit( bson::BSONObj &retObject ) ;
   } ;

   typedef class _omaStartStpCMD omaStartStpCMD ;

   /*
      _omaStopStpCMD define
    */
   // _omaStopStpCMD stops STP node
   class _omaStopStpCMD : public _omaCommand
   {
      DECLARE_OACMD_AUTO_REGISTER()

   public:
      _omaStopStpCMD() ;
      virtual ~_omaStopStpCMD() ;

   public:
      OSS_INLINE virtual BOOLEAN needCheckBusiness() const
      {
         return FALSE ;
      }

      OSS_INLINE virtual const CHAR *name()
      {
         return CMD_NAME_STP_STOP ;
      }

      virtual INT32 init( const CHAR *information ) ;
      virtual INT32 doit( bson::BSONObj &retObject ) ;
   } ;

   typedef class _omaStopStpCMD omaStopStpCMD ;

   /*
      _omaGetStpCMD define
    */
   // _omaGetStpCMD gets STP node
   class _omaGetStpCMD : public _omaCommand
   {
      DECLARE_OACMD_AUTO_REGISTER()

   public:
      _omaGetStpCMD() ;
      virtual ~_omaGetStpCMD() ;

   public:
      OSS_INLINE virtual BOOLEAN needCheckBusiness() const
      {
         return FALSE ;
      }

      OSS_INLINE virtual const CHAR *name()
      {
         return CMD_NAME_STP_GET ;
      }

      virtual INT32 init( const CHAR *information ) ;
      virtual INT32 doit( bson::BSONObj &retObject ) ;
   } ;

   typedef class _omaGetStpCMD omaGetStpCMD ;

}

#endif // OMAGENT_STP_CMD_HPP__
