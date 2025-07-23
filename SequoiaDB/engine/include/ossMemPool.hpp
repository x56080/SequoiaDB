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

   Source File Name = ossMem.hpp

   Descriptive Name = Operating System Services Memory Header

   When/how to use: this program may be used on binary and text-formatted
   versions of OSS component. This file contains declares for all memory
   allocation/free operations.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          01/09/2019  CW  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef OSSMEMPOOL_HPP_
#define OSSMEMPOOL_HPP_

#define SDB_USE_UTIL_ALLOCATOR
//#define SDB_USE_BOOST_ALLOCATOR

#include "ossTypes.h"

#ifdef SDB_USE_BOOST_ALLOCATOR
   #include <boost/pool/pool_alloc.hpp>
#elif defined( SDB_USE_UTIL_ALLOCATOR )
   #include "utilPooledAllocator.hpp"
#endif

#include <map>
#include <set>
#include <list>
#include <string>
#include <vector>
#include <deque>

/*
 * Memory pool ideal for allocation of objects one chunk at a time, such as
 * std::list, std::map. Not optimal for contiuous allocation such as std::vector
 */
template<typename T>
struct ossPoolAllocator {

#ifdef SDB_USE_BOOST_ALLOCATOR
    typedef boost::fast_pool_allocator<T>       Type ;
#elif defined (SDB_USE_UTIL_ALLOCATOR)
    typedef engine::_utilPooledAllocator<T>     Type ;
#else
    typedef std::allocator<T>                   Type ;
#endif
};

/*
 * Containers that utilize the pool allocator. Currently only has map/list/set,
 * as these are the most commonly used containers. Will add other containers in
 * the future.
 * When C++11 is supported, the following structures should be modified to use
 * alias declaration instead.
 */

/*
 * Map utilizing memory pool
 */
template < typename K, typename V, class Compare = std::less<K> >
class ossPoolMap : public std::map<K, V, Compare, typename ossPoolAllocator<std::pair<const K, V> >::Type >{
  /**
   * DO NOT ADD ANY MEMBER/FUNCTION IN THIS CLASS
   * DO NOT USE THIS CLASS IN POLYMORPHISM
   */
};

/*
 * Multi map utilizing memory pool
 */
template < typename K, typename V, class Compare = std::less<K> >
class ossPoolMultiMap : public std::multimap<K, V, Compare, typename ossPoolAllocator<std::pair<const K, V> >::Type > {
   /**
    * DO NOT ADD ANY MEMBER/FUNCTION IN THIS CLASS
    * DO NOT USE THIS CLASS IN POLYMORPHISM
    */
};

/*
 * Set utilizing memory pool
 */
template < typename K, class Compare = std::less<K> >
class ossPoolSet : public std::set<K, Compare, typename ossPoolAllocator<K>::Type >{
  /**
   * DO NOT ADD ANY MEMBER/FUNCTION IN THIS CLASS
   * DO NOT USE THIS CLASS IN POLYMORPHISM
   */
};

/*
 * Multi Set utilizing memory pool
 */
template < typename K, class Compare = std::less<K> >
class ossPoolMultiSet : public std::multiset<K, Compare, typename ossPoolAllocator<K>::Type > {
   /**
    * DO NOT ADD ANY MEMBER/FUNCTION IN THIS CLASS
    * DO NOT USE THIS CLASS IN POLYMORPHISM
    */
};

typedef ossPoolSet<UINT64>             SET_UINT64 ;
typedef ossPoolSet<UINT32>             SET_UINT32 ;

/*
 * List utilizing memory pool
 */
template < typename K >
class ossPoolList : public std::list<K, typename ossPoolAllocator<K>::Type > {
   typedef typename std::list<K,typename ossPoolAllocator<K>::Type>::size_type    size_type ;

   public:
      ossPoolList() {}
      ossPoolList( size_type n, const K& val = K() )
      : std::list< K, typename ossPoolAllocator<K>::Type>( n, val )
      {}
  /**
   * DO NOT ADD ANY MEMBER/FUNCTION IN THIS CLASS
   * DO NOT USE THIS CLASS IN POLYMORPHISM
   */
};

/*
 * String
 */
typedef std::basic_string< char, std::char_traits<char>,
                           ossPoolAllocator<char>::Type > ossPoolString ;

typedef std::basic_stringstream< char, std::char_traits< char >,
                                 ossPoolAllocator< char >::Type > ossPoolStringStream ;

/*
 * Vector
 */
template< typename T >
class ossPoolVector : public std::vector< T, typename ossPoolAllocator<T>::Type > {
   typedef typename std::vector<T,typename ossPoolAllocator<T>::Type>::size_type    size_type ;

   public:
      ossPoolVector() {}

      ossPoolVector( size_type count, const T& value = T() )
      : std::vector<T,typename ossPoolAllocator<T>::Type>( count, value )
      {}

      template< typename InputIt >
      ossPoolVector( InputIt first, InputIt last )
      : std::vector<T,typename ossPoolAllocator<T>::Type>( first, last )
      {}
   /**
    * DO NOT ADD ANY MEMBER/FUNCTION IN THIS CLASS
    * DO NOT USE THIS CLASS IN POLYMORPHISM
    */
} ;

typedef ossPoolVector<UINT64>             VEC_UINT64 ;
typedef ossPoolVector<UINT32>             VEC_UINT32 ;
typedef ossPoolVector<INT32>              VEC_INT32 ;
typedef ossPoolVector<INT64>              VEC_INT64 ;
typedef ossPoolVector<BOOLEAN>            VEC_BOOLEAN ;
typedef ossPoolVector<std::string>        VEC_STRING ;
typedef ossPoolVector<ossPoolString>      VEC_POOLSTR ;
typedef ossPoolVector<const CHAR*>        VEC_POOLCHARSTR ;

/*
 * Deque
 */
template < typename K >
class ossPoolDeque : public std::deque< K, typename ossPoolAllocator<K>::Type >
{
   typedef typename std::deque< K, typename ossPoolAllocator<K>::Type >::size_type size_type ;

public:
   ossPoolDeque()
   : std::deque< K, typename ossPoolAllocator<K>::Type >()
   {
   }

   ossPoolDeque( size_type n, const K& val = K() )
   : std::deque< K, typename ossPoolAllocator<K>::Type >( n, val )
   {
   }

   /**
    * DO NOT ADD ANY MEMBER/FUNCTION IN THIS CLASS
    * DO NOT USE THIS CLASS IN POLYMORPHISM
    */
} ;

#endif


