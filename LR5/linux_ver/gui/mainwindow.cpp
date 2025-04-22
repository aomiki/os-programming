#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "shared_objects.h"
#include <QScrollBar>
#include <QColorDialog>
#include <signal.h>
#include <sys/stat.h>
#include <iostream>
#include <filesystem>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <iomanip>
#include <fstream>
#include <filesystem>
#include <format>
#include <QFile>

namespace fs = std::filesystem;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->label_log->setText("");

    setClientDisconnectedState();

    auto t = std::time(nullptr);
    auto tm = *std::localtime(&t);
    std::ostringstream oss;
    oss << std::put_time(&tm, "%d-%m-%Y_%H-%M-%S");
    curr_log_filename = "log/log_" + oss.str();

    updateLogList();

    //Create pipe
    log((QString)"creating pipe..");
    int ret_status = mkfifo(pipe_name.c_str(), S_IWUSR | S_IRUSR | S_IRGRP | S_IROTH);

    if (ret_status == -1 && errno != EEXIST)
    {
        log(std::format("error creating named pipe: {0}.\n", std::to_string(errno)));
    }

    log((QString)"creating client...");
    ret_status = system("alacritty --title client -e ./client.out &");
    if(ret_status == -1)
    {
        log(std::format("client creation failed ({0}).\n", std::to_string(errno)));
    }
    connect(ui->comboBox_selectedLog, SIGNAL(currentIndexChanged(int)), this, SLOT(switchLogView(int)));
    connect(ui->button_saveLog, SIGNAL(clicked()), this, SLOT(buttonSaveClicked()));
    connect(ui->pushButton_sendCommand, SIGNAL(clicked()), this, SLOT(buttonSendCommandClicked()));
    connect(ui->pushButton_bg_colorDialog, SIGNAL(clicked()), this, SLOT(chooseBgColorClicked()));
    connect(ui->pushButton_connectToClient, SIGNAL(clicked()), this, SLOT(buttonConnectClicked()));
}

void MainWindow::switchLogView(int index)
{
    QString selected_logfile = ui->comboBox_selectedLog->currentText();

    if (curr_log_selected)
    {
        curr_log = ui->label_log->text();
    }

    curr_log_selected = (selected_logfile == ("current: " + curr_log_filename));
    
    if (curr_log_selected)
    {
        ui->label_log->setText(curr_log);
    }
    else
    {
        QFile f(selected_logfile);
        if (!f.open(QFile::ReadOnly | QFile::Text)) return;
        QTextStream in(&f);

        ui->label_log->setText(in.readAll());
    }
}

void MainWindow::buttonConnectClicked()
{
    log((QString)"waiting for connection...");

    // wait for someone to connect to the pipe
    pipe_fd = open(pipe_name.c_str(), O_WRONLY);

    if (pipe_fd == -1)
    {
        log(std::format("opening pipe failed ({0}).\n", std::to_string(errno)));
    }

    setClientConnectedState();

    log((QString)"client connected.");
}

void MainWindow::buttonSendCommandClicked()
{
    QString user_choice = ui->comboBox_clientCommandType->currentText();
    CommandType clientCommand = Null;

    if (user_choice == "Reset")
    {
        clientCommand = ResetColor;
        log((QString)"sending reset command");
    }
    else if (user_choice == "Set color")
    {
        clientCommand = SetColor;
        log((QString)"sending set color command");
    }

    unsigned char msg[4];
    msg[0] = clientCommand;
    msg[1] = curr_bgColor[0];
    msg[2] = curr_bgColor[1];
    msg[3] = curr_bgColor[2];

    int ret_status = write(pipe_fd, msg, sizeof(unsigned char) * 4);

    if (ret_status != sizeof(unsigned char) * 4)
    {
        if (errno == EPIPE)
        {
            log((QString)"client closed");
            setClientDisconnectedState();
        }
        else
        {
            log(std::format("writing to pipe failed ({0}).\n", std::to_string(errno)));
        }
        return;
    }
    log((QString)"command sent");
}

void MainWindow::chooseBgColorClicked()
{
    QColorDialog colorPickerDialog;

    QColor chosenColor = colorPickerDialog.getColor(QColor(curr_bgColor[0], curr_bgColor[1], curr_bgColor[2]));

    curr_bgColor[0] = chosenColor.red();
    curr_bgColor[1] = chosenColor.green();
    curr_bgColor[2] = chosenColor.blue();
}

void MainWindow::buttonSaveClicked()
{
    QString curr_log = ui->label_log->text();

    log((QString)"saving log to file...");

    std::ofstream output_file(curr_log_filename, std::ios::out | std::ios::binary);
    output_file.write(curr_log.toUtf8().constData(), curr_log.size());
    output_file.close();

    updateLogList();
}

void MainWindow::updateLogList()
{
    std::string log_dir = "log";

    if (!fs::is_directory(log_dir) || !fs::exists(log_dir)) { // Check if src folder exists
        fs::create_directory(log_dir); // create src folder
    }

    ui->comboBox_selectedLog->clear();
    ui->comboBox_selectedLog->addItem(QString::fromStdString("current: " + curr_log_filename));
    for (const auto & entry : fs::directory_iterator(log_dir))
        ui->comboBox_selectedLog->addItem(QString::fromStdString(entry.path()));
}

void MainWindow::setClientConnectedState()
{
    ui->label_clientConn->setText("yes");
    ui->pushButton_sendCommand->setDisabled(false);
    ui->pushButton_connectToClient->setDisabled(true);
}

void MainWindow::setClientDisconnectedState()
{
    ui->pushButton_sendCommand->setDisabled(true);
    ui->label_clientConn->setText("no");
}

void MainWindow::log(const std::string str)
{
    QString txt = QString::fromStdString(str);

    log(txt);
}

void MainWindow::log(const QString txt)
{
    if (curr_log_selected)
    {
        ui->label_log->setText(ui->label_log->text() + txt + "\n");
        ui->label_log->repaint();
        ui->scrollArea_log->verticalScrollBar()->setValue(ui->label_log->height());
    }
    else
    {
        curr_log.append(txt);   
    }
}

MainWindow::~MainWindow()
{
    delete ui;
}
