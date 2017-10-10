/**************************************************************
 * @Description: singleton to get arguments from console.
 * @Modify     : Liang xuewang 
 *               2017-09-17
 ***************************************************************/
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <gtest/gtest.h>
#include "arguments.hpp"

using namespace std ;
using testing::internal::g_argvs ;

extern vector<testing::internal::String> g_argvs ;

arguments* arguments::getInstance()
{
   static arguments args ;
   return &args ;
}

arguments::arguments()
{
   // set default value
   strcpy( _hostName, "localhost" ) ;
   strcpy( _svcName, "11810" ) ;
   strcpy( _user, "" ) ;
   strcpy( _passwd, "" ) ;
   strcpy( _coordUrl, "localhost:11810" ) ;
   strcpy( _changedPrefix, "sdv_cpp_test" ) ;
   strcpy( _rsrvPortBegin, "26000" ) ;
   strcpy( _rsrvPortEnd, "27000" ) ;
   strcpy( _rsrvNodeDir, "/opt/sequoiadb/database/" ) ;
   strcpy( _workDir, "/tmp/cpptest" ) ;
   _forceClear = TRUE ;

   // get arguments from outside
   for( INT32 i = 0;i < g_argvs.size();i++ )
   {
      string para = g_argvs[i] ;
      if( para == "--HOSTNAME" || para == "-n" )
         strcpy( _hostName,g_argvs[i+1].c_str() ) ;
      else if( para == "--SVCNAME" || para == "-s" )
         strcpy( _svcName,g_argvs[i+1].c_str() ) ;
      else if( para == "--CHANGEDPREFIX" || para == "-c" )
         strcpy( _changedPrefix,g_argvs[i+1].c_str() ) ;
      else if( para == "--RSRVPORTBEGIN" || para == "-b" )
         strcpy( _rsrvPortBegin,g_argvs[i+1].c_str() ) ;
      else if( para == "--RSRVPORTEND" || para == "-e" )
         strcpy( _rsrvPortEnd,g_argvs[i+1].c_str() ) ;
      else if( para == "--RSRVNODEDIR" || para == "-d" )
         strcpy( _rsrvNodeDir,g_argvs[i+1].c_str() ) ;
      else if( para == "--WORKDIR" || para == "-w" )
         strcpy( _workDir,g_argvs[i+1].c_str() ) ; 
      else if( para == "--FORCECLEAR" || para == "-f" )
         _forceClear = atoi( g_argvs[i+1].c_str() ) ? TRUE : FALSE ;
   }
   sprintf( _coordUrl, "%s%s%s", _hostName, ":", _svcName ) ;
   sscanf( _svcName, "%d", &_port ) ; // TODO: if svcName is not number, how to map?
}

void arguments::print()
{
   // print arguments for debug
   cout << "HOSTNAME      : " << _hostName << endl ;
   cout << "SVCNAME       : " << _svcName << endl ;
   cout << "CHANGEDPREFIX : " << _changedPrefix << endl ;
   cout << "RSPVPORTBEGIN : " << _rsrvPortBegin << endl ;
   cout << "RSPVPORTEND   : " << _rsrvPortEnd << endl ;
   cout << "RSPVNODEDIR   : " << _rsrvNodeDir << endl ;
   cout << "WORKDIR       : " << _workDir << endl ;
   cout << "COORDURL      : " << _coordUrl << endl ;
   cout << "FORCECLEAR    : " << _forceClear << endl ;
}

const CHAR* arguments::hostName() 
{
   return _hostName ;
}

const CHAR* arguments::svcName() 
{
   return _svcName ;
}

const INT32 arguments::port()
{
   return _port ;
}

const CHAR* arguments::user() 
{
   return _user ;
}

const CHAR* arguments::passwd() 
{
   return _passwd ;
}

const CHAR* arguments::coordUrl() 
{
   return _coordUrl ;
}

const CHAR* arguments::changedPrefix() 
{
   return _changedPrefix ;
}

const CHAR* arguments::rsrvPortBegin() 
{
   return _rsrvPortBegin ;
}

const CHAR* arguments::rsrvPortEnd() 
{
   return _rsrvPortEnd ;
}

const CHAR* arguments::rsrvNodeDir() 
{
   return _rsrvNodeDir ;
}

const CHAR* arguments::workDir() 
{
   return _workDir ;
}

BOOLEAN arguments::forceClear()
{
   return _forceClear ;
}
