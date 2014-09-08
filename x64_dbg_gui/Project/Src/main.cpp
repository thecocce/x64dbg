#include "main.h"
#include <QAbstractEventDispatcher>
#include <QMessageBox>
#include "Bridge.h"
#include "Configuration.h"
#include "MainWindow.h"

MyApplication::MyApplication(int & argc, char** argv) : QApplication(argc, argv)
{
}

bool MyApplication::winEventFilter(MSG* message, long* result)
{
    return DbgWinEvent(message, result);
}

bool MyApplication::globalEventFilter(void* message)
{
    return DbgWinEventGlobal((MSG*)message);
}

bool MyApplication::notify(QObject* receiver, QEvent* event)
{
    bool done = true;
    try
    {
        done = QApplication::notify(receiver, event);
    }
    catch(const std::exception & ex)
    {
        const char* message = QString().sprintf("Fatal GUI Exception: %s!\n", ex.what()).toUtf8().constData();
        GuiAddLogMessage(message);
        puts(message);
        OutputDebugStringA(message);
    }
    catch(...)
    {
        const char* message = "Fatal GUI Exception: (...)!\n";
        GuiAddLogMessage(message);
        puts(message);
        OutputDebugStringA(message);
    }
    return done;
}

static Configuration* mConfiguration;

int main(int argc, char* argv[])
{
    MyApplication application(argc, argv);
    QAbstractEventDispatcher::instance(application.thread())->setEventFilter(MyApplication::globalEventFilter);

    // load config file + set config font
    mConfiguration = new Configuration;
    application.setFont(ConfigFont("Application"));

    // Register custom data types
    qRegisterMetaType<int_t>("int_t");
    qRegisterMetaType<uint_t>("uint_t");
    qRegisterMetaType<byte_t>("byte_t");
    qRegisterMetaType<DBGSTATE>("DBGSTATE");

    // Init communication with debugger
    Bridge::initBridge();

    // Start GUI
    MainWindow mainWindow;
    mainWindow.show();

    // Set some data
    Bridge::getBridge()->winId = (void*)mainWindow.winId();

    // Init debugger
    const char* errormsg = DbgInit();
    if(errormsg)
    {
        QMessageBox msg(QMessageBox::Critical, "DbgInit Error!", QString(errormsg));
        msg.setWindowIcon(QIcon(":/icons/images/compile-error.png"));
        msg.setWindowFlags(msg.windowFlags() & (~Qt::WindowContextHelpButtonHint));
        msg.exec();
        ExitProcess(1);
    }

    //execute the application
    int result = application.exec();
    mConfiguration->save(); //save config on exit
    return result;
}
