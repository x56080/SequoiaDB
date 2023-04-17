import("../lib/basic_operation/commlib.js");
import("../lib/main.js");

/*******************************************************************************
@Description : 随机生成字符串
@param :
*******************************************************************************/

function generateRandomString() {
  var minLength = 0;
  var maxLength = 128 * 1024;
  var range = maxLength - minLength;
  const strSet =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789!@#$%^&*()_+{}[]|;:,.<>?中文中文";
  var result = "";
  var randomLen = Math.floor(Math.random() * range);
  for (var i = 0; i < randomLen - 1; i++) {
    result += strSet.charAt(Math.floor(Math.random() * strSet.length));
  }
  return result;
}
