/* *****************************************************************************
@description: seqDB-21997:向子表中的分区键字段插入记录
@author: 2020-3-31 zhaoxiaoni  Init
***************************************************************************** */
testConf.skipStandAlone = true;

main( test );

function test( )
{
   var mainCLName = CHANGEDPREFIX + "_mainCL_21997";
   var subCLName = CHANGEDPREFIX + "_subCL_21997";

   commDropCL( db, COMMCSNAME, mainCLName );
   var mainCL = commCreateCL( db, COMMCSNAME, mainCLName, { ShardingKey: { a: 1 }, ShardingType: "range", IsMainCL: true } );
   var subCL = commCreateCL( db, COMMCSNAME, subCLName, { ShardingKey: { a: 1 }, ShardingType: "hash" } );
   mainCL.attachCL( COMMCSNAME + "." + subCLName, { LowBound: { a: 0 }, UpBound: { a: 100 } } );

   var array = new Array( 4000 );
   array = array.join( "a" );
   subCL.insert( { a: array } );

   array = new Array( 1024*1024+1 );
   array = array.join( "aaaaaaaaaaaaaaa" );
   try
   {
      subCL.insert( { a: array } );
   }
   catch( e )
   {
      if( e !== -39 )
      {
         throw e;
      }
   }

   array = new Array( 1024*1024+1 );
   array = array.join( "aaaaaaaaaaaaaaaaa" );
   try
   {
      subCL.insert( { a: array } );
      throw new Error( "Insert should be failed!" );
   }
   catch( e )
   {
      if( e !== -10 )
      {
         throw e;
      }
   }

   commDropCL( db, COMMCSNAME, mainCLName, false );
}
