/******************************************************************************
@Description : seqDB-8195:执行存储过程db.eval（异常）
@Modify list :               
               2016-9-11   TingYU      modify
******************************************************************************/
var csName = COMMCSNAME + 'procedure_8195';
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
      excuteWrongPcd();
      excuteNotExistPcd();
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

function excuteWrongPcd ()
{
   println( "\n---begin to excute procedure that expect throw error" );

   var cmd = "db.createProcedure( function " + pcdName + "(x){return db.getCS(x);} )";
   db.eval( cmd );

   try
   {
      var cmd = pcdName + '("' + csName + '")';
      db.eval( cmd )
      throw "did not throw error";
   }
   catch( e )
   {
      if( e !== -34 )
      {
         throw buildException( "excuteWrongPcd()", "", "db.eval(" + cmd + ")",
            "throw -34", e );
      }
   }
}

function excuteNotExistPcd ()
{
   println( "\n---begin to excute nonexistent procedure" );

   fmpRemoveProcedures( [pcdName], true );

   try
   {
      var cmd = pcdName + "()";
      db.eval( cmd );
      throw "did not throw error";
   }
   catch( e )
   {
      if( e !== -152 )
      {
         throw buildException( "excuteNotExistPcd()", "", "db.eval(" + cmd + ")",
            "throw -152", e );
      }
   }
}

function ready ()
{
   println( "\n---begin to remove procedures and cs in ready" );
   fmpRemoveProcedures( [pcdName], true );
   commDropCS( db, csName, true, "drop cs[" + csName + "] in ready" );
}

function clean ()
{
   println( "\n---begin to clean environment" );
   fmpRemoveProcedures( [pcdName], true );
   commDropCS( db, csName, true, "drop cs[" + csName + "] in clean" );
}