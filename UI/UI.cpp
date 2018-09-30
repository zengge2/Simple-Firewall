// UI.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "resource.h"
#include <time.h>
#include <fstream>
#include <ostream>
#include <shellapi.h>
#pragma comment(lib,"comctl32.lib")

#include "..\IP_Moniter\IP_Moniter.h"
#include "CInstall.h"

#define IDC_STATUS 111

BOOL __stdcall DlgProc(HWND, UINT, WPARAM, LPARAM);

void ReadRuleFile(HWND hDlg);//导入规则文件
void WriteRuleFile(XFW_RULE *rules);
void WriteLogFile(XFW_RULE rule, const std::string logFilePathname);

TCHAR sProvider[MAX_PATH];
#define ERROR_INFO_LEN 200
TCHAR errorInfo[ERROR_INFO_LEN];

HMODULE hDll;
XF_IO_CONTROL IoControl;

XFW_RULE			rules[XFW_RULE_LEN];
int secGlobal = 0;  //总的规则的条数
int ruleNumFromFile = 0;  //从文件中读出的规则的条数
BOOL isChanged = false;
//int ruleIndex = 0;

TCHAR *filePath = ".\\Rules.ini";
//TCHAR *filePath = "F:\\Network_SEC\\simpleFW\\Debug\\Rules.ini";
TCHAR *logFilePath = ".\\SimpleFW.log";
//TCHAR *filePath = "F:\\Network_SEC\\simpleFW\\Debug\\SimpleFW.log"

CInstall install;


int GetPath(OUT TCHAR *sPath) 
{
	TCHAR sFilename[_MAX_PATH];
	TCHAR sDrive[_MAX_DRIVE];
	TCHAR sDir[_MAX_DIR];
	TCHAR sFname[_MAX_FNAME];
	TCHAR sExt[_MAX_EXT];

	//得到当前进程模块的完整路径
	GetModuleFileName(NULL, sFilename, _MAX_PATH);
	
	//将sFilename划分成四个部分分别放入对应的组件中
	_tsplitpath(sFilename, sDrive, sDir, sFname, sExt);

	_tcscpy(sPath, sDrive);
	_tcscat(sPath, sDir);

	if(sPath[_tcslen(sPath) - 1] != _T('\\'))
		_tcscat(sPath, _T("\\"));

	if(sPath[0] == '\0')
	{
		MessageBox(NULL, _T("Can't find the application path"), NULL, MB_OK);
		return -1;
	}

	_tcscat(sPath, "IP_MONITER.DLL");

	if(_taccess(sPath, 0) == -1)
	{
		_sntprintf(errorInfo, ERROR_INFO_LEN, "%s%s",_T("Can't find "), sPath);
		MessageBox(NULL, errorInfo, NULL, MB_OK);
		return -1;
	}

	return 0;
}  

int Install()
{
	int retCode;

	retCode = install.InstallProvider(sProvider);
	if(retCode != XERR_SUCCESS){
		_sntprintf(errorInfo, ERROR_INFO_LEN, "%s%d",_T("InstallProvider error="), retCode);
		MessageBox(NULL, _T(errorInfo), NULL, MB_OK);
		return retCode;
	}
	return retCode;
}

int Remove(){
	int retCode = install.RemoveProvider();
	return retCode;
}

int	LoadDLL()
{
	if ((hDll = LoadLibrary(sProvider)) == NULL)
	{
		_sntprintf(errorInfo, ERROR_INFO_LEN, "%s%s",_T("Can't load dll:"), sProvider);
		MessageBox(NULL, _T(errorInfo), NULL, MB_OK);
		return -1;
	}

	IoControl	= (XF_IO_CONTROL)GetProcAddress(hDll, _T("IOCtrl"));

	if (IoControl == NULL)
	{
		MessageBox(NULL, _T("Can't find IoCtrl function"), NULL, MB_OK);
		return -1;
	}
	return 0;
}

int InitDLLData(HWND hDlg)
{
	XIO_CONTROL ioControl;
	ioControl.hwnd = hDlg;
	IoControl(XIO_TYPE_INIT, &ioControl);
	return 0;
}

int EndDLLData()
{
	IoControl(XIO_TYPE_END, NULL);
	return 0;
}

TCHAR* GetAppPath()
{
	static TCHAR path[_MAX_PATH];	
	XIO_CONTROL ioControl;
	IoControl(XIO_TYPE_GET_PATH, &ioControl);
	_tcscpy(path, ioControl.path);

	return path;
}

//设置应用程序的访问规则
int SetAppControl(BOOL permit, TCHAR * pApp)
{
	XIO_CONTROL ioControl;
	ioControl.permit = permit;
	ioControl.path = pApp;
	IoControl(XIO_TYPE_SET_CONTROL, &ioControl);

	return 0;
}


XFW_RULE* GetAppControlRule()
{
	XIO_CONTROL ioControl;
	IoControl(XIO_TYPE_GET_RULE, &ioControl);
	return ioControl.pRule;
}


