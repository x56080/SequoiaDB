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

   Source File Name = omagentRemoteUsrFile.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08/03/2016  WJM Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef OMAGENT_REMOTE_USRFILE_HPP_
#define OMAGENT_REMOTE_USRFILE_HPP_

#include "omagentCmdBase.hpp"
#include "omagentRemoteBase.hpp"

using namespace bson ;
using namespace std ;

namespace engine
{
   /*
      _remoteFileOpen define
   */
   class _remoteFileOpen : public _remoteExec
   {
      DECLARE_OACMD_AUTO_REGISTER()
      public:
         _remoteFileOpen() ;

         ~_remoteFileOpen() ;

         const CHAR *name() ;

         INT32 doit( BSONObj &retObj ) ;
   } ;

   /*
      _remoteFileRead define
   */
   class _remoteFileRead : public _remoteExec
   {
      DECLARE_OACMD_AUTO_REGISTER()
      public:
         _remoteFileRead() ;

         ~_remoteFileRead() ;

         INT32 init( const CHAR * pInfomation ) ;

         const CHAR *name() ;

         INT32 doit( BSONObj &retObj ) ;

      private:
         string _filename ;
         UINT32 _location ;
         UINT32 _size ;
   } ;

   /*
      _remoteFileWrite define
   */
   class _remoteFileWrite : public _remoteExec
   {
      DECLARE_OACMD_AUTO_REGISTER()
      public:
         _remoteFileWrite() ;

         ~_remoteFileWrite() ;

         INT32 init( const CHAR * pInfomation ) ;

         const CHAR *name() ;

         INT32 doit( BSONObj &retObj ) ;

      private:
         string _filename ;
         UINT32 _location ;
         UINT32 _size ;
         string _content ;
   } ;

   /*
      _remoteFileRemove define
   */
   class _remoteFileRemove : public _remoteExec
   {
      DECLARE_OACMD_AUTO_REGISTER()
      public:
         _remoteFileRemove() ;

         ~_remoteFileRemove() ;

         const CHAR *name() ;

         INT32 doit( BSONObj &retObj ) ;
   } ;

   /*
      _remoteFileExist define
   */
   class _remoteFileExist : public _remoteExec
   {
      DECLARE_OACMD_AUTO_REGISTER()
      public:
         _remoteFileExist() ;

         ~_remoteFileExist() ;

         const CHAR *name() ;

         INT32 doit( BSONObj &retObj ) ;
   } ;

   /*
      _remoteFileCopy define
   */
   class _remoteFileCopy : public _remoteExec
   {
      DECLARE_OACMD_AUTO_REGISTER()
      public:
         _remoteFileCopy() ;

         ~_remoteFileCopy() ;

         const CHAR *name() ;

         INT32 doit( BSONObj &retObj ) ;
   } ;

   /*
      _remoteFileMove define
   */
   class _remoteFileMove : public _remoteExec
   {
      DECLARE_OACMD_AUTO_REGISTER()
      public:
         _remoteFileMove() ;

         ~_remoteFileMove() ;

         const CHAR *name() ;

         INT32 doit( BSONObj &retObj ) ;
   } ;

   /*
      _remoteFileMkdir define
   */
   class _remoteFileMkdir : public _remoteExec
   {
      DECLARE_OACMD_AUTO_REGISTER()
      public:
         _remoteFileMkdir() ;

         ~_remoteFileMkdir() ;

         const CHAR *name() ;

         INT32 doit( BSONObj &retObj ) ;
   } ;

   /*
      _remoteFileFind define
   */
   class _remoteFileFind : public _remoteExec
   {
      DECLARE_OACMD_AUTO_REGISTER()
      public:
         _remoteFileFind() ;

         ~_remoteFileFind() ;

         const CHAR *name() ;

         INT32 doit( BSONObj &retObj ) ;

         INT32 _extractFindInfo( const CHAR* buf, bson::BSONObjBuilder &builder ) ;
   } ;

