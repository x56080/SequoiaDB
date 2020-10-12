/*******************************************************************************
@Description : 比较查询返回的结果（游标）与预期结果( 数组 )是否一致
@Modify list : 2018-10-15 zhaoyu init
*******************************************************************************/
import( "../lib/basic_operation/commlib.js" );
import( "../lib/main.js" );

function checkRec ( rc, expRecs )
{
   //get actual records to array
   var actRecs = [];
   while( rc.next() )
   {
      actRecs.push( rc.current().toObj() );
   }

   //check count
   assert.equal( actRecs.length, expRecs.length );

   //check every records every fields
   for( var i in expRecs )
   {
      var actRec = actRecs[i];
      var expRec = expRecs[i];
      for( var f in expRec )
      {
         assert.equal( actRec[f], expRec[f] );
      }
   }

   //check every records every fields,actRecs as compare source
   for( var i in actRecs )
   {
      var actRec = actRecs[i];
      var expRec = expRecs[i];
      for( var f in actRec )
      {
         if( f == "_id" )
         {
            continue;
         }
         assert.equal( actRec[f], expRec[f] );
      }
   }
}


/************************************
*@Description: insert data
*@author:      wuyan
*@createDate:  2018.1.22
**************************************/
function insertData ( dbcl, number )
{
   if( undefined == number ) { number = 1000; }
   var docs = [];
   for( var i = 0; i < number; ++i )
   {
      var no = i;
      var a = i;
      var user = "test" + i;
      var phone = 13700000000 + i;
      var time = new Date().getTime();
      var doc = { no: no, a: a, customerName: user, phone: phone, openDate: time };
      //data example: {"no":5, customerName:"test5", "phone":13700000005, "openDate":1402990912105

      docs.push( doc );
   }
   dbcl.insert( docs );
}

/************************************
*@Description: 校验记录数
*@author:      luweikang
*@createDate:  2018.10.13
**************************************/
function checkCount ( dbcl, expRecordNums, options )
{
   if( options == undefined )
   {
      options = null;
   }
   var count = dbcl.count( options );
   assert.equal( count, expRecordNums );
}