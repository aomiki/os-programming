#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "shared_objects.h"
#include <QScrollBar>
#include <QColorDialog>
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

    setClientsStateNoneConnected();

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
        std::cout << "error creating named pipe: " << errno << std::endl;
    }

    sem_unlink(semaphore_name.c_str()); //clear semaphore
	sem_handle = sem_open(semaphore_name.c_str(), O_CREAT, S_IWUSR | S_IRUSR | S_IRGRP | S_IROTH, 0);

	if(sem_handle == SEM_FAILED)
	{
		printf("failed to open semaphore (%d).\n", errno);
	}

    connect(ui->comboBox_selectedLog, SIGNAL(currentIndexChanged(int)), this, SLOT(switchLogView(int)));
    connect(ui->button_saveLog, SIGNAL(clicked()), this, SLOT(buttonSaveClicked()));
    connect(ui->pushButton_receiveCommand, SIGNAL(clicked()), this, SLOT(buttonReceiveCommandClicked()));
    connect(ui->pushButton_spawnClients, SIGNAL(clicked()), this, SLOT(buttonSpawnClicked()));
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

void MainWindow::buttonSpawnClicked()
{
    clients_num = ui->spinBox_clientsNum->value();

    log(std::format("creating {0} clients...\n", std::to_string(clients_num)));

    labels = new QLabel*[clients_num];
    for (size_t i = 0; i < clients_num; i++)
    {
        std::cout << "Creating client " << i << std::endl;

        int ret_status = system(("alacritty --title client -e ./client.out " + std::to_string(i) + "&").c_str());
        if(ret_status == -1)
        {
            log(std::format("client {0} creation failed ({1}).\n", i, std::to_string(errno)));
            continue;
        }

        QLabel* status_label = new QLabel("", ui->scrollAreaWidgetContents_statuses);
        labels[i] = status_label;

        ui->scrollAreaWidgetContents_statuses->layout()->addWidget(status_label);
        status_label->show();
    }

    log((QString)"clients created.");
    setClientStateSomeConnected();
}

void MainWindow::buttonReceiveCommandClicked()
{
    // Release ownership of the semaphore object
    if (sem_post(sem_handle) == -1)
    {
        log(std::format("error releasing semaphore ({0}).\n", std::to_string(errno)));
        return;
    }

    log((QString)"waiting for connection...");

    // wait for someone to connect to the pipe
    int pipe_fd = open(pipe_name.c_str(), O_RDONLY);

    if (pipe_fd == -1)
    {
        log(std::format("opening pipe failed ({0}).\n", std::to_string(errno)));
        return;
    }

    log((QString)"client connected");

    // Read from the pipe.
    unsigned char message[2];

    int ret_status = read(pipe_fd, message, sizeof(unsigned char) * 2);

    if (ret_status != sizeof(unsigned char) * 2)
    {
        if (ret_status == -1)
        {
            log(std::format("reading from pipe failed ({0}).\n", std::to_string(errno)));
        }
        else
        {
            log((QString)"client disconnected");
        }
    }

    log((QString)"message received");

    ret_status = ::close(pipe_fd);

    if(ret_status == -1)
    {
        log(std::format("closing pipe failed ({0}).\n", std::to_string(errno)));
        return;
    }

    std::string str_status;
    switch (message[1])
    {
        case ShredingerStatus::Null:
            str_status = "null";
            break;
        case ShredingerStatus::Undefined:
            str_status = "not defined";
            break;
        case ShredingerStatus::Alive:
            str_status = "alive";
            break;
        case ShredingerStatus::Dead:
            str_status = "dead";
            clients_num--;
            break;
        default:
            str_status = "uhm";
            break;
    }

    labels[message[0]]->setText(QString::fromStdString(std::format("client {0} | status: {1}\n", std::to_string(message[0]), str_status)));
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

void MainWindow::setClientsStateNoneConnected()
{
    ui->pushButton_receiveCommand->setDisabled(true);
    ui->pushButton_spawnClients->setDisabled(false);
}

void MainWindow::setClientStateSomeConnected()
{
    ui->pushButton_receiveCommand->setDisabled(false);
    ui->pushButton_spawnClients->setDisabled(true);
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
