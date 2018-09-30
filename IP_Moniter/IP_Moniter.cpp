// IP_Moniter.cpp : Defines the entry point for the DLL application.

//

#include "stdafx.h"
#include "ip_moniter.h"

BOOL GetHookProvider(IN WSAPROTOCOL_INFOW *pProtocolInfo,OUT TCHAR *sPathName);
SOCKET WSPAPI WSPSocket(
	int		af,
	int		type,
	int		protocol,
	LPWSAPROTOCOL_INFOW lpProtocolInfo,
	GROUP	g,
	DWORD	dwFlags,
	LPINT	lpErrno
);
void GetRightEntryIdItem(IN WSAPROTOCOL_INFOW * pProtocolInfo,
						 OUT TCHAR *sItem);

WSPPROC_TABLE		NextProcTable;
TCHAR				ProcessName[MAX_PATH];

#pragma data_seg(".uniData")
HWND				UIHandle=0;
HANDLE				hMutex = NULL;
#pragma data_seg()

#pragma bss_seg(".uni2Data")
XFW_RULE			rules[XFW_RULE_LEN];
XFW_RULE			queryRule;
#pragma bss_seg()

BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
					 )
{
	if(ul_reason_for_call == DLL_PROCESS_ATTACH){
		//初始化
		GetModuleFileName(NULL, ProcessName, MAX_PATH);
			
	}
    return TRUE;
}


//WINAPI == __stdcall
int WINAPI IOCtrl(int type, XIO_CONTROL *pContrl)
{
//拷贝新规则到对应的rules队列	
	if(type == XIO_READ_RULES)
		{
	        XFW_RULE *rulesData = (XFW_RULE*)pContrl;
			memcpy(rules, rulesData, sizeof(XFW_RULE)*XFW_RULE_LEN);
		}
	
	if(type == XIO_TYPE_INIT){
		UIHandle = pContrl->hwnd;
	
		if((hMutex = CreateMutex(NULL, FALSE, QUERY_MUTEX_NAME))==NULL){
			MessageBox(NULL, "Error create Mutex", NULL, MB_OK);
			return XERR_CREATE_MUTEX_FAILED;
		}
		queryRule.control = XCTRL_IDLE;
		for(int i = 0; i<XFW_RULE_LEN; i++){
			rules[i].control = XCTRL_IDLE;
		}
	}
	if(type == XIO_TYPE_END){
			
		CloseHandle(hMutex);
			
	}
	if(type == XIO_TYPE_GET_PATH){
		pContrl->path = queryRule.appName;
	}
	if(type == XIO_TYPE_SET_CONTROL){
		if(_tcscmp(queryRule.appName, pContrl->path)==0){
			if(pContrl->permit == TRUE)
					queryRule.control = XCTRL_PERMIT;
			else 
					queryRule.control = XCTRL_DENY;

			//纪录访问控制规则		
			for(int i = 0; i<XFW_RULE_LEN; i++){
				//TCHAR temp[30];
				//_stprintf(temp, "find idle=%d", rules[i].control); 
				//MessageBox(NULL, temp, NULL, MB_OK);
				if(rules[i].control == XCTRL_IDLE){
					//TCHAR temp[30];
					//_stprintf(temp, "find idle=%d", i); 
					//MessageBox(NULL, temp, NULL, MB_OK);
					rules[i].control = queryRule.control;
					_tcscpy(rules[i].appName, pContrl->path);
					break;
				}
			}
		}
	}
	if(type == XIO_TYPE_GET_RULE){
		pContrl->pRule = (XFW_RULE*)rules;
	}

	return XERR_SUCCESS;
}

int  WSPAPI WSPStartup(WORD wVersionRequested,
					  LPWSPDATA lpWSPData,
					  LPWSAPROTOCOL_INFOW lpProtocolInfo,
					  WSPUPCALLTABLE upcallTable,
					  LPWSPPROC_TABLE lpProcTable)
{
	OutputDebugString(_T("WSPStartup"));

	TCHAR sLibraryPath[512];
	LPWSPSTARTUP	WSPStartupFunc = NULL;
	HMODULE	hLibraryHandle = NULL;
	INT		ErrorCode = 0;

	if(!GetHookProvider(lpProtocolInfo, sLibraryPath)
		|| (hLibraryHandle = LoadLibrary(sLibraryPath))== NULL
		|| (WSPStartupFunc = (LPWSPSTARTUP)GetProcAddress(hLibraryHandle, "WSPStartup"))==NULL)

		return WSAEPROVIDERFAILEDINIT;

	//调用系统的启动函数，得到winsock方法表
	if((ErrorCode = WSPStartupFunc(wVersionRequested, lpWSPData, lpProtocolInfo, upcallTable,
								 lpProcTable))!= ERROR_SUCCESS)
		return ErrorCode;

	NextProcTable = *lpProcTable;

	//将系统的winsock方法表中WSPSocket函数替换成为自己的函数
	lpProcTable->lpWSPSocket = WSPSocket;

	return 0;
}

BOOL GetHookProvider(IN WSAPROTOCOL_INFOW *pProtocolInfo,
					 OUT TCHAR *sPathName)
{
	TCHAR sItem[21];

	GetRightEntryIdItem(pProtocolInfo, sItem);

	HKEY hSubkey;
	DWORD ulDateLength = MAX_PATH;
	TCHAR	sTemp[MAX_PATH];

	if(RegOpenKeyEx(HKEY_LOCAL_MACHINE, REG_INSTALL_KEY, 0, KEY_ALL_ACCESS, &hSubkey)!=
		ERROR_SUCCESS)
		return FALSE;

	if(RegQueryValueEx(hSubkey, sItem, 0, NULL, (BYTE*)sTemp, &ulDateLength)
		|| ExpandEnvironmentStrings(sTemp, sPathName, ulDateLength) == 0)
		return FALSE;

	if(sPathName[0]=='\0' && sTemp[0] != '\0')
		_tcscpy(sPathName, sTemp);

	RegCloseKey(hSubkey);

	return TRUE;

}

void GetRightEntryIdItem(IN WSAPROTOCOL_INFOW * pProtocolInfo,
						 OUT TCHAR *sItem)
{
	if(pProtocolInfo->ProtocolChain.ChainLen <= 1)
	{
		_stprintf(sItem, _T("%u"), pProtocolInfo->dwCatalogEntryId);
	}
}

SOCKET WSPAPI WSPSocket(
	int		af,
	int		type,
	int		protocol,
	LPWSAPROTOCOL_INFOW lpProtocolInfo,
	GROUP	g,
	DWORD	dwFlags,
	LPINT	lpErrno
)
{
	OutputDebugString(_T("IP_MONITER:WSPSocket run"));
	DWORD dwWaitResult; 
	//ATLTRACE("\n############################################\n");
	//for(int i = 0; i < 4; i++){
	//		ATLTRACE("\nRULES:%d,%s\n",rules[i].control,rules[i].appName);
	//	}
	//	ATLTRACE("\n############################################\n");
	
	
	//ATLTRACE("ProcessName = %s",ProcessName);
	//ATLTRACE("\n############################################\n");

	
	//MessageBox(NULL,ProcessName,NULL,MB_OK); 
	//察看是否能够访问网络
	for(int i = 0; i<XFW_RULE_LEN; i++){
		if(rules[i].control != XCTRL_IDLE){
			//ATLTRACE("\nrules[%d].appName = %s\n",i,rules[i].appName);

			if(_tcscmp(rules[i].appName, ProcessName)==0){
                //MessageBox(NULL,"=",NULL,MB_OK); 
				//MessageBox(NULL,rules[i].appName,NULL,MB_OK); 
				if(rules[i].control == XCTRL_PERMIT)
					goto permit;
				else
					return WSAEACCES;
			}
		}	
	}

    //ATLTRACE("\n############################################\n");

	hMutex = OpenMutex(SYNCHRONIZE, FALSE, QUERY_MUTEX_NAME);
	if(hMutex == NULL){
		MessageBox(NULL,_T("Error open Mutex"),NULL,MB_OK);
		return WSAEACCES;
	}

    dwWaitResult = WaitForSingleObject( 
         hMutex,        // handle of mutex
         1000L);        // 1 second time-out interval
 
      switch (dwWaitResult) 
      {
         case WAIT_OBJECT_0: 
            __try 
            { 
				//其他进程在查询
				if(queryRule.control != XCTRL_IDLE){
					return WSAEACCES; //返回错误，使用Winsock标准错误
				}
				queryRule.control = XCTRL_USE;
            } 
            __finally 
            { 
               ReleaseMutex(hMutex);
            }
			break;
		 default:
			 return WSAEACCES; //得不到锁，返回错误
	  }		


	//占用了queryRule	
	_tcscpy(queryRule.appName, ProcessName);
	::PostMessage(UIHandle, WM_QUERY_FW_NOTIFY, NULL, NULL);
	//等待回应
	while(queryRule.control == XCTRL_USE){
			int sec = 0;
			sec++;
			if(sec > 120)
				return FALSE;
			Sleep(1000);
	}

	
	dwWaitResult = WaitForSingleObject( 
         hMutex,        // handle of mutex
         2000L);        // 2 second time-out interval
 
      switch (dwWaitResult) 
      {
         case WAIT_OBJECT_0: 
            __try 
            { 
				if(queryRule.control != XCTRL_PERMIT){
					return WSAEACCES;
				}
            } 
            __finally 
            { 
			   queryRule.control = XCTRL_IDLE;
               ReleaseMutex(hMutex);
            }
	  }		

permit:
	return NextProcTable.lpWSPSocket(af,type,protocol,lpProtocolInfo,g,dwFlags,lpErrno);
}
