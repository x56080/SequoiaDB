/*******************************************************************************
*@Description :  seqDB-13729:游标未关闭时，在同一个session中删除cs
                 seqDB-13730:游标未关闭时，在不同个session中删除cs
*@Modify List : 2014-9-26   xiaojunHu  Init
                2016-3-17   Ting YU    modify
                2020-10-12 XiaoNi Huang  modify
*******************************************************************************/
testConf.csName = COMMCSNAME + "_13729";
testConf.clName = COMMCLNAME + "_13729";

main( test );
function test ( testPara )
{
   var rd = new commDataGenerator();
   var recs = rd.getRecords( 1000, "string", ['a', 'b', 'c'] );
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
   return hasDataContext;
}

function dropcsSameSession ( csName )
{
   db.dropCS( csName );
   assert.tryThrow( -34, function()
   {
      db.getCS( csName );
   } );
}

function dropcsDiffSession ( csName )
{
   var dbAnother = null;
   try
   {
      dbAnother = new Sdb( COORDHOSTNAME, COORDSVCNAME );
      assert.tryThrow( -147, function()
      {
         dbAnother.dropCS( csName );
      } );
      assert.equal( dbAnother.getCS( csName )._name, csName );
   }
   finally
   {
      if( dbAnother != null )
      {
         dbAnother.close();
      }
   }
}