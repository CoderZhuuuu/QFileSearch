#include "QFileSearch.h"

const char* COLTAG[] = { "�ļ���" ,"�ļ���" ,"��С" ,"�޸�ʱ��" };
const int COLWIDTH[] = { 200, 300, 80, 200 };
const char* FILESIZEUNIT[] = { " B", " KB", " MB", " G" };
const char* FORMAT = "%u��%u��%u�� %uʱ%u��%u��";

QFileSearch::QFileSearch(QWidget *parent)
	: QMainWindow(parent)
{
	QMutexLocker locker(&lock);
	initUI();
	initSearchWork();
}

void QFileSearch::initUI(void)
{
	// �̶����ڴ�С
	setMinimumSize(WINDOWWIDTH, WINDOWHEIGHT);
	setMaximumSize(WINDOWWIDTH, WINDOWHEIGHT);

	// ��ʼ��Item�˵�
	itemMenu = nullptr;

	// ��ʼ�����⻬�ֿ�
	fakeScrollArea = new QScrollArea(this);
	fakeScrollArea->setGeometry(0, OFFSET + (GAP << 1), WINDOWWIDTH, WINDOWHEIGHT - (OFFSET * 2 + (GAP << 1)));
	fakeScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);

	// ��ʼ��������
	searchLineEdit = new QLineEdit(this);
	searchLineEdit->setGeometry(GAP, GAP, WINDOWWIDTH - (GAP << 1), OFFSET);
	QRegExp rx("[a-z A-Z 0-9 .-|#*$?]{25}");	// ��������
	QRegExpValidator *pRevalidotor = new QRegExpValidator(rx, this);
	searchLineEdit->setValidator(new QRegExpValidator(rx, this));
	searchLineEdit->setDisabled(true);

	// ��ʼ��item����
	fileItemModel = new QStandardItemModel(this);
	fileItemModel->setColumnCount(COLNUM);

	// ��ʼ����ʾ�ļ��б��
	fileListTableView = new QTableView(fakeScrollArea);
	fileListTableView->setGeometry(0, 0, WINDOWWIDTH, WINDOWHEIGHT - ((OFFSET + GAP) * 2));
	fileListTableView->setModel(fileItemModel);
	fileListTableView->horizontalHeader()->setDefaultAlignment(Qt::AlignCenter);	// �������
	fileListTableView->verticalHeader()->setDefaultSectionSize(20);					// �̶��и߶�Ϊ6
	fileListTableView->verticalHeader()->setVisible(false);							// �б�ͷ���ɼ�
	fileListTableView->setShowGrid(true);											// ���������߿ɼ�
	fileListTableView->setEditTriggers(QAbstractItemView::NoEditTriggers);			// ���ɸ��ı�������
	fileListTableView->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(fileListTableView, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(showMenuClickedSlots(const QPoint&)));
	// �����ͷ
	for (int i = 0; i < COLNUM; ++i) {
		fileItemModel->setHeaderData(i, Qt::Horizontal, QString::fromLocal8Bit(COLTAG[i]));
		fileListTableView->setColumnWidth(i, COLWIDTH[i]);
	}

	// ��ʼ��״̬��
	searchInfoStatusBar = new QStatusBar(this);
	searchInfoStatusBar->setGeometry(0, WINDOWHEIGHT - OFFSET, WINDOWWIDTH, OFFSET);
	searchInfoStatusBar->showMessage("Initializing, please wait ^_^");

	// ��ʼ�����ڰ�ť
	aboutPushButton = new QPushButton(this);
	aboutPushButton->setText(QString::fromLocal8Bit("����"));
	aboutPushButton->setGeometry(WINDOWWIDTH - BUTTONWIDTH, WINDOWHEIGHT - BUTTONHEIGHT, BUTTONWIDTH, BUTTONHEIGHT);
	connect(aboutPushButton, SIGNAL(clicked(void)), this, SLOT(onAboutPushButtonClicked(void)));

	// ��ʼ����ش���
	ULONG m_ulSHChangeNotifyRegister = NULL;
	SHChangeNotifyEntry shCNF = { 0 };
	shCNF.pidl = NULL;;
	shCNF.fRecursive = TRUE;
	m_ulSHChangeNotifyRegister = SHChangeNotifyRegister((HWND)winId(),
		SHCNRF_ShellLevel,
		SHCNE_ALLEVENTS,
		WM_FILEMODIFY,
		1,
		&shCNF);

	filter = new QEventFilter;
}

