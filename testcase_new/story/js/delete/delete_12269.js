/******************************************************************************
*@Description : delete basic: [cond]
*@Modify list :
*               2015-01-27  pusheng Ding  Init
******************************************************************************/

main( test );
function test ()
{
   var clName = COMMCLNAME + "_12269";
   rowcnt = 100;

   commDropCL( db, COMMCSNAME, clName, true, true );

   //create cl
   var varCL = commCreateCL( db, COMMCSNAME, clName );

   //insert data
   var docs = [];
   for( var i = 0; i < rowcnt; i++ )
   {
      docs.push( { a: rowcnt - i, b: i, c: "abcdefghijkl" + i } );

   }
   varCL.insert( docs );

   //delete
   cond = 50;
   varCL.remove( { b: { $lt: 50 } } );
   docs.splice( 0, 50 );
   var cursor = varCL.find();
   commCompareResults( cursor, docs );
   //clean test-env
   commDropCL( db, COMMCSNAME, clName, true, true );
}