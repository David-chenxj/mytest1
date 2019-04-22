#include <QGuiApplication>
#include <qtwebengineglobal.h>
#include <QQmlApplicationEngine>
#include <QtCore>
#include <QScreen>
#include <iostream>
#include <QDebug>
#include <windows.h>
#include <DbgHelp.h>
// #include "json.hpp"
#include "getversion.h"
#include "useroperationdb.h"
#include "datainteraction.h"
#include "ditdef.h"
#include "datasettings.h"
#include "geturlreplycontent.h"
#include "fpdataclient.h"
#include "baseutility.h"
#include "bkdata.h"
#include "drawline.h"

using namespace std;
#if defined Q_OS_WIN32   //for win
#include <windows.h>

bool checkPrgIsRunning()
{
    //  创建互斥量
    HANDLE m_hMutex  =  CreateMutex(NULL, FALSE,  L"mutex_single" );
    //  检查错误代码
    if  (GetLastError()  ==  ERROR_ALREADY_EXISTS)  {
      //  如果已有互斥量存在则释放句柄并复位互斥量
     CloseHandle(m_hMutex);
     m_hMutex  =  NULL;
      //  程序退出
      return  true;
    }
    else
        return false;
}
#else
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
bool checkPrgIsRunning()
{
    const char filename[]  = "/tmp/lockfile";
    int fd = open (filename, O_WRONLY | O_CREAT , 0644);
    int flock = lockf(fd, F_TLOCK, 0 );
    if (fd == -1) {
            perror("open lockfile/n");
            return true;
    }
    //给文件加锁
    if (flock == -1) {
            perror("lock file error/n");
            return true;
    }
    //程序退出后，文件自动解锁
    return false;
}
#endif

// define the DataInteraction singleton type provider function (callback).
static QObject *datainteraction_singletontype_provider(QQmlEngine *engine, QJSEngine *scriptEngine)
{
    Q_UNUSED(engine)
    Q_UNUSED(scriptEngine)

    return DataInteraction::qmlInstance();
}
// define the BkData singleton type provider function (callback).
static QObject *bkdata_singletontype_provider(QQmlEngine *engine, QJSEngine *scriptEngine)
{
    Q_UNUSED(engine)
    Q_UNUSED(scriptEngine)
    return BkData::qmlInstance();
}

static QObject *fpdataclient_singletontype_provider(QQmlEngine *engine, QJSEngine *scriptEngine)
{
    Q_UNUSED(engine)
    Q_UNUSED(scriptEngine)

    return FPDataClient::GetInstance();
}
static QObject *datasettings_singletontype_provider(QQmlEngine *engine, QJSEngine *scriptEngine)
{
    Q_UNUSED(engine)
    Q_UNUSED(scriptEngine)

    return DataSettings::qmlInstance();
}
static QObject *userDb_singletontype_provider(QQmlEngine *engine, QJSEngine *scriptEngine)
{
    Q_UNUSED(engine)
    Q_UNUSED(scriptEngine)

    return UserOperationDb::getInstance();
}

static LONG ApplicationCrashHandler(EXCEPTION_POINTERS *pException){
{
        // 在程序exe的上级目录中创建dmp文件夹
        QDir *dmp = new QDir;
        bool exist = dmp->exists("../dmp/");
        if(exist == false)
            dmp->mkdir("../dmp/");
        }
        QDateTime current_date_time = QDateTime::currentDateTime();
        QString current_date = current_date_time.toString("yyyy_MM_dd_hh_mm_ss");
        QString time =  current_date + ".dmp";
          EXCEPTION_RECORD *record = pException->ExceptionRecord;
          QString errCode(QString::number(record->ExceptionCode, 16));
          QString errAddr(QString::number((uint)record->ExceptionAddress, 16));
          QString errFlag(QString::number(record->ExceptionFlags, 16));
          QString errPara(QString::number(record->NumberParameters, 16));
          qDebug()<<"errCode: "<<errCode;
          qDebug()<<"errAddr: "<<errAddr;
          qDebug()<<"errFlag: "<<errFlag;
          qDebug()<<"errPara: "<<errPara;
          HANDLE hDumpFile = CreateFile((LPCWSTR)QString("../dmp/" + time).utf16(),
                   GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
          if(hDumpFile != INVALID_HANDLE_VALUE) {
              MINIDUMP_EXCEPTION_INFORMATION dumpInfo;
              dumpInfo.ExceptionPointers = pException;
              dumpInfo.ThreadId = GetCurrentThreadId();
              dumpInfo.ClientPointers = TRUE;
              MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(),hDumpFile, MiniDumpNormal, &dumpInfo, NULL, NULL);
              CloseHandle(hDumpFile);
          }
          else{
              qDebug()<<"hDumpFile == null";
          }
          return EXCEPTION_EXECUTE_HANDLER;
}

