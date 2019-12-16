/******************************************************************************
*@Description : test find special decimal value with match symbol
*               $gt $gte $lt $lte $et $ne $mod $type $elemMatch 
*               $+标识符 $field 
*               seqDB-13993:使用匹配符查询特殊decimal值           
*@author      : Liang XueWang 
******************************************************************************/
main();

function main ()
{
   var docs = [{ a: { $decimal: "MAX" } },
   { a: { $decimal: "MIN" } },
   { a: { $decimal: "NaN" } }];

   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true, "drop CL in the beginning" );
   var cl = commCreateCL( db, COMMCSNAME, COMMCLNAME );
   insertData( cl, docs );

   println( "test $gt" );
   var cursor = findData( cl, { a: { $gt: 0 } } );
   var expRecs = [docs[0]];
   checkRec( cursor, expRecs );

   println( "test $gte" );
   cursor = findData( cl, { a: { $gte: 0 } } );
   expRecs = [docs[0]];
   checkRec( cursor, expRecs );

   // test $lt NaN < 0 ?
   println( "test $lt" );
   cursor = sortFindData( cl, { a: { $lt: 0 } }, {}, { _id: 1 } );
   expRecs = [docs[1], docs[2]];
   checkRec( cursor, expRecs );

   // test $lte NaN < 0 ?
   println( "test $lte" );
   cursor = sortFindData( cl, { a: { $lte: 0 } }, {}, { _id: 1 } );
   expRecs = [docs[1], docs[2]];
   checkRec( cursor, expRecs );

   println( "test $et" );
   for( var i = 0; i < docs.length; i++ )
   {
      cursor = findData( cl, { a: { $et: docs[i].a } } );
      expRecs = [docs[i]];
      checkRec( cursor, expRecs );
   }

   println( "test $ne" );
   for( var i = 0; i < docs.length; i++ )
   {
      cursor = sortFindData( cl, { a: { $ne: docs[i].a } }, {}, { _id: 1 } );
      var idx1 = ( i + 1 ) % docs.length;
      var idx2 = ( i + 2 ) % docs.length;
      var minIdx = ( idx1 > idx2 ) ? idx2 : idx1;
      var maxIdx = idx1 + idx2 - minIdx;
      expRecs = [docs[minIdx], docs[maxIdx]];
      checkRec( cursor, expRecs );
   }

   println( "test $mod" );
   cursor = findData( cl, { a: { $mod: [5, 3] } } );
   expRecs = [];
   checkRec( cursor, expRecs );

   println( "test $type" );
   cursor = sortFindData( cl, { a: { $type: 1, $et: 100 } }, {}, { _id: 1 } );
   expRecs = [docs[0], docs[1], docs[2]];
   checkRec( cursor, expRecs );

   println( "test $elemMatch" );
   testElemMatch( cl );

   println( "test $1" );
   testIdentifier( cl );

   println( "test $field" );
   testField( cl );
}

function testElemMatch ( cl )
{
   var docs = [{ b: { no: { $decimal: "MAX" } } },
   { b: { no: { $decimal: "MIN" } } },
   { b: { no: { $decimal: "NaN" } } }];
   insertData( cl, docs );
   for( var i = 0; i < docs.length; i++ )
   {
      var cursor = findData( cl, { b: { $elemMatch: { no: docs[i].b.no } } } );
      var expRecs = [docs[i]];
      checkRec( cursor, expRecs );
   }
   deleteData( cl, { b: { $exists: 1 } } );
}

function testIdentifier ( cl )
{
   var docs = [{ b: [{ $decimal: "MAX" }] },
   { b: [{ $decimal: "MIN" }] },
   { b: [{ $decimal: "NaN" }] }];
   insertData( cl, docs );
   for( var i = 0; i < docs.length; i++ )
   {
      var cursor = findData( cl, { "b.$1": docs[i]["b"][0] } );
      var expRecs = [docs[i]];
      checkRec( cursor, expRecs );
   }
   deleteData( cl, { b: { $exists: 1 } } );
}

function testField ( cl )
{
   var docs = [{ b1: { $decimal: "MAX" }, b2: { $decimal: "MAX" } },
   { b1: { $decimal: "MIN" }, b2: { $decimal: "MIN" } },
   { b1: { $decimal: "NaN" }, b2: { $decimal: "NaN" } },
   { b1: { $decimal: "MAX" }, b2: { $decimal: "MIN" } },
   { b1: { $decimal: "MAX" }, b2: { $decimal: "NaN" } },
   { b1: { $decimal: "MIN" }, b2: { $decimal: "NaN" } }];
   insertData( cl, docs );
   var cursor = sortFindData( cl, { b1: { $field: "b2" } }, {}, { _id: 1 } );
   var expRecs = [docs[0], docs[1], docs[2]];
   checkRec( cursor, expRecs );
   deleteData( cl, { b1: { $exists: 1 } } );
}