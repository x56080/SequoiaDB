/*******************************************************************************


   Copyright (C) 2011-2018 SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   Source File Name = sdbConnectionPoolStrategy.hpp

   Descriptive Name = SDB connection pool Strategy Include Header

   When/how to use: this program may be used on sequoiadb connection pool function.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
         06/30/2016   LXJ Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef SDB_CONNECTIONPOOL_STRATEGY_HPP_
#define SDB_CONNECTIONPOOL_STRATEGY_HPP_

#include "ossTypes.h"
#include <vector>
#include <map>
#include <set>
#include "ossLatch.hpp"
#include <string>
#include "client.hpp"

using std::string;
using std::vector;
using std::map;
using std::set;

namespace sdbclient
{
   enum SYNC_CHOICE
   {
      DELIDLECONN,
      DELBUSYCONN,
      ADDBUSYCONN,
      ADDIDLECONN
   } ;

   class sdbConnPoolStrategy  : public SDBObject
   {
   private:
      sdbConnPoolStrategy( const sdbConnPoolStrategy &strategy ) ;
      sdbConnPoolStrategy& operator=( const sdbConnPoolStrategy &strategy ) ;
      
   protected:
      // abnormal address list
      vector<string> _abnormalCoordList ;
      // normal address list
      vector<string> _normalCoordList ;
      // lock for addr lists
      ossSpinXLatch _coordMutex ;
      // abnormal coordlist pos ;
      INT32 _abPos ;

   public:
      sdbConnPoolStrategy()
         :_abPos(0) {}
      virtual ~sdbConnPoolStrategy() {}

   public:
      
      virtual void addCoord( const string &coord ) ;

      virtual void removeCoord( const string &coord ) ;
      
      virtual INT32 getNormalCoordNum() ;

      virtual INT32 getAbnormalCoordNum() ;

      virtual INT32 getLocalCoordNum() ;

      virtual INT32 getNextCoord( string& nCoord ) = 0 ;

      virtual INT32 getNextAbnormalCoord( string& nCoord ) ;

      // move coord from normal list to abnormal list
      virtual void mvCoordToAbnormal( const string &coord ) ;

      // move coord from abnormal list to normal list
      virtual void mvCoordToNormal( const string &coord ) ;

      // sync strategy
      virtual void sync( sdb *conn, SYNC_CHOICE choice ) {}

      virtual void syncAddNewConn( sdb *conn, const string &coord ) {}

   protected:
      // convert hostname to ip
      BOOLEAN _converToIP( const string &oldcoord, string& newcoord ) ;
   private:
      BOOLEAN _isLocalIP(const string &ipstr) ;
   } ;


   class sdbConnPoolSerialStrategy : public sdbConnPoolStrategy
   {
   private:
      sdbConnPoolSerialStrategy( const sdbConnPoolSerialStrategy &strategy ) ;
      sdbConnPoolSerialStrategy& operator=( const sdbConnPoolSerialStrategy &strategy ) ;

   public:
      sdbConnPoolSerialStrategy()
         : _curPos(0) {}
      virtual ~sdbConnPoolSerialStrategy(){}

   public:
      virtual INT32 getNextCoord( string& nCoord ) ;

   private:
      INT32 _curPos ;
   } ;


   class sdbConnPoolRandomStrategy : public sdbConnPoolStrategy
   {
   private:
      sdbConnPoolRandomStrategy( const sdbConnPoolRandomStrategy &strategy ) ;
      sdbConnPoolRandomStrategy& operator=( const sdbConnPoolRandomStrategy &strategy ) ;

   public:
      sdbConnPoolRandomStrategy() 
      {
         srand((unsigned)time( NULL )) ;
      }
      virtual ~sdbConnPoolRandomStrategy(){}

   public:
      virtual INT32 getNextCoord( string& nCoord ) ;
   } ;

   class sdbConnPoolLocalStrategy : public sdbConnPoolStrategy
   {
   private:
      sdbConnPoolLocalStrategy( const sdbConnPoolLocalStrategy &strategy ) ;
      sdbConnPoolLocalStrategy& operator=( const sdbConnPoolLocalStrategy &strategy ) ;

   public:
      sdbConnPoolLocalStrategy()
         : _localPos(0),
         _normalPos(0) {}
      virtual ~sdbConnPoolLocalStrategy(){}

   public:

      virtual INT32 getLocalCoordNum() ;

      virtual void addCoord( const string &coord ) ;

      virtual void removeCoord( const string &coord ) ;

      virtual INT32 getNextCoord( string& nCoord ) ;

      // move coord from normal list to abnormal list
      virtual void mvCoordToAbnormal( const string &coord ) ;

      // move coord from abnormal list to normal list
      virtual void mvCoordToNormal( const string &coord ) ;

   private:
      // check coord is local coord or not
      BOOLEAN _isLocalCoord( const string &coord ) ;

   private:
      vector<string> _localCoordList ;

   private:
      INT32 _localPos ;
      INT32 _normalPos ;
   } ;

   struct coordInfo : public SDBObject
   {
      INT32 usedNum ;
      INT32 totalNum ;
      BOOLEAN bAvailable ;
      string coord ;
      coordInfo( const string &c )
         :usedNum(0),
         totalNum(0),
         bAvailable(TRUE),
         coord(c) {}
   } ;

   typedef struct coordInfo coordInfo ;

   struct coordInfoCmp
   {
      bool operator()( const coordInfo *left, const coordInfo *right )
      {
         // normal before, abnormal after,
         // light load before, weight load after
         if ( left->bAvailable != right->bAvailable )
            return left->bAvailable > right->bAvailable ;
         if ( left->totalNum != right->totalNum )
            return left->totalNum < right->totalNum ;
         if ( left->usedNum != right->usedNum )
            return left->usedNum < right->usedNum ;
         INT32 res = ( left->coord ).compare( right->coord ) ;
         if ( 0 != res )
         {
            if ( res < 0 )
               return true ;
            else
               return false ;
         }
         return false ;
      }
   } ;

   class sdbConnPoolBalanceStrategy : public sdbConnPoolStrategy
   {
   private:
      sdbConnPoolBalanceStrategy( const sdbConnPoolBalanceStrategy &strategy ) ;
      sdbConnPoolBalanceStrategy& operator=( const sdbConnPoolBalanceStrategy &strategy ) ;

   public:
      sdbConnPoolBalanceStrategy() {}
      virtual ~sdbConnPoolBalanceStrategy() ;

   public:
      virtual void addCoord( const string &coord ) ;

      virtual void removeCoord( const string &coord ) ;
      
      virtual INT32 getNormalCoordNum() ;

      virtual INT32 getAbnormalCoordNum() ;

      virtual INT32 getNextCoord( string& nCoord ) ;

      virtual INT32 getNextAbnormalCoord( string& nCoord ) ;

      // move coord from normal list to abnormal list
      virtual void mvCoordToAbnormal( const string &coord ) ;

      // move coord from abnormal list to normal list
      virtual void mvCoordToNormal( const string &coord ) ;

      // sync strategy
      virtual void sync( sdb *conn, SYNC_CHOICE choice ) ;

      virtual void syncAddNewConn( sdb *conn, const string &coord ) ;

   private:
      set<coordInfo*, coordInfoCmp>::const_iterator 
         _findCoord( const string &coord ) const  ;

      // flag: TRUE get normal coord number
      // FALSE get abnormal coord number
      INT32 _getCoordNum( BOOLEAN flag ) ;

      void _syncDelIdleConn( sdb *conn ) ;

      void _syncDelBusyConn( sdb *conn ) ;

      void _syncAddBusyConn( sdb *conn ) ;

      void _syncAddIdleConn( sdb *conn ) ;

   private:
      set< coordInfo*, coordInfoCmp > _coordInfoSet ;
      map< sdb*, coordInfo* > _connToCoord ;
   } ;
}

#endif
