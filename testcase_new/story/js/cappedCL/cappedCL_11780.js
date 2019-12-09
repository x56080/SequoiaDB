/************************************
*@Description: 固定集合lob操作 
*@author:      luweikang
*@createdate:  2018.2.13
*@testlinkCase:seqDB-11780
**************************************/

main();

function main ()
{
   var testFile = CHANGEDPREFIX + "_lobTest11780.file";
   var getTestFile = CHANGEDPREFIX + "_lobTestGet11780.file";
   var clName = COMMCAPPEDCLNAME + "_11780";
   var putNum = 10;
   var cmd = new Cmd();
   var oids = new Array();

   //create cappedCL
   var optionObj = { Capped: true, Size: 128, Max: 10000000, AutoIndexId: false };
   var cl = commCreateCLByOption( db, COMMCAPPEDCSNAME, clName, optionObj, false, false, "create cappedCL" );

   //create 15M lob.file
   lobGenerateFile( testFile, 100000 );

   //get file MD5
   var md5Arr = cmd.run( "md5sum " + testFile ).split( " " );
   var md5 = md5Arr[0];

   // put Lob
   //32M>lob
   putLob( cl, testFile, 2, 2, oids );
   insertData( cl, 10, 1080 );
   //32M<lob<96M
   putLob( cl, testFile, 4, 6, oids );
   insertData( cl, 10, 2280 );
   //128M<lob
   putLob( cl, testFile, 4, 10, oids );
   insertData( cl, 10, 3480 );

   // get lob
   getLob( cl, getTestFile, oids, testFile, md5, cmd );

   //clean environment after test
   commDropCL( db, COMMCAPPEDCSNAME, clName, true, true, "drop CL in the end" );
   println( "---end the test---" );
}


function putLob ( cl, fileName, putNum, lobNum, oids )
{
   try
   {
      println( "---begin to put lob---" );
      for( var i = 0; i < putNum; ++i )
      {
         oids.push( cl.putLob( fileName ) );
      }
      println( "put lob over" );
      // verify
      var cursor = cl.listLobs().toArray();
      if( lobNum != cursor.length )
      {
         println( "collection have lob: " + cursor.length );
         throw "ErrNumberPutLob";
      }
      println( "success to put lob in colleciton" );
   }
   catch( e )
   {
      println( "failed to put lob in collection, rc = " + e );
      throw e;
   }
}


function insertData ( cl, insertNum, expID )
{
   var str = "qwertyuioxcvbnmqwertyuiopasdfghjklzxcvbnmqwertyuiopasdfghjklzxcvb";
   var doc = new Array();
   for( i = 0; i < insertNum; i++ )
   {
      doc.push( { a: str } );
   }
   cl.insert( doc );

   var cursor = cl.findOne().sort( { _id: -1 } );
   var id = cursor.current().toObj()._id;
   if( expID !== id )
   {
      throw buildExecption( "insertData()", e, "check record id", expID, id );
   }
}


function getLob ( cl, getTestFile, oids, testFile, md5, cmd )
{
   try
   {
      for( var i = 0; i < oids.length; ++i )
      {
         cl.getLob( oids[i], getTestFile, true );
         md5Arr = cmd.run( "md5sum " + getTestFile ).split( " " );
         getMd5 = md5Arr[0];
         if( getMd5 !== md5 )   // verify put file is equal get file or not
         {
            println( "put lob file md5: " + md5 );
            println( "get lob file md5: " + getMd5 );
            throw "NotEqualMd5";
         }
      }
      println( "success to get lob in colleciton" );
      // delete lobs
      for( var i = 0; i < oids.length; ++i )
      {
         cl.deleteLob( oids[i] );
      }
      println( "success to delete lob in colleciton" );
      // remove lobfile
      //cmd.run( "rm -rf " + testFile ) ;
      //cmd.run( "rm -rf " + getTestFile ) ;
   }
   catch( e )
   {
      // remove lobfile
      //cmd.run( "rm -rf " + testFile ) ;
      println( "failed to get lob, rc = " + e );
      throw e;
   }
   finally
   {
      cmd.run( "rm -rf " + testFile );
      cmd.run( "rm -rf " + getTestFile );
   }
}


function lobFileIsExist ( fileName )
{
   var isExist = false;
   try
   {
      var cmd = new Cmd();
      cmd.run( "ls " + fileName );
      isExist = true;
   }
   catch( e )
   {
      if( 2 == e ) { isExist = false; }
   }
   return isExist;
}

function getMd5ForFile ( testFile )
{
   try
   {
      var cmd = new Cmd();
      var md5Arr = cmd.run( "md5sum " + testFile ).split( " " );
      var md5 = md5Arr[0];
   } catch( e )
   {
      throw e;
   }
   return md5;
}

function lobGenerateFile ( fileName, fileLine )
{
   if( undefined == fileLine )
   {
      fileLine = 1000;
   }

   try
   {
      var cnt = 0;
      while( true == lobFileIsExist( fileName ) )
      {
         File.remove( fileName );
         if( cnt > 10 ) break;
         cnt++;
         sleep( 10 );
      }

      if( 10 <= cnt )
      {
         throw "failed to remove file: " + fileName;
      }
      var file = new File( fileName );
      for( var i = 0; i < fileLine; ++i )
      {
         var record = '{ no:' + i + ', score:' + i + ', interest:["movie", "photo"],' +
            '  major:"计算机软件与理论", dep:"计算机学院",' +
            '  info:{name:"Holiday", age:22, sex:"男"} }';
         file.write( record );
      }
      if( false == lobFileIsExist( fileName ) )
      {
         throw "NoFile: " + fileName;
      }
   }
   catch( e )
   {
      println( "faile to auto generate file, rc = " + e );
      throw e;
   }
}

