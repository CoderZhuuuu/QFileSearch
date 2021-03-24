#pragma once

#include <QtWidgets/QMainWindow>

#include "Global.h"
#include "Volume.h"

class QEventFilter :public QObject, public QAbstractNativeEventFilter
{
	Q_OBJECT

public:
	virtual bool nativeEventFilter(const QByteArray &eventType, void *message, long *)
		Q_DECL_OVERRIDE
	{
		if (eventType == "windows_generic_MSG" || eventType == "windows_dispatcher_MSG")
		{
			//MSG* pMsg = reinterpret_cast<MSG*>(message);
			//if (WM_FILEMODIFY == pMsg->message)
			//{
			//	FileChangeInfo *fciPtr = (FileChangeInfo*)pMsg->wParam;
			//	WCHAR path[MAX_PATH], path2[MAX_PATH];
			//	SHGetPathFromIDList((LPCITEMIDLIST)fciPtr->dwItem1, path);
			//	SHGetPathFromIDList((LPCITEMIDLIST)fciPtr->dwItem3, path2);
			//	switch (pMsg->lParam)
			//	{
			//	case SHCNE_MKDIR:			//创建目录
			//	case SHCNE_CREATE:			//创建文件
			//		if ('A' <= path[0] && path[0] <= 'Z')
			//		{
			//			emit sigCreate(QString::fromWCharArray(path));
			//		}
			//		break;
			//	case SHCNE_RMDIR:			//删除目录
			//	case SHCNE_DELETE:			//删除文件
			//		if ('A' <= path[0] && path[0] <= 'Z')
			//		{
			//			emit sigDelete(QString::fromWCharArray(path));
			//		}
			//		break;
			//	case SHCNE_RENAMEFOLDER:	//文件夹重命名
			//	case SHCNE_RENAMEITEM:		//文件重命名
			//		if ('A' <= path[0] && path[0] <= 'Z' && 'A' <= path2[0] && path2[0] <= 'Z')
			//		{
			//			emit sigRename(QString::fromWCharArray(path), QString::fromWCharArray(path2));
			//		}
			//		break;
			//	default:
			//		break;
			//	}
			//}
		}
		return false;
	}

signals:
	void sigCreate(QString);
	void sigDelete(QString);
	void sigRename(QString, QString);
};

class QFileSearch : public QMainWindow
{
	Q_OBJECT

public:
	QFileSearch(QWidget *parent = Q_NULLPTR);
	virtual ~QFileSearch(void) {}
	QEventFilter *filter;

protected:

private:
	QLineEdit *searchLineEdit;			// 搜索栏
	QScrollArea *fakeScrollArea;		// 模拟虚拟列表用的假的滚动条
	QTableView *fileListTableView;		// 显示文件信息的列表
	QStandardItemModel *fileItemModel;	// 显示文件信息的Item容器

	QStatusBar *searchInfoStatusBar;	// 显示查找信息的状态栏
	QMenu *itemMenu;					// 右键点击按钮时的菜单
	int num;							// 当前搜索完毕的磁盘数
	int diskNum;						// 磁盘总数

	QPushButton *aboutPushButton;

	QDialog *aboutDialog;
	QLabel *aboutTextLabel;

	std::vector<FILE_ENTRY> res[ALPHABETNUM];

	QMutex lock;

private:
	void initUI(void);			// 初始化界面
	void initSearchWork(void);	// 初始化搜索线程

signals:

private slots:
	void updateItemModel(int);
	void showMenuClickedSlots(const QPoint&);

	void onAboutPushButtonClicked(void);
};