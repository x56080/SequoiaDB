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

   Source File Name = omagentTPCmd.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/01/2019  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef OMAGENT_TP_CMD_HPP__
#define OMAGENT_TP_CMD_HPP__

#include "omagentCmdBase.hpp"

namespace engine
{

   /*
      _omaCreateTPCMD define
    */
   class _omaCreateTPCMD : public _omaCommand
   {
      DECLARE_OACMD_AUTO_REGISTER()

   public:
      _omaCreateTPCMD() ;
      virtual ~_omaCreateTPCMD() ;

   public:
      OSS_INLINE virtual BOOLEAN needCheckBusiness() const
      {
         return FALSE ;
      }

      OSS_INLINE virtual const CHAR *name()
      {
         return CMD_NAME_TP_CREATE ;
      }

      virtual INT32 init( const CHAR *information ) ;
      virtual INT32 doit( bson::BSONObj &retObject ) ;

   protected:
      bson::BSONObj _config ;
   } ;

   typedef class _omaCreateTPCMD omaCreateTPCMD ;

   /*
      _omaRemoveTPCMD define
    */
   class _omaRemoveTPCMD : public _omaCommand
   {
      DECLARE_OACMD_AUTO_REGISTER()

   public:
      _omaRemoveTPCMD() ;
      virtual ~_omaRemoveTPCMD() ;

   public:
      OSS_INLINE virtual BOOLEAN needCheckBusiness() const
      {
         return FALSE ;
      }

      OSS_INLINE virtual const CHAR *name()
      {
         return CMD_NAME_TP_REMOVE ;
      }

      virtual INT32 init( const CHAR *information ) ;
      virtual INT32 doit( bson::BSONObj &retObject ) ;
   } ;

   typedef class _omaRemoveTPCMD omaRemoveTPCMD ;

   /*
      _omaStartTPCMD define
    */
   class _omaStartTPCMD : public _omaCommand
   {
      DECLARE_OACMD_AUTO_REGISTER()

   public:
      _omaStartTPCMD() ;
      virtual ~_omaStartTPCMD() ;

   public:
      OSS_INLINE virtual BOOLEAN needCheckBusiness() const
      {
         return FALSE ;
      }

      OSS_INLINE virtual const CHAR *name()
      {
         return CMD_NAME_TP_START ;
      }

      virtual INT32 init( const CHAR *information ) ;
      virtual INT32 doit( bson::BSONObj &retObject ) ;
   } ;

   typedef class _omaStartTPCMD omaStartTPCMD ;

   /*
      _omaStopTPCMD define
    */
   class _omaStopTPCMD : public _omaCommand
   {
      DECLARE_OACMD_AUTO_REGISTER()

   public:
      _omaStopTPCMD() ;
      virtual ~_omaStopTPCMD() ;

   public:
      OSS_INLINE virtual BOOLEAN needCheckBusiness() const
      {
         return FALSE ;
      }

      OSS_INLINE virtual const CHAR *name()
      {
         return CMD_NAME_TP_STOP ;
      }

      virtual INT32 init( const CHAR *information ) ;
      virtual INT32 doit( bson::BSONObj &retObject ) ;
   } ;

   typedef class _omaStopTPCMD omaStopTPCMD ;

   /*
      _omaGetTPCMD define
    */
   class _omaGetTPCMD : public _omaCommand
   {
      DECLARE_OACMD_AUTO_REGISTER()

   public:
      _omaGetTPCMD() ;
      virtual ~_omaGetTPCMD() ;

   public:
      OSS_INLINE virtual BOOLEAN needCheckBusiness() const
      {
         return FALSE ;
      }

      OSS_INLINE virtual const CHAR *name()
      {
         return CMD_NAME_TP_GET ;
      }

      virtual INT32 init( const CHAR *information ) ;
      virtual INT32 doit( bson::BSONObj &retObject ) ;
   } ;

   typedef class _omaGetTPCMD omaGetTPCMD ;

}

#endif // OMAGENT_TP_CMD_HPP__
