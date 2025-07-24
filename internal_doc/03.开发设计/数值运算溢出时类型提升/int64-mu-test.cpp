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

   Source File Name = int64-mu-test.cpp

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
#include <ctime>
#include <cstdlib>
#include <cassert>
using namespace std ;

typedef long long INT64 ;
typedef unsigned long long UINT64 ;

//                    1234567890123456
const INT64 MAX64 = 0x7fffffffffffffffLL ;
const INT64 MIN64 = 0x8000000000000000LL ;

const INT64 MAX32 = 0x000000007fffffffLL ;

//                      1234567890123456
const INT64 POW2_31 = 0x0000000080000000LL ;
const INT64 POW2_39 = POW2_31 << 8 ;
const INT64 POW2_23 = POW2_31 >> 8 ;
const INT64 POW2_47 = POW2_31 << 16 ;
const INT64 POW2_15 = POW2_31 >> 16 ;
const INT64 POW2_55 = POW2_31 << 24 ;
const INT64 POW2_7  = POW2_31 >> 24 ;

//int loop, count, safeCount, unsafeCount ;
int loop, count, overflowCount, count_31_31, count_23_39, count_15_47, count_7_55 ;
INT64 *aNums, *bNums ;
ofstream ofs( "mu.autoTest.txt", ofstream::out ) ;

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

inline bool safe( UINT64 a, UINT64 b )
{
   if( a < b )
   {
      return safe(b,a) ;
   }

   if( a < POW2_31 )
   {
      return b < POW2_31 ;
   }
   else if( a < POW2_39 )
   {
      return b < POW2_23 ;
   }
   else if( a < POW2_47 )
   {
      return b < POW2_15 ;
   }
   else if( a < POW2_55 )
   {
      return b < POW2_7 ;
   }

   return false ;
}

bool safeTest( INT64 a, INT64 b )
{
   UINT64 x = a < 0 ? -a : a ;
   UINT64 y = b < 0 ? -b : b ;
   return safe(x,y) ;
}

bool overflowTest( INT64 a, INT64 b )
{
   if( a > 0 && b > 0 ) return a > MAX64/b ;
   if( a > 0 && b < 0 ) return a > MIN64/b ;
   if( a < 0 && b > 0 ) return a < MIN64/b ;
   if( a < 0 && b < 0 ) return a < MAX64/b ;
   return false ;
}

void generateData()
{
   for( int i = 0; i < count_31_31; ++i )
   {
      UINT64 a = rand64() % POW2_31 ;
      UINT64 b = rand64() % POW2_31 ;
      if(a<b)
      {
         bNums[i] = a ;
         aNums[i] = b ;
      }
      else
      {
         aNums[i] = a ;
         bNums[i] = b ;
      }
      assert( aNums[i] < POW2_31 && bNums[i] < POW2_31 ) ;
      assert( !overflowTest(aNums[i],bNums[i]) ) ;
   }
   for( int i = count_31_31; i < count_23_39+count_31_31; ++i )
   {
      aNums[i] = rand64() % POW2_39 ;
      bNums[i] = rand64() % POW2_23 ;
      assert( aNums[i] < POW2_39 && bNums[i] < POW2_23 ) ;
      assert( !overflowTest(aNums[i],bNums[i]) ) ;
   }
   for( int i = count_23_39+count_31_31; i < count_15_47+count_23_39+count_31_31; ++i )
   {
      aNums[i] = rand64() % POW2_47 ;
      bNums[i] = rand64() % POW2_15 ;
      assert( aNums[i] < POW2_47 && bNums[i] < POW2_15 ) ;
      assert( !overflowTest(aNums[i],bNums[i]) ) ;
   }
   for( int i = count_15_47+count_23_39+count_31_31; i < count_7_55+count_15_47+count_23_39+count_31_31; ++i )
   {
      aNums[i] = rand64() % POW2_55 ;
      bNums[i] = rand64() % POW2_7 ;
      assert( aNums[i] < POW2_55 && bNums[i] < POW2_7 ) ;
      assert( !overflowTest(aNums[i],bNums[i]) ) ;
   }
   for( int i = count_7_55+count_15_47+count_23_39+count_31_31; i < count; ++i )
   {
      int r = rand()%4 ;
      if(0==r)
      {
         aNums[i] = rand() + POW2_31*8 ;
         bNums[i] = rand() + POW2_31 ;
      }
      if(1==r)
      {
         aNums[i] = rand() + POW2_23*8 ;
         bNums[i] = rand() + POW2_39 ;
      }
      if(2==r)
      {
         aNums[i] = rand() + POW2_15*8 ;
         bNums[i] = rand() + POW2_47 ;
      }
      if(3==r)
      {
         aNums[i] = rand() + POW2_7*8 ;
         bNums[i] = rand() + POW2_55 ;
      }
      assert( !safeTest(aNums[i],bNums[i]) ) ;
      assert( overflowTest(aNums[i],bNums[i]) ) ;
   }

   for( int i = 0; i < count; ++i )
      if( rand() % 2 )
         aNums[i] = -aNums[i] ;
   for( int i = 0; i < count; ++i )
      if( rand() % 2 )
         bNums[i] = -bNums[i] ;
}

