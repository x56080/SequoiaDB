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

   Source File Name = sdbConnectionPool.hpp

   Descriptive Name = SDB Connection Pool Include Header

   When/how to use: this program may be used on sequoiadb connection pool function.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
         06/30/2016  LXJ  Initial Draft

   Last Changed =

*******************************************************************************/

/** \file sdbConnectionPool.hpp
    \brief C++ sdb connection pool
*/


#ifndef SDB_CONNECTIONPOOL_HPP_
#define SDB_CONNECTIONPOOL_HPP_

#include "sdbConnectionPoolComm.hpp"
#include <list>

#ifndef SDB_CLIENT
#define SDB_CLIENT
#endif

#include "ossLatch.hpp"
#include "ossMem.hpp"
#include "ossAtomic.hpp"


/** \namespace sdbclient
    \brief SequoiaDB Driver for C++
*/

namespace sdbclient
{
   class sdbConnPoolStrategy ;
   class sdbConnPoolWorker ;
   /** \class sdbConnectionPool
       \brief The sdb connection pool
   */
   class DLLEXPORT sdbConnectionPool : public SDBObject
   {
      friend void createConnFunc( void *args ) ;
      friend void destroyConnFunc( void *args ) ;
      friend void bgTaskFunc( void *args ) ;

   public:
      /** \fn sdbConnectionPool()
         \brief The constructor of sdbConnectionPool
      */
      sdbConnectionPool()
         : _idleList(),
         _idleSize(0),
         _busyList(),
         _busySize(0),
         _destroyList(),
         _conf(),
         _strategy(NULL),
         _connMutex(),
         _globalMutex(),
         _isInited(FALSE),
         _isEnabled(FALSE),
         _toCreateConn(FALSE),
         _toDestroyConn(FALSE),
         _toStopWorkers(FALSE),
         _createConnWorker(NULL),
         _destroyConnWorker(NULL),
         _bgTaskWorker(NULL) {}

      /** \fn ~sdbConnectionPool()
         \brief The destructor of sdbConnectionPool
      */
      ~sdbConnectionPool() ;

   private:
      sdbConnectionPool( const sdbConnectionPool &connPool ) ;
      sdbConnectionPool& operator=( const sdbConnectionPool &connPool ) ;

   public:
      /** \fn INT32 init(const std::string &url,
         const sdbConnectionPoolConf &conf)
         \brief Initialize connection pool
         \param [in] url A coord node("ubuntu-xxx:11810")
         \param [in] conf The sdbConnectionPoolConf
         \retval SDB_OK Operation Success
         \retval Others Operation Fail
      */
      INT32 init( const std::string &url, const sdbConnectionPoolConf &conf ) ;

      /** \fn INT32 init(const std::vector<std::string> &vUrls,
         const sdbConnectionPoolConf &conf)
         \brief Initialize connection pool
         \param [in] vUrls A list of coord node("ubuntu-xxx:11810")
         \param [in] conf The sdbConnectionPoolConf
         \retval SDB_OK Operation Success
         \retval Others Operation Fail
      */
      INT32 init(
         const std::vector<std::string> &vUrls,
         const sdbConnectionPoolConf &conf ) ;

      /** \fn INT32 getIdleConnNum()const
         \brief Get idle connection number or -1 for connection pool
                has not been initialized yet.
         \retval The number of idle connection
      */
      INT32 getIdleConnNum()const  ;

      /** \fn INT32 getUsedConnNum()const
         \brief Get used connection number or -1 for connection pool
                has not been initialized yet.
         \retval The number of used connection
      */
      INT32 getUsedConnNum()const  ;

      /** \fn INT32 getNormalCoordNum()const
         \brief Get the number of reachable coord nodes or -1 for connection pool
                has not been initialized yet.
         \retval The number of reachable coord nodes
      */
      INT32 getNormalCoordNum()const  ;

      /** \fn INT32 getAbnormalCoordNum()const
         \brief Get the number of unreachable coord nodes or -1 for connection pool
                has not been initialized yet.
         \retval The number of unreachable coord nodes
      */
      INT32 getAbnormalCoordNum() const  ;

      /** \fn INT32 getLocalCoordNum()const
         \brief Get the number of local coord nodes or -1 for connection pool
                has not been initialized yet.
         \retval The number of local coord nodes
      */
      INT32 getLocalCoordNum()const  ;

