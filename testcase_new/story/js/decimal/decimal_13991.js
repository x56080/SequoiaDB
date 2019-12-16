/******************************************************************************
*@Description : test insert special decimal value
*               seqDB-13991:插入特殊decimal值              
*@author      : Liang XueWang 
******************************************************************************/
main();

function main ()
{
   var docs = [{ a: { $decimal: "MAX" } },
   { a: { $decimal: "INF" } },
   { a: { $decimal: "MAX", $precision: [1000, 999] } }, // MAX
   { a: { $decimal: "MIN" } },
   { a: { $decimal: "-INF" } },
   { a: { $decimal: "MIN", $precision: [1000, 100] } }, // MIN
   { a: { $decimal: "NaN" } }];

   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true, "drop CL in the beginning" );
   var cl = commCreateCL( db, COMMCSNAME, COMMCLNAME );
   insertData( cl, docs );

   var maxRecs = [docs[0], docs[0], docs[0]];
   var minRecs = [docs[3], docs[3], docs[3]];
   var nanRecs = [docs[6]];

   for( var i = 0; i < docs.length; i++ )
   {
      var cursor = findData( cl, docs[i] );
      if( i < 3 )
         checkRec( cursor, maxRecs );
      else if( i < 6 )
         checkRec( cursor, minRecs );
      else
         checkRec( cursor, nanRecs );
   }
}