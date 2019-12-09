/******************************************************************************
*@Description : test SdbQueryOption
*               TestLink :    seqDB-15750:ʹ��SdbOptionBase��ѯ��¼
                              seqDB-15751:ʹ��SdbQueryOption�����洢����
*@auhor       : CSQ 
******************************************************************************/

function main ()
{
   try
   {
      commDropCS( db, COMMCSNAME + "15750", true, "drop CS " + COMMCSNAME + "15750" );
   } catch( e ) { }
   var varCS = commCreateCS( db, COMMCSNAME + "15750", true, "create CS" );
   var varCL = varCS.createCL( COMMCLNAME + "15750" );
   varCL.createIndex( "bindex", { b: 1 }, false );
   insertRecord( varCL );
   test15750( varCL );
   test15751( varCL );

   try
   {
      commDropCS( db, COMMCSNAME + "15750", true, "drop CS " + COMMCSNAME + "15750" );
   }
   catch( e )
   {
      throw buildException( "teardown 15750 fail", e, "clear", "success", e );
   }
}

function test15750 ( varCL )
{
   var option = new SdbOptionBase();
   option.cond( { b: { $lt: 5 } } ).sel( { _id: { $include: 1 }, a: { $include: 1 }, b: { $include: 1 } } ).sort( { _id: 1 } ).hint( { "": "bindex" } ).limit( 3 ).skip( 1 ).flags( 1 );
   var cur = varCL.find( option );
   var expFindResult = [{ "_id": 1, "a": 1, "b": 1 },
   { "_id": 2, "a": 2, "b": 2 },
   { "_id": 3, "a": 3, "b": 3 }];
   checkRec( cur, expFindResult );
}

function test15751 ( varCL )
{
   if( commIsStandalone( db ) )
   {
      println( "skip standalone environment" );
      return;
   }
   db.createProcedure( function test15750 () { return new SdbQueryOption().cond( { b: { $lt: 5 } } ).sel( { _id: { $include: 0 } } ).sort( { b: -1 } ).limit( 7 ).skip( 2 ).update( { $inc: { c: 1 } }, true, { KeepShardingKey: true } ); } )
   var a = db.eval( 'test15750()' );
   var cur = varCL.find( a );
   var expFindResult = [{ "a": 2, "b": 2, "c": -1 },
   { "a": 1, "b": 1, "c": 0 },
   { "a": 0, "b": 0, "c": 1 }
   ];
   checkRec( cur, expFindResult );
   try
   {
      db.removeProcedure( "test15750" );
   }
   catch( e )
   {
      throw buildException( "teardown test15751 fail", e, "clear", "success", e );
   }
}

function insertRecord ( varCL )
{
   for( var i = 0; i <= 100; i++ )
   {
      varCL.insert( { _id: i, a: i, b: i, c: -i } );
   }
}

main();