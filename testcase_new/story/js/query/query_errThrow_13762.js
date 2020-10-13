/*******************************************************************************
*@Description : [seqDB-13762]when query like: db.foo.bar.find({$a:1}), we should
*               throw error -6 and print string: Invalid Argument
*@Modify List :
*               2014-9-26   xiaojunHu  Init
                2020-08-13 Zixian Yan  Modify
*******************************************************************************/
testConf.clName = COMMCLNAME + "_13762";
main( test );

function test ( testPara )
{
   var cl = testPara.testCL;

   // insert record
   cl.insert( { a: 1 } );
   cl.insert( { b: "testcase" } );
   // query by use db.cs.cl.find({$a:1}).getLastErrMsg() will get the message
   try
   {
      println( cl.find( { $a: 1 } ) );  //Cause Sdb find()- Invalid Argument Error
   }
   catch( errExcuteTest )
   {
      var lastOut = getLastErrMsg();
      if( getErr( -6 ) != lastOut )
      {
         throw new Error( "\nFailed to print correct errorMsg. \nCurrently ErrMsg: " + errExcuteTest );
      }
   }

}
