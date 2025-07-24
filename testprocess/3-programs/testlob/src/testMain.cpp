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

   Source File Name = testMain.cpp

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
#include "mongo/client/dbclient.h"

using namespace mongo;
using namespace std;

void run()
{
   DBClientConnection c ;
   c.connect("localhost:27017") ;
   string dbName = "shardtest" ;
   string filename = "hadoop" ;
   string outfile = "out.mongo" ;
   string infile = "hadoop-2.2.0.tar.gz" ;
   string remoteName = "hadoop-2.2.0" ;
   GridFS* gridfs = new GridFS(c, dbName) ;

   //list file info
  auto_ptr<DBClientCursor> cursor = gridfs->list( ) ;

   while(cursor->more())
   {
      BSONObj p = cursor->next() ;
      cout << p << endl ;
   }

   //put file to mongo
   BSONObj put = gridfs->storeFile(infile, remoteName, "tar.gz") ;
   cout << "put file:" << put << endl ;
   
/* 
   //get file from mongo
   GridFile gfile = gridfs->findFile(filename) ;
   cout << "FileName: " << gfile.getFilename() << endl ;
   cout << "ChunkSize: " << gfile.getChunkSize() << endl ;
   cout << "ContentLength: " << gfile.getContentLength() << endl ;
   cout << "ContentType: " << gfile.getContentType() << endl ;
   cout << "UploadDate: " << gfile.getUploadDate() << endl ;
   cout << "MD5: " << gfile.getMD5() << endl ;

   cout << "begin to write to " << outfile << endl ;
   gridfs_offset tmp = gfile.write(outfile) ;
   cout << "write offset:" << tmp << endl ;
   */
 
/*
   auto_ptr<DBClientCursor> cursor = c.query("shardtest.people", BSONObj()) ;
   while(cursor->more())
   {
      BSONObj p = cursor->next() ;
      cout << p << endl ;
   }
 */
}

int main()
{
   try{
      run() ;
      cout << "connected ok!" << endl ;
   }catch ( DBException &e ) {
      cout << "caught " << e.what() << endl ;
   }

   return 0 ;
      
}

