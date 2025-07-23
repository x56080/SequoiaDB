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

   Source File Name = seAdptCommand.hpp

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef SEADPT_COMMAND_HPP__
#define SEADPT_COMMAND_HPP__

#include "pmdEDU.hpp"
#include "utilESClt.hpp"
#include "utilCommObjBuff.hpp"

using namespace engine ;

namespace seadapter
{
   class _seAdptCommand: public SDBObject
   {
      public:
         _seAdptCommand() {}
         virtual ~_seAdptCommand() {}

         virtual INT32 init( INT32 flags, INT64 numToSkip, INT64 numToReturn,
                             const CHAR *matcherBuff, const CHAR *selectorBuff,
                             const CHAR *orderByBuff, const CHAR *hintBuff ) = 0 ;
         virtual INT32 doit( pmdEDUCB *cb, utilCommObjBuff &objBuff ) = 0 ;
   } ;
   typedef _seAdptCommand seAdptCommand ;

   class _seAdptGetCount : public seAdptCommand
   {
      public:
         _seAdptGetCount() ;
         virtual ~_seAdptGetCount() {}

         INT32 init( INT32 flags, INT64 numToSkip, INT64 numToReturn,
                     const CHAR *matcherBuff, const CHAR *selectorBuff,
                     const CHAR *orderByBuff, const CHAR *hintBuff ) ;
         INT32 doit( _pmdEDUCB *cb, utilCommObjBuff &objBuff ) ;
      private:
         BOOLEAN _isMatchAll() ;

      private:
         const CHAR *_clFullName ;
         UINT16 _indexID ;
         BSONObj _condition ;
   } ;

   typedef _seAdptGetCount seAdptGetCount ;

   BOOLEAN seAdptIsCommand( const CHAR *name ) ;
   INT32 seAdptGetCommand( const CHAR *name, seAdptCommand *&command ) ;
   void seAdptReleaseCommand( seAdptCommand *&command ) ;
}

#endif /* SEADPT_COMMAND_HPP__ */
