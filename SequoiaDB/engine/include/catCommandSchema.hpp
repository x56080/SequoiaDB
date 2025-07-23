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

   Source File Name = catCommandSchema.hpp

   Descriptive Name = Catalogue schema commands.

   When/how to use: this program may be used on binary and text-formatted
   versions of runtime component. This file contains catalog command class.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          02/07/2023  HGM Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef CAT_COMMAND_SCHEMA_HPP__
#define CAT_COMMAND_SCHEMA_HPP__

#include "catCommand.hpp"
#include "utilSchema.hpp"

namespace engine
{

   /*
      _catCMDCreateSchema define
    */
   class _catCMDCreateSchema : public _catWriteCMDBase
   {
      CAT_DECLARE_CMD_AUTO_REGISTER()

   public:
      _catCMDCreateSchema() ;
      virtual ~_catCMDCreateSchema() ;

      INT32 init( const CHAR *pQuery,
                  const CHAR *pSelector = NULL,
                  const CHAR *pOrderBy = NULL,
                  const CHAR *pHint = NULL,
                  INT32 flags = 0,
                  INT64 numToSkip = 0,
                  INT64 numToReturn = -1 ) ;

      INT32 doit( _pmdEDUCB *cb,
                  rtnContextBuf &ctxBuf,
                  INT64 &contextID ) ;

      const CHAR *name() const
      {
         return CMD_NAME_CREATE_SCHEMA ;
      }

   private:
      utilSchema _schema ;
   } ;
   typedef class _catCMDCreateSchema catCMDCreateSchema ;

   /*
      _catCMDDropSchema define
    */
   class _catCMDDropSchema : public _catWriteCMDBase
   {
      CAT_DECLARE_CMD_AUTO_REGISTER()

   public:
      _catCMDDropSchema() ;
      virtual ~_catCMDDropSchema() ;

      INT32 init( const CHAR *pQuery,
                  const CHAR *pSelector = NULL,
                  const CHAR *pOrderBy = NULL,
                  const CHAR *pHint = NULL,
                  INT32 flags = 0,
                  INT64 numToSkip = 0,
                  INT64 numToReturn = -1 ) ;

      INT32 doit( _pmdEDUCB *cb,
                  rtnContextBuf &ctxBuf,
                  INT64 &contextID ) ;

      const CHAR *name() const
      {
         return CMD_NAME_DROP_SCHEMA ;
      }

   private:
      const CHAR *_schemaName ;
   } ;
   typedef class _catCMDDropSchema catCMDDropSchema ;

   /*
      _catCMDAlterSchema define
    */
   class _catCMDAlterSchema : public _catWriteCMDBase
   {
      CAT_DECLARE_CMD_AUTO_REGISTER()

   public:
      _catCMDAlterSchema() ;
      virtual ~_catCMDAlterSchema() ;

      INT32 init( const CHAR *pQuery,
                  const CHAR *pSelector = NULL,
                  const CHAR *pOrderBy = NULL,
                  const CHAR *pHint = NULL,
                  INT32 flags = 0,
                  INT64 numToSkip = 0,
                  INT64 numToReturn = -1 ) ;

      INT32 doit( _pmdEDUCB *cb,
                  rtnContextBuf &ctxBuf,
                  INT64 &contextID ) ;

      const CHAR *name() const
      {
         return CMD_NAME_ALTER_SCHEMA ;
      }

   protected:
      utilSchemaAlterAction _action ;
   } ;
   typedef class _catCMDAlterSchema catCMDAlterSchema ;

}

#endif // CAT_COMMAND_SCHEMA_HPP__
