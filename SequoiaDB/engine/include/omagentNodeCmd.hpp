/*******************************************************************************


   Copyright (C) 2011-2018 SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   Source File Name = omagentNodeCmd.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          20/08/2014  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef OMAGENT_NODECMD_HPP__
#define OMAGENT_NODECMD_HPP__

#include "omagentCmdBase.hpp"
#include <string>

using namespace bson ;
using namespace std ;

namespace engine
{

   /*
      _omaShutdownCmd define
   */
   class _omaShutdownCmd : public _omaCommand
   {
      DECLARE_OACMD_AUTO_REGISTER()

      virtual BOOLEAN needCheckBusiness() const { return FALSE ; }

      public:
         _omaShutdownCmd() ;
         virtual ~_omaShutdownCmd() ;

         virtual const CHAR * name () ;

         virtual INT32 init ( const CHAR *pInfomation ) ;

         virtual INT32 doit ( BSONObj &retObj ) ;
   } ;

   /*
      _omaSetPDLevelCmd define
   */
   class _omaSetPDLevelCmd : public _omaCommand
   {
      DECLARE_OACMD_AUTO_REGISTER()

      virtual BOOLEAN needCheckBusiness() const { return FALSE ; }

      public:
         _omaSetPDLevelCmd() ;
         virtual ~_omaSetPDLevelCmd() ;

         virtual const CHAR * name () ;

         virtual INT32 init ( const CHAR *pInfomation ) ;

         virtual INT32 doit ( BSONObj &retObj ) ;

      private:
         INT32       _pdLevel ;
   } ;

   /*
      _omaCreateNodeCmd define
   */
   class _omaCreateNodeCmd : public _omaCommand
   {
      DECLARE_OACMD_AUTO_REGISTER()

      virtual BOOLEAN needCheckBusiness() const { return FALSE ; }

      public:
         _omaCreateNodeCmd() ;
         virtual ~_omaCreateNodeCmd() ;

         virtual const CHAR * name () ;

         virtual INT32 init ( const CHAR *pInfomation ) ;

         virtual INT32 doit ( BSONObj &retObj ) ;

      protected:
         BSONObj        _config ;
         string         _roleStr ;
   } ;

   /*
      _omaRemoveNodeCmd define
   */
   class _omaRemoveNodeCmd : public _omaCreateNodeCmd
   {
      DECLARE_OACMD_AUTO_REGISTER()

      public:
         _omaRemoveNodeCmd() ;
         virtual ~_omaRemoveNodeCmd() ;

         virtual const CHAR * name () ;
         virtual INT32 doit ( BSONObj &retObj ) ;
   } ;

   /*
      _omaStartNodeCmd define
   */
   class _omaStartNodeCmd : public _omaCommand
   {
      DECLARE_OACMD_AUTO_REGISTER()

      virtual BOOLEAN needCheckBusiness() const { return FALSE ; }

      public:
         _omaStartNodeCmd() ;
         virtual ~_omaStartNodeCmd() ;

         virtual const CHAR * name () ;

         virtual INT32 init ( const CHAR *pInfomation ) ;

         virtual INT32 doit ( BSONObj &retObj ) ;

      protected:
         const CHAR        *_pData ;

   } ;

   /*
      _omaStopNodeCmd define
   */
   class _omaStopNodeCmd : _omaStartNodeCmd
   {
      DECLARE_OACMD_AUTO_REGISTER()

      public:
         _omaStopNodeCmd() ;
         virtual ~_omaStopNodeCmd() ;

         virtual const CHAR * name () ;

         virtual INT32 doit ( BSONObj &retObj ) ;
   } ;

}

#endif //OMAGENT_NODECMD_HPP__

