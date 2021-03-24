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
			//	case SHCNE_MKDIR:			//����Ŀ¼
			//	case SHCNE_CREATE:			//�����ļ�
			//		if ('A' <= path[0] && path[0] <= 'Z')
			//		{
			//			emit sigCreate(QString::fromWCharArray(path));
			//		}
			//		break;
			//	case SHCNE_RMDIR:			//ɾ��Ŀ¼
			//	case SHCNE_DELETE:			//ɾ���ļ�
			//		if ('A' <= path[0] && path[0] <= 'Z')
			//		{
			//			emit sigDelete(QString::fromWCharArray(path));
			//		}
			//		break;
			//	case SHCNE_RENAMEFOLDER:	//�ļ���������
			//	case SHCNE_RENAMEITEM:		//�ļ�������
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
	QLineEdit *searchLineEdit;			// ������
	QScrollArea *fakeScrollArea;		// ģ�������б��õļٵĹ�����
	QTableView *fileListTableView;		// ��ʾ�ļ���Ϣ���б�
	QStandardItemModel *fileItemModel;	// ��ʾ�ļ���Ϣ��Item����

	QStatusBar *searchInfoStatusBar;	// ��ʾ������Ϣ��״̬��
	QMenu *itemMenu;					// �Ҽ������ťʱ�Ĳ˵�
	int num;							// ��ǰ������ϵĴ�����
	int diskNum;						// ��������

	QPushButton *aboutPushButton;

	QDialog *aboutDialog;
	QLabel *aboutTextLabel;

	std::vector<FILE_ENTRY> res[ALPHABETNUM];

	QMutex lock;

private:
	void initUI(void);			// ��ʼ������
	void initSearchWork(void);	// ��ʼ�������߳�

signals:

private slots:
	void updateItemModel(int);
	void showMenuClickedSlots(const QPoint&);

	void onAboutPushButtonClicked(void);
};