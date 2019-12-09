/************************************************************************
*@Description:  seqDB-1956:$sort指定多个字段排序,且field1有多个相同的字段值_SD.aggregate.01.009
*@Author:  2015/11/3  huangxiaoni
************************************************************************/
main();

function main ()
{
   try
   {
      var clName = COMMCLNAME + "_aggre";

      var cl = readyCL( clName );

      insertRecs( cl );
      var rc = aggreOper( cl );
      checkResult( rc );

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

   cl.insert( { no: 2, score: 60, name: "Tom", age: 12 } );
   cl.insert( { no: 1, score: 70, name: "Json", age: 13 } );
}

function aggreOper ( cl )
{
   println( "\n---Begin to aggregate records." );

   var rc = cl.aggregate( { $sort: { no: 1 } }, { $sort: { no: -1 } } );

   return rc;
}

function checkResult ( rc )
{
   println( "\n---Begin to check result." );

   //compare the returned records
   var no1 = rc.current().toObj()["no"];
   var no2 = rc.next().toObj()["no"];
   //expect results:{no1:2,no2:1}
   var expNo1 = 2;
   var expNo2 = 1;
   if( no1 != expNo1 || no2 != expNo2 )
   {
      throw buildException( "checkResult", null, "[compare the records]",
         "[no1:" + expNo1 + ",no2:" + expNo2 + "]",
         "[no1:" + no1 + ",no2:" + no2 + "]" );
   }

   //compare the number of records
   var nextRecs = rc.next();
   if( nextRecs !== undefined )
   {
      throw buildException( "checkResult", null, "[compare the number of records]",
         "[nextRecs:undefined]", "[nextRecs:" + nextRecs + "]" )
   }
}