int main(int argc, char *argv[])
{
    //检查程序是否 已经启动过
        if(checkPrgIsRunning()){
            qDebug() << "program is running ...........";
            return 0;
        }

    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QGuiApplication app(argc, argv);
    QtWebEngine::initialize();; //初始化

    //获取屏幕尺寸
    QScreen *screen=QGuiApplication::primaryScreen ();
    QRect mm=screen->availableGeometry() ;
    SetScreenInfo(mm.width (),mm.height ());

    //QSetting set
    QCoreApplication::setOrganizationName("keystock");
    QCoreApplication::setOrganizationDomain("keystock.com.cn");
    QCoreApplication::setApplicationName("RTClient");
    QCoreApplication::setApplicationVersion("1.0");

    Q_INIT_RESOURCE(currentver);

    // data interaction not owned by qml engine
    DataInteraction *pdi = DataInteraction::qmlInstance();
    QQmlEngine::setObjectOwnership(pdi, QQmlEngine::CppOwnership);
    BkData *pbk = BkData::qmlInstance();
    QQmlEngine::setObjectOwnership(pbk, QQmlEngine::CppOwnership);

    DataSettings *pdatasetting = DataSettings::qmlInstance();
    QQmlEngine::setObjectOwnership(pdatasetting, QQmlEngine::CppOwnership);
    UserOperationDb * puserDbOperation = UserOperationDb::getInstance();
    QQmlEngine::setObjectOwnership(puserDbOperation, QQmlEngine::CppOwnership);
    FPDataClient *fp = FPDataClient::GetInstance();
    QQmlEngine::setObjectOwnership(fp, QQmlEngine::CppOwnership);
    // register qml types
    qmlRegisterSingletonType<UserOperationDb>("Hyby.RT", 1, 0, "UserOperationDb",userDb_singletontype_provider);
    qmlRegisterType<GetUrlReplyContent>("Hyby.RT", 1, 0, "GetUrlReplyContent");
    qmlRegisterSingletonType<FPDataClient>("Hyby.RT", 1, 0, "FP", fpdataclient_singletontype_provider);

    qmlRegisterSingletonType<DataInteraction>("Hyby.RT", 1, 0, "DI", datainteraction_singletontype_provider);
    qmlRegisterType<BaseUtility>("Hyby.RT", 1, 0, "BaseUtility");
    qmlRegisterType<DrawLine>("Hyby.RT", 1, 0, "DrawLine");
    qmlRegisterUncreatableMetaObject(DIt::staticMetaObject, // static meta object
                                     "Hyby.RT",             // import statement
                                     1, 0,                  // major and minor version of the import
                                     "DIt",                 // name in QML
                                     "Error Uncreatable");  // error in tries to create a object
    qmlRegisterSingletonType<DataSettings>("Hyby.RT", 1, 0, "DataSettings", datasettings_singletontype_provider);
    // bkdata interface
    qmlRegisterSingletonType<BkData>("Hyby.RT", 1, 0, "BK", bkdata_singletontype_provider);

    QQmlApplicationEngine engine;
    engine.load(QUrl(QLatin1String("qrc:/main.qml")));
    SetUnhandledExceptionFilter((LPTOP_LEVEL_EXCEPTION_FILTER)ApplicationCrashHandler);
    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}
