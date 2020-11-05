/******************************************************************************
@Description : seqDB-9927:删除存储过程（异常）
@Modify list :
               2016-9-11   TingYU      Init
******************************************************************************/
var pcdName = COMMCLNAME + '_procedurename_9927';
main( test );

function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }
   ready();
   assert.tryThrow( -233, function()
   {
      db.removeProcedure( pcdName );
   } );
   parameterCheck();
   clean();
}


function parameterCheck ()
{
   assert.tryThrow( -259, function()
   {
      db.removeProcedure();
   } );

   assert.tryThrow( -6, function()
   {
      db.removeProcedure( 123 );
   } );

}

function ready ()
{
   fmpRemoveProcedures( [pcdName], true );
}

function clean ()
{
   fmpRemoveProcedures( [pcdName], true );
}
