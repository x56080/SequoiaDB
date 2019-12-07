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
