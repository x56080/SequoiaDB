function main ()
{
   var cl = new collection( db, COMMCSNAME, COMMCLNAME );
   cl.create();
   cl.bulkInsert();
   var cursor = cl.execAggregate( { $group: { _id: "$dep", addtoset_major: { $addtoset: "$major" }, push_no: { $push: "$no" } } }, { $skip: 1 }, { $limit: 1 } );
   var expectResult = [
      {
         "addtoset_major": ["计算机科学与技术", "计算机软件与理论", "计算机工程"],
         "push_no": [1000, 1001, 1002, 1003, 1004, 1005]
      }];
   var ret = checkResult( cursor, expectResult );
   if( !ret[0] )
   {
      var parameter = "{$group:{_id:'$dep', addtoset_major:{$addtoset:'$major'}, push_no:{$push:'$no'}}}, {$skip:1}, {$limit:1}"
      throw buildException( "main", 0, "cl.aggregate( " + parameter + " )",
         JSON.stringify( expectResult ), JSON.stringify( retResult[1] ) );
   }

   cl.drop();
}

main()
