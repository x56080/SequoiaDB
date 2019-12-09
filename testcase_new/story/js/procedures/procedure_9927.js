/******************************************************************************
@Description : seqDB-9927:删除存储过程（异常）
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
   try
   {
      ready();
      removeNotExistPcd();
      parameterCheck();
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

function removeNotExistPcd ()
{
   println( "\n---begin to remove nonexistent procedure" );
   try
   {
      db.removeProcedure( pcdName );
      throw "did not throw error";
   }
   catch( e )
   {
      if( e !== -233 )
      {
         throw buildException( "removeNotExistPcd()", "",
            'db.removeProcedure(' + pcdName + ')', "throw -233", e );
      }
   }

}

function parameterCheck ()
{
   println( "\n---begin to check parameter of removeProcedure" );
   try
   {
      db.removeProcedure();
      throw "did not throw error";
   }
   catch( e )
   {
      if( -259 != e )
      {
         throw buildException( "parameterCheck()", "", 'db.removeProcedure()',
            "throw error: wrong arguments", e );
      }
   }

   try
   {
      db.removeProcedure( 123 );
      throw "did not throw error";
   }
   catch( e )
   {
      if( -6 != e )
      {
         throw buildException( "parameterCheck()", "", 'db.removeProcedure(123)',
            -6, e );
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