      /** \fn INT32 addCoord(const string &url)
         \brief Add a coord node
         \param [in] url A coord node("ubuntu-xxx:11810")
      */
      void addCoord( const string &url ) ;

      /** \fn INT32 removeCoord(const string &url)
         \brief Remove a coord node
         \param [in] url A coord node("ubuntu-xxx:11810")
      */
      void removeCoord( const string &url ) ;

      /** \fn INT32 enable()
         \brief Enable connection pool
         \retval SDB_OK Operation Success
         \retval Others Operation Fail
      */
      INT32 enable() ;

      /** \fn INT32 disable()
         \brief Disable connection pool. After disable, the connection pool will
                disconnect all the connections and release the handle of
                the connections. So stop using the connection handle which has
                not been released to the connection pool.
         \retval SDB_OK Operation Success
         \retval Others Operation Fail
      */
      INT32 disable() ;

      /** \fn INT32 getConnection(sdb*& conn, INT64 timeoutsec = 5000)
         \brief Get a connection form connection pool
         \param [out] conn A connection
         \param [in] timeoutms The time to wait when connection number reach to
         max connection number,default:5000ms. when timeoutms is set to 0,
         means waiting until a connection is available. when timeoutms is less
         than 0, set it to be 0.
         \retval SDB_OK Operation Success
         \retval Others Operation Fail
      */
      INT32 getConnection( sdb*& conn, INT64 timeoutms = 5000 ) ;

      /** \fn INT32 releaseConnection(sdb *conn)
         \brief Give back a connection to connection pool
         \param [in] conn A connection
         \retval SDB_OK Operation Success
         \retval Others Operation Fail
      */
      void releaseConnection(sdb *conn) ;

      /** \fn void close()
         \brief Close connection pool
      */
      void close() ;


//#if defined (_DEBUG)
//   private:
//      void printCreateInfo(const std::string& coord) ;
//#endif


   private:
      // check address arguments, if valid, add it
      BOOLEAN _checkAddrArg( const string &url ) ;

      // new a strategy with config
      INT32 _buildStrategy() ;

      // clear connection pool
      void _clearConnPool() ;

      // try to get a connection
      BOOLEAN _tryGetConn( sdb*& conn ) ;

      // create connection by a number
      INT32 _createConnByNum( INT32 num ) ;

      // sync coord node info
      void _syncCoordNodes() ;

      // get back the coord address from the abnormal address list
      INT32 _retrieveAddrFromAbnormalList() ;

      // check keep alive time out or not
      BOOLEAN _keepAliveTimeOut( sdb *conn ) ;

      // check max connection number intervally
      void _checkMaxIdleConn() ;

      // add new connection and make sure not reach max connection count
      BOOLEAN _addNewConnSafely( sdb *conn, const std::string &coord );

   private:
      // create connections function
      void _createConn() ;

      //destroy connections function
      void _destroyConn() ;

      // connect to db
      INT32 _connect( sdb *conn, const std::string &address ) ;

      // background task function
      void _bgTask() ;

   private:
      // idle connection list
      std::list<sdb*>         _idleList ;
      ossAtomic32             _idleSize ;
      // busy connection list
      std::list<sdb*>         _busyList ;
      ossAtomic32             _busySize ;
      // to be destroyed connection list
      std::list<sdb*>         _destroyList ;
      // connection pool confiture
      sdbConnectionPoolConf   _conf ;
      // connection pool strategy
      sdbConnPoolStrategy*    _strategy ;
      // lock for connection lists
      ossSpinXLatch           _connMutex ;
      // lock for global commuincate
      ossSpinXLatch           _globalMutex ;
      // if has been inited
      BOOLEAN                 _isInited ;
      // if is enabled
      BOOLEAN                 _isEnabled ;

   private:
      BOOLEAN                 _toCreateConn ;
      BOOLEAN                 _toDestroyConn ;
      BOOLEAN                 _toStopWorkers ;
      sdbConnPoolWorker*      _createConnWorker ;
      sdbConnPoolWorker*      _destroyConnWorker ;
      sdbConnPoolWorker*      _bgTaskWorker ;
   } ;
}

#endif /* SDB_CONNECTIONPOOL_HPP_ */
