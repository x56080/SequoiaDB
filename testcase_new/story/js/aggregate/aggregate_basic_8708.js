/*******************************************************************************
*@Description : Aggregate match|group|match combination
*@Modify list :
*               2016-03-18  wenjing wang rewrite
*******************************************************************************/
function main ()
{
   var cl = new collection( db, COMMCSNAME, COMMCLNAME );
   cl.create();
   cl.bulkInsert();
   var cursor = cl.execAggregate( { $match: { no: { $gt: 1001 }, "info.sex": "男" } }, { $group: { _id: "$major", major: { $first: "$major" }, last_score: { $last: "$score" } } }, { $match: { last_score: { $gte: 70 } } } );

   var retObj = null;
   var expectResult = [{ major: "光学", last_score: 78 }, { major: "电学", last_score: 92 }, { major: "计算机软件与理论", last_score: 90 }];
   var ret = checkResult( cursor, expectResult, retObj );
   if( !ret[0] )
   {
      throw buildException( "main", 0, "cl.aggregate( {$match:{no:{$gt:1001}, info.sex:'男'}}, {$group:{_id:'$major', major:{$first:'$major'}, last_score:{$last:'$score'}}}, {$match:{last_score:{$gte:70}}} )",
         JSON.stringify( ret[1] ), JSON.stringify( ret[2] ) );
   }
   cl.drop();
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