int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
 	// TODO: Place code here.
	if(GetPath(sProvider))
		return -1;

	//LoadRules();

	if(LoadDLL())
		return -1;
	IoControl(0x12,(XIO_CONTROL*)rules);

	// Install the SPI dll 
	if(Install())
		return -1;

	// Show the UI
	::InitCommonControls();
    
	::DialogBoxParam(hInstance, (LPCTSTR)IDD_DIALOG, NULL, DlgProc, NULL);

	// Remove the SPI dll 
	Remove();

	EndDLLData();

	return 0;
}

BOOL __stdcall DlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch(message)
	{
	case WM_INITDIALOG:
					{
						HWND hWndList = ::GetDlgItem(hDlg, IDC_LIST);
	
						// 设置它的扩展风格
						::SendMessage(hWndList, LVM_SETEXTENDEDLISTVIEWSTYLE, 
										0, LVS_EX_FULLROWSELECT|LVS_EX_GRIDLINES);
						LVCOLUMN column;
						// 指定LVCOLUMN结构中的pszText、fmt、cx域有效
						column.mask = LVCF_TEXT|LVCF_FMT|LVCF_WIDTH;
						// 设置有效的域的属性
						column.fmt = LVCFMT_LEFT;	// 指定文本靠左显示
						column.cx = 58;		// 指定此栏的宽度
						column.pszText = _T("动作");	// 指定此栏显示的文本
						
						// 添加一个新的专栏
						::SendMessage(hWndList, LVM_INSERTCOLUMN, 0, (LPARAM)&column);
						// 再添加一个专栏
						column.pszText = _T("程序名");
						column.cx = 450;
						::SendMessage(hWndList, LVM_INSERTCOLUMN, 1, (LPARAM)&column);

						// 初始化状态栏

						// 创建状态栏
						HWND hWndStatus = ::CreateStatusWindow(WS_CHILD|WS_VISIBLE|SBS_SIZEGRIP, 
										NULL, hDlg, IDC_STATUS);
												
						// 设置文本
						::SendMessage(hWndStatus, SB_SETTEXT, 0, (long)_T(" 准备就绪"));

						// Init the DLL
						if(InitDLLData(hDlg))
							::EndDialog(hDlg, IDOK);

						//读规则文件
						ReadRuleFile(hDlg);
						secGlobal = ruleNumFromFile;
						}
					return TRUE;
		case WM_COMMAND:
					switch(LOWORD(wParam))
					{
					case IDC_ABOUT:
						MessageBox(hDlg, _T("1.0版"),
							_T("关于 SimpleFW"), MB_OK);
						break;
					case IDC_VIEWLOGFILE:
						ShellExecute(hDlg, "open", "notepad.exe", "SimpleFW.log", "", SW_SHOW);
						break;
						
					case IDOK:
						//::EndDialog(hDlg, IDOK);
						//break;
					case IDCANCEL:
						if (isChanged)
						{
							WriteRuleFile(rules);
						}
						::EndDialog(hDlg, IDCANCEL);
						break;
					}
					return TRUE;
		case WM_QUERY_FW_NOTIFY:
					
					//得到应用程序路径
					TCHAR * pApp = GetAppPath();

					//进行匹配，判断是否已经存在该程序的控制规则
					for(int num = 0; num <= secGlobal; num++) {
						if(_tcscmp(rules[num].appName,pApp)==0){
							SetAppControl((rules[num].control == 0X01)?TRUE:FALSE,pApp);
							//写入日志文件
							WriteLogFile(rules[num], logFilePath);
							return TRUE;
						}
					}

					int ctrl;
					//std::string path;//新的应用程序的控制规则和路径

					if(MessageBox(hDlg, pApp, _T("是否允许该程序访问网络"), 
						MB_YESNO|MB_ICONQUESTION|MB_TOPMOST)==IDYES){
                        SetAppControl(TRUE, pApp);
						ctrl = 1;
					}
					else{
						SetAppControl(FALSE, pApp);
						ctrl = -1;
					}
					//刷新界面的列表
					TCHAR Control[10];
					int ListItemNum = secGlobal;
					XFW_RULE * pRule = GetAppControlRule();

					XFW_RULE *tempRule = new XFW_RULE();
					(*tempRule).control = (ctrl == 1) ? XCTRL_PERMIT : XCTRL_DENY;
					memcpy((*tempRule).appName,std::string(pApp).c_str(),std::string(pApp).length());
					(*tempRule).appName[std::string(pApp).length()] = '\0';

					WriteLogFile(*tempRule, logFilePath);

					HWND hWndList = ::GetDlgItem(hDlg, IDC_LIST);
//################################################################
//以下程序在ListView中显示程序路径及访问控制规则
					if(ctrl == 1)
						_tcscpy(Control,_T("允许"));
					else
						_tcscpy(Control,_T("禁止"));
					LVITEM item = {0};
					item.iItem = ListItemNum;
					item.mask = LVIF_TEXT;
					item.pszText = (LPTSTR)Control;
					::SendMessage(hWndList, LVM_INSERTITEM, 0, (long)&item);

					LVITEM lvi;
					lvi.iSubItem = 1;
					lvi.pszText = (LPTSTR)pApp;
					::SendMessage(hWndList, LVM_SETITEMTEXT, ListItemNum, (LPARAM)&lvi);
					
					if(ListItemNum >= XFW_RULE_LEN)
						break;
//拷贝新添加的规则到rules队列
					memcpy(rules[ListItemNum].appName,
						std::string((*tempRule).appName).c_str(),
						std::string((*tempRule).appName).length());
					rules[ListItemNum].appName[std::string((*tempRule).appName).length()] = '\0';
					rules[ListItemNum].control = (*tempRule).control;

					ListItemNum++;

					pRule++;

					secGlobal++;
					isChanged = true;				
    				return TRUE;	
	}
	return FALSE;
}

