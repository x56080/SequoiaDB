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

   Source File Name = int64-add-test.cpp

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
#include <iostream>
#include <fstream>
#include <cassert>
#include <cstdlib>
#include <ctime>

using namespace std ;

typedef long long INT64 ;
typedef unsigned long long UINT64 ;

//                    1234567890123456
const INT64 MAX64 = 0x7fffffffffffffffLL ;
const INT64 MIN64 = 0x8000000000000000LL ;

//                       1234567890123456
const INT64 SAFE_MAX = 0x3fffffffffffffffLL ;
const INT64 SAFE_MIN = 0xc000000000000000LL ;

int loop, count, sameCount, diffCount, overflowCount ;
INT64 *aNums, *bNums ;
ofstream ofs( "add.autoTest.txt", ofstream::out ) ;

INT64 rand64()
{
   int r[4] ;
   r[0] = rand() ;
   r[1] = rand() ;
   r[2] = 0 ;
   r[3] = rand() ;
   int i = rand() % 3 ;
   return *( (INT64*)(r+i) ) ;
}

bool addOverflowTest( INT64 a, INT64 b )
{
   if(b<0)  
   {
      return a < MIN64 - b ;
   }
   else  
   {
      return a > (MAX64 - b) ;
   }
}

void generateData()
{
   for( int i = 0; i < diffCount; ++i )
   {
      INT64 a, b ;
      a = -rand64() ;
      b = rand64() ;
      assert( (a^b) < 0 ) ;
      aNums[i] = a ;
      bNums[i] = b ;
   }
   for( int i = diffCount; i < sameCount+diffCount; ++i )
   {
      int r = rand() % 2 ;
      INT64 a, b ;
      if(0==r)
      {
         INT64 x = rand64() % SAFE_MAX ;
         INT64 y = rand64() % SAFE_MAX ;
         a = SAFE_MAX - x ;
         b = SAFE_MAX - y ;
      }
      else
      {
         INT64 x = rand64() % (-SAFE_MIN) ;
         INT64 y = rand64() % (-SAFE_MIN) ;
         a = SAFE_MIN + x ;
         b = SAFE_MIN + y ;
      }
      assert( (a^b) >= 0 && a <= SAFE_MAX && b <= SAFE_MAX ) ;
      assert( (a^b) >= 0 && a >= SAFE_MIN && b >= SAFE_MIN ) ;
      aNums[i] = a ;
      bNums[i] = b ;
   }
   for( int i = sameCount+diffCount; i < count; ++i )
   {
      INT64 a, b ;
      if(0==rand()%2)
      {
         a = rand64() ;
         b = rand64() ;
         a |= (1+SAFE_MAX) ;
         b |= (1+SAFE_MAX) ;
      }
      else
      {
         a = -rand64() ;
         b = -rand64() ;
         a &= (SAFE_MIN-1) ;
         b &= (SAFE_MIN-1) ;
      }

      if( !addOverflowTest(a,b) )
      {
         cout << "should be overflow ; "
              << i << " ; " << a << "," << b << "," <<(a+b)<< " ; "
              << hex << "0x" << a << ",0x" << b << ",0x" << (a+b) << dec
              << endl ;
      }
      assert( addOverflowTest(a,b) ) ;
      aNums[i] = a ;
      bNums[i] = b ;
   }
}

bool add_1( INT64 a, INT64 b )
{
   if( b >= 0 ) 
   {
      return a > MAX64 - b ;
   }
   else
   {
      return a < MIN64 - b ;
   }
}

bool add_2( INT64 a, INT64 b )
{
   if( (a^b) < 0 ) 
   {
      return false ;
   }
   else if( b >= 0 )
   {
      return a > MAX64 - b ;
   }
   else
   {
      return a < MIN64 - b ;
   }
}

bool add_3( INT64 a, INT64 b )
{
   if( (a^b) < 0 ) 
   {
      return false ;
   }
   else if( a >= 0 )
   {
      if( a <= SAFE_MAX && b <= SAFE_MAX )
      {
         return false ;
      }
      else
      {
         return b > MAX64 - a ;
      }
   }
   else // a < 0
   {
      if( a >= SAFE_MIN && b >= SAFE_MIN )
      {
         return false ;
      }
      else 
      {
         return b < MIN64 - a ;
      }
   }
}

typedef bool (*addFunc)(INT64,INT64) ;
void addTest( const char *desc, addFunc func )
{
   int ov = 0 ;
   clock_t beg, end ;
   beg = clock() ;
   for( int i = 0; i < loop; ++i )
      for( int x = 0; x < count; ++x )
         if( func(aNums[x],bNums[x]) )
            ++ov ;
   end = clock() ;
   cout << desc << " : " << endl ;

   cout << "   " << (ov/loop) << " overflow, " << (end-beg) << " clock ticks" << endl ;
}

inline clock_t useTime( addFunc func, int times )
{
   clock_t used  = 0 ;
   clock_t beg = 0, end = 0 ;
   for( int i = 0; i < times; ++i )
   {
      beg = clock() ;
      for( int lp = 0; lp < loop; ++lp )
         for( int x = 0; x < count; ++x )
            func( aNums[x], bNums[x] ) ;
      end = clock() ;
      used += (end-beg) ;
   }
   return used/times ;
}

inline void autoTestEach()
{
   int times = 5 ;
   generateData() ;
   cout << "diffCount = " << diffCount << ", sameCount = " << sameCount 
       << ", overflowCount = " << overflowCount << endl ;
   ofs << diffCount << ", " ;
   ofs << sameCount << ", " ;
   ofs << overflowCount << " : " ;
   ofs << useTime( add_1, times ) << ", " ;
   ofs << useTime( add_2, times ) << ", " ;
   ofs << useTime( add_3, times ) << endl ;
}

void autoTest( )
{
   loop = 1000 ;
   count = 10000 ;
   aNums = new INT64[count] ;
   bNums = new INT64[count] ;
   ofs << "diffCount, sameCount, overflowCount : "
          "overflow, diff+overflow, diff+safe+overflow ( clock ticks )" << endl ;
   ofs << endl ;
   for( diffCount = count, sameCount = 0, overflowCount = 0; 
        diffCount >= 0; diffCount -= 2000, sameCount += 2000 )
      autoTestEach() ;
   ofs << endl ;
   for( diffCount = count, sameCount = 0, overflowCount = 0; 
        diffCount >= 0; diffCount -= 2000, sameCount += 2000 )
      autoTestEach() ;
   ofs << endl ;
   for( diffCount = count, sameCount = 0, overflowCount = 0; 
        diffCount >= 0; diffCount -= 2000, overflowCount += 2000 )
      autoTestEach() ;
   ofs << endl ;
   for( sameCount = count, diffCount = 0, overflowCount = 0; 
        sameCount >= 0; sameCount -= 2000, overflowCount += 2000 )
      autoTestEach() ;
   cout << "done" << endl ;
}

int main( int cnt, char **argv )
{
   srand(time(0)) ;
   if( cnt > 1 )
   {
      autoTest() ;
      return 0 ;
   }
   cout << "input <loop> <diff-sign-count> <same-sign-but-not-overflow-count> <overflow-count>" << endl ;
   cin >> loop >> diffCount >> sameCount >> overflowCount ;
   count = diffCount + sameCount + overflowCount ;
   aNums = new INT64[count] ;
   bNums = new INT64[count] ;
   generateData() ;
   addTest( "overflow", add_1 ) ;
   addTest( "sign_check + overflow", add_2 ) ;
   addTest( "sign_check + safe_check + overflow", add_3 ) ;
}
