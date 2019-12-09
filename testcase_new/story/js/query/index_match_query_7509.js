/*******************************************************************************
*@Description : create index and query. query match $gt/$gte/$lt/$lte/$ne
*                                                   $and/$not/$or
*@Modify list :
*               2014-5-20  xiaojun Hu  Init
*******************************************************************************/

function main ()
{
   var insertNum = 10;
   var cl = commCreateCL( db, COMMCSNAME, COMMCLNAME, 0, true, true, false,
      "create collection in the beginning" );
   // insert data
   idxAutoGenData( cl, insertNum );
   println( "insert " + insertNum + " data successful" );

   // create Index
   var idxName = "noIndex";
   var indexDef = { "no": 1, "no1": 1, "no2": -1, "no3": 1 };
   commCreateIndex( cl, idxName, indexDef, false, false );
   commCheckIndex( cl, idxName, true );
   println( "create index successful" );

   // query data
   var queryCond = {
      $and: [{ "no": { $gt: 2 } }, { "no1": { $gte: 4 } },
      { "no2": { $lt: 18 } },
      { "array": { "$in": ["5arr5", 5] } },
      { "no3": { $lte: 28 } }]
   };
   println( "" );
   println( "Condition: " + JSON.stringify( queryCond ) );
   idxQueryCheck( cl, queryCond, 1, idxName );
   var queryCond = {
      $and: [{ "no": { $gt: 3 } }, { "no1": { $gte: 4 } },
      { "no2": { $lt: 20 } },
      { "array": { "$nin": [25, 20] } },
      { "no3": { $lte: 28 } }]
   };
   println( "" );
   println( "Condition: " + JSON.stringify( queryCond ) );
   idxQueryCheck( cl, queryCond, 1, idxName );
   var queryCond = {
      $not: [{ "no": { $gt: 7 } }, { "no1": { $gte: 10 } },
      { "no": { "$ne": 9 } },
      { "no2": { $lt: 30 } },
      { "no3": { $lte: 40 } }]
   };
   println( "" );
   println( "Condition: " + JSON.stringify( queryCond ) );
   idxQueryCheck( cl, queryCond, 9, idxName );
   var queryCond = {
      $or: [{
         $and: [{ "no": { $gt: 6 } },
         { "no": { $lt: 10 } }]
      },
      { "array": { "$all": ["7arr7", 35, "14ARR7", "arrayIndex"] } },
      {
         $and: [{ "no1": { $gte: 0 } },
         { "no1": { $lte: 6 } }]
      }]
   };
   println( "" );
   println( "Condition: " + JSON.stringify( queryCond ) );
   idxQueryCheck( cl, queryCond, 7, idxName );

}

// Run Main

try
{
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true,
      "failed to drop collection in the begin" );
   main( db );
   commDropCL( db, COMMCSNAME, COMMCLNAME, false, false,
      "failed to drop collection in the end, correct" );
   db.close();
}
catch( e )
{
   //commDropCL( db, COMMCSNAME, COMMCLNAME, true, true,
   //            "failed to drop collection in the end, wrong" ) ;
   //db.close() ;
   throw e;
}

function checkResult ( cursor, expectResult )
{
   var ret = [];
   if( cursor.constructor !== SdbCursor ||
      expectResult.constructor !== Array )
   {
      throw buildException( "checkResult", "parameter error" );
   }

   var i = 0;
   while( cursor.next() )
   {
      retObj = cursor.current().toObj();
      if( !compareObj( retObj, expectResult, true ) )
      {
         ret.push( false );
         ret.push( retObj );
         return ret;
      }
      i++;
   }

   if( i === 0 )
   {
      ret.push( false );
   }
   else
   {
      ret.push( true );
   }
   ret.push( {} );
   return ret;
}
