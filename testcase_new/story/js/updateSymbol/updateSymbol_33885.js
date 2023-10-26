/******************************************************************************
 * @Description   : seqDB-33885:$inc,$mul更新字段值为空对象
 * @Author        : chenzejia
 * @CreateTime    : 2023.10.26
 * @LastEditTime  : 
 * @LastEditors   : 
 ******************************************************************************/
testConf.clName = COMMCLNAME + "33885";
main( test );

function test ( args )
{
   var cl = args.testCL;
   cl.insert( { name: "str", age: 18 } );
   expLog = { "UpdatedNum": 1, "ModifiedNum": 0, "InsertedNum": 0 };
   expResult = [{ name: "str", age: 18 }];

   // $inc
   result = cl.update( { $inc: { age: {} } } )
   actResult = cl.find();
   commCompareObject( expLog, result );
   commCompareResults( actResult, expResult );
   result = cl.upsert( { $inc: { age: {} } } )
   actResult = cl.find();
   commCompareObject( expLog, result );
   commCompareResults( actResult, expResult );

   // $mul
   result = cl.update( { $mul: { age: {} } } )
   actResult = cl.find();
   commCompareObject( expLog, result );
   commCompareResults( actResult, expResult );
   result = cl.upsert( { $mul: { age: {} } } )
   actResult = cl.find();
   commCompareObject( expLog, result );
   commCompareResults( actResult, expResult );
}
