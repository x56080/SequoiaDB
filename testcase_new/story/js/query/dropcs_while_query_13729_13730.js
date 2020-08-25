/*******************************************************************************
*@Description :  seqDB-13729:游标未关闭时，在同一个session中删除cs
                 seqDB-13730:游标未关闭时，在不同个session中删除cs
*@Modify List : 2014-9-26   xiaojunHu  Init
                2016-3-17   Ting YU    modify
*******************************************************************************/

testConf.csName = COMMCSNAME + "_query_cs_13729";
testConf.clName = COMMCLNAME + "_query_cl_13729";
main( test );

function test (testPara)
{
   var rd = new commDataGenerator();
   var recs = rd.getRecords( 1000, "string", ['a', 'b', 'c']);
   testPara.testCL.insert( recs );
   
   //cursor not close
   var rc = testPara.testCL.find();
   rc.next();
   var hasDataContext = checkContext();

   //drop cs
   if( hasDataContext === true )
   {
      dropcsDiffSession( testConf.csName );
      dropcsSameSession( testConf.csName );
   }
}

function checkContext ()
{
   println( "---begin to check context is exits or not" );
   var sp = db.snapshot( SDB_SNAP_CONTEXTS_CURRENT );
   var hasDataContext = false;
   while( sp.next() && hasDataContext === false )
   {
      var ct = sp.current().toObj().Contexts;
      for( var i in ct )
      {
         var contextType = ct[i].Type;
         if( contextType == "DATA" ) 
         {
            hasDataContext = true;
            break;
         }
      }
   }
   println( "---'DATA' Context = " + hasDataContext );
   return hasDataContext;
}

function dropcsSameSession ( csName )
{
   println( "---begin to drop cs in the same session" );
   db.dropCS( csName );

   println( "---begin to get cs" );
   try
   {
      db.getCS( csName );
      throw "did not throw error, expect throw -34";
   }
   catch( e )
   {
      if( e !== -34 )
      {
         throw e;
      }
   }
}

function dropcsDiffSession ( csName )
{
   println( "---begin to drop cs in another session" );
   try
   {
      dbAnother = new Sdb( COORDHOSTNAME, COORDSVCNAME );
      dbAnother.dropCS( csName );
      throw "did not throw error, expect throw -147";
   }
   catch( e )
   {
      if( e !== -147 )
      {
         throw e;
      }
   }

   println( "---begin to get cs" );
   db.getCS( csName );
}