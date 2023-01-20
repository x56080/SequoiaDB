/*******************************************************************************


   Copyright (C) 2011-2021 SequoiaDB Ltd.

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
