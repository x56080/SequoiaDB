/************************************************************************
*@Description:   seqDB-11052:支持code类型
*@Author:  2017/2/7  huangxiaoni init
*		     2019/11/18 zhaoyu modify
************************************************************************/
try
{
   main();
}
catch( e )
{
   if( e.constructor === Error )
   {
      println( e.stack );
   }
   throw e;
}

function main ()
{
   if( commIsStandalone( db ) )
   {
      println( "\n---Deploy mode is standalone!" );
      return;
   }
   //ready env
   var procedureName = "abc11052";
   commRemoveProcedure( db, procedureName );

   //structural data
   println( "\n---Begin to createProcedure." );
   var cmd = "db.createProcedure( function " + procedureName + "(x, y){return x+y;} )";
   db.eval( cmd );

   checkResult( procedureName );
   commRemoveProcedure( db, procedureName );
}

function checkResult ( procedureName )
{
   //compare the returned records
   var rc = db.listProcedures( { name: procedureName } ).current().toObj().func;

   var expRlt = '{"$code":"function ' + procedureName + '(x, y) {\\n    return x + y;\\n}"}';
   var actRlt = JSON.stringify( rc );
   if( expRlt !== actRlt )
   {
      throw new Error( "expect: " + expRlt + ",actual: " + actRlt );
   }
}
