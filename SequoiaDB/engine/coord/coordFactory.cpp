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

   Source File Name = coordFactory.cpp

   Descriptive Name = Coord

   When/how to use: this program may be used on binary and text-formatted
   versions of runtime component. This file contains code logic for
   command factory on coordinator node.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/19/2017  XJH Initial Draft
   Last Changed =

*******************************************************************************/
#include "coordFactory.hpp"
#include "ossErr.h"
#include "pdTrace.hpp"
#include "coordTrace.hpp"

namespace engine
{

   /*
      _coordCommandFactory implement
   */
   _coordCommandFactory::_coordCommandFactory()
   {
   }

   _coordCommandFactory::~_coordCommandFactory()
   {
      _mapCommand.clear() ;
   }

   INT32 _coordCommandFactory::create( const CHAR *pCmdName,
                                       coordOperator *&pOperator )
   {
      INT32 rc = SDB_OK ;
      SDB_ASSERT( NULL == pOperator, "Operator must be NULL" ) ;

      if ( !pCmdName || !(*pCmdName) )
      {
         rc = SDB_INVALIDARG ;
      }
      else
      {
         MAP_COMMAND_IT it = _mapCommand.find( pCmdName ) ;
         if ( it != _mapCommand.end() )
         {
            coordFactoryItem &item = it->second ;
            pOperator = (*item._pFunc)() ;
            if ( !pOperator )
            {
               rc = SDB_OOM ;
            }
            else
            {
               pOperator->setReadOnly( item._isReadOnly ) ;
            }
         }
         else
         {
            rc = SDB_COORD_UNKNOWN_OP_REQ ;
         }
      }
      return rc ;
   }

   void _coordCommandFactory::release( coordOperator *pOperator )
   {
      if ( pOperator )
      {
         SDB_OSS_DEL pOperator ;
         pOperator = NULL ;
      }
   }

   INT32 _coordCommandFactory::_register( const CHAR *pCmdName,
                                          BOOLEAN isReadOnly,
                                          COORD_NEW_OPERATOR pFunc )
   {
      INT32 rc = SDB_OK ;

      if ( !pCmdName || !(*pCmdName) || !pFunc )
      {
         SDB_ASSERT( FALSE, "Invalid parameters" ) ;
         rc = SDB_SYS ;
      }
      else
      {
         MAP_COMMAND_IT it = _mapCommand.find( pCmdName ) ;
         if ( it != _mapCommand.end() )
         {
            SDB_ASSERT( FALSE, "Command already exist" ) ;
            rc = SDB_SYS ;
         }
         else
         {
            try
            {
               coordFactoryItem item( isReadOnly, pFunc ) ;
               _mapCommand.insert( MAP_COMMAND::value_type( pCmdName, item ) ) ;
            }
            catch( std::exception &e )
            {
               rc = SDB_OOM ;
            }
         }
      }
      return rc ;
   }

   coordCommandFactory* coordGetFactory()
   {
      static coordCommandFactory s_factory ;
      return &s_factory ;
   }

   /*
      _coordCommandAssit implement
   */
   _coordCommandAssit::_coordCommandAssit( const CHAR *pCmdName,
                                           BOOLEAN isReadOnly,
                                           COORD_NEW_OPERATOR pFunc )
   {
      coordGetFactory()->_register( pCmdName, isReadOnly, pFunc ) ;
   }

   _coordCommandAssit::_coordCommandAssit( COORD_NEW_OPERATOR pFunc )
   {
      coordOperator *pOperator = (*pFunc)() ;
      if ( pOperator )
      {
         coordGetFactory()->_register( pOperator->getName(),
                                       pOperator->isReadOnly(),
                                       pFunc ) ;
         SDB_OSS_DEL pOperator ;
      }
      else
      {
         SDB_ASSERT( FALSE, "Create operator failed" ) ;
      }
   }

   _coordCommandAssit::~_coordCommandAssit()
   {
   }

}

