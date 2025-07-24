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

   
*******************************************************************************/
#ifndef SDB_CL_PTR__H
#define SDB_CL_PTR__H

class sdb_cl_auto_ptr ;
class sdb_cl ;
class sdb_cl_ref_ptr
{
public:

   friend class sdb_cl_auto_ptr ;

protected:

   sdb_cl_ref_ptr( sdb_cl *collection ) ;

   virtual ~sdb_cl_ref_ptr() ;

protected:
   sdb_cl                           *sdb_collection ;
   Atomic_int32                     ref ; 
} ;

class sdb_cl_auto_ptr
{
public:

   sdb_cl_auto_ptr() ;

   virtual ~sdb_cl_auto_ptr() ;

   sdb_cl_auto_ptr( sdb_cl *collection ) ;

   sdb_cl_auto_ptr( const sdb_cl_auto_ptr &other ) ;

   sdb_cl_auto_ptr & operator = ( sdb_cl_auto_ptr &other ) ;

   sdb_cl& operator *() ;

   sdb_cl* operator ->() ;

   int ref() ;

   void clear() ;

private:
   sdb_cl_ref_ptr                   *ref_ptr ;
} ;
#endif