void QFileSearch::initSearchWork(void)
{
	diskNum = 0;
	// ��ȡ���̵�����
	for (char diskName = 'C'; diskName <= 'Z'; ++diskName)
	{
		Volume *v = new Volume(diskName);
		if (v->isNTFS())
		{
			diskNum++;
		}
	}
	num = diskNum - 1;
	// ɨ��ÿһ����
	for (char diskName = 'C'; diskName <= 'Z'; ++diskName)
	{
		Volume *v = new Volume(diskName);
		// �����ǰ�̴��ڲ����� NTFS �ṹ������һ���߳�
		if (v->isNTFS())	// ������ QStorageInfo ���ж�
		{
			QThread *thread = new QThread;
			v->moveToThread(thread);

			connect(thread, &QThread::started, v, &Volume::initUSN);
			/*connect(filter, SIGNAL(sigCreate(QString)), v, SLOT(sltCreate(QString)));
			connect(filter, SIGNAL(sigDelete(QString)), v, SLOT(sltDelete(QString)));
			connect(filter, SIGNAL(sigCreate(QString, QString)), v, SLOT(sltRename(QString, QString)));*/

			connect(this, &QFileSearch::destroy, this, [=]() {
				thread->wait();
				thread->quit();
				lock.lock();
				v->deleteLater();
				thread->deleteLater();
				lock.unlock();
			});

			connect(searchLineEdit, &QLineEdit::textChanged, v, &Volume::search);	// ���������ı�ʱ������

			// ���������ٶȹ���
			connect(searchLineEdit, &QLineEdit::textChanged, this, [=]() {
				searchLineEdit->blockSignals(true);
			});


			connect(v, &Volume::sigInitEnd, this, [=]() {
				if ((diskNum - 1) == num)
				{
					searchLineEdit->setDisabled(false);
				}
			});

			// �����ļ��б�
			connect(fakeScrollArea->verticalScrollBar(), &QScrollBar::valueChanged, this, &QFileSearch::updateItemModel);

			connect(v, &Volume::sigSearchEnd, this, [=]() {	// ������Ϸ����ź�
				lock.lock();
				num = (num + 1) % diskNum;
				lock.unlock();
				if ((diskNum - 1) == num)
				{
					searchLineEdit->blockSignals(false);
					Singleton& fileArr = Singleton::get_instance();
					int sum = 0;
					for (int i = 0; i < ALPHABETNUM; ++i)
					{
						sum += fileArr.res[i].size();
					}
					fakeScrollArea->verticalScrollBar()->setMaximum((sum > ROWNUM) ? sum - ROWNUM : 0);	// ���ù����������ֵ
					fakeScrollArea->verticalScrollBar()->setSliderPosition(0);							// �ù������ص����Ϸ�
					updateItemModel(0);	// �����ļ��б�
					updateItemModel(0);	// �����ļ��б�
				}
			});
			thread->start();
		}
	}
}

