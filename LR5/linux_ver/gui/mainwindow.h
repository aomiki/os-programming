#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

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
    void buttonSendCommandClicked();
    void buttonSaveClicked();
    void buttonConnectClicked();
    void chooseBgColorClicked();
    void switchLogView(int index);

private:
    int pipe_fd = 0;
    unsigned char curr_bgColor[3] = {255, 255, 255};
    std::string curr_log_filename = "";
    QString curr_log = "";
    bool curr_log_selected = true;

    void updateLogList();
    void setClientConnectedState();
    void setClientDisconnectedState();
    void log(const std::string str);
    void log(const QString txt);
    Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H