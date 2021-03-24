/* ����(��)�� */
#pragma once

#include "Global.h"
#include "kmp.h"

class Volume : public QObject
{
	Q_OBJECT

public:
	explicit Volume(const char & diskName);
	virtual ~Volume(void);

	bool isNTFS(void);		// �Ƿ�Ϊ NFTS ��ʽ
	bool initUSN(void);		// ��ʼ�� USN

	void search(QString);

private:
	char diskName;					// ������
	HANDLE hVol;					// ���
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
	bool getHandle(void);		// ��ȡ���
	bool createUSN(void);		// ���� USN ��־
	bool getUSNInfo(void);		// ��ȡ USN ��־��Ϣ
	bool getUSNJournal(void);	// ��ȡ USN Journal����Ϣ
	bool deleteUSN(void);		// ɾ�� USN ��־�ļ�
	bool searchP(const char * src, const char * pat);	// ����

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

bool matchSuffix(const char * src, const char * pat);		// ��׺��ƥ��]
bool matchType(const char * src, const char * pat);			// ƥ�� type