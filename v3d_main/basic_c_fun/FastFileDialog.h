#ifndef VAA3D_FAST_FILE_DIALOG_H
#define VAA3D_FAST_FILE_DIALOG_H
#include <QFileDialog>
#if defined(Q_OS_LINUX) && defined(USE_Qt5)
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QListWidget>
#include <QScrollBar>
#include <QWheelEvent>
#include <QLabel>
#include <QComboBox>
#include <QCheckBox>
#include <QTimer>
#include <QSettings>
#include <QRegExp>
#include <QStyle>
#include <QFileInfo>
#include <QDir>
#include <QFile>
#include <atomic>
#include <memory>
#include <thread>
#include <algorithm>
#include <vector>
#include <dirent.h>
#include <cerrno>
#include <cstring>

// Large directories must not create one widget item/icon/stat request per file.
// Only names and entry types are scanned on a worker; the UI renders one page.
class FastFileDialog : public QDialog
{
    struct Entry {QString name;bool directory;};
    struct Scan {
        std::atomic<bool> cancelled{false},done{false};
        std::vector<Entry> entries;
        QString error;
    };
    std::shared_ptr<Scan> scan;
    std::vector<Entry> entries;
    QVector<int> matches;
    bool directoriesOnly;
    QString currentPath,selectedPath;
    QLineEdit *pathEdit,*searchEdit,*fileEdit;
    QListWidget *list;
    QLabel *status;
    QComboBox *filterBox,*pathHistory,*fileHistory;
    QCheckBox *hidden;
    QPushButton *previous,*next;
    QTimer *poll,*searchDelay;
    int page=0;
    const int pageSize=500;
    QIcon folderIcon,fileIcon;

