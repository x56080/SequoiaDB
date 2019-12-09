/* *****************************************************************************
@Description:  seqDB-8192:创建执行存储过程
               seqDB-8194:删除查询存储过程
@modify list:
            2016-9-11   TingYU   modify
***************************************************************************** */
var csName = COMMCSNAME;
var pcdName1 = COMMCLNAME + '_procedurename';
var pcdName2 = 'sum_procedure_8192';

main();

function main ()
{
   if( commIsStandalone( db ) )
   {
      println( " Deploy mode is standalone!" );
      return;
   }
   try
   {
      ready();
      createPcd();
      excutePcd();
      listPcd();
      removePcd();
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

function ready ()
{
   println( "\n---begin to remove procedures and cs in ready" );
   fmpRemoveProcedures( [pcdName1, pcdName2], true );
   commDropCS( db, csName, true, "drop cs[" + csName + "] in ready" );
}

function createPcd ()
{
   println( "\n---begin to create procedures" );
   var str = "db.createProcedure( function " + pcdName1 + "("
      + ") {db.createCS('" + csName + "')} )";
   db.eval( str );
   db.createProcedure( function sum_procedure_8192 ( x, y, z ) { return x + y + z; } );
}

function excutePcd ()
{
   println( "\n---begin to excute procedures" );
   db.eval( pcdName1 + "()" );
   try
   {
      db.getCS( csName );
   }
   catch( e )
   {
      throw buildException( "get cs", "",
         'db.getCS(' + csName + ')',
         "not throw error", e );
   }

   var sumRst = db.eval( pcdName2 + "(1,2,3)" );
   if( sumRst !== 6 )
   {
      throw buildException( "check sum", "",
         'check sum result of 1+2+3',
         6, sumRst );
   }
}

function listPcd ()
{
   println( "\n---begin to list procedures" );
   var rc = db.listProcedures();

   println( "\n---begin to list procedures by filter" );

   var expPcd1 = {};
   expPcd1["name"] = pcdName1;
   expPcd1["func"] = { "$code": "function " + pcdName1 + "() {\n    db.createCS(\"" + csName + "\");\n}" };
   expPcd1["funcType"] = 0;

   var rc = db.listProcedures( { name: pcdName1 } );
   checkResult( rc, [expPcd1] );

   var expPcd2 = {};
   expPcd2["name"] = pcdName2;
   expPcd2["func"] = { "$code": "function " + pcdName2 + "(x, y, z) {\n    return x + y + z;\n}" };
   expPcd2["funcType"] = 0;

   var rc = db.listProcedures( { name: pcdName2 } );
   checkResult( rc, [expPcd2] );
}

function removePcd ()
{
   println( "\n---begin to remove procedure" );
   db.removeProcedure( pcdName1 );

   var expPcd2 = {};
   expPcd2["name"] = pcdName2;
   expPcd2["func"] = { "$code": "function " + pcdName2 + "(x, y, z) {\n    return x + y + z;\n}" };
   expPcd2["funcType"] = 0;

   var rc = db.listProcedures( { name: pcdName1 } );
   checkResult( rc, [] );

   var rc = db.listProcedures( { name: pcdName2 } );
   checkResult( rc, [expPcd2] );

}

function clean ()
{
   println( "\n---begin to clean environment" );
   fmpRemoveProcedures( [pcdName1, pcdName2], true );
   commDropCS( db, csName, true, "drop cs in clean()" )
}