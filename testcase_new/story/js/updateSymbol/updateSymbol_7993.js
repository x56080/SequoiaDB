/************************************
*@Description: upsert any object(exist or not exist) use operator unset
*@author:      zhaoyu
*@createdate:  2016.5.17
**************************************/
function main ()
{
   //clean environment before test
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true, "drop CL in the beginning" );

   //create cl
   var dbcl = commCreateCL( db, COMMCSNAME, COMMCLNAME );

   //insert object
   var Doc = { a: 1 };
   insertData( dbcl, Doc );

   //upsert any object when match nothing,use matches and
   var upsertCondition3 = {
      $unset: {
         object1: "",
         object13: "",
         object14: "",
         "object2.0": "",
         "e.name.midName": ""
      }
   };
   var findCondition3 = {
      $and: [{ object1: { $et: { $decimal: "2" } } },
      { object13: { $all: [10, 20, 30] } },
      { object2: { $all: [15, 25, 35] } },
      { c: { $gt: 100 } },
      { d: 1000 },
      { "e.name.firstName": "han", "e.name.lastName": "meimei", "e.name.midName": "test" }]
   };
   upsertData( dbcl, upsertCondition3, findCondition3 );

   //check result
   var expRecs3 = [{ d: 1000, e: { name: { firstName: "han", lastName: "meimei" } }, object2: [null, 25, 35] },
   { a: 1 }];
   checkResult( dbcl, null, null, expRecs3, { a: 1 } );

   //upsert any object when match nothing,use matches or
   /*var upsertCondition4 = {$unset:{object1:"",
                                   object13:"",
                                   object14:"",
                                   "object2.1":"",
                                   "e.name.lastName":""}};
   var findCondition4 = {$or:[{object1:{$et:{$decimal:"2"}}},
                               {object13:{$all:[10,20,30]}},
                               {object2:{$all:[15,25,35]}},
                               {c:{$gt:100}},
                               {d:1001},
                               {"e.name.firstName":"han","e.name.lastName":"meimei","e.name.midName":"test"}]};
   upsertData( dbcl, upsertCondition4, findCondition4 );
   
   //check result
   var expRecs4 = [{d:1000,e:{name:{firstName:"han",lastName:"meimei"}},object2:[null,25,35]},
                   {a:1}];
   checkResult( dbcl, null, null, expRecs4, {a:1} );
   
   //delete all data
   deleteData( dbcl, null );
   
   //upsert any object when match nothing,use matches not
   var upsertCondition5 = {$unset:{object1:"",
                                   object13:"",
                                   object14:"",
                                   "object2.1":"",
                                   "e.name.lastName":""}};
   var findCondition5 = {$not:[{object1:{$et:{$decimal:"2"}}},
                               {object13:{$all:[10,20,30]}},
                               {object2:{$all:[15,25,35]}},
                               {c:{$gt:100}},
                               {d:1001},
                               {"e.name.firstName":"han","e.name.lastName":"meimei","e.name.midName":"test"}]};
   upsertData( dbcl, upsertCondition5, findCondition5 );
   
   //check result
   var expRecs5 = [{d:1000,e:{name:{firstName:"han",lastName:"meimei"}},object2:[null,25,35]},
                   {a:1}];
   checkResult( dbcl, null, null, expRecs5, {a:1} );*/
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