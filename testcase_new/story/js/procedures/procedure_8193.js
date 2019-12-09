/******************************************************************************
@Description : seqDB-8193:创建存储过程（异常）
@Modify list :
               2014-7-30   xiaojun Hu  Init
               2016-9-11   TingYU      modify
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
   try
   {
      ready();
      createExistPcd();
      lackName();
      wrongParameterType();
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

function createExistPcd ()
{
   println( "\n---begin to create procedure that has existed" );
   var cmd = "db.createProcedure( function " + pcdName + "(x, y){return x-y;} )";
   db.eval( cmd );

   try
   {
      db.eval( cmd );
      throw "did not throw error";
   }
   catch( e )
   {
      if( e !== -342 )
      {
         throw buildException( "testExistPcd()", "", cmd, "throw -342", e );
      }
   }

   println( "\n---begin to create procedure that has the same name and different parameter" );
   var cmd = "db.createProcedure( function " + pcdName + "(x){return x;} )";

   try
   {
      db.eval( cmd );
      throw "did not throw error";
   }
   catch( e )
   {
      if( e !== -342 )
      {
         throw buildException( "testExistPcd()", "", cmd, "throw -342", e );
      }
   }
}

function lackName ()
{
   println( "\n---begin to create procedure that lack of function name" );
   db.eval( cmd );

   try
   {
      var cmd = "db.createProcedure( function " + "(x, y){return x-y;} )";
      db.eval( cmd );
      throw "did not throw error";
   }
   catch( e )
   {
      if( e !== -6 )
      {
         throw buildException( "testExistPcd()", "", cmd, "throw -6", e );
      }
   }
}

function wrongParameterType ()
{
   println( "\n---begin to create procedure with wrong parameter type: eg string" );
   db.eval( cmd );

   try
   {
      var cmd = "db.createProcedure( function " + pcdName + "{return x-y;} )";
      db.createProcedure( "" );
      throw "did not throw error";
   }
   catch( e )
   {
      if( -6 != e )
      {
         throw buildException( "testExistPcd()", "", 'db.createProcedure("")',
            "-6", e );
      }
   }
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
