/******************************************************************************
@Description : 1.columnts-number to sort: 16 columns
@Modify list :
               2015-01-15 pusheng Ding  Init
******************************************************************************/
main();
function main ()
{
   var indexName = "index13740";
   var rownums = 10000;
   var csName = COMMCSNAME;
   var clName = "cl13740";

   commDropCL( db, csName, clName, true, true, "drop cl in the beginning" );
   var options = { ReplSize: 0 };
   var varCL = commCreateCL( db, csName, clName, options, true, false, "create cl." );

   //insert data
   var records = [];
   for( var i = 0; i < rownums; i++ )
   {
      records.push( { a1: i, a2: i + 1, a3: i + 2, a4: i + 3, a5: i + 4, a6: i + 5, a7: i + 6, a8: i + 7, a9: i + 8, a10: i + 9, a11: i + 10, a12: i + 11, a13: i + 12, a14: i + 13, a15: i + 14, a16: i + 15 } );
   }
   varCL.insert( records );
   println( "insert data finished!" );

   //query1
   //select a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,a15,a16 from foo.bar order by a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,a15,a16
   var sel = varCL.find( null, { a1: null, a2: null, a3: null, a4: null, a5: null, a6: null, a7: null, a8: null, a9: null, a10: null, a11: null, a12: null, a13: null, a14: null, a15: null, a16: null } ).sort( { a1: 1, a2: 1, a3: 1, a4: 1, a5: 1, a6: 1, a7: 1, a8: 1, a9: 1, a10: 1, a11: 1, a12: 1, a13: 1, a14: 1, a15: 1, a16: 1 } );
   checkRec( sel, records );
   println( "'select a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,a15,a16 from foo.bar order by a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,a15,a16' finished!" );

   //create index
   varCL.createIndex( indexName, { a1: 1, a2: 1, a3: 1, a4: 1, a5: 1, a6: 1, a7: 1, a8: 1, a9: 1, a10: 1, a11: 1, a12: 1, a13: 1, a14: 1, a15: 1, a16: 1 } );

   //query2
   //select a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,a15,a16 from foo.bar order by a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,a15,a16
   var sel = varCL.find( null, { a1: null, a2: null, a3: null, a4: null, a5: null, a6: null, a7: null, a8: null, a9: null, a10: null, a11: null, a12: null, a13: null, a14: null, a15: null, a16: null } ).sort( { a1: 1, a2: 1, a3: 1, a4: 1, a5: 1, a6: 1, a7: 1, a8: 1, a9: 1, a10: 1, a11: 1, a12: 1, a13: 1, a14: 1, a15: 1, a16: 1 } ).hint( { "": indexName } );
   checkRec( sel, records );
   println( "'select a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,a15,a16 from foo.bar order by a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,a15,a16' with index finished!" );

   commDropCL( db, csName, clName, false, false, "drop cl in the end" );
}
