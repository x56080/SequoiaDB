/************************************************************************
*@Description:  seqDB-1952:$group+$match掉换顺序组合查询_SD.aggregate.01.005
*@Author:  2015/10/10  huangxiaoni
************************************************************************/
main();

function main ()
{
   try
   {
      var clName = COMMCLNAME + "_aggre";

      var cl = readyCL( clName );

      insertRecs( cl );
      var rc1 = aggreOper( cl, { $sort: { no: 1 } }, { $match: { score: 80 } }, { $group: { _id: "$name" } } );
      var rc2 = aggreOper( cl, { $sort: { no: 1 } }, { $group: { _id: "$name" } }, { $match: { score: 80 } } );

      checkResult( rc1, rc2 );

      cleanCL( clName );
   }
   catch( e )
   {
      throw e;
   }
}

function insertRecs ( cl )
{
   println( "\n---Begin to insert records." );

   cl.insert( { no: 2, score: 80, name: "Tom" } );
   cl.insert( { no: 1, score: 80, name: "Json" } );
   cl.insert( { no: 4, score: 80, name: "Tom" } );
   cl.insert( { no: 3, score: 92, name: "Tom" } );

}

function aggreOper ( cl, optObj1, optObj2, optObj3 )
{
   println( "\n---Begin to aggregate records." );

   var rc = cl.aggregate( optObj1, optObj2, optObj3 );

   return rc;
}

function checkResult ( rc1, rc2 )
{
   println( "\n---Begin to check result." );

   //compare the returned records
   var recs1 = rc1.current().toJson();        //return: {no:1,score:80,name:"Json"}
   var no1 = rc1.current().toObj()["no"];   //return: no=1
   var recs2 = rc1.next().toJson();           //return: {no:2,score:80,name:"Tom"}
   var no2 = rc1.current().toObj()["no"];   //return: no=2

   var recs3 = rc2.current().toJson();  //equal recs1
   var recs4 = rc2.next().toJson();	    //equal recs2

   var expRecs1 = { no: 1, score: 80, name: "Json" };
   var expRecs2 = { no: 2, score: 80, name: "Tom" };
   var expNo1 = 1;
   var expNo2 = 2;
   if( no1 !== expNo1 || no2 !== expNo2 || recs1 !== recs3 || recs2 !== recs4 )
   {
      throw buildException( "checkResult", null, "[compare the records]",
         "[no1:" + expNo1 + ",no2:" + expNo2 + ",recs1:" + expRecs1
         + ",recs2:" + expRecs2 + ",recs3:" + expRecs1 + ",recs4:" + expRecs2 + "]",
         "[no1:" + no1 + ",no2:" + no2 + ",recs1:" + recs1 + ",recs2:" + recs2
         + ",recs3:" + recs3 + ",recs4:" + recs4 + "]" );
   }

   //compare the number of records
   var nextRecs1 = rc1.next();
   var nextRecs2 = rc2.next();
   if( nextRecs1 !== undefined || nextRecs2 !== undefined )
   {
      throw buildException( "checkResult", null, "[compare the number of records]",
         "[nextRecs1:undefined,nextRecs2:undefined]",
         "[nextRecs1:" + nextRecs1 + ",nextRecs2:" + nextRecs2 + "]" )
   }
}