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

   Source File Name = bsonintrusiveptr.h

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef BSONINTRUSIVEPTR_HPP__
#define BSONINTRUSIVEPTR_HPP__

namespace bson
{
   template<class T>class bson_intrusive_ptr
   {
   private :
      typedef bson_intrusive_ptr this_type ;
   public :
      typedef T element_type ;
      bson_intrusive_ptr(): px(0)
      {}
      bson_intrusive_ptr( T * p, bool add_ref = true ) : px (p)
      {
         if ( px != 0 && add_ref ) intrusive_ptr_add_ref ( px ) ;
      }
      bson_intrusive_ptr(bson_intrusive_ptr const &rhs ) : px ( rhs.px )
      {
         if ( px != 0 ) intrusive_ptr_add_ref ( px ) ;
      }
      ~bson_intrusive_ptr ()
      {
         if ( px != 0 ) intrusive_ptr_release ( px ) ;
      }
      bson_intrusive_ptr & operator=(bson_intrusive_ptr const & rhs)
      {
         this_type(rhs).swap(*this);
         return *this;
      }
      bson_intrusive_ptr & operator=(T * rhs)
      {
         this_type(rhs).swap(*this);
         return *this;
      }
      void reset()
      {
         this_type().swap( *this );
      }
      void reset( T * rhs )
      {
         this_type( rhs ).swap( *this );
      }
      T * get() const
      {
         return px;
      }
      T & operator*() const
      {
         return *px;
      }
      T * operator->() const
      {
         return px;
      }
      void swap(bson_intrusive_ptr & rhs)
      {
         T * tmp = px;
         px = rhs.px;
         rhs.px = tmp;
      }
   private:
      T * px;
   };
}

#endif
