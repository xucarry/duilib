#include "stdafx.h"
#include "WindowWnd.h"
#include <WindowsX.h>

CWindowWnd::CWindowWnd(void)
	: m_bSubclassed(false)
	, m_bIsAutoDelete(false)
	, m_OldWndProc(::DefWindowProc)
	, m_hWnd(NULL)
{
}


CWindowWnd::~CWindowWnd(void)
{
}

HWND CWindowWnd::Create(HWND hwndParent, LPCTSTR lpszWindowName, DWORD dwStyle, DWORD dwExStyle, int x /*= CW_USEDEFAULT*/, int y /*= CW_USEDEFAULT*/, int cx /*= CW_USEDEFAULT*/, int cy /*= CW_USEDEFAULT*/)
{
	if ( RegisterWindowClass())
	{
		m_hWnd = ::CreateWindowEx(dwExStyle, GetWindowClassName(), lpszWindowName, dwStyle, x, y, cx, cy,
			hwndParent, NULL, CUIEngine::GetInstance()->GetInstanceHandler(), this);
		DWORD dwLastError = ::GetLastError();
		ASSERT(m_hWnd!=NULL);
		return m_hWnd;
	}
	return NULL;
}

bool CWindowWnd::RegisterWindowClass()
{
	WNDCLASS wc = { 0 };
	wc.style = GetClassStyle();
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hIcon = NULL;
	wc.lpfnWndProc = CWindowWnd::__WndProc;
	wc.hInstance = CUIEngine::GetInstance()->GetInstanceHandler();
	wc.hCursor = ::LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = NULL;
	wc.lpszMenuName  = NULL;
	wc.lpszClassName = GetWindowClassName();
	ATOM ret = ::RegisterClass(&wc);
	ASSERT(ret!=NULL || ::GetLastError()==ERROR_CLASS_ALREADY_EXISTS);
	return ret != NULL || ::GetLastError() == ERROR_CLASS_ALREADY_EXISTS;
}

LPCTSTR CWindowWnd::GetWindowClassName() const
{
	return _T("WindowWnd");
}

UINT CWindowWnd::GetClassStyle() const
{
	return NULL;
}

LRESULT CWindowWnd::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	return ::CallWindowProc(m_OldWndProc,m_hWnd,uMsg,wParam,lParam);
}

void CWindowWnd::OnFinalMessage(HWND hWnd)
{

}

LRESULT CALLBACK CWindowWnd::__WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	// 窗口默认消息处理函数
	CWindowWnd* pThis = NULL;
	if( uMsg == WM_NCCREATE )
	{
		// 设置窗口实例指针到hWnd窗口上
		LPCREATESTRUCT lpcs = reinterpret_cast<LPCREATESTRUCT>(lParam);
		pThis = static_cast<CWindowWnd*>(lpcs->lpCreateParams);
		pThis->m_hWnd = hWnd;
		::SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LPARAM>(pThis));
	} 
	else
	{
		// 非 WM_NCCREATE ，从hWnd中读取窗口实例指针
		pThis = reinterpret_cast<CWindowWnd*>(::GetWindowLongPtr(hWnd, GWLP_USERDATA));
		if( uMsg == WM_NCDESTROY && pThis != NULL )
		{
			// 如果当前消息是销毁窗口
			LRESULT lRes = ::CallWindowProc(pThis->m_OldWndProc, hWnd, uMsg, wParam, lParam);
			::SetWindowLongPtr(pThis->m_hWnd, GWLP_USERDATA, 0L);

			// 当前窗口是子类化的，恢复原窗口过程
			if( pThis->m_bSubclassed )
				pThis->Unsubclass();
			pThis->m_hWnd = NULL;

			// 窗口销毁完成，用户可以安全销毁窗口实例
			pThis->OnFinalMessage(hWnd);
			if ( pThis->m_bIsAutoDelete )
				delete pThis;
			return lRes;
		}
	}

	if( pThis != NULL )
	{
		// 窗口消息发送到窗口实例中继续处理
		// CWindowWnd::HandleMessage 为虚函数
		// CWindowUI::HandleMessage重写实现了消息处理
		return pThis->HandleMessage(uMsg, wParam, lParam);
	} 
	else
	{
		// 没有找到窗口实例指针，调用默认处理
		return ::DefWindowProc(hWnd, uMsg, wParam, lParam);
	}
	return 0;
}

HWND CWindowWnd::Subclass(HWND hWnd)
{
	ASSERT(::IsWindow(hWnd));
	ASSERT(m_hWnd==NULL);
	m_OldWndProc = SubclassWindow(hWnd, CWindowWnd::__WndProc);
	if( m_OldWndProc == NULL )
	{
		// 子类化失败
		return NULL;
	}

	m_bSubclassed = true;
	m_hWnd = hWnd;
	::SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LPARAM>(this));
	return m_hWnd;
}

void CWindowWnd::Unsubclass()
{
	ASSERT(::IsWindow(m_hWnd));
	if( !::IsWindow(m_hWnd) )
		return;
	if( !m_bSubclassed )
		return;
	SubclassWindow(m_hWnd, m_OldWndProc);
	m_OldWndProc = ::DefWindowProc;
	m_bSubclassed = false;
}

void CWindowWnd::CenterWindow()
{
	ASSERT(::IsWindow(m_hWnd));
	//ASSERT((GetWindowStyle(m_hWnd)&WS_CHILD)==0);
	RECT rcDlg = { 0 };
	::GetWindowRect(m_hWnd, &rcDlg);
	RECT rcArea = { 0 };
	RECT rcCenter = { 0 };
	HWND hWnd=m_hWnd;
	HWND hWndParent = ::GetParent(m_hWnd);
	HWND hWndCenter = ::GetWindowOwner(m_hWnd);
	if (hWndCenter!=NULL)
		hWnd=hWndCenter;

	// 处理多显示器模式下屏幕居中
	MONITORINFO oMonitor = {};
	oMonitor.cbSize = sizeof(oMonitor);
	::GetMonitorInfo(::MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST), &oMonitor);
	rcArea = oMonitor.rcWork;

	if( hWndCenter == NULL )
		rcCenter = rcArea;
	else
		::GetWindowRect(hWndCenter, &rcCenter);

	int DlgWidth = rcDlg.right - rcDlg.left;
	int DlgHeight = rcDlg.bottom - rcDlg.top;

	// Find dialog's upper left based on rcCenter
	int xLeft = (rcCenter.left + rcCenter.right) / 2 - DlgWidth / 2;
	int yTop = (rcCenter.top + rcCenter.bottom) / 2 - DlgHeight / 2;

	// The dialog is outside the screen, move it inside
	if( xLeft < rcArea.left ) xLeft = rcArea.left;
	else if( xLeft + DlgWidth > rcArea.right ) xLeft = rcArea.right - DlgWidth;
	if( yTop < rcArea.top ) yTop = rcArea.top;
	else if( yTop + DlgHeight > rcArea.bottom ) yTop = rcArea.bottom - DlgHeight;
	::SetWindowPos(m_hWnd, NULL, xLeft, yTop, -1, -1, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
}

HWND CWindowWnd::GetHWND() const
{
	return m_hWnd;
}

CWindowWnd::operator HWND() const
{
	return m_hWnd;
}

void CWindowWnd::SetAutoDelete(bool bAutoDelete /*= true*/)
{
	m_bIsAutoDelete = bAutoDelete;
}
