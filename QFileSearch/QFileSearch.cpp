#include "QFileSearch.h"

const char* COLTAG[] = { "文件名" ,"文件夹" ,"大小" ,"修改时间" };
const int COLWIDTH[] = { 200, 300, 80, 200 };
const char* FILESIZEUNIT[] = { " B", " KB", " MB", " G" };
const char* FORMAT = "%u年%u月%u日 %u时%u分%u秒";

QFileSearch::QFileSearch(QWidget *parent)
	: QMainWindow(parent)
{
	QMutexLocker locker(&lock);
	initUI();
	initSearchWork();
}

void QFileSearch::initUI(void)
{
	// 固定窗口大小
	setMinimumSize(WINDOWWIDTH, WINDOWHEIGHT);
	setMaximumSize(WINDOWWIDTH, WINDOWHEIGHT);

	// 初始化Item菜单
	itemMenu = nullptr;

	// 初始化虚拟滑轮框
	fakeScrollArea = new QScrollArea(this);
	fakeScrollArea->setGeometry(0, OFFSET + (GAP << 1), WINDOWWIDTH, WINDOWHEIGHT - (OFFSET * 2 + (GAP << 1)));
	fakeScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);

	// 初始化搜索栏
	searchLineEdit = new QLineEdit(this);
	searchLineEdit->setGeometry(GAP, GAP, WINDOWWIDTH - (GAP << 1), OFFSET);
	QRegExp rx("[a-z A-Z 0-9 .-|#*$?]{25}");	// 限制输入
	QRegExpValidator *pRevalidotor = new QRegExpValidator(rx, this);
	searchLineEdit->setValidator(new QRegExpValidator(rx, this));
	searchLineEdit->setDisabled(true);

	// 初始化item容器
	fileItemModel = new QStandardItemModel(this);
	fileItemModel->setColumnCount(COLNUM);

	// 初始化显示文件列表框
	fileListTableView = new QTableView(fakeScrollArea);
	fileListTableView->setGeometry(0, 0, WINDOWWIDTH, WINDOWHEIGHT - ((OFFSET + GAP) * 2));
	fileListTableView->setModel(fileItemModel);
	fileListTableView->horizontalHeader()->setDefaultAlignment(Qt::AlignCenter);	// 字体居中
	fileListTableView->verticalHeader()->setDefaultSectionSize(20);					// 固定行高度为6
	fileListTableView->verticalHeader()->setVisible(false);							// 列表头不可见
	fileListTableView->setShowGrid(true);											// 表中网格线可见
	fileListTableView->setEditTriggers(QAbstractItemView::NoEditTriggers);			// 不可更改表中内容
	fileListTableView->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(fileListTableView, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(showMenuClickedSlots(const QPoint&)));
	// 添加行头
	for (int i = 0; i < COLNUM; ++i) {
		fileItemModel->setHeaderData(i, Qt::Horizontal, QString::fromLocal8Bit(COLTAG[i]));
		fileListTableView->setColumnWidth(i, COLWIDTH[i]);
	}

	// 初始化状态栏
	searchInfoStatusBar = new QStatusBar(this);
	searchInfoStatusBar->setGeometry(0, WINDOWHEIGHT - OFFSET, WINDOWWIDTH, OFFSET);
	searchInfoStatusBar->showMessage("Initializing, please wait ^_^");

	// 初始化关于按钮
	aboutPushButton = new QPushButton(this);
	aboutPushButton->setText(QString::fromLocal8Bit("关于"));
	aboutPushButton->setGeometry(WINDOWWIDTH - BUTTONWIDTH, WINDOWHEIGHT - BUTTONHEIGHT, BUTTONWIDTH, BUTTONHEIGHT);
	connect(aboutPushButton, SIGNAL(clicked(void)), this, SLOT(onAboutPushButtonClicked(void)));

	// 初始化监控窗口
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
	// 获取磁盘的数量
	for (char diskName = 'C'; diskName <= 'Z'; ++diskName)
	{
		Volume *v = new Volume(diskName);
		if (v->isNTFS())
		{
			diskNum++;
		}
	}
	num = diskNum - 1;
	// 扫描每一个盘
	for (char diskName = 'C'; diskName <= 'Z'; ++diskName)
	{
		Volume *v = new Volume(diskName);
		// 如果当前盘存在并且是 NTFS 结构就增加一个线程
		if (v->isNTFS())	// 可以用 QStorageInfo 来判断
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

			connect(searchLineEdit, &QLineEdit::textChanged, v, &Volume::search);	// 当搜索栏改变时就搜索

			// 避免输入速度过快
			connect(searchLineEdit, &QLineEdit::textChanged, this, [=]() {
				searchLineEdit->blockSignals(true);
			});


			connect(v, &Volume::sigInitEnd, this, [=]() {
				if ((diskNum - 1) == num)
				{
					searchLineEdit->setDisabled(false);
				}
			});

			// 更新文件列表
			connect(fakeScrollArea->verticalScrollBar(), &QScrollBar::valueChanged, this, &QFileSearch::updateItemModel);

			connect(v, &Volume::sigSearchEnd, this, [=]() {	// 搜索完毕发送信号
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
					fakeScrollArea->verticalScrollBar()->setMaximum((sum > ROWNUM) ? sum - ROWNUM : 0);	// 设置滚动条的最大值
					fakeScrollArea->verticalScrollBar()->setSliderPosition(0);							// 让滚动条回到最上方
					updateItemModel(0);	// 更新文件列表
					updateItemModel(0);	// 更新文件列表
				}
			});
			thread->start();
		}
	}
}

