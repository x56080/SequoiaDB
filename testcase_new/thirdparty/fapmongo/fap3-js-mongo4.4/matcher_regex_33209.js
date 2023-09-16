/******************************************************************************
 * @Description   : seqDB-33209:匹配符 $regex
 * @Author        : XiaoNi Huang
 * @CreateTime    : 2023.08.16
 * @LastEditTime  : 2023.08.16
 * @LastEditors   : XiaoNi Huang
 ******************************************************************************/

main();
function main ()
{
   var clName = "cl_33209";
   var cl = db.getCollection( clName );
   cl.drop();
   cl.insert( [
      { "_id": 1, "a": "app" },
      { "_id": 2, "a": "napp2" },
      { "_id": 3, "a": "nApp3" },
      { "_id": 4, "a": "application" },
      { "_id": 5, "a": "Appliance" },
      { "_id": 6, "a": "boxa" },
      { "_id": 7, "a": "boxA" },
      { "_id": 8, "a": "choclate" },
      { "_id": 9, "a": "use openai is good." },
      { "_id": 10, "a": "use OpenAI is good." }
   ] );

   // 基本功能测试，覆盖 $options:i/m/x/s
   // $options:m
   var rc = cl.find( { "a": { "$regex": "app", "$options": "m" } } );
   checkResults( rc, "[{\"_id\":1,\"a\":\"app\"},{\"_id\":2,\"a\":\"napp2\"},{\"_id\":4,\"a\":\"application\"}]" );
   // $options:x
   var rc = cl.find( { "a": { "$regex": "Appli", "$options": "x" } } );
   checkResults( rc, "[{\"_id\":5,\"a\":\"Appliance\"}]" );
   // $options:i
   var rc = cl.find( { "a": { "$regex": "openai", "$options": "i" } } );
   checkResults( rc, "[{\"_id\":9,\"a\":\"use openai is good.\"},{\"_id\":10,\"a\":\"use OpenAI is good.\"}]" );
   // $options:s
   var rc = cl.find( { "a": { "$regex": "openai", "$options": "s" } } );
   checkResults( rc, "[{\"_id\":9,\"a\":\"use openai is good.\"}]" );


   // 常用通配符查找
   // ^, 如查找以字母 "A" 开头的文档
   var rc = cl.find( { "a": { "$regex": "^A" } } );
   checkResults( rc, "[{\"_id\":5,\"a\":\"Appliance\"}]" );
   // $, 如查找以字母 "A" 结尾的文档
   var rc = cl.find( { "a": { "$regex": "A$" } } );
   checkResults( rc, "[{\"_id\":7,\"a\":\"boxA\"}]" );
   // |, 如查找包含 "app" 或 "box" 的文档
   var rc = cl.find( { "a": { "$regex": "app|box" } } );
   checkResults( rc, "[{\"_id\":1,\"a\":\"app\"},{\"_id\":2,\"a\":\"napp2\"},{\"_id\":4,\"a\":\"application\"},{\"_id\":6,\"a\":\"boxa\"},{\"_id\":7,\"a\":\"boxA\"}]" );
   // [a-z], 如查找包含任何小写字母的文档
   var rc = cl.find( { "a": { "$regex": "[a-z]" } } );
   checkResults( rc, "[{\"_id\":1,\"a\":\"app\"},{\"_id\":2,\"a\":\"napp2\"},{\"_id\":3,\"a\":\"nApp3\"},{\"_id\":4,\"a\":\"application\"},{\"_id\":5,\"a\":\"Appliance\"},{\"_id\":6,\"a\":\"boxa\"},{\"_id\":7,\"a\":\"boxA\"},{\"_id\":8,\"a\":\"choclate\"},{\"_id\":9,\"a\":\"use openai is good.\"},{\"_id\":10,\"a\":\"use OpenAI is good.\"}]" );
   // .*, 如查找包含 "a.*p" 这种模式的文档，其中 .* 表示零个或多个字符
   var rc = cl.find( { "a": { "$regex": "a.*p" } } );
   checkResults( rc, "[{\"_id\":1,\"a\":\"app\"},{\"_id\":2,\"a\":\"napp2\"},{\"_id\":4,\"a\":\"application\"}]" );
   // ?!, 查找不包含字母组合 "xyz" 的文档，并区分大小写
   var rc = cl.find( { "a": { "$regex": "^((?!app).)*$", "$options": "i" } } );
   checkResults( rc, "[{\"_id\":6,\"a\":\"boxa\"},{\"_id\":7,\"a\":\"boxA\"},{\"_id\":8,\"a\":\"choclate\"},{\"_id\":9,\"a\":\"use openai is good.\"},{\"_id\":10,\"a\":\"use OpenAI is good.\"}]" );


   // $regex跟其他匹配符组合使用
   // $regex + $in
   var rc = cl.find( { "a": { "$regex": "a.*p", "$in": ["app", "napp2"] } } );
   checkResults( rc, "[{\"_id\":1,\"a\":\"app\"},{\"_id\":2,\"a\":\"napp2\"}]" );
   // $not + $regex, sdb not support $not
   // var rc = cl.find( { "a": { "$not": { "$regex": "a.*p", "$options": "i" } } } );
   // checkResults( rc, "[{\"_id\":6,\"a\":\"boxa\"},{\"_id\":7,\"a\":\"boxA\"},{\"_id\":8,\"a\":\"choclate\"},{\"_id\":9,\"a\":\"use openai is good.\"},{\"_id\":10,\"a\":\"use OpenAI is good.\"}]" );
   // $and + $regex
   var rc = cl.find( { "$and": [{ "a": { "$regex": "a.*p" } }, { "a": { "$regex": "app" } }] } );
   checkResults( rc, "[{\"_id\":1,\"a\":\"app\"},{\"_id\":2,\"a\":\"napp2\"},{\"_id\":4,\"a\":\"application\"}]" );

   // $options多个选项组合使用
   var rc = cl.find( { "a": { "$regex": "a.*p", "$options": "sx" } } );
   checkResults( rc, "[{\"_id\":1,\"a\":\"app\"},{\"_id\":2,\"a\":\"napp2\"},{\"_id\":4,\"a\":\"application\"}]" );
   var rc = cl.find( { "a": { "$regex": "a.*p", "$options": "si" } } );
   checkResults( rc, "[{\"_id\":1,\"a\":\"app\"},{\"_id\":2,\"a\":\"napp2\"},{\"_id\":3,\"a\":\"nApp3\"},{\"_id\":4,\"a\":\"application\"},{\"_id\":5,\"a\":\"Appliance\"}]" );


   // mongo官网示例
   cl.drop();
   cl.insert( [
      { "_id": 1, "sku": "abc123", "description": "Single line description." },
      { "_id": 2, "sku": "abc789", "description": "First line\nSecond line" },
      { "_id": 3, "sku": "xyz456", "description": "Many spaces before     line" },
      { "_id": 4, "sku": "xyz789", "description": "Multiple\nline description" },
      { "_id": 5, "sku": "Abc789", "description": "SKU starts with A" },
   ] );
   // 测试 / / 输入格式
   // 查找以 "789" 结尾的文档
   var rc = cl.find( { "sku": { "$regex": /789$/ } } );
   checkResults( rc, "[{\"_id\":2,\"sku\":\"abc789\",\"description\":\"First line\\nSecond line\"},{\"_id\":4,\"sku\":\"xyz789\",\"description\":\"Multiple\\nline description\"},{\"_id\":5,\"sku\":\"Abc789\",\"description\":\"SKU starts with A\"}]" );
   // 查找以 "789" 开头的文档，不区分大小写
   var rc = cl.find( { "sku": { "$regex": /^ABC/i } } );
   checkResults( rc, "[{\"_id\":1,\"sku\":\"abc123\",\"description\":\"Single line description.\"},{\"_id\":2,\"sku\":\"abc789\",\"description\":\"First line\\nSecond line\"},{\"_id\":5,\"sku\":\"Abc789\",\"description\":\"SKU starts with A\"}]" );
   // 查找 /S/ 的文档
   var rc = cl.find( { "description": { "$regex": /^S/ } } );
   checkResults( rc, "[{\"_id\":1,\"sku\":\"abc123\",\"description\":\"Single line description.\"},{\"_id\":5,\"sku\":\"Abc789\",\"description\":\"SKU starts with A\"}]" );
   // 查找 /S/ 的文档，多行模式
   var rc = cl.find( { "description": { "$regex": /^S/m } } );
   checkResults( rc, "[{\"_id\":1,\"sku\":\"abc123\",\"description\":\"Single line description.\"},{\"_id\":2,\"sku\":\"abc789\",\"description\":\"First line\\nSecond line\"},{\"_id\":5,\"sku\":\"Abc789\",\"description\":\"SKU starts with A\"}]" );
   // 查找 /S/ 的文档
   var rc = cl.find( { "description": { "$regex": /S/ } } );
   checkResults( rc, "[{\"_id\":1,\"sku\":\"abc123\",\"description\":\"Single line description.\"},{\"_id\":2,\"sku\":\"abc789\",\"description\":\"First line\\nSecond line\"},{\"_id\":5,\"sku\":\"Abc789\",\"description\":\"SKU starts with A\"}]" );

   // 查找 /m.*line/ 的文档, 匹配字符串包括空格和换行符
   // $options:i
   var rc = cl.find( { "description": { "$regex": /m.*line/, "$options": "i" } } );
   checkResults( rc, "[{\"_id\":3,\"sku\":\"xyz456\",\"description\":\"Many spaces before     line\"}]" );
   // $options:s
   var rc = cl.find( { "description": { "$regex": /m.*line/, "$options": "s" } } );
   checkResults( rc, "[]" );
   // 查找 /m.*line/ 的文档, $options:si，匹配字符串包括空格和换行符
   var rc = cl.find( { "description": { "$regex": /m.*line/, "$options": "si" } } );
   checkResults( rc, "[{\"_id\":3,\"sku\":\"xyz456\",\"description\":\"Many spaces before     line\"},{\"_id\":4,\"sku\":\"xyz789\",\"description\":\"Multiple\\nline description\"}]" );

   // 匹配条件包含换行符的文档
   var pattern = "abc #category code\n123 #item number";
   var rc = cl.find( { "sku": { "$regex": pattern, "$options": "x" } } );
   checkResults( rc, "[{\"_id\":1,\"sku\":\"abc123\",\"description\":\"Single line description.\"}]" );

   // 通配符匹配大小写adb/Abc的文档
   var rc = cl.find( { "sku": { "$regex": "(?i)a(?-i)bc" } } );
   checkResults( rc, "[{\"_id\":1,\"sku\":\"abc123\",\"description\":\"Single line description.\"},{\"_id\":2,\"sku\":\"abc789\",\"description\":\"First line\\nSecond line\"},{\"_id\":5,\"sku\":\"Abc789\",\"description\":\"SKU starts with A\"}]" );


   // 不带 $regex 字段正则匹配   
   var rc = cl.find( { "description": /m.*line/i } );
   checkResults( rc, "[{\"_id\":3,\"sku\":\"xyz456\",\"description\":\"Many spaces before     line\"}]" );


   cl.drop();
}

function checkResults ( cursor, expDocs )
{
   var docs = new Array();
   while( cursor.hasNext() )
   {
      var doc = cursor.next();
      docs.push( doc );
   }
   cursor.close();
   assert.eq( JSON.stringify( docs ), expDocs );
}