   /*
      _remoteFileChmod define
   */
   class _remoteFileChmod : public _remoteExec
   {
      DECLARE_OACMD_AUTO_REGISTER()
      public:
         _remoteFileChmod() ;

         ~_remoteFileChmod() ;

         const CHAR *name() ;

         INT32 doit( BSONObj &retObj ) ;
   } ;

   /*
      _remoteFileChown define
   */
   class _remoteFileChown : public _remoteExec
   {
      DECLARE_OACMD_AUTO_REGISTER()
      public:
         _remoteFileChown() ;

         ~_remoteFileChown() ;

         const CHAR *name() ;

         INT32 doit( BSONObj &retObj ) ;
   } ;

   /*
      _remoteFileChgrp define
   */
   class _remoteFileChgrp : public _remoteExec
   {
      DECLARE_OACMD_AUTO_REGISTER()
      public:
         _remoteFileChgrp() ;

         ~_remoteFileChgrp() ;

         const CHAR *name() ;

         INT32 doit( BSONObj &retObj ) ;
   } ;

   /*
      _remoteFileGetUmask define
   */
   class _remoteFileGetUmask : public _remoteExec
   {
      DECLARE_OACMD_AUTO_REGISTER()
      public:
         _remoteFileGetUmask() ;

         ~_remoteFileGetUmask() ;

         const CHAR *name() ;

         INT32 doit( BSONObj &retObj ) ;
   } ;

   /*
      _remoteFileSetUmask define
   */
   class _remoteFileSetUmask : public _remoteExec
   {
      DECLARE_OACMD_AUTO_REGISTER()
      public:
         _remoteFileSetUmask() ;

         ~_remoteFileSetUmask() ;

         const CHAR *name() ;

         INT32 doit( BSONObj &retObj ) ;
   } ;

   /*
      _remoteFileList define
   */
   class _remoteFileList : public _remoteExec
   {
      DECLARE_OACMD_AUTO_REGISTER()
      public:
         _remoteFileList() ;

         ~_remoteFileList() ;

         const CHAR *name() ;

         INT32 doit( BSONObj &retObj ) ;

         INT32 _extractListInfo( const CHAR* buf, BSONObjBuilder &builder,
                                 BOOLEAN showDetail ) ;
   } ;

   /*
      _remoteFileGetPathType define
   */
   class _remoteFileGetPathType : public _remoteExec
   {
      DECLARE_OACMD_AUTO_REGISTER()
      public:
         _remoteFileGetPathType() ;

         ~_remoteFileGetPathType() ;

         const CHAR *name() ;

         INT32 doit( BSONObj &retObj ) ;
   } ;

   /*
      _remoteFileIsEmptyDir define
   */
   class _remoteFileIsEmptyDir : public _remoteExec
   {
      DECLARE_OACMD_AUTO_REGISTER()
      public:
         _remoteFileIsEmptyDir() ;

         ~_remoteFileIsEmptyDir() ;

         const CHAR *name() ;

         INT32 doit( BSONObj &retObj ) ;
   } ;

   /*
      _remoteFileStat define
   */
   class _remoteFileStat : public _remoteExec
   {
      DECLARE_OACMD_AUTO_REGISTER()
      public:
         _remoteFileStat() ;

         ~_remoteFileStat() ;

         const CHAR *name() ;

         INT32 doit( BSONObj &retObj ) ;
   } ;

   /*
      _remoteFileMd5 define
   */
   class _remoteFileMd5 : public _remoteExec
   {
      DECLARE_OACMD_AUTO_REGISTER()
      public:
         _remoteFileMd5() ;

         ~_remoteFileMd5() ;

         const CHAR *name() ;

         INT32 doit( BSONObj &retObj ) ;
   } ;

   /*
      _remoteFileGetContentSize define
   */
   class _remoteFileGetContentSize : public _remoteExec
   {
      DECLARE_OACMD_AUTO_REGISTER()
      public:
         _remoteFileGetContentSize() ;

         ~_remoteFileGetContentSize() ;

         const CHAR *name() ;

         INT32 doit( BSONObj &retObj ) ;
   } ;
}
#endif