//##########################################################################################
//
//

//读取保存好的规则文件Rules.ini
void ReadRuleFile(HWND hDlg)
{
	HWND hWndList = ::GetDlgItem(hDlg, IDC_LIST);
	SendMessage(hWndList, LVM_DELETEALLITEMS , 0, 0);

	TCHAR Control[10];
	BOOL permit;

	char buf[1024];  //缓存
	std::string message;
	std::ifstream in;

	in.open(filePath, std::ios::in);
	if (in.is_open())  //文件打开成功
	{
		int listItemNum = 0;

		while (!in.eof())
		{
			memset(buf, 0, 1024);
			in.getline(buf, 1024);
			message = buf;
			if (message[0] == '[') //读出的为section，跳过，直接读下一行
				continue;
			if (message[0] == 'a' && message[1] == 'c') //读出的为action，即动作
			{
				if (message[7] == '1') //permit
				{
					_tcscpy(Control,_T("允许"));
					permit = XCTRL_PERMIT;
				}
				else
				{
					_tcscpy(Control,_T("禁止"));
			        permit = XCTRL_DENY;
				}
				LVITEM item = {0};
				item.iItem = listItemNum;
				item.mask = LVIF_TEXT;
				item.pszText = (LPTSTR)Control;
				::SendMessage(hWndList, LVM_INSERTITEM, 0, (long)&item);

				rules[listItemNum].control = permit;
			}
			else if (message[0] == 'a' && message[1] == 'p') //读出的为appname，即程序名
			{
				std::string tmp(message, 8, message.length());
				TCHAR appName[_MAX_PATH];
				memcpy(appName,tmp.c_str(),tmp.length());
				appName[tmp.length()] = '\0';

				LVITEM lvi;
				lvi.iSubItem = 1;
				lvi.pszText = (LPTSTR)appName;
				::SendMessage(hWndList, LVM_SETITEMTEXT, listItemNum, (LPARAM)&lvi);

				memcpy(rules[listItemNum].appName,appName,std::string(appName).length());
				rules[listItemNum].appName[std::string(appName).length()] = '\0';  //将规则保存到rules数组
				listItemNum++;
			}
		}
		ruleNumFromFile = listItemNum;
	}
	in.close();
}

//保存规则到文件Rules.ini文件中
void WriteRuleFile(XFW_RULE *rules)
{
	std::string path;
	std::ofstream out;

	out.open(filePath, std::ios::out);//打开文件进行写入
	if (out.is_open())
	{
		for (int i = 0; i < secGlobal; i++)
		{
			out << "[" << i << "]\n";
			out << "action=" ;
			if (rules[i].control == XCTRL_DENY)	
				out << "-1\n"; 
			else
				out << "1\n";
			out << "appname=" << std::string(rules[i].appName).c_str() << "\n";
		}
	}
	out.close();
}


//写入日志文件
void WriteLogFile(XFW_RULE rule, const std::string logFilePathname)
{
	std::ofstream writeToLogFile;
	writeToLogFile.open(logFilePathname.c_str(), std::ios::out|std::ios::app);

	//测试文件是否成功打开
	if(!writeToLogFile.is_open())
		MessageBox(NULL, _T("Error opening log file!!"), NULL, MB_OK);	
	time_t  ltime;   
	time(&ltime);
	std::string strBuf(ctime(&ltime));
	std::string subStrBuf(strBuf, 0, strBuf.length()-1);
	writeToLogFile<<"["<<subStrBuf<<"]"<<"\n    应用程序："<<rule.appName;
	if (rule.control == 1)
		writeToLogFile<<"\n    规则：允许\n";//如果程序执行的是拒允许命令则在日志里会输入程序的名称以及允许
	else
		writeToLogFile<<"\n    规则：禁止\n";//如果程序执行的是拒绝命令则在日志里会输入程序的名称以及禁止
	
	writeToLogFile.close();
}