/************************************
*@Description: upsert any object(exist or not exist) use operator pop
*@author:      zhaoyu
*@createdate:  2016.5.18
**************************************/
function main ()
{
   //clean environment before test
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true, "drop CL in the beginning" );

   //create cl
   var dbcl = commCreateCL( db, COMMCSNAME, COMMCLNAME, 0 );

   //insert object
   var Doc = { a: 1 };
   insertData( dbcl, Doc );

   //upsert any object when match nothing,use matches and
   var upsertCondition1 = {
      $pop: {
         "object12.1.2": 6, object6: -5, object7: 0,
         object2: 2,
         object1: 4, object3: 2, object11: 0,
         object5: -3, object8: -11,
         "object4.1": 1, "object9.1": -1, object10: 0
      }
   };
   var findCondition1 = {
      $and: [{ object12: [50, [30, 50, [90, 40], 80], 25, 40, 15] },
      { object6: [5, 7, 9, 2, 8, 4] },
      { object7: [3, 7] },
      { object1: { $et: { $decimal: "2" } } },
      { object13: { $all: [10, 20, 30] } },
      { object2: { $all: [15, 25, 35] } },
      { object4: [5, [9, 8, 4, 3], 10] },
      { object9: [5, [9, 8, 4, 3], 10] },
      { c: { $gt: 100 } },
      { d: 1000 },
      { "e.name.firstName": "han", "e.name.lastName": "meimei", "e.name.midName": "test" }]
   };
   upsertData( dbcl, upsertCondition1, findCondition1 );

   //check result
   var expRecs1 = [{
      object12: [50, [30, 50, [], 80], 25, 40, 15],
      object6: [4],
      object7: [3, 7],
      object1: { $decimal: "2" },
      object13: [10, 20, 30],
      object2: [15],
      object4: [5, [9, 8, 4], 10],
      object9: [5, [8, 4, 3], 10],
      d: 1000,
      e: { name: { firstName: "han", lastName: "meimei", midName: "test" } }
   },
   { a: 1 }];
   checkResult( dbcl, null, null, expRecs1, { a: 1 } );

   //upsert any object when match nothing,use matches or
   /*var upsertCondition2 = {$pop:{object1:1,
                                 object2:-2,
                                 object3:0,
                                 "object4.1":1,
                                 "object9.1":-1,
                                 "object6.1":0}};
   var findCondition2 = {$or:[{object1:{$et:{$decimal:"63"}}},
                              {object13:{$all:[100,200,300]}},
                              {object2:{$all:[15,25,35]}},
                              {c:{$gt:100}},
                              {d:1001}]};
   upsertData( dbcl, upsertCondition2, findCondition2 );
   
   //check result
   var expRecs2 = [{object12:[50,[30,50,[],80],25,40,15],
                   object6:[4],
                   object7:[3,7],
                   object1:{$decimal:"2"},
                   object13:[10,20,30],
                   object2:[15],
                   object4:[5,[9,8,4],10],
                   object9:[5,[8,4,3],10],
                   d:1000,
                   e:{name:{firstName:"han",lastName:"meimei",midName:"test"}}},
                  {a:1}];
   checkResult( dbcl, null, null, expRecs2, {a:1} );
   
   //delete all data
   deleteData( dbcl, null );
   
   //upsert any object when match nothing,use matches not
   var upsertCondition3 = {$pop:{object1:1,
                                 object2:-2,
                                 object3:0,
                                 "object4.1":1,
                                 "object5.1":-1,
                                 "object6.1":0}};
   var findCondition3 = {$not:[{object1:{$et:{$decimal:"63"}}},
                               {object13:{$all:[100,200,300]}},
                               {object2:{$all:[15,25,35]}},
                               {c:{$gt:100}},
                               {d:1001}]};
   upsertData( dbcl, upsertCondition3, findCondition3 );
   
   //check result
   var expRecs3 = [];
   checkResult( dbcl, null, null, expRecs3, {a:1} );*/
}

try
{
   main();
}
catch( e )
{
   if( e.constructor === Error )
   {
      println( e.stack );
   }
   throw e;
}
;