    static void remember(const QString &key,const QString &path) {
        QSettings settings("HHMI","Vaa3D");
        QStringList paths=settings.value(key).toStringList();
        paths.removeAll(path);paths.prepend(path);
        while(paths.size()>20)paths.removeLast();
        settings.setValue(key,paths);
    }
    static QComboBox *historyBox(const QString &key,const QString &name) {
        auto box=new QComboBox;box->setObjectName(name);box->setEditable(true);
        box->setInsertPolicy(QComboBox::NoInsert);box->setMaxVisibleItems(12);
        box->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Fixed);
        box->setMinimumContentsLength(30);
        box->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
        QSettings settings("HHMI","Vaa3D");box->addItems(settings.value(key).toStringList());
        box->setCurrentIndex(-1);
        return box;
    }
    QString absolute(const QString &text) const {
        QString p=text;
        if(p=="~")p=QDir::homePath();
        else if(p.startsWith("~/"))p=QDir::homePath()+p.mid(1);
        return QDir::cleanPath(QFileInfo(p).isAbsolute()?p:QDir(currentPath).absoluteFilePath(p));
    }
    void choose() {
        QString path=directoriesOnly?absolute(pathEdit->text()):absolute(fileEdit->text());
        if(!directoriesOnly && fileEdit->text().isEmpty()) {status->setText(QString::fromUtf8("请选择文件，或输入完整文件路径。"));return;}
        if(directoriesOnly && path==currentPath && list->currentItem()) {
            const Entry &entry=entries[list->currentItem()->data(Qt::UserRole).toInt()];
            if(entry.directory)path=QDir(currentPath).absoluteFilePath(entry.name);
        }
        QFileInfo info(path);
        if(!info.exists()) {status->setText(QString::fromUtf8("路径不存在：")+path);return;}
        if(!directoriesOnly && info.isDir()) {navigate(path);return;}
        if((directoriesOnly && !info.isDir()) || (!directoriesOnly && !info.isFile())) {
            status->setText(QString::fromUtf8("请选择有效的")+(directoriesOnly?QString::fromUtf8("文件夹"):QString::fromUtf8("文件")));return;
        }
        selectedPath=info.absoluteFilePath();
        QSettings settings("HHMI","Vaa3D");settings.setValue("FastFileDialog/lastDirectory",directoriesOnly?selectedPath:info.absolutePath());
        remember("FastFileDialog/recentOpenedDirectories",directoriesOnly?selectedPath:info.absolutePath());
        if(!directoriesOnly)remember("FastFileDialog/recentFiles",selectedPath);
        accept();
    }
    void navigate(const QString &input) {
        QString destination=absolute(input);
        QFileInfo info(destination);
        if(info.isFile() && !directoriesOnly) {fileEdit->setText(destination);choose();return;}
        if(!info.isDir()) {status->setText(QString::fromUtf8("无法打开目录：")+destination);return;}
        if(scan)scan->cancelled=true;
        currentPath=destination;pathEdit->setText(destination);fileEdit->clear();
        entries.clear();matches.clear();list->clear();page=0;previous->setEnabled(false);next->setEnabled(false);
        setProperty("directoryLoaded",false);
        status->setText(QString::fromUtf8("正在读取目录，可直接输入路径或取消…"));
        scan=std::make_shared<Scan>();auto state=scan;
        const bool showHidden=hidden->isChecked(),onlyDirs=directoriesOnly;
        const QByteArray native=QFile::encodeName(destination);
        std::thread([state,native,showHidden,onlyDirs](){
            DIR *dir=opendir(native.constData());
            if(!dir) {state->error=QString::fromLocal8Bit(std::strerror(errno));state->done=true;return;}
            while(!state->cancelled) {
                errno=0;dirent *item=readdir(dir);
                if(!item) {if(errno)state->error=QString::fromLocal8Bit(std::strerror(errno));break;}
                if(!std::strcmp(item->d_name,".")||!std::strcmp(item->d_name,".."))continue;
                if(!showHidden && item->d_name[0]=='.')continue;
                bool isDir=item->d_type==DT_DIR;
                // Local filesystems supply d_type. Only ambiguous entries need stat.
                if(item->d_type==DT_UNKNOWN || item->d_type==DT_LNK)
                    isDir=QFileInfo(QFile::decodeName(native)+"/"+QFile::decodeName(item->d_name)).isDir();
                if(onlyDirs && !isDir)continue;
                state->entries.push_back({QFile::decodeName(item->d_name),isDir});
            }
            closedir(dir);
            if(!state->cancelled)std::sort(state->entries.begin(),state->entries.end(),[](const Entry&a,const Entry&b){
                if(a.directory!=b.directory)return a.directory;
                return a.name<b.name;
            });
            state->done=true;
        }).detach();
        poll->start();
    }
    void filter() {
        if(scan && !scan->done)return;
        matches.clear();page=0;
        QString patterns=filterBox->currentText();int begin=patterns.lastIndexOf('('),end=patterns.lastIndexOf(')');
        patterns=begin>=0 && end>begin?patterns.mid(begin+1,end-begin-1):"*";
        const bool matchAll=patterns=="*";
        QList<QRegExp> rules;
        for(const QString &p:patterns.split(QRegExp("\\s+"),QString::SkipEmptyParts))
            rules.append(QRegExp(p,Qt::CaseInsensitive,QRegExp::Wildcard));
        const QString query=searchEdit->text();
        for(int i=0;i<int(entries.size());++i) {
            const Entry&e=entries[i];
            if(!e.name.contains(query,Qt::CaseInsensitive))continue;
            bool allowed=e.directory||directoriesOnly||matchAll;
            if(!allowed)for(const QRegExp&rule:rules)if(rule.exactMatch(e.name)){allowed=true;break;}
            if(allowed)matches.append(i);
        }
        renderPage();
    }
    void renderPage() {
        list->clear();int first=page*pageSize,last=qMin(first+pageSize,matches.size());
        for(int i=first;i<last;++i) {
            const Entry&e=entries[matches[i]];
            auto item=new QListWidgetItem(e.directory?folderIcon:fileIcon,e.name,list);
            item->setData(Qt::UserRole,matches[i]);
        }
        previous->setEnabled(page>0);next->setEnabled(last<matches.size());
        status->setText(QString::fromUtf8("共 %1 项 · 第 %2 / %3 页（每页 %4 项）").arg(matches.size()).arg(page+1).arg(qMax(1,(matches.size()+pageSize-1)/pageSize)).arg(pageSize));
        setProperty("matchingEntries",matches.size());
    }