void QFileSearch::updateItemModel(int pos)
{
	while (fileItemModel->removeRow(1));	// ���֮ǰ���б�
	Singleton& fileArr = Singleton::get_instance();
	int sum = 0;	// ��ȡ�ļ�������
	for (int i = 0; i < ALPHABETNUM; ++i)
	{
		sum += fileArr.res[i].size();
	}
	searchInfoStatusBar->showMessage("find: " + QString::number(sum) + " files");
	QStandardItem *items[COLNUM];
	int dn = 0;	// ��ʾ�ڼ�������
	int j = 1;
	for (int count = 0; count < ROWNUM && count < sum; ++count, ++pos, ++j)
	{
		while (dn < ALPHABETNUM && pos >= fileArr.res[dn].size())	// �ҵ� pos ��Ӧ��Ԫ��λ�����ĸ���
		{
			pos = pos - fileArr.res[dn].size();
			dn++;
		}
		if (dn == ALPHABETNUM)
		{
			break;
		}

		std::string tmp = fileArr.res[dn].at(pos).filePath + fileArr.res[dn].at(pos).fileName;
		wchar_t fullPath[MAX_PATH];
		swprintf(fullPath, MAX_PATH, L"%S", tmp.c_str());							// ע���д
		GET_FILEEX_INFO_LEVELS fInfoLevelId = GetFileExInfoStandard;				// ��ȡ���
		WIN32_FILE_ATTRIBUTE_DATA FileInformation;									// �洢�ļ���Ϣ
		BOOL bGet = GetFileAttributesEx(fullPath, fInfoLevelId, &FileInformation);	// ��ȡ�ļ���Ϣ
		SYSTEMTIME sysTime;															// ϵͳʱ��
		FileTimeToSystemTime(&FileInformation.ftCreationTime, &sysTime);			// ���ļ�ʱ��ת��Ϊ����ϵͳʱ��

		QString fileSize = "";
		if (0 != FileInformation.nFileSizeLow)		// �����ļ���С
		{
			int sCount = 0;
			while (FileInformation.nFileSizeLow > 1024)
			{
				sCount++;
				FileInformation.nFileSizeLow >>= 10;
			}
			fileSize = QString::number(FileInformation.nFileSizeLow) + FILESIZEUNIT[sCount];
		}

		QString fileTime = "";
		if (0xffff != sysTime.wYear)		// �����ļ�ʱ�����ʾ
		{
			char ft[0x3f];
			sprintf(ft, FORMAT, sysTime.wYear, sysTime.wMonth, sysTime.wDay, sysTime.wHour, sysTime.wMinute, sysTime.wSecond);
			fileTime = QString::fromLocal8Bit(ft);
		}
		// �ļ��� �ļ�·�� �ļ���С �ļ�����
		items[0] = new QStandardItem(QString::fromStdString(fileArr.res[dn].at(pos).fileName));
		items[0]->setEditable(false);
		items[1] = new QStandardItem(QString::fromStdString(fileArr.res[dn].at(pos).filePath));
		items[1]->setEditable(false);
		items[2] = new QStandardItem(fileSize);
		items[2]->setEditable(false);
		items[2]->setTextAlignment(Qt::AlignRight);
		items[3] = new QStandardItem(fileTime);
		items[3]->setEditable(false);
		items[3]->setTextAlignment(Qt::AlignRight);
		fileItemModel->setItem(j, 0, items[0]);
		fileItemModel->setItem(j, 1, items[1]);
		fileItemModel->setItem(j, 2, items[2]);
		fileItemModel->setItem(j, 3, items[3]);
	}
}

void QFileSearch::showMenuClickedSlots(const QPoint& pos)
{
	int row = fileListTableView->verticalHeader()->logicalIndexAt(pos);	// ��ȡ��ǰ�����Itemλ��

	if (-1 != row)
	{
		if (nullptr != itemMenu)	// ����ռ���ڴ�
		{
			delete itemMenu;
		}

		itemMenu = new QMenu(this);

		QAction *openFile = itemMenu->addAction(QString::fromLocal8Bit("���ļ�"));
		QAction *openPath = itemMenu->addAction(QString::fromLocal8Bit("��·��"));
		QAction *copyPath = itemMenu->addAction(QString::fromLocal8Bit("�����ļ�����·�����ļ���"));

		connect(openFile, &QAction::triggered, this, [=]() {
			QDesktopServices::openUrl(QUrl::fromLocalFile(fileItemModel->item(row, 1)->text() + fileItemModel->item(row, 0)->text()));
		});

		connect(openPath, &QAction::triggered, this, [=]() {
			QDesktopServices::openUrl(QUrl::fromLocalFile(fileItemModel->item(row, 1)->text()));
		});

		connect(copyPath, &QAction::triggered, this, [=]() {
			QClipboard *clip = QApplication::clipboard();		// ��ȡ���а�
			clip->setText(fileItemModel->item(row, 1)->text() + fileItemModel->item(row, 0)->text());
		});

		itemMenu->exec(QCursor::pos());	//�ڵ�ǰ���λ����ʾ
	}
}

void QFileSearch::onAboutPushButtonClicked(void)
{
	aboutDialog = new QDialog(this);
	aboutDialog->setAttribute(Qt::WA_DeleteOnClose, true);
	aboutDialog->setMinimumSize(DIALOGWIDTH, DIALOGHEIGHT);
	aboutDialog->setMaximumSize(DIALOGWIDTH, DIALOGHEIGHT);
	aboutDialog->setWindowTitle("About");
	aboutDialog->show();
	//������ʾԭ��
	QRect rect = geometry();
	int x = rect.x() + ((WINDOWWIDTH - DIALOGWIDTH) >> 1);
	int y = rect.y() + ((WINDOWHEIGHT - DIALOGHEIGHT) >> 1);
	aboutDialog->move(x, y);
	aboutTextLabel = new QLabel(aboutDialog);
	aboutTextLabel->setAttribute(Qt::WA_DeleteOnClose, true);
	aboutTextLabel->setText(ABOUTTEXT);
	aboutTextLabel->setGeometry(20, 20, 400, 300);
	aboutTextLabel->setAlignment(Qt::AlignLeft | Qt::AlignTop);
	aboutTextLabel->show();
}
