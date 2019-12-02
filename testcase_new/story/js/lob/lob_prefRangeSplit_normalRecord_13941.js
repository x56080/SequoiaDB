/******************************************************************************
*@Description : the collection do range split.test input normal record and
*               lob data into collection
*@Modify list :
*               2014-12-18  xiaojun Hu  Init
******************************************************************************/
function main( db )
{
   var testFile = CHANGEDPREFIX + "lobTest.file"; 
   var getTestFile = CHANGEDPREFIX + "lobTestGet.file"; 
   var putNum = 50; 
   
   var names = lobGetAllGroupNames( db ); 
   if( 1 == names.length )
   {
      return; 
   }
   
   lobGenerateFile( testFile ); // auto file
   // create collection
   var optionObj = { "ShardingKey":{"no":1}, "ShardingType":"range", "ReplSize":0, 
   "Compressed":true }; 
   var cl = commCreateCLByOption( db, COMMCSNAME, COMMCLNAME, optionObj, true, 
   true, "create collection for hash split" ); 
   // do range split collection before put data
   try
   {
      var FULLCLNAME = COMMCSNAME + "." + COMMCLNAME; 
      var clRg = commGetCLGroups( db, FULLCLNAME ); 
      var cond = Math.floor( putNum/names.length ); 
      //println( "the group length: " + cond ); 
      var loopCond = cond; 
      for( var i = 0; i < names.length; ++i )
      {
         if( clRg[0] != names[i] )
         {
            var firstCond = { "no":( loopCond-cond )}; 
            var secondCond = { "no": loopCond }; 
            lobSplit( cl, clRg[0], names[i], firstCond, secondCond ); 
            loopCond += cond; 
         }
      }
      println( "success to do range split before input data" ); 
      lobInsertDoc( cl, putNum ); // will be OK
      println( "suceess to put normal record data" ); 
      var oids = lobPutLob( cl, testFile, putNum ); // will throw exception
      throw "range collection write lob expect failed!!!"; 
   }
   catch( e )
   {
      if( -6 != e )
      {
         println( "failed to normal record and lob data in" + 
         " collection, rc = " + e ); 
         throw e; 
      }
      else
      println( "success to test execute put lob when range split" ); 
   }
   
   // get lob
   try
   {
      for( var i = 0; i < cl.count(); ++i )
      {
         var count = cl.find( {"no":i} ).count(); 
         if( 1 != count )
         {
            println( "failed to query data, rc = " + cl.find( {"no":i} ) ); 
            throw "ErrNumberQuery"; 
         }
      }
      println( "success to query records" ); 
      
      if( typeof( oids )== "undefined" ) return; 
      for( var i = 0; i < oids.length; ++i )// oid equal 0
      {
         cl.getLob( oids[i], getTestFile, true ); 
      }
   }
   catch( e )
   {
      println( "failed to query nomral data, rc = " + e ); 
      throw e; 
   }
   finally
   {
      // remove lobfile
      cmd = new Cmd(); 
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
      //commDropCL( db, COMMCSNAME, COMMCLNAME, false, false, 
      //            "drop collection in the end, correct" ); 
   }
}
catch( e )
{
   //commDropCL( db, COMMCSNAME, COMMCLNAME, true, true, 
   //            "drop collection in the end, error" ); 
   throw e; 
}
finally
{
   db.close(); 
}
