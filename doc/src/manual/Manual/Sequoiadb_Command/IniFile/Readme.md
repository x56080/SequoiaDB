IniFile 类主要用于操作 INI 文件对象，包含的函数如下：

| 名称 | 描述 |
|------|------|
| IniFile() | 新建一个 INI 文件对象 |
| addComment() | 为指定的 item 追加注释 |
| addSectionComment() | 为指定的 section 追加注释 |
| addLastComment() | 在尾部追加注释 |
| setComment() | 设置指定 item 的注释 |
| setSectionComment() | 设置指定 section 的注释 |
| setLastComment() | 设置尾部的注释 |
| delComment() | 删除指定 item 的注释 |
| delSectionComment() | 删除指定 section 的注释 |
| delLastComment() | 删除尾部注释 |
| enableItem() | 取消注释指定的 item |
| disableItem() | 注释指定的 item |
| disableAllItem() | 注释全部 item |
| getComment() | 获取指定 item 的注释 |
| getSectionComment() | 获取指定 section 的注释 |
| getLastComment() | 获取尾部注释 |
| getValue() | 获取指定 item 的值 |
| setValue() | 设置指定 item 的值 |
| toObj() | 转换成对象 |
| toString() | 转换成文本 |
| save() | 保存 INI 配置 |