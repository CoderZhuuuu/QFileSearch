/* 磁盘(卷)类 */
#pragma once

#include "Global.h"
#include "kmp.h"

class Volume : public QObject
{
	Q_OBJECT

public:
	explicit Volume(const char & diskName);
	virtual ~Volume(void);

	bool isNTFS(void);		// 是否为 NFTS 格式
	bool initUSN(void);		// 初始化 USN

	void search(QString);

private:
	char diskName;					// 磁盘名
	HANDLE hVol;					// 句柄
	USN_JOURNAL_DATA ujd;
	CREATE_USN_JOURNAL_DATA cujd;

	CUR_ENTRY curEntry;
	PFRN_ENTRY pfrnEntry;
	FRN_ENTRY_MAP frnEntryMap;

	QString exps;

	int searchMode;
	QMutex lock;
	std::vector<CUR_ENTRY> curEntryArr;
	std::string path;

private:
	bool getHandle(void);		// 获取句柄
	bool createUSN(void);		// 创建 USN 日志
	bool getUSNInfo(void);		// 获取 USN 日志信息
	bool getUSNJournal(void);	// 获取 USN Journal的信息
	bool deleteUSN(void);		// 删除 USN 日志文件
	bool searchP(const char * src, const char * pat);	// 搜索

	void initAllFilePath(void);
	void getFilePathFromFrn(DWORDLONG, std::string &);

signals:
	void sigSearchEnd(void);
	void sigInitEnd(void);

private slots:
	void sltCreate(QString);
	void sltDelete(QString);
	void sltRename(QString, QString);
};

bool matchSuffix(const char * src, const char * pat);		// 后缀名匹配]
bool matchType(const char * src, const char * pat);			// 匹配 type