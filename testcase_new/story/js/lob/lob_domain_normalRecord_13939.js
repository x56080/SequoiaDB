/******************************************************************************
*@Description : the collection do autosplit in domain.test input normal record
*               and lob data into collection
*@Modify list :
*               2014-12-18  xiaojun Hu  Init
******************************************************************************/

function main( db )
{
   var putNum = 50; 
   
   var partitionNum = 2048; 
   var testFile = CHANGEDPREFIX + "lobTest.file"; 
   var getTestFile = CHANGEDPREFIX + "lobTestGet.file"; 
   var DOMCSNAME = CHANGEDPREFIX + "_domainCS"; 
   var domName = CHANGEDPREFIX + "_domName"; 
   var cmd = new Cmd(); 
   
   lobGenerateFile( testFile ); // auto file
   var originMd5 = getMd5ForFile( testFile ); 
   // create domain
   try
   {
      var names = lobGetAllGroupNames( db ); 
      commDropDomain( db, domName ); 
      var domain = commCreateDomain( db, domName, names, { "AutoSplit": true } ); 
      println( "success to create domain" ); 
      var cs = lobCreateCS( db, DOMCSNAME, domName ); 
      println( "success to create collection space attach domain" ); 
      
      // create collection
      var optionObj = { "ShardingKey":{"no":1}, "ShardingType":"hash", "ReplSize":0, 
      "Partition":partitionNum, "Compressed":true }; 
      var cl = commCreateCLByOption( db, DOMCSNAME, COMMCLNAME, optionObj, true, 
      true, "create collection for hash split" ); 
      lobInsertDoc( cl, putNum ); 
      println( "success to put normal record data" ); 
      var oids = lobPutLob( cl, testFile, putNum ); 
      println( "success to put lob data" ); 
      for( var i = 0; i < oids.length; ++i )
      {
         cl.getLob( oids[i], getTestFile, true ); 
         var curMd5 = getMd5ForFile( testFile ); 
         if( originMd5 !== curMd5 )
         {
            throw "origin file's md5=" + originMd5 + "getLob's md5=" + curMd5; 
         }
      }
      println( "success to get lob" ); 
      for( var i = 0; i < cl.count(); ++i )
      {
         var count = cl.find( {"no":i} ).count(); 
         if( 1 != count )
         {
            println( "failed to query data, rc = " + cl.find( {"no":i} ) ); 
            throw "ErrNumberQuery"; 
         }
      }
      println( "success to query" ); 
   }
   catch( e )
   {
      println( "failed to create domain and CS, rc = " + e ); 
      throw e; 
   }
   finally
   {
      if( typeof( cs )!== "undefined" )
      {
         commDropCS( db, DOMCSNAME, true, "drop collection in the end, correct" ); 
      }
      
      commDropDomain( db, domName ); 
      
      cmd.run( "rm -rf " + testFile ); 
      if( lobFileIsExist( getTestFile ) )
      {
         cmd.run( "rm -rf " + getTestFile ); 
      }
   }
}

// Run Main
try
{
   if( !commIsStandalone( db ) )
   {
      commDropCL( db, COMMCSNAME, COMMCLNAME, true, true, 
      "clear collection in the beginning" ); 
      main( db ); 
   }
}
catch( e )
{
   throw e; 
}
finally
{
   db.close(); 
}
