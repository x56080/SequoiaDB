/******************************************************************************
*@Description : delete basic: [cond]
*@Modify list :
*               2015-01-27  pusheng Ding  Init
******************************************************************************/
rowcnt = 100;

try
{
   var db = new Sdb( COORDHOSTNAME, COORDSVCNAME );
}
catch( e )
{
   println( "can't connect to db" );
   throw e;
}

db.setSessionAttr( { "PreferedInstance": "M" } );

try
{
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true,
      "drop cl in the beginning" );
} catch( e ) { }

//create CS
try
{
   var varCS = commCreateCS( db, COMMCSNAME, true, "create CS" );
}
catch( e )
{
   println( "can't create CS:" + COMMCSNAME + " rc=" + e );
   throw e;
}
println( "createCS " + COMMCSNAME + " finished" );

//create cl
try
{
   var varCL = varCS.createCL( COMMCLNAME );
}
catch( e )
{
   println( "can't create CL:" + COMMCLNAME + " rc=" + e );
   throw e;
}
println( "createCL " + COMMCLNAME + " finished" );

//insert data
try
{
   var docs = [];
   for( var i = 0; i < rowcnt; i++ )
   {
      docs.push( { a: rowcnt - i, b: i, c: "abcdefghijkl" + i } );

   }
   varCL.insert( docs );
}
catch( e )
{
   println( "insert-data to " + COMMCLNAME1 + " fail! rc=" + e );
   throw e;
}
println( "insert data finished" );

//delete
try
{
   cond = 50;
   varCL.remove( { b: { $lt: 50 } } );
   var cnt = varCL.count();
   if( cnt != ( rowcnt - cond ) )
   {
      throw "result-error";
   }
}
catch( e )
{
   if( e == "result-error" )
   {
      println( "result error! \nexpect:" + ( rowcnt - cond ) + " \nreturn:" + cnt );
   }
   else
   {
      println( "test remove failed! rc=" + e );
   }
   throw e;
}

docs.splice( 0, 50 );
checkResult( varCL, {}, docs );
//clean test-env
try
{
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true,
      "drop cl in the end" );
}
catch( e )
{
   println( "clean test-evn fail! rc=" + e );
   throw e;
}
println( "clean test-evn succ!" ); 
