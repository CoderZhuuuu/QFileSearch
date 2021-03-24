#include "Volume.h"

const char* FILTER[] = { "type:", "size:" };

Volume::Volume(const char & diskName)
{
	this->diskName = diskName;
	QMutexLocker locker(&lock);
}

Volume::~Volume(void)
{
	deleteUSN();
}

bool Volume::isNTFS(void)
{
	bool diskIsNTFS = false;
	char sysNameBuf[MAX_PATH] = { 0 };
	char* s = new char[4];
	sprintf(s, "%c:\\", diskName);
	if (GetVolumeInformationA(
		s,
		NULL,
		0,
		NULL,
		NULL,
		NULL,
		sysNameBuf,
		MAX_PATH))
	{
		if (0 == strcmp(sysNameBuf, "NTFS"))
		{
			diskIsNTFS = true;
		}
	}
	return diskIsNTFS;
}

bool Volume::initUSN(void)
{
	bool initSucc = false;
	if (getHandle() && createUSN() && getUSNInfo() && getUSNJournal())
	{
		initSucc = true;
	}
	return initSucc;
}

// ƥ���׺��
bool matchSuffix(const char * src, const char * pat)
{
	int srcL = strlen(src);
	int patL = strlen(pat);
	bool match = true;
	if (srcL >= patL)
	{
		int i = 0;
		for (i = 1; i <= patL; ++i)
		{
			if (src[srcL - i] != pat[patL - i])
			{
				match = false;
				break;
			}
		}
		if ('.' != src[srcL - i])
		{
			match = false;
		}
	}
	else
	{
		match = false;
	}
	return match;
}

bool matchType(const char * src, const char * pat)
{
	bool match = false;
	if (0 == strcmp("image", pat))
	{
		if (matchSuffix(src, "png") || matchSuffix(src, "jpg") || matchSuffix(src, "gif"))
		{
			match = true;
		}
	}
	else if (0 == strcmp("video", pat))
	{
		if (matchSuffix(src, "mp3") || matchSuffix(src, "mp4") || matchSuffix(src, "wav"))
		{
			match = true;
		}
	}
	else if (0 == strcmp("docum", pat))
	{
		if (matchSuffix(src, "txt") || matchSuffix(src, "doc"))
		{
			match = true;
		}
	}
	else if (0 == strcmp("cmprs", pat))
	{
		if (matchSuffix(src, "zip") || matchSuffix(src, "7z") || matchSuffix(src, "rar"))
		{
			match = true;
		}
	}
	else if (matchSuffix(src, pat))
	{
		match = true;
	}
	return match;
}

bool Volume::searchP(const char * src, const char * pat)
{
	switch (searchMode)
	{
	case 0:
		return (nullptr != strstr(src, pat));
	case 1:
		return wildcardMatch(src, strlen(src), pat, strlen(pat));
	default:
		break;
	}
	return false;
}

void Volume::search(QString exps)
{
	this->exps = exps;
	exps.remove(QRegExp("\\s"));	// ȥ�����еĿո�
	exps = exps.toLower();			// ȫ�����Сд

	std::string pattern = exps.toStdString();
	std::string type = "";
	// Ѱ���Ƿ��� type

	int pos = -1;
	if (std::string::npos != (pos = pattern.find("|type:")))
	{
		type = pattern.substr(pos + 6, pattern.length() - 6 - pos);
		pattern = pattern.substr(0, pos);
	}

	if (std::string::npos == pattern.find('*') && std::string::npos == pattern.find('?'))
	{
		searchMode = 0;
	}
	else
	{
		searchMode = 1;
	}

	const char* tp = type.c_str();
	const char* pat = pattern.c_str();

	int patL = strlen(pat);

	Singleton& fileArr = Singleton::get_instance();	// ��ȡ�����ļ�����
	fileArr.res[diskName - 'A'].clear();
	if ("" != exps)
	{
		for (FILE_ENTRY it : fileArr.resFileArr[diskName - 'A'])
		{
			std::string tmp = it.fileName;
			transform(tmp.begin(), tmp.end(), tmp.begin(), ::tolower);	// ת����Сд(��Сд������)
			// ��ƥ��ģʽ��
			if (0 == strcmp(pat, "") || searchP(tmp.c_str(), pat))
			{
				// ��ƥ�䵽��ģʽ����ƥ�� type
				if (0 != strcmp(tp, ""))
				{
					if (matchType(tmp.c_str(), tp))
					{
						fileArr.res[diskName - 'A'].push_back(it);
					}
				}
				else
				{
					fileArr.res[diskName - 'A'].push_back(it);
				}
			}
		}
	}

	else
	{
		fileArr.res[diskName - 'A'] = fileArr.resFileArr[diskName - 'A'];
	}

	emit sigSearchEnd();
}