void QFileSearch::updateItemModel(int pos)
{
	while (fileItemModel->removeRow(1));	// 清空之前的列表
	Singleton& fileArr = Singleton::get_instance();
	int sum = 0;	// 获取文件的总数
	for (int i = 0; i < ALPHABETNUM; ++i)
	{
		sum += fileArr.res[i].size();
	}
	searchInfoStatusBar->showMessage("find: " + QString::number(sum) + " files");
	QStandardItem *items[COLNUM];
	int dn = 0;	// 表示第几个磁盘
	int j = 1;
	for (int count = 0; count < ROWNUM && count < sum; ++count, ++pos, ++j)
	{
		while (dn < ALPHABETNUM && pos >= fileArr.res[dn].size())	// 找到 pos 对应的元素位置在哪个盘
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
		swprintf(fullPath, MAX_PATH, L"%S", tmp.c_str());							// 注意大写
		GET_FILEEX_INFO_LEVELS fInfoLevelId = GetFileExInfoStandard;				// 获取句柄
		WIN32_FILE_ATTRIBUTE_DATA FileInformation;									// 存储文件信息
		BOOL bGet = GetFileAttributesEx(fullPath, fInfoLevelId, &FileInformation);	// 获取文件信息
		SYSTEMTIME sysTime;															// 系统时间
		FileTimeToSystemTime(&FileInformation.ftCreationTime, &sysTime);			// 将文件时间转换为本地系统时间

		QString fileSize = "";
		if (0 != FileInformation.nFileSizeLow)		// 处理文件大小
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
		if (0xffff != sysTime.wYear)		// 处理文件时间的显示
		{
			char ft[0x3f];
			sprintf(ft, FORMAT, sysTime.wYear, sysTime.wMonth, sysTime.wDay, sysTime.wHour, sysTime.wMinute, sysTime.wSecond);
			fileTime = QString::fromLocal8Bit(ft);
		}
		// 文件名 文件路径 文件大小 文件日期
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
	int row = fileListTableView->verticalHeader()->logicalIndexAt(pos);	// 获取当前点击的Item位置

	if (-1 != row)
	{
		if (nullptr != itemMenu)	// 避免占用内存
		{
			delete itemMenu;
		}

		itemMenu = new QMenu(this);

		QAction *openFile = itemMenu->addAction(QString::fromLocal8Bit("打开文件"));
		QAction *openPath = itemMenu->addAction(QString::fromLocal8Bit("打开路径"));
		QAction *copyPath = itemMenu->addAction(QString::fromLocal8Bit("复制文件完整路径和文件名"));

		connect(openFile, &QAction::triggered, this, [=]() {
			QDesktopServices::openUrl(QUrl::fromLocalFile(fileItemModel->item(row, 1)->text() + fileItemModel->item(row, 0)->text()));
		});

		connect(openPath, &QAction::triggered, this, [=]() {
			QDesktopServices::openUrl(QUrl::fromLocalFile(fileItemModel->item(row, 1)->text()));
		});

		connect(copyPath, &QAction::triggered, this, [=]() {
			QClipboard *clip = QApplication::clipboard();		// 获取剪切板
			clip->setText(fileItemModel->item(row, 1)->text() + fileItemModel->item(row, 0)->text());
		});

		itemMenu->exec(QCursor::pos());	//在当前鼠标位置显示
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
	//计算显示原点
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
