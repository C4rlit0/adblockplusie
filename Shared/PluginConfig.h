#ifndef _PLUGIN_CONFIG_H_
#define _PLUGIN_CONFIG_H_


#include "PluginTypedef.h"
#include "PluginIniFileW.h"
#include "PluginChecksum.h"


class CPluginConfig
{

private:

	static CPluginConfig* s_instance;

    static CComAutoCriticalSection s_criticalSection;

	TDownloadFileProperties m_downloadFileProperties;
	TDownloadFileCategories m_downloadFileCategories;
	TDownloadDomainTitles m_downloadDomainTitles;

	// private constructor used by the singleton pattern
	CPluginConfig();
	
	void Create();

public:
	
	~CPluginConfig();
	
	// Returns an instance of the Dictionary
	static CPluginConfig* GetInstance(); 

    static bool Download(const CString& url, const CString& filename);
    bool GetDownloadProperties(const CString& headers, SDownloadFileProperties& properties) const;
    
    void Read();
	int GenerateFilterString(TCHAR* pBuffer, SDownloadFileProperties& properties, std::vector<std::pair<CString,CString>>& filterData, bool allowConversion) const;

	void GetDownloadDomainTitles(TDownloadDomainTitles& domainTitles) const;
};


#endif // _PLUGIN_CONFIG_H_