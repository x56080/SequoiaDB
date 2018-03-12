用于删除集合空间中的集合。

##语法##
***drop collection \<cs_name\>.\<cl_name\>***

##参数###
| 参数名| 参数类型 | 描述 | 是否必填 |
|-------|----------|------|----------|
| cs_name | string | 集合空间名。 | 是 |
| cl_name | string | 集合名。 | 是 |
> **Note:**
>
> * 集合空间必须在数据库中存在。
> * 集合必须在该集合空间中存在。

##返回值##
无。

##示例##

   * 删除集合空间 foo 中的集合 bar。
   
   ```lang-javascript
   //等价于 db.foo.dropCL("bar")
   > db.execUpdate("drop collection foo.bar") 
   Takes 0.4329s.
   ```
