
class CInstall
{
private:
	int EnumHookKey(BOOL IsRemove = FALSE);
	int SaveHookKey(HKEY hkey, LPCTSTR sSubKey, BOOL IsRemove = FALSE);
public:
	BOOL IsWinsock2();
	BOOL IsInstalled(TCHAR *sPathName = NULL);
	int	 InstallProvider(TCHAR *sPathName);
	int  RemoveProvider();
public:
	BOOL  ReadReg(TCHAR *sKey, BYTE *pBuffer, DWORD dwBufSize,
				HKEY hkey = HKEY_LOCAL_MACHINE,
				TCHAR *sSubKey = REG_INSTALL_KEY,
				DWORD ulType = REG_BINARY);
	BOOL  SaveReg(TCHAR *sKey, BYTE *pBuffer, DWORD dwBufSize,
				HKEY hkey = HKEY_LOCAL_MACHINE,
				TCHAR *sSubKey = REG_INSTALL_KEY,
				DWORD ulType = REG_BINARY);
	BOOL CInstall::DeleteReg(
			HKEY hkey = HKEY_LOCAL_MACHINE,
			TCHAR *sSubKey = REG_INSTALL_KEY ,
			TCHAR *sItem = NULL );

public:
	TCHAR m_sPathName[MAX_PATH];
};
