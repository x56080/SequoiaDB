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

   Source File Name = mongolob.cpp

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
#include "mongolob.hpp"

using namespace mongo ;
using namespace std ;

CMongoLob::CMongoLob ( const  string & strSrvAddr, const std :: string & strDbName, int startID )
:m_strSrvAddr( strSrvAddr ) ,
 m_dbName ( strDbName ) ,
 m_startID ( startID )  ,
 sourceDir( "lobsource" ) ,
 outDir( "lobout" ) 
{
   srand( (unsigned int) time(NULL) ) ;

   DIR* dp = NULL ;
   struct dirent* direntp ;
   fileslist.clear ( ) ;
   dp = opendir ( lobdir.c_str ( ) ) ;
   if ( dp == NULL )
      return ;
   while ( (direntp=readdir(dp)) != NULL )
   {
      if ( *(direntp->d_name) != '.' && direntp->d_type!=DT_DIR )
      {
         fileslist.push_back ( direntp->d_name ) ;
      }
   }

   closedir( dp ) ;
   
}

CMongoLob::~CMongoLob ( )
{

}

int CMongoLob::Connect ( )
{
   try
   {
      m_conn.connect( m_strSrvAddr ) ;
      cout << m_strSrvAddr << " connected ok " << endl ;
      gridfs = new GridFS ( m_conn, m_dbName ) ;
      return 0 ;
   }
   catch ( DBException &e)
   {
      cout << " exception msg:  " << e.what() << endl ;
      return -1 ;
   }
}

int CMongoLob::Putfile ( int id )
{
   try
   {
      std::ostringstream dbfileName ;
      dbfileName << "dbfile_" << id ;
      gridfs->storeFile( ChooseFile( id ), dbfileName.str( ) , " ") ;
      return 0 ;
   }
   catch ( DBException &e )
   {
      cout << " put exception msg: " << e.what() << endl ;
      return -1 ;
   }
}

int CMongoLob::Getfile( int id )
{
   try
   {
      std::ostringstream dbfileName ;
      int getid = rand( ) % ( id + 1 ) ;
      dbfileName << "dbfile_" << getid ;
      GridFile gfile = gridfs->findFile( dbfileName.str( ) ) ;

      std::ostringstream outfileName ;
      outfileName << outDir << "/" << m_startID << "_out_" << id << "_" << getid ;
      gridfs_offset tmp = gfile.write( outfileName.str( ) ) ;
      return 0 ;
   }
   catch ( DBException &e )
   {
      cout << " get exception msg: " << e.what ( ) << endl ;
      return -1 ;
   }
}

int CMongoLob::Mix( int id )
{
   int mode = rand( ) % 10 ;

   if ( mode < 7 )
      return Getfile( id ) ;
   else
      return Putfile( id ) ;
}

void CMongoLob::Release ( )
{
   delete this ;
}

string CMongoLob::ChooseFile( int id )
{
   return lobdir + "/" + fileslist[rand( ) % fileslist.size()] ;
}

IClient* CreateClient ( const char* pSrvAddr , const char* pDbName, int startID )
{
   return new CMongoLob ( pSrvAddr, pDbName, startID ) ;
}

