#pragma once
#pragma warning(disable:4312)

#include <QUrl>
#include <QDir>
#include <QMenu>
#include <QFile>
#include <QEvent>
#include <QDebug>
#include <QMutex>
#include <QLabel>
#include <QDialog>
#include <QString>
#include <QObject>
#include <QThread>
#include <QAction>
#include <QDateTime>
#include <QFileInfo>
#include <QLineEdit>
#include <QClipboard>
#include <QStatusBar>
#include <QTableView>
#include <QScrollBar>
#include <QPushButton>
#include <QHeaderView>
#include <QScrollArea>
#include <QStringList>
#include <QPushButton>
#include <QApplication>
#include <QFontMetrics>
#include <QStandardItem>
#include <QDesktopWidget>
#include <QCoreApplication>
#include <QDesktopServices>
#include <QWinEventNotifier>
#include <QFileSystemWatcher>
#include <QStandardItemModel>
#include <QAbstractNativeEventFilter>

#include <map>
#include <list>
#include <queue>
#include <cmath>
#include <deque>
#include <vector>
#include <string>
#include <tchar.h>
#include <fstream>
#include <Shlobj.h>
#include <shlobj.h>
#include <iostream>
#include <atlconv.h>
#include <Windows.h>
#include <algorithm>
#include <winioctl.h>

#define WM_FILEMODIFY 0x6666

struct FileChangeInfo {
	DWORD dwItem1;
	DWORD dwItem2;
	DWORD dwItem3;
	DWORD dwItem4;
};

#define BUF_LEN 0x10000

struct FILE_ENTRY
{
	std::string fileName;
	std::string filePath;
	FILE_ENTRY(std::string fn = "", std::string fp = "")
		:fileName(fn), filePath(fp)
	{}
	bool operator<(const FILE_ENTRY & another) { return (filePath + fileName) < (another.filePath + another.fileName); }
};

struct FRN_ENTRY
{
	DWORDLONG pfrn;
	std::string filename;
	FRN_ENTRY(DWORDLONG frn = 0, std::string fn = "")
		:pfrn(frn), filename(fn)
	{}
};

typedef FRN_ENTRY CUR_ENTRY;
typedef FRN_ENTRY PFRN_ENTRY;
typedef std::map<DWORDLONG, FRN_ENTRY> FRN_ENTRY_MAP;

const int COLNUM = 4;
const int ROWNUM = 24;
const int ALPHABETNUM = 26;

const int WINDOWWIDTH = 800;
const int WINDOWHEIGHT = 600;

const int BUTTONWIDTH = 60;
const int BUTTONHEIGHT = 20;

const int DIALOGWIDTH = 450;
const int DIALOGHEIGHT = 200;

// 设置位置用的
const int OFFSET = 20;
const int GAP = 5;

static const char* ABOUTTEXT = "Welcome to use this application!\n"
"This is a small file search application like everything\n"
"You can use wildcard * and ? to search \n"
"or not use it to fuzzy search\n"
"we only support a-z A-Z 0-9 and \".-|#*$?\"\n"
"and also no more than 25 letters\n"
"You can also use \"|type:\" to find a series type of file\n"
"and especially:\n"
"\t|type:image to find image file like .png, .jpg, .gif\n"
"\t|type:video to find video file like .mp3, .mp4, .wav\n"
"\t|type:docum to find document file like .txt, .doc\n"
"\t|type:cmprs to find compressed file like .zip, .7z, .rar\n";

class Singleton		// 单例模式
{
public:
	~Singleton() {}
	Singleton(const Singleton&) = delete;
	Singleton& operator=(const Singleton&) = delete;
	static Singleton& get_instance()
	{
		static Singleton instance;
		return instance;
	}

	std::vector<FILE_ENTRY> resFileArr[ALPHABETNUM];
	std::vector<FILE_ENTRY> res[ALPHABETNUM];
private:
	Singleton() {}
};
