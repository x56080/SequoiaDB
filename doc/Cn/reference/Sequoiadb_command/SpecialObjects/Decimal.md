##Json 格式##

***{ "$decimal" : &lt; 使用字符串指定数值 &gt; }***

***{ "$decimal" : < 使用字符串指定数值 > , "$precision" : [最大精度, 小数位精度] }***

##函数格式##

无

**Note:**

NumberDecimal 最大精度、小数位精度使用整数类型的参数。

##示例##

-   插入一个decimal类型

<pre class="prettyprint lang-javascript">
> db.foo.bar.insert({number:{$decimal:"100.01"}})
> db.foo.bar.insert({number:{$decimal:"100.01", $precision:[10,2]}})</pre>