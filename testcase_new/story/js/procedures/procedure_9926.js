/******************************************************************************
@Description : seqDB-9926:在不同的coord上操作存储过程
@Modify list :
               2016-9-11   TingYU      Init
******************************************************************************/
var pcdName = COMMCLNAME + '_procedurename';

main();

function main ()
{
   if( commIsStandalone( db ) )
   {
      println( " Deploy mode is standalone!" );
      return;
   }
   var coordArr = getCoord();
   if( coordArr.length < 2 )
   {
      println( "This testcase needs at least 2 coord!" );
      return;
   }
   try
   {
      var db1 = new Sdb( coordArr[0] );
      var db2 = new Sdb( coordArr[1] );
      createPcd( db1, db2 );
      removePcd( db1, db2 );
      createPcdAgain( db1, db2 );
   }
   catch( e )
   {
      throw e;
   }
   finally
   {
      clean();
   }
}

function createPcd ( db1, db2 )
{
   println( "\n---begin to create procedures by coord " + db1.toString() );
   var str = "db.createProcedure( function " + pcdName + "() {return 100;} )";
   db1.eval( str );

   println( "\n---begin to list procedures by coord " + db2.toString() );

   var expPcds = [];

   var expPcd1 = {};
   expPcd1["name"] = pcdName;
   expPcd1["func"] = { "$code": "function " + pcdName + "() {\n    return 100;\n}" };
   expPcd1["funcType"] = 0;
   expPcds.push( expPcd1 );

   var rc = db2.listProcedures( { name: pcdName } );
   checkResult( rc, expPcds );
}

function removePcd ( db1, db2 )
{
   println( "\n---begin to remove procedures by coord " + db1.toString() );
   db1.removeProcedure( pcdName );

   println( "\n---begin to list procedures by coord " + db2.toString() );
   var expPcds = [];
   var rc = db2.listProcedures( { name: pcdName } );
   checkResult( rc, expPcds );
}

function createPcdAgain ( db1, db2 )
{
   println( "\n---begin to create procedures again by coord " + db1.toString() );
   var str = "db.createProcedure( function " + pcdName + "() {return 99;} )";
   db1.eval( str );

   println( "\n---begin to list procedures by coord " + db2.toString() );

   var expPcds = [];

   var expPcd1 = {};
   expPcd1["name"] = pcdName;
   expPcd1["func"] = { "$code": "function " + pcdName + "() {\n    return 99;\n}" };
   expPcd1["funcType"] = 0;
   expPcds.push( expPcd1 );

   var rc = db2.listProcedures( { name: pcdName } );
   checkResult( rc, expPcds );
}

function ready ()
{
   println( "\n---begin to remove procedures in ready" );
   fmpRemoveProcedures( [pcdName], true );
}

function clean ()
{
   println( "\n---begin to clean environment" );
   fmpRemoveProcedures( [pcdName], true );
}

function getCoord ()
{
   var coordInfo = db.getRG( 'SYSCoord' ).getDetail().current().toObj().Group;
   var coordArr = [];
   for( var i in coordInfo )
   {
      var hostname = coordInfo[i].HostName;
      var port = coordInfo[i].Service[0].Name;
      var hostPort = hostname + ':' + port;
      coordArr.push( hostPort );
   }

   return coordArr;
}