bool Volume::getHandle(void)
{
	bool getSucc = FALSE;
	// Ϊ\\.\C:����ʽ
	char* s = new char[7];
	sprintf(s, "\\\\.\\%c:", diskName);
	hVol = CreateFileA(
		s,
		GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL,
		OPEN_EXISTING,
		0,
		NULL);
	if (INVALID_HANDLE_VALUE != hVol)
	{
		getSucc = true;
	}
	return getSucc;
}

bool Volume::createUSN(void)
{
	bool createSucc = false;
	cujd.MaximumSize = 0;		// 0��ʾʹ��Ĭ��ֵ  
	cujd.AllocationDelta = 0;	// 0��ʾʹ��Ĭ��ֵ
	DWORD br;
	if (DeviceIoControl(
		hVol,						// handle to volume
		FSCTL_CREATE_USN_JOURNAL,   // dwIoControlCode
		&cujd,						// input buffer
		sizeof(cujd),				// size of input buffer
		NULL,                       // lpOutBuffer
		0,                          // nOutBufferSize
		&br,						// number of bytes returned
		NULL))						// OVERLAPPED structure	 
	{
		createSucc = true;
	}
	return createSucc;
}

bool Volume::getUSNInfo(void)
{
	bool getSucc = false;
	DWORD br;
	if (DeviceIoControl(
		hVol,		// handle to volume
		FSCTL_QUERY_USN_JOURNAL,	// dwIoControlCode
		NULL,						// lpInBuffer
		0,							// nInBufferSize
		&ujd,						// output buffer
		sizeof(ujd),				// size of output buffer
		&br,						// number of bytes returned
		NULL))						// OVERLAPPED structure
	{
		getSucc = true;
	}
	return getSucc;
}

bool Volume::getUSNJournal(void)
{
	// ����ö������
	MFT_ENUM_DATA med;
	med.StartFileReferenceNumber = 0;
	med.MaxMajorVersion = ujd.MinSupportedMajorVersion;	// ������ܵ�
	med.LowUsn = 0;
	med.HighUsn = ujd.NextUsn;

	CHAR pData[BUF_LEN];
	DWORD usnDataSize;
	PUSN_RECORD UsnRecord;
	// ͳ���ļ�����ȡö������
	while (0 != DeviceIoControl(
		hVol,
		FSCTL_ENUM_USN_DATA,
		&med,
		sizeof(med),
		pData,
		sizeof(pData),
		&usnDataSize,
		NULL))
	{
		DWORD dwRetBytes = usnDataSize - sizeof(USN);
		// �ҵ���һ�� USN ��¼  
		UsnRecord = (PUSN_RECORD)(((PCHAR)pData) + sizeof(USN));
		while (dwRetBytes > 0) {
			lock.lock();
			WCHAR tmp[MAX_PATH];
			LARGE_INTEGER liFileSize;
			memcpy(tmp, UsnRecord->FileName, UsnRecord->FileNameLength);
			tmp[UsnRecord->FileNameLength / 2] = L'\0';	// ��ȡ�ļ���
			pfrnEntry.filename = curEntry.filename = QString::fromWCharArray(tmp).toStdString();
			pfrnEntry.pfrn = curEntry.pfrn = UsnRecord->ParentFileReferenceNumber;	// ��ȡ�ļ��� frn ���ڲ���������·��
			curEntryArr.push_back(curEntry);
			frnEntryMap[UsnRecord->FileReferenceNumber] = pfrnEntry;	// �ڹ�ϣ���д��� entry
			// ��ȡ��һ����¼
			DWORD recordLen = UsnRecord->RecordLength;
			dwRetBytes -= recordLen;
			UsnRecord = (PUSN_RECORD)(((PCHAR)UsnRecord) + recordLen);
			lock.unlock();
		}
		// ��ȡ��һҳ���� 
		med.StartFileReferenceNumber = *(USN *)&pData;
	}
	initAllFilePath();
	curEntryArr.clear();
	frnEntryMap.clear();
	search("");
	emit sigInitEnd();
	return true;
}

bool Volume::deleteUSN(void)
{
	bool deleteSucc = false;
	DELETE_USN_JOURNAL_DATA dujd;
	dujd.UsnJournalID = ujd.UsnJournalID;
	dujd.DeleteFlags = USN_DELETE_FLAG_DELETE;
	DWORD br;
	if (DeviceIoControl(hVol,
		FSCTL_DELETE_USN_JOURNAL,
		&dujd,
		sizeof(dujd),
		NULL,
		0,
		&br,
		NULL))
	{
		deleteSucc = true;
	}
	CloseHandle(hVol);
	return deleteSucc;
}

