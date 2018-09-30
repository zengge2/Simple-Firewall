#ifndef __IP_MONITER_H__
#define __IP_MONITER_H__

#define REG_INSTALL_KEY				_T("SYSTEM\\CurrentControlSet\\Services\\WinSock2\\MiniSPI")
#define REG_INSTALL_PATH_ITEM		_T("PathName")
#define	REG_PROTOCOL_CATALOG_KEY	_T("SYSTEM\\CurrentControlSet\\Services\\WinSock2\\Parameters\\Protocol_Catalog9\\Catalog_Entries")
#define REG_PROTOCOL_CATALOG_ITEM	_T("PackedCatalogItem")

#define MAX_PROTOCOL_CATALOG_LENTH		sizeof(WSAPROTOCOL_INFOW) + MAX_PATH

#define	XERR_SUCCESS						0
#define XERR_PROVIDER_NOT_INSTALL			-801
#define XERR_PROVIDER_ALREADY_INSTALL		-802
#define XERR_PROVIDER_OPEN_REG_FAILED		-803
#define XERR_PROVIDER_SAVE_PATH_FAILED		-804
#define XERR_PROVIDER_READ_VALUE_FAILED		-805
#define XERR_PROVIDER_CREATE_ITEM_FAILED	-806
#define XERR_PROVIDER_SET_VALUE_FAILED		-807
#define XERR_PROVIDER_REG_DELETE_FAILED		-808

#define XERR_CREATE_MUTEX_FAILED            -809
#define XERR_RELEASE_MUTEX_FAILED           -810

#define WM_QUERY_FW_NOTIFY WM_USER+15



#define XCTRL_IDLE		0X0
#define XCTRL_PERMIT	0X01
#define XCTRL_DENY		0X02
#define XCTRL_USE		0X04
//#define XCTRL_ENDUSE	0X08

typedef struct _XFW_RULE{
	BYTE control;
	TCHAR appName[MAX_PATH];
}XFW_RULE;
#define XFW_RULE_LEN 100

#define XIO_TYPE_NULL		0X0
#define XIO_TYPE_INIT		0X01
#define XIO_TYPE_END		0X02
#define XIO_TYPE_GET_PATH   0X04
#define XIO_TYPE_SET_CONTROL 0x08
#define XIO_TYPE_GET_RULE	0x10
#define XIO_READ_RULES      0x12

typedef struct _XIO_CONTROL{
	HWND hwnd;   //界面窗口句柄
	TCHAR *path; //应用程序路径
	BOOL permit;
	XFW_RULE *pRule;
}XIO_CONTROL;

typedef int (WINAPI *XF_IO_CONTROL)(int type, XIO_CONTROL *pContrl);

#define	QUERY_MUTEX_NAME _T("simple_fw_query_mutex")

#endif