protected:
    bool eventFilter(QObject *object,QEvent *event) override {
        if(object==list->viewport() && event->type()==QEvent::Wheel) {
            auto wheel=static_cast<QWheelEvent*>(event);
            const int delta=wheel->pixelDelta().isNull()?wheel->angleDelta().y():wheel->pixelDelta().y();
            auto bar=list->verticalScrollBar();
            // Let Qt scroll normally within a page; cross pages only at an edge.
            if(delta<0 && bar->value()==bar->maximum() && next->isEnabled()) {
                ++page;renderPage();list->scrollToTop();wheel->accept();return true;
            }
            if(delta>0 && bar->value()==bar->minimum() && previous->isEnabled()) {
                --page;renderPage();list->scrollToBottom();wheel->accept();return true;
            }
        }
        return QDialog::eventFilter(object,event);
    }
public:
    FastFileDialog(QWidget *parent,const QString &title,const QString &directory,const QString &nameFilter,bool onlyDirs):QDialog(parent),directoriesOnly(onlyDirs) {
        setObjectName("fastFileDialog");setWindowTitle(title.isEmpty()?QString::fromUtf8("选择文件"):title);resize(820,580);
        currentPath=QDir::homePath();
        // The legacy history mixed confirmed selections with every browsed directory.
        // Seed the new history only from known selections; retain the old key for recovery.
        QSettings historySettings("HHMI","Vaa3D");
        const QString historyKey="FastFileDialog/recentOpenedDirectories";
        if(!historySettings.contains(historyKey)) {
            QStringList confirmed;
            const QString last=historySettings.value("FastFileDialog/lastDirectory").toString();
            if(!last.isEmpty())confirmed<<last;
            for(const QString &file:historySettings.value("FastFileDialog/recentFiles").toStringList()) {
                if(file.isEmpty())continue;
                const QString folder=QFileInfo(file).absolutePath();
                if(!confirmed.contains(folder))confirmed<<folder;
            }
            historySettings.setValue(historyKey,QStringList(confirmed.mid(0,20)));
        }
        auto layout=new QVBoxLayout(this);auto navigation=new QHBoxLayout;layout->addLayout(navigation);
        auto up=new QPushButton(QString::fromUtf8("上级"));auto home=new QPushButton(QString::fromUtf8("主目录"));
        pathHistory=historyBox("FastFileDialog/recentOpenedDirectories","fastPathHistory");pathEdit=pathHistory->lineEdit();pathEdit->setObjectName("fastPath");auto go=new QPushButton(QString::fromUtf8("转到"));
        navigation->addWidget(up);navigation->addWidget(home);navigation->addWidget(pathHistory,1);navigation->addWidget(go);
        auto options=new QHBoxLayout;layout->addLayout(options);
        searchEdit=new QLineEdit;searchEdit->setObjectName("fastSearch");searchEdit->setPlaceholderText(QString::fromUtf8("搜索当前目录中的文件名"));
        hidden=new QCheckBox(QString::fromUtf8("显示隐藏项"));options->addWidget(searchEdit,1);options->addWidget(hidden);
        list=new QListWidget;list->setObjectName("fastEntries");list->setUniformItemSizes(true);
        list->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
        list->viewport()->installEventFilter(this);layout->addWidget(list,1);
        folderIcon=style()->standardIcon(QStyle::SP_DirIcon);fileIcon=style()->standardIcon(QStyle::SP_FileIcon);
        auto pages=new QHBoxLayout;layout->addLayout(pages);status=new QLabel;status->setObjectName("fastStatus");status->setWordWrap(true);
        previous=new QPushButton(QString::fromUtf8("上一页"));next=new QPushButton(QString::fromUtf8("下一页"));previous->setObjectName("fastPrevious");next->setObjectName("fastNext");pages->addWidget(status,1);pages->addWidget(previous);pages->addWidget(next);
        fileHistory=historyBox("FastFileDialog/recentFiles","fastFileHistory");fileEdit=fileHistory->lineEdit();fileEdit->setObjectName("fastFilename");fileEdit->setPlaceholderText(QString::fromUtf8("文件名或完整路径"));layout->addWidget(fileHistory);fileHistory->setVisible(!onlyDirs);
        filterBox=new QComboBox;filterBox->setObjectName("fastFilter");filterBox->addItems(nameFilter.isEmpty()?QStringList(QString::fromUtf8("所有文件 (*)")):nameFilter.split(";;"));layout->addWidget(filterBox);filterBox->setVisible(!onlyDirs);
        auto actions=new QHBoxLayout;layout->addLayout(actions);actions->addStretch();
        auto open=new QPushButton(onlyDirs?QString::fromUtf8("选择此文件夹"):QString::fromUtf8("打开"));open->setObjectName("fastOpen");auto cancel=new QPushButton(QString::fromUtf8("取消"));actions->addWidget(open);actions->addWidget(cancel);
        // Return is handled by each path field, rather than accidentally accepting a directory.
        for(auto button:findChildren<QPushButton*>())button->setAutoDefault(false);
        poll=new QTimer(this);poll->setInterval(25);searchDelay=new QTimer(this);searchDelay->setSingleShot(true);searchDelay->setInterval(120);
        connect(poll,&QTimer::timeout,this,[this](){
            if(!scan || !scan->done)return;poll->stop();
            if(!scan->error.isEmpty()){status->setText(QString::fromUtf8("读取失败：")+scan->error);return;}
            entries=std::move(scan->entries);filter();setProperty("directoryLoaded",true);
        });
        connect(pathHistory,static_cast<void(QComboBox::*)(int)>(&QComboBox::activated),this,[this](int){navigate(pathEdit->text());});
        // Selecting a recent file fills the field; Open/Return confirms the choice.
        connect(go,&QPushButton::clicked,this,[this](){navigate(pathEdit->text());});
        connect(pathEdit,&QLineEdit::returnPressed,this,[this](){navigate(pathEdit->text());});
        connect(up,&QPushButton::clicked,this,[this](){navigate(QDir(currentPath).absoluteFilePath(".."));});
        connect(home,&QPushButton::clicked,this,[this](){navigate(QDir::homePath());});
        connect(hidden,&QCheckBox::toggled,this,[this](bool){navigate(currentPath);});
        connect(searchEdit,&QLineEdit::textChanged,this,[this](const QString&){searchDelay->start();});
        connect(searchDelay,&QTimer::timeout,this,[this](){filter();});
        connect(filterBox,static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),this,[this](int){filter();});
        connect(previous,&QPushButton::clicked,this,[this](){--page;renderPage();});
        connect(next,&QPushButton::clicked,this,[this](){++page;renderPage();});
        connect(list,&QListWidget::itemClicked,this,[this](QListWidgetItem*item){const Entry&e=entries[item->data(Qt::UserRole).toInt()];if(!e.directory)fileEdit->setText(e.name);else fileEdit->clear();});
        connect(list,&QListWidget::itemActivated,this,[this](QListWidgetItem*item){Entry e=entries[item->data(Qt::UserRole).toInt()];if(e.directory)navigate(QDir(currentPath).absoluteFilePath(e.name));else{fileEdit->setText(e.name);choose();}});
        connect(open,&QPushButton::clicked,this,[this](){choose();});connect(fileEdit,&QLineEdit::returnPressed,this,[this](){choose();});connect(cancel,&QPushButton::clicked,this,&QDialog::reject);
        QSettings settings("HHMI","Vaa3D");QString initial=directory.isEmpty()?settings.value("FastFileDialog/lastDirectory",QDir::homePath()).toString():directory;
        QFileInfo info(initial);QString file;
        if(info.isFile()){file=info.fileName();initial=info.absolutePath();}
        if(!QFileInfo(initial).isDir())initial=QDir::homePath();
        navigate(initial);fileEdit->setText(file);
    }
    ~FastFileDialog(){if(scan)scan->cancelled=true;}
    QString selectedFile()const{return selectedPath;}
    static QString getOpenFileName(QWidget *parent=0,const QString &title=QString(),const QString &directory=QString(),const QString &filter=QString()) {
        FastFileDialog dialog(parent,title,directory,filter,false);return dialog.exec()==QDialog::Accepted?dialog.selectedPath:QString();
    }
    static QString getExistingDirectory(QWidget *parent=0,const QString &title=QString(),const QString &directory=QString(),QFileDialog::Options=QFileDialog::ShowDirsOnly) {
        FastFileDialog dialog(parent,title,directory,QString(),true);return dialog.exec()==QDialog::Accepted?dialog.selectedPath:QString();
    }
};
#else
// Keep the platform chooser on unsupported build targets.
typedef QFileDialog FastFileDialog;
#endif
#endif