void Volume::initAllFilePath(void)
{

	////�ļ���С
	//HANDLE hFileRead;
	////�����ļ���С
	//LARGE_INTEGER liFileSize;
	Singleton& fileArr = Singleton::get_instance();
	// ��ȡ�����ļ���·�������� resFileArr
	for (std::vector<CUR_ENTRY>::const_iterator cit = curEntryArr.begin();
		cit != curEntryArr.end(); ++cit)
	{
		path = "";
		getFilePathFromFrn(cit->pfrn, path);
		path = QString(diskName).toStdString() + ":\\" + path;
		lock.lock();
		fileArr.resFileArr[diskName - 'A'].push_back(FILE_ENTRY(cit->filename, path));
		lock.unlock();
		/*std::string str = (path + cit->filename);
		std::wstring wstr(str.length(), L' ');
		std::copy(str.begin(), str.end(), wstr.begin());*/
		//wchar_t fullPath[MAX_PATH];
		//swprintf(fullPath, MAX_PATH, L"%S", str.c_str());							// ע���д
		//WIN32_FILE_ATTRIBUTE_DATA fileAttr;
		//GetFileAttributesEx(fullPath, GetFileExInfoStandard, &fileAttr);
		//hFileRead = CreateFile(wstr.c_str(),			//name
		//	GENERIC_READ | GENERIC_WRITE,						//�Զ���ʽ��
		//	FILE_SHARE_READ | FILE_SHARE_WRITE,					//�ɹ����
		//	NULL,								//Ĭ�ϰ�ȫ����
		//	OPEN_EXISTING,						//ֻ���Ѿ����ڵ��ļ�
		//	FILE_ATTRIBUTE_NORMAL,				//�����ļ�����
		//	NULL);								//��ģ��
		//if (INVALID_HANDLE_VALUE != hFileRead)
		//{
		//	if (!GetFileSizeEx(hFileRead, &liFileSize))
		//	{
		//	}
		//}
		//CloseHandle(hFileRead);
	}
	sort(fileArr.resFileArr[diskName - 'A'].begin(), fileArr.resFileArr[diskName - 'A'].end());
}

void Volume::getFilePathFromFrn(DWORDLONG frn, std::string & path)
{
	// ͨ�� frn �ݹ�Ѱ�����ļ�·��
	FRN_ENTRY_MAP::iterator it = frnEntryMap.find(frn);
	if (frnEntryMap.end() != it)
	{
		if (0 != it->second.pfrn)
		{
			getFilePathFromFrn(it->second.pfrn, path);
		}
		path = path + it->second.filename;
		path = path + "\\";
	}
}

void Volume::sltCreate(QString path)
{
	if (diskName == path[0])
	{
		Singleton& fileArr = Singleton::get_instance();
		for (uint pos = fileArr.resFileArr[diskName - 'A'].size() - 1; pos >= 0; --pos)
		{
			if (path == QString::fromStdString((fileArr.resFileArr[diskName - 'A'].at(pos).filePath + fileArr.resFileArr[diskName - 'A'].at(pos).fileName).c_str()))
			{
				return;
			}
		}
		QFileInfo fileInfo(path);
		path = QString::fromStdString(path.toStdString().substr(0, path.toStdString().rfind('\\')));
		fileArr.resFileArr[diskName - 'A'].push_back(FILE_ENTRY(fileInfo.fileName().toStdString(), path.toStdString() + '\\'));
	}
	emit search(exps);
}

void  Volume::sltDelete(QString path)
{
	if (diskName == path[0])
	{
		Singleton& fileArr = Singleton::get_instance();
		for (uint pos = 0; pos < fileArr.resFileArr[diskName - 'A'].size(); ++pos)
		{
			if (path == QString::fromStdString(fileArr.resFileArr[diskName - 'A'].at(pos).filePath + fileArr.resFileArr[diskName - 'A'].at(pos).fileName))
			{
				fileArr.resFileArr[diskName - 'A'].erase(fileArr.resFileArr[diskName - 'A'].begin() + pos);
				break;
			}
		}
	}
	emit search(exps);
}

void  Volume::sltRename(QString path, QString path2)
{
	if (diskName == path[0])
	{
		Singleton& fileArr = Singleton::get_instance();
		QFileInfo fileInfo(path2);
		for (uint pos = 0; pos < fileArr.resFileArr[diskName - 'A'].size(); ++pos)
		{
			if (path == QString::fromStdString(fileArr.resFileArr[diskName - 'A'].at(pos).filePath + fileArr.resFileArr[diskName - 'A'].at(pos).fileName))
			{
				path2 = QString::fromStdString(path2.toStdString().substr(0, path2.toStdString().rfind('\\')));
				fileArr.resFileArr[diskName - 'A'].at(pos).fileName = fileInfo.fileName().toStdString();
				fileArr.resFileArr[diskName - 'A'].at(pos).filePath = path2.toStdString() + '\\';
				break;
			}
		}
	}
	emit search(exps);
}
