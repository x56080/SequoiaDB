/*******************************************************************************
*@Description : create index and query. query match $type/$exists/$elemmathc/
*                                                   /$+标识符/$size/$regex
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
      $and: [{ "string": { "$type": 1, $et: 2 } },
      { "obj_id": { "$exists": 1 } },
      { "subobj": { "$elemMatch": { "obj": { "val": "sub" } } } },
      { "array.$1": "5arr5" },
      { "array": { "$size": 1, "$et": 4 } },
      {
         "string": {
            "$regex": "西边个喇嘛，东边个哑巴*",
            "$options": "i"
         }
      }]
   };
   println( "" );
   println( "Condition: " + JSON.stringify( queryCond ) );
   idxQueryCheck( cl, queryCond, 1, idxName );

   var queryCond = {
      $not: [{
         $and: [{ "string": { "$type": 1, $et: 2 } },
         { "obj_id": { "$exists": 1 } },
         { "subobj": { "$elemMatch": { "obj": { "val": "sub" } } } },
         { "array.$1": "5arr5" },
         { "array": { "$size": 1, "$et": 4 } },
         {
            "string": {
               "$regex": "西边个喇嘛，东边个哑巴*",
               "$options": "i"
            }
         }]
      }]
   };
   println( "" );
   println( "Condition: " + JSON.stringify( queryCond ) );
   idxQueryCheck( cl, queryCond, 9, idxName );

   var queryCond = {
      $or: [{
         $not: [{
            $and: [{ "string": { "$type": 1, $et: 2 } },
            { "obj_id": { "$exists": 1 } },
            { "subobj": { "$elemMatch": { "obj": { "val": "sub" } } } },
            { "array.$1": "5arr5" },
            { "array": { "$size": 1, "$et": 4 } },
            {
               "string": {
                  "$regex": "西边个喇嘛，东边个哑巴*",
                  "$options": "i"
               }
            }]
         }]
      },
      {
         $and: [{ "string": { "$type": 1, $et: 2 } },
         { "obj_id": { "$exists": 1 } },
         { "subobj": { "$elemMatch": { "obj": { "val": "sub" } } } },
         { "array.$1": "5arr5" },
         { "array": { "$size": 1, "$et": 4 } },
         {
            "string": {
               "$regex": "西边个喇嘛，东边个哑巴*",
               "$options": "i"
            }
         }]
      }
      ]
   };
   println( "" );
   println( "Condition: " + JSON.stringify( queryCond ) );
   idxQueryCheck( cl, queryCond, 10, idxName );

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
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true,
      "failed to drop collection in the end, wrong" );
   db.close();
   throw e;
}
