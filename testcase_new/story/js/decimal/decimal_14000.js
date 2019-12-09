/******************************************************************************
*@Description : test special decimal value with index
*               seqDB-14000:创建decimal字段索引后插入特殊decimal值         
*@author      : Liang XueWang 
******************************************************************************/
main();

function main ()
{
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true, "drop CL in the beginning" );

   var cl = commCreateCL( db, COMMCSNAME, COMMCLNAME, 0 );
   commCreateIndex( cl, "aIndex", { a: 1 }, true );

   var docs = [{ a: { $decimal: "MAX" } },
   { a: { $decimal: "MIN" } },
   { a: { $decimal: "NaN" } }];
   insertData( cl, docs );

   for( var i = 0; i < docs.length; i++ )
   {
      var cursor = findData( cl, docs[i] );
      var expRecs = [docs[i]];
      checkRec( cursor, expRecs );
   }

   for( var i = 0; i < docs.length; i++ )
   {
      var cursor = findData( cl, docs[i] );
      var expRes = { ScanType: "ixscan", IndexName: "aIndex" };
      checkExplain( cursor, expRes );
   }
}

function checkExplain ( cursor, expRes )
{
   var actRes = cursor.explain().next().toObj();
   for( var k in expRes )
   {
      if( expRes[k] !== actRes[k] )
      {
         throw buildException( "checkExplain", null, "check explain info",
            expRes[k], actRes[k] );
      }
   }
}