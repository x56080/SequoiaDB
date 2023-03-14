/******************************************************************************
 * @Description   : seqDB-30346:内部模式字段总长度累计超过64kb
 * @Author        : liuli
 * @CreateTime    : 2023.03.06
 * @LastEditTime  : 2023.03.06
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.schemaName = COMMSCHEMANAME + "_30346";
testConf.schemaDef = { "a": { Type: "int32" } };
testConf.clName = COMMCLNAME + "_30346";
testConf.clOpt = { EnableInfoSchema: true };

main( test );
function test ( testPara )
{
   var dbcl = testPara.testCL;
   var schema = testPara.testSchema;

   // 集合绑定外部模式
   dbcl.addSchema( testConf.schemaName );

   // 插入数据包含外部模式字段
   var docs = [];
   for( var i = 0; i < 100; i++ )
   {
      docs.push( { a: i } );
   }
   dbcl.insert( docs );

   // 随机生成64kb字段名称，每个字段长8字节
   var fields = [];
   var suffix = 1000;
   for( var i = 0; i < 64 * 1024 / 8; i++ )
   {
      var str = '';
      for( var j = 0; j < 4; j++ )
      {
         str += String.fromCharCode( ( Math.floor( Math.random() * 94 ) + 33 ) );
      }
      str += suffix;
      suffix++;
      fields.push( str );
   }

   // 外部模式增加大量字段
   for( var i in fields )
   {
      schema.addColumn( fields[i], { Type: "int32", ReadDefault: 100 } );
   }

   // 外部模式删除字段
   schema.dropColumn( fields[0] );

   // 外部模式字段重命名
   schema.renameCloumn( fields[1], "field1" );

   // 插入数据使内部模式字段超过最大长度限制
   dbcl.insert( { field1: 1, field2: 1, field3: 1, field4: 1, field5: 1, field6: 1, field7: 1 } );
}