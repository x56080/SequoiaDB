/******************************************************************************
*@Description : insert, test flag and options                                
*               seqDB-18010:插入数据_id索引键冲突
*@Author      : 2019-3-13  XiaoNi Huang
******************************************************************************/
main();

function main ()
{
   println( "\n---Begin to run test" );
   var clName = "insertFlag_18010";
   var cl = readyCL( clName );
   cl.insert( { _id: 1 } );

   // test
   // ReturnOID:true,ContOnDup:true
   println( "\n---Begin to insert, options[ReturnOID:true,ContOnDup:true]" );
   var recsArray = [{ _id: 1, c: 1 }, { _id: 2 }];
   var rc = cl.insert( recsArray, { ReturnOID: true, ContOnDup: true } );
   checkReturnOid( rc, [1, 2] );
   var expRecs = [{ "_id": 1 }, { "_id": 2 }];
   checkRecords( cl, expRecs );

   // ReturnOID:true,ReplaceOnDup:true
   println( "\n---Begin to insert, options[ReturnOID:true,ContOnDup:true]" );
   var recsArray = [{ _id: 3 }, { _id: 1, c: 2 }, { _id: 4 }];
   var rc = cl.insert( recsArray, { ReturnOID: true, ReplaceOnDup: true } );
   checkReturnOid( rc, [3, 1, 4] );
   var expRecs = [{ "_id": 1, "c": 2 }, { "_id": 2 }, { "_id": 3 }, { "_id": 4 }];
   checkRecords( cl, expRecs );

   cleanCL( clName );
}

function checkRecords ( cl, recs ) 
{
   var rc = cl.find( {} );
   var rcRecs = new Array();
   while( tmpRecs = rc.next() )
   {
      rcRecs.push( tmpRecs.toObj() );
   }

   var expRecs = JSON.stringify( recs );
   var actRecs = JSON.stringify( rcRecs );
   if( expRecs !== actRecs )
   {
      throw buildException( "checkResult", null, "", expRecs, "  " + actRecs );
   }
}

function checkReturnOid ( rc, expOid )
{
   var actOid = rc.toObj()["_id"];
   for( var i = 0; i < actOid.length; i++ )
   {
      if( expOid[i] !== actOid[i] )
      {
         throw buildException( "checkReturnOid", null, "", expOid[i], "  " + actOid[i] );
      }
   }
}