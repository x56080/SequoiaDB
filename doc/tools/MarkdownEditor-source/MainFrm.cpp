
// MainFrm.cpp : CMainFrame 类的实现
//

#include "stdafx.h"
#include "MarkdownEditor.h"

#include "MainFrm.h"
#include "LeftView.h"
#include "MarkdownEditorView.h"
#include "MarkdownEditorDoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CMainFrame

IMPLEMENT_DYNCREATE(CMainFrame, CFrameWnd)

	BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)
		ON_WM_CREATE()
		ON_WM_SIZE()
		ON_COMMAND( IDM_SWITCH, &CMainFrame::OnSwitch)
		ON_COMMAND( IDM_ABOUT, &CMainFrame::OnAbout)
      ON_COMMAND( IDM_EXAMPLE, &CMainFrame::OnExample)
      ON_COMMAND( ID_NORMAL, &CMainFrame::OnNormal )
      ON_COMMAND( ID_WWW, &CMainFrame::OnWww )
      ON_UPDATE_COMMAND_UI( ID_NORMAL, &CMainFrame::OnUpdateNormal )
      ON_UPDATE_COMMAND_UI( ID_WWW, &CMainFrame::OnUpdateWww )
   END_MESSAGE_MAP()

	static UINT indicators[] =
	{
		ID_SEPARATOR,           // 状态行指示器
		ID_INDICATOR_CAPS,
		ID_INDICATOR_NUM,
		ID_INDICATOR_SCRL,
	};

	// CMainFrame 构造/析构

	CMainFrame::CMainFrame()
	{
		_bInited = false;
		_bShowLeft = true;
      _type = 1 ;
		// TODO: 在此添加成员初始化代码
	}

	CMainFrame::~CMainFrame()
	{
	}

	int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
	{
		if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
			return -1;

		this->MoveWindow(0,0,800,600);
		this->CenterWindow();

		if (!m_wndStatusBar.Create(this))
		{
			TRACE0("未能创建状态栏\n");
			return -1;      // 未能创建
		}
		m_wndStatusBar.SetIndicators(indicators, sizeof(indicators)/sizeof(UINT));



		return 0;
	}

	BOOL CMainFrame::OnCreateClient(LPCREATESTRUCT /*lpcs*/,
		CCreateContext* pContext)
	{
		// 创建拆分窗口
		if (!m_wndSplitter.CreateStatic(this, 1, 2))
			return FALSE;

		CRect rect;
		this->GetWindowRect(&rect);
		CSize size(rect.Width() /2 , rect.Height() /2);

		if (!m_wndSplitter.CreateView(0, 0, RUNTIME_CLASS(CLeftView), size, pContext) ||
			!m_wndSplitter.CreateView(0, 1, RUNTIME_CLASS(CMarkdownEditorView), size, pContext))
		{
			m_wndSplitter.DestroyWindow();
			return FALSE;
		}

		_bInited = true;

		return TRUE;
	}

	BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
	{
		if( !CFrameWnd::PreCreateWindow(cs) )
			return FALSE;
		// TODO: 在此处通过修改
		//  CREATESTRUCT cs 来修改窗口类或样式

		return TRUE;
	}

	// CMainFrame 诊断

#ifdef _DEBUG
	void CMainFrame::AssertValid() const
	{
		CFrameWnd::AssertValid();
	}

	void CMainFrame::Dump(CDumpContext& dc) const
	{
		CFrameWnd::Dump(dc);
	}
