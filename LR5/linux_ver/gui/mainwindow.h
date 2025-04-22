#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <semaphore.h>
#include <QMainWindow>
#include <QLabel>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

public slots:
    void buttonReceiveCommandClicked();
    void buttonSaveClicked();
    void buttonSpawnClicked();
    void switchLogView(int index);

private:
    int pipe_fd = 0;
    sem_t* sem_handle;
    std::string curr_log_filename = "";
    QString curr_log = "";
    bool curr_log_selected = true;
    QLabel** labels;
    unsigned clients_num;

    void setClientsStateNoneConnected();
    void setClientStateSomeConnected();

    void updateLogList();
    void log(const std::string str);
    void log(const QString txt);
    Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H