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

   Source File Name = omToolCmdBase.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08/09/2019  HJW  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef OMTOOL_CMD_BASE_HPP_
#define OMTOOL_CMD_BASE_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "ossUtil.hpp"
#include "omToolOptions.hpp"
#include <iostream>
#include <string>

using namespace std ;

namespace omTool
{
   #define DECLARE_OMTOOL_CMD_AUTO_REGISTER()                           \
      public:                                                           \
         static omToolCmdBase* newThis() ;  \

   #define IMPLEMENT_OMTOOL_CMD_AUTO_REGISTER(theClass)                       \
      omToolCmdBase* theClass::newThis()          \
      {                                                                       \
         return SDB_OSS_NEW theClass() ;        \
      }                                                                       \
      _omToolCmdAssit theClass##Assit( theClass::newThis ) ;

   class omToolCmdBase : public SDBObject
   {
   public:
      omToolCmdBase() ;

      virtual ~omToolCmdBase() ;

      void setOptions( omToolOptions *options ) ;

      virtual INT32 doCommand() = 0 ;

      virtual const CHAR *name() = 0 ;

   protected:
      void _setErrorMsg( const CHAR *pMsg ) ;
      void _setErrorMsg( const string &msg ) ;

   protected:
      omToolOptions *_options ;

   private:
      string _errMsg ;
   } ;

   typedef omToolCmdBase* (*OMTOOL_NEW_FUNC)() ;

   class _omToolCmdAssit : public SDBObject
   {
   public:
      _omToolCmdAssit( OMTOOL_NEW_FUNC ) ;
      virtual ~_omToolCmdAssit() ;
   } ;

   struct _classComp
   {
      bool operator()( const CHAR *lhs, const CHAR *rhs ) const
      {
         return ossStrcasecmp( lhs, rhs ) < 0 ;
      }
   } ;

   typedef map<const CHAR*, OMTOOL_NEW_FUNC, _classComp> MAP_CMD ;

#if defined (_WINDOWS)
   typedef MAP_CMD::iterator MAP_CMD_IT ;
#else
   typedef map<const CHAR*, OMTOOL_NEW_FUNC>::iterator MAP_CMD_IT ;
#endif // _WINDOWS

   class _omToolCmdBuilder : public SDBObject
   {
   friend class _omToolCmdAssit ;

   public:
      _omToolCmdBuilder () ;
      ~_omToolCmdBuilder () ;

   public:
      omToolCmdBase *create( const CHAR *command ) ;

      void release( omToolCmdBase *&pCommand ) ;

      INT32 _register( const CHAR *name, OMTOOL_NEW_FUNC pFunc ) ;

      OMTOOL_NEW_FUNC _find( const CHAR *name ) ;

   private:
      MAP_CMD _cmdMap ;
   } ;

   _omToolCmdBuilder* getOmToolCmdBuilder() ;
}

#endif /* OMT_CMD_BASE_HPP_ */