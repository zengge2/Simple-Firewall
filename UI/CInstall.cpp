
#include "stdafx.h"
#include "..\IP_Moniter\IP_Moniter.h"
#include "CInstall.h"

#pragma comment(lib, "ws2_32.lib")

BOOL CInstall::IsWinsock2()
{
	WORD wVersionRequested = MAKEWORD(2,0);
	WSADATA wsaData;

	if(WSAStartup(wVersionRequested, &wsaData) != 0)
		return FALSE;

	if(LOBYTE(wsaData.wVersion)!=2){
		WSACleanup();
		return FALSE;
	}
	return FALSE;
}


BOOL CInstall::IsInstalled(TCHAR *sPathName)
{
	TCHAR tsPathName[MAX_PATH];

	if(ReadReg(REG_INSTALL_PATH_ITEM,(BYTE*)tsPathName, MAX_PATH,HKEY_LOCAL_MACHINE,REG_INSTALL_KEY,REG_SZ)){
		if(sPathName != NULL)
			_tcscpy(sPathName, tsPathName);

		return TRUE;
	}
	return FALSE;
}	
int	 CInstall::InstallProvider(TCHAR *sPathName)
{
	if(IsInstalled())
		return XERR_PROVIDER_ALREADY_INSTALL;

	_tcscpy(m_sPathName, sPathName);

	int iRet;

	if((iRet = EnumHookKey(FALSE))!= XERR_SUCCESS)
		return iRet;

	if(!SaveReg(REG_INSTALL_PATH_ITEM,(BYTE*)sPathName,_tcslen(sPathName),
				HKEY_LOCAL_MACHINE, REG_INSTALL_KEY, REG_SZ))
		return XERR_PROVIDER_SAVE_PATH_FAILED;

	return XERR_SUCCESS;
}

int CInstall::RemoveProvider()
{
	int iRet = XERR_SUCCESS;

	if(!IsInstalled())
		return XERR_PROVIDER_NOT_INSTALL;

	if(iRet = EnumHookKey(TRUE) != XERR_SUCCESS)
		return iRet;

	if(!DeleteReg())
		return XERR_PROVIDER_REG_DELETE_FAILED;

	return XERR_SUCCESS;
	
}

int CInstall::EnumHookKey(BOOL IsRemove)
{
	HKEY hkey = NULL;

	if(RegOpenKeyEx(HKEY_LOCAL_MACHINE, REG_PROTOCOL_CATALOG_KEY,0,KEY_READ, &hkey) != ERROR_SUCCESS)
		return XERR_PROVIDER_OPEN_REG_FAILED;

	__try{
		TCHAR sSubKey[MAX_PATH];
		DWORD dwIndex = 0;
		int iRet = 0;

		while(RegEnumKey(hkey, dwIndex, sSubKey, MAX_PATH)==ERROR_SUCCESS){
			if((iRet = SaveHookKey(hkey, sSubKey, IsRemove))!= XERR_SUCCESS)
				return iRet;
			dwIndex++;
		}
	}
	__finally
	{
		RegCloseKey(hkey);
	}
	return 0;
}

int CInstall::SaveHookKey(HKEY hkey, LPCTSTR sSubKey, BOOL IsRemove)
{
	HKEY hSubkey = NULL;
	BYTE ItemValue[MAX_PROTOCOL_CATALOG_LENTH];
	DWORD ItemSize=MAX_PROTOCOL_CATALOG_LENTH;

	if(RegOpenKeyEx(hkey, sSubKey, 0, KEY_ALL_ACCESS, &hSubkey) != ERROR_SUCCESS)
		return XERR_PROVIDER_OPEN_REG_FAILED;

	__try{
		if(RegQueryValueEx(hSubkey, REG_PROTOCOL_CATALOG_ITEM,0,NULL,ItemValue, &ItemSize)!=ERROR_SUCCESS
				||(ItemSize != MAX_PROTOCOL_CATALOG_LENTH))
			return XERR_PROVIDER_OPEN_REG_FAILED;

		WSAPROTOCOL_INFOW *mProtocolInfo = (WSAPROTOCOL_INFOW*)(ItemValue + MAX_PATH);

		if(mProtocolInfo->ProtocolChain.ChainLen == 1
				&& mProtocolInfo->iAddressFamily == AF_INET)
		{
			TCHAR sItem[21];
			_stprintf(sItem, _T("%u"), mProtocolInfo->dwCatalogEntryId);
			if(!IsRemove){
				if(!SaveReg(sItem, ItemValue, _tcslen((TCHAR*)ItemValue),
						HKEY_LOCAL_MACHINE,REG_INSTALL_KEY,REG_SZ))
					return XERR_PROVIDER_CREATE_ITEM_FAILED;
				_tcscpy((TCHAR*)ItemValue, m_sPathName); //replace the path
				if(RegSetValueEx(hSubkey, REG_PROTOCOL_CATALOG_ITEM,0, REG_BINARY, ItemValue, ItemSize)
						!=ERROR_SUCCESS)
					return XERR_PROVIDER_SET_VALUE_FAILED;
			}	
			else
			{
			TCHAR sProvider[MAX_PATH];
			
			if(!ReadReg(sItem, (BYTE*)sProvider, MAX_PATH, HKEY_LOCAL_MACHINE, REG_INSTALL_KEY,REG_SZ))
				return XERR_PROVIDER_READ_VALUE_FAILED;
			_tcscpy((TCHAR*)ItemValue, sProvider);
			if(RegSetValueEx(hSubkey, REG_PROTOCOL_CATALOG_ITEM,0,REG_BINARY,ItemValue,ItemSize)!=
					ERROR_SUCCESS)
				return XERR_PROVIDER_SET_VALUE_FAILED;
			}
		}
	}
	__finally
	{
		RegCloseKey(hSubkey);
	}

	return XERR_SUCCESS;	
}	
		
BOOL CInstall::ReadReg(TCHAR *sKey, BYTE *pBuffer, DWORD dwBufSize,
			HKEY hkey ,
			TCHAR *sSubKey ,
			DWORD ulType )
{
	HKEY hSubkey;
	if(RegOpenKeyEx(hkey, sSubKey, 0, KEY_ALL_ACCESS, &hSubkey)!= ERROR_SUCCESS)
		return FALSE;

	DWORD dwType;
	if(RegQueryValueEx(hSubkey, sKey, 0, &dwType, pBuffer, &dwBufSize) == ERROR_SUCCESS){
		RegCloseKey(hSubkey);
		return TRUE;
	}	
	RegCloseKey(hSubkey);
	return FALSE;
}

BOOL CInstall::SaveReg(TCHAR *sKey, BYTE *pBuffer, DWORD dwBufSize,
			HKEY hkey ,
			TCHAR *sSubKey ,
			DWORD ulType )
{
	HKEY hSubkey;
	DWORD dwDisposition;
	
	if(RegCreateKeyEx(hkey, sSubKey, 0, NULL, REG_OPTION_NON_VOLATILE,KEY_ALL_ACCESS,NULL,&hSubkey,
				&dwDisposition)!=ERROR_SUCCESS)
		return FALSE;

	if(RegSetValueEx(hSubkey, sKey, 0, ulType, pBuffer, dwBufSize) != ERROR_SUCCESS ){
		RegCloseKey(hSubkey);
		return FALSE;
	}	
	RegCloseKey(hSubkey);
	return TRUE;
}

BOOL CInstall::DeleteReg(
			HKEY hkey ,
			TCHAR *sSubKey ,
			TCHAR *sItem )
{
	if(hkey == NULL || sSubKey == NULL)
		return FALSE;

	if(sItem == NULL)
	{
		if(RegDeleteKey(hkey, sSubKey)!=ERROR_SUCCESS)
			return FALSE;
		else
			return TRUE;
	}

	HKEY hSubKey;

	if(RegOpenKeyEx(hkey, sSubKey, 0, KEY_ALL_ACCESS, &hSubKey)!=ERROR_SUCCESS)
		return FALSE;

	__try{

		if(RegDeleteValue(hSubKey, sItem) == ERROR_SUCCESS)
			return TRUE;
	}
	__finally{
		RegCloseKey(hSubKey);
	}

	return FALSE;
}
	
