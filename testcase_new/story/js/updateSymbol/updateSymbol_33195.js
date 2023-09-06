/******************************************************************************
 * @Description   : seqDB-33195:$inc字段不存在时，使用扩展语法和字段操作符指定非数值字段
 * @Author        : chenzejia
 * @CreateTime    : 2023.09.06
 * @LastEditTime  : 
 * @LastEditors   : 
 ******************************************************************************/
testConf.clName = COMMCLNAME + "33195";
main( test );

function test ( args )
{
   var cl = args.testCL;
   cl.insert( { a: 1, b: "str" } );

   // default is null
   cl.update( { $inc: { c: { Value: { $field: "b" }, Default: null } } } )
   actResult = cl.find();
   expResult = [{ a: 1, b: "str" }];
   commCompareResults( actResult, expResult );

   // miss default
   cl.update( { $inc: { c: { Value: { $field: "b" } } } } )
   var actResult = cl.find();
   var expResult = [{ a: 1, b: "str", c: 0 }];
   commCompareResults( actResult, expResult );

   // default is number
   cl.update( { $inc: { d: { Value: { $field: "b" }, Default: 1 } } } )
   var actResult = cl.find();
   var expResult = [{ a: 1, b: "str", c: 0, d: 1 }];
   commCompareResults( actResult, expResult );
}