//only overflow
bool multiply_1( INT64 a, INT64 b )
{
   UINT64 max = ( ( a^b ) < 0 ) ? MIN64 : MAX64 ;
   UINT64 x = a < 0 ? -a : a ;
   UINT64 y = b < 0 ? -b : b ;
   bool willOverflow =( x != 0 && y > max/x ) ;
   return willOverflow ;
}


//safe + overflow
bool multiply_2( INT64 a, INT64 b )
{
   UINT64 x = a < 0 ? -a : a ;
   UINT64 y = b < 0 ? -b : b ;
   bool multiplySafe = safe(x,y) ;
   if( multiplySafe )
   {
      return false ;
   }
   else
   {
      UINT64 max = ( ( a^b ) < 0 ) ? MIN64 : MAX64 ;
      bool willOverflow =( x != 0 && y > max/x ) ;
      return willOverflow ;
   }
}

typedef bool (*multiplyFunc)(INT64,INT64) ;
void test( const char *desc, multiplyFunc func )
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
   cout << (ov/loop) << " overflow, " << (end-beg) << " clock ticks" << endl ;
}

inline clock_t useTime( multiplyFunc func, int times )
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
   cout << "count_31_31 = " << count_31_31 << ", count_23_39 = " << count_23_39
        << ", count_15_47 = " << count_15_47 << ", count_7_55 = " << count_7_55
        << ", overflowCount = " << overflowCount << endl ;
   ofs << count_31_31 << ", " ;
   ofs << count_23_39 << ", " ;
   ofs << count_15_47 << ", " ;
   ofs << count_7_55 << ", " ;
   ofs << overflowCount << " : " ;
   ofs << useTime( multiply_1, times ) << ", " ;
   ofs << useTime( multiply_2, times ) << endl ;
}

void autoTest( )
{
   loop = 1000 ;
   count = 10000 ;
   aNums = new INT64[count] ;
   bNums = new INT64[count] ;
  
   ofs << "31_31, 23_39, 15_47, 7_55 : overflow, safe+overflow" << endl ; 

   count_31_31 = count ;
   count_23_39 = count_15_47 = count_7_55 = overflowCount = 0 ;
   autoTestEach() ;

   count_23_39 = count ;
   count_31_31 = count_15_47 = count_7_55 = overflowCount = 0 ;
   autoTestEach() ;

   count_15_47 = count ;
   count_23_39 = count_31_31 = count_7_55 = overflowCount = 0 ;
   autoTestEach() ;

   count_7_55 = count ;
   count_23_39 = count_15_47 = count_31_31 = overflowCount = 0 ;
   autoTestEach() ;

   overflowCount = count ;
   count_23_39 = count_15_47 = count_7_55 = count_31_31 = 0 ;
   autoTestEach() ;

   overflowCount = count * 0.2;
   count_7_55 = count * 0.8;
   count_23_39 = count_15_47 = count_31_31 = 0 ;
   autoTestEach() ;
}

int main( int cnt, char **argv )
{
   srand(time(0)) ;
   if( cnt > 1 )
   {
      autoTest() ;
      return 0 ;
   }
   cout << "input (loop) (count of a<2^31,b<2^31) (count of a<2^39,b<2^23)"
           " (count of a<2^47,b<2^15) (count of a<2^55,b<2^7) (count of overflow)" << endl ;
   cin >> loop >> count_31_31 >> count_23_39 >> count_15_47 >> count_7_55 >> overflowCount ;
   count =  count_31_31+count_23_39+count_15_47+count_7_55+overflowCount;
   aNums = new INT64[count] ;
   bNums = new INT64[count] ;
   generateData() ;
   test( "only overflow", multiply_1 ) ;
   test( "safe+overflow", multiply_2 ) ;
}