#endif //_DEBUG


	// CMainFrame 消息处理程序


	void CMainFrame::OnSize(UINT nType, int cx, int cy)
	{
		CFrameWnd::OnSize(nType, cx, cy);
		if(!_bInited)
			return;
		if(cx == 0 || cy == 0)
			return;
		int cxCur, cxMin;
		m_wndSplitter.GetColumnInfo(0, cxCur, cxMin); 
		if(cxCur <= 0)
			return;
		m_wndSplitter.SetColumnInfo(0,cx/2,10);
		m_wndSplitter.RecalcLayout();
	}


	void CMainFrame::OnSwitch(){
		_bShowLeft = !_bShowLeft;
		m_wndSplitter.ShowLeft(_bShowLeft);

	}


	const string STR_ABOUT = "#MarkdownEditor 1.0\nProject: <https://github.com/jijinggang/MarkdownEditor>\n##Author\njijinggang@gmail.com\n##Copyright\nFree For All";
	//注意，此相应函数必须放在MainFrame中，如果放在MarkdownEditorView中，如果MarkdownEditorView失去焦点，则菜单不能点
	void CMainFrame::OnAbout()
	{
		static bool s_bShowAbout = false;
		CMarkdownEditorView* pView = dynamic_cast<CMarkdownEditorView*>(m_wndSplitter.GetPane(0,1));
		if(pView == NULL)
			return;
		if(!s_bShowAbout)
			pView->UpdateMd(STR_ABOUT);
		else{
			CLeftView* pLeft = dynamic_cast<CLeftView*>(m_wndSplitter.GetPane(0,0));
			pView->GetDocument()->UpdateAllViews(pLeft, LPARAM_Update);
		}
		s_bShowAbout = !s_bShowAbout;
	}

   CString STR_EXAMPLE = "\
# 1. 标题\r\n\
\r\n\
# 这是 h1\r\n\
\r\n\
## 这是 h2\r\n\
\r\n\
这也是 h2\r\n\
---\r\n\
\r\n\
### 这是 h3\r\n\
\r\n\
###### 这是 h6\r\n\
\r\n\
> **Note:**  \r\n\
> 在官网模式，会根据 H2 生成本页导航\r\n\
\r\n\
--\r\n\
\r\n\
# 2. 区块引用\r\n\
\r\n\
> This is a blockquote with two paragraphs. Lorem ipsum dolor sit amet,  \r\n\
> consectetuer adipiscing elit. Aliquam hendrerit mi posuere lectus.  \r\n\
> Vestibulum enim wisi, viverra nec, fringilla in, laoreet vitae, risus.\r\n\
> \r\n\
> Donec sit amet nisl. Aliquam semper ipsum sit amet velit. Suspendisse  \r\n\
> id sem consectetuer libero luctus adipiscing.\r\n\
\r\n\
**区块引用内部同样支持双空格强制换行！**\r\n\
\r\n\
--\r\n\
\r\n\
# 3. 换行\r\n\
\r\n\
**换行有两种形式，第一种是普通换行，第二种是隔行**\r\n\
\r\n\
**换行：在后面加两个空格就可以，如：**\r\n\
\r\n\
床前明月光  \r\n\
疑是地上霜  \r\n\
举头望明月  \r\n\
低头思故乡\r\n\
\r\n\
**隔行：两个回车，如：**\r\n\
\r\n\
床前明月光\r\n\
\r\n\
疑是地上霜\r\n\
\r\n\
举头望明月\r\n\
\r\n\
低头思故乡\r\n\
\r\n\
--\r\n\
\r\n\
# 4. 列表\r\n\
\r\n\
**无序列表**\r\n\
\r\n\
- Red\r\n\
- Green\r\n\
- Blue\r\n\
\r\n\
**有序列表**\r\n\
\r\n\
1. Bird\r\n\
2. McHale\r\n\
3. Parish\r\n\
\r\n\
**嵌入列表**\r\n\
\r\n\
- item 1\r\n\
- item 2\r\n\
  - item 2_a\r\n\
  - item 2_b\r\n\
- item 3\r\n\
  - item 3_a\r\n\
  - item 3_b\r\n\
  - item 3_c\r\n\
\r\n\
**分隔比较大的列表**\r\n\
\r\n\
1. A\r\n\
\r\n\
2. B\r\n\
\r\n\
3. C\r\n\
\r\n\
\r\n\
--\r\n\
\r\n\
\r\n\
# 5. 注释\r\n\
\r\n\
[^_^]: 单行注释\r\n\
\r\n\
[^_^]: 多行注释\r\n\
   你看不到我 1\r\n\
   你看不到我 2\r\n\
   你看不到我 3\r\n\
\r\n\
--\r\n\
\r\n\
# 6. 图片\r\n\
\r\n\
**图片支持两种语法，两种语法前面都有方括号，里面写的是图片的标注，  \r\n\
它规定在图像无法显示时的替代文本，同时也有助于搜索引擎的检索。**\r\n\
\r\n\
第一种：\r\n\
\r\n\
![这是logo](logo.png)\r\n\
\r\n\
第二种（推荐）：\r\n\
\r\n\
![这是logo][logo_img]\r\n\
![这是logo][logo_img]\r\n\
\r\n\
\r\n\
[logo_img]: logo.png\r\n\
\r\n\
--\r\n\
\r\n\
# 7. 链接\r\n\
\r\n\
**链接支持两种语法，两种语法前面都有方括号，里面写的是链接的文本。**\r\n\
\r\n\
第一种：\r\n\
\r\n\
[百度](http://www.baidu.com)\r\n\
\r\n\
第二种（推荐）：\r\n\
\r\n\
[百度][baidu_link]\r\n\
\r\n\
[baidu][baidu_link]\r\n\
\r\n\
\r\n\
[baidu_link]: http://www.baidu.com\r\n\
\r\n\
\r\n\
--\r\n\
\r\n\
# 8. 突出显示\r\n\
\r\n\
**加粗1**\r\n\
__加粗2__\r\n\
\r\n\
*倾斜1*\r\n\
_倾斜2_\r\n\
\r\n\
_你 **可以** 混合使用_\r\n\
\r\n\
\r\n\
# 9. 代码\r\n\
\r\n\
官网样式，需要设置【菜单】-【模式】-【官网模式】\r\n\
\r\n\
```shell\r\n\
cd /opt\r\n\
mkdir sequoiadb\r\n\
```\r\n\
\r\n\
# 10. 高亮\r\n\
\r\n\
事实上这是单行代码，一般用在代码、路径或者比较重要的特殊内容，依旧需要与中文之间前后保持一个空格。如果后接标点符号则不需要空格\r\n\
\r\n\
你好 `this is demo` 结束\r\n\
\r\n\
# 11. 表格\r\n\
\r\n\
| First Header | Second Header |\r\n\
| ------------ | ------------- |\r\n\
| Content from cell 1 | Content from cell 2 |\r\n\
| Content in the first column | Content in the second column |\r\n\
| 我要<br/>换行 | 我不换行 |\r\n\
\r\n\
# 12. 删除线\r\n\
\r\n\
~~123456~~\r\n\
\r\n\
\r\n\
# 13. 转义字符\r\n\
\r\n\
Markdown 支持以下这些符号前面加上反斜杠来帮助插入普通的符号：\r\n\
\r\n\
| 符号 | 注释 |\r\n\
| ---- | ---- |\r\n\
| \\\\   | 反斜线 |\r\n\
| \\`   | 反引号 |\r\n\
| \\*   | 星号 |\r\n\
| \\_   | 底线 |\r\n\
| \\{ \\} | 花括号 |\r\n\
| \\[ \\] | 方括号 |\r\n\
| \\( \\) | 括弧 |\r\n\
| \\#   | 井字号 |\r\n\
| \\+   | 加号 |\r\n\
| \\-   | 减号 |\r\n\
| \\.   | 英文句点 |\r\n\
| \\!   | 惊叹号 |\r\n\
| \\^   | 托字符 |\r\n\
| \\|   | 竖线 |\r\n\
| \\<\> | 尖括号 |\r\n\
\r\n\
# 14. 分页\r\n\
\r\n\
**我们利用注释开发的语法，通过模式切换【普通模式】和【官网模式】查看效果  \r\n\
【普通模式】是列表，【官网模式】是分页**\r\n\
\r\n\
**列表支持有序列表和无序列表**\r\n\
\r\n\
```\r\n\
[^_^]:tab\r\n\
1. [Title1]:\r\n\
\r\n\
   Content1\r\n\
\r\n\
2. [Title2]:\r\n\
\r\n\
   Content2\r\n\
```\r\n\
\r\n\
--\r\n\
\r\n\
[^_^]:tab\r\n\
1. Linux:\r\n\
\r\n\
   Linux是一套免费使用和自由传播的类Unix操作系统\r\n\
\r\n\
   ```\r\n\
   还能嵌入代码\r\n\
   123\r\n\
   456\r\n\
   ```\r\n\
\r\n\
2. Windows:\r\n\
\r\n\
   Microsoft Windows操作系统是美国微软公司研发的一套操作系统\r\n\
\r\n\
   ```\r\n\
   还能嵌入代码\r\n\
   789\r\n\
   abc\r\n\
   ```\r\n\
\r\n\
" ;
   void CMainFrame::OnExample()
   {
      static bool s_bShowExample = false;
      static CString str ;
		CMarkdownEditorView* pView = dynamic_cast<CMarkdownEditorView*>(m_wndSplitter.GetPane(0,1));
      CLeftView* pLeft = dynamic_cast<CLeftView*>( m_wndSplitter.GetPane( 0, 0 ) );

		if(pView == NULL)
			return;
      if(pLeft == NULL)
			return;

      if( !s_bShowExample )
      {
         pLeft->GetWindowText( str ) ;
         pLeft->SetWindowText( STR_EXAMPLE ) ;
         pView->UpdateMd( STR_EXAMPLE.GetBuffer(0) );
      }
		else
      {
         pLeft->SetWindowText( str ) ;
			CLeftView* pLeft = dynamic_cast<CLeftView*>(m_wndSplitter.GetPane(0,0));
			pView->GetDocument()->UpdateAllViews(pLeft, LPARAM_Update);
		}
		s_bShowExample = !s_bShowExample;
   }


   void CMainFrame::OnNormal()
   {
      _type = 1 ;
      CString str ;
      CMarkdownEditorView* pView = dynamic_cast<CMarkdownEditorView*>(m_wndSplitter.GetPane(0,1));
      CLeftView* pLeft = dynamic_cast<CLeftView*>( m_wndSplitter.GetPane( 0, 0 ) );
      pView->setType( _type ) ;
      pView->GetDocument()->UpdateAllViews( pLeft, LPARAM_Update );
   }

   void CMainFrame::OnWww()
   {
      _type = 2 ;
      CString str ;
      CMarkdownEditorView* pView = dynamic_cast<CMarkdownEditorView*>(m_wndSplitter.GetPane(0,1));
      CLeftView* pLeft = dynamic_cast<CLeftView*>( m_wndSplitter.GetPane( 0, 0 ) );
      pView->setType( _type ) ;
      pView->GetDocument()->UpdateAllViews( pLeft, LPARAM_Update );
   }


   void CMainFrame::OnUpdateNormal( CCmdUI *pCmdUI )
   {
      pCmdUI->SetCheck(_type == 1);
   }


   void CMainFrame::OnUpdateWww( CCmdUI *pCmdUI )
   {
      pCmdUI->SetCheck(_type == 2);
   }
