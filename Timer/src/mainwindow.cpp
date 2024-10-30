#include "./ui_mainwindow.h"
#include "mainwindow.h"

MainWindow::MainWindow(QWidget * parent)
: QMainWindow(parent), ui(new Ui::MainWindow), serial(new Serial), msTimer(new QTimer)
{
  ui->setupUi(this);
  QIcon icon(":/image/images/icon.png");
  setWindowIcon(icon);

  initializeSerial();
  timerSetDialog(msec, sec, min);

  timerType = TimerType::stopwatch;
  ui->modeLabel->setText("Stopwatch");
}

MainWindow::~MainWindow()
{
  delete serial;
  delete ui;
}

//////////////////// Functions ////////////////////

void MainWindow::initializeSerial()
{
  QStringList portNames = serial->getPortNames();
  ui->deviceList->addItems(portNames);
}

void MainWindow::timerSetDialog(int & msec, int & sec, int & min)
{
  ui->minLCD->display(QString::number(min).rightJustified(2, '0'));
  ui->secLCD->display(QString::number(sec).rightJustified(2, '0'));
  ui->msecLCD->display(QString::number(msec).rightJustified(3, '0'));
}

//////////////////// Slots ////////////////////

void MainWindow::on_deviceList_currentIndexChanged(int index)
{
  device_name = ui->deviceList->currentText();
}

void MainWindow::on_connectBtn_clicked()
{
  if (isOpen) {
    disconnect(serial->serial, &QSerialPort::readyRead, serial, &Serial::readDevice);

    ui->deviceList->setEnabled(true);
    ui->connectBtn->setText("Connect");
    ui->connectBtn->setStyleSheet("color: rgb(255, 0, 0);");
    ui->deviceResetBtn->setEnabled(false);
    ui->servoInitBtn->setEnabled(false);

    isOpen = false;

    initializeSerial();
  } else if (!isOpen) {
    bool status = serial->openPort(device_name, SERIAL_BAUDRATE);

    if (status) {
      ui->deviceList->setEnabled(false);
      ui->connectBtn->setText("Disconnect");
      ui->connectBtn->setStyleSheet("color: rgb(0, 255, 0);");
      ui->deviceResetBtn->setEnabled(true);
      ui->servoInitBtn->setEnabled(true);

      isOpen = true;

      connect(serial->serial, &QSerialPort::readyRead, serial, &Serial::readDevice);
    } else {
      QMessageBox::critical(
        nullptr, "Device Status", "Failed to open device: " + serial->serial->errorString());
      ui->deviceList->setEnabled(true);
      ui->connectBtn->setText("Connect");
      ui->connectBtn->setStyleSheet("color: rgb(255, 0, 0);");
      ui->deviceResetBtn->setEnabled(false);
      ui->servoInitBtn->setEnabled(false);

      isOpen = false;
      initializeSerial();
    }
  }
}

void MainWindow::on_deviceResetBtn_clicked()
{
  QByteArray packet;
  packet.append("##");
  packet.append("0x02");  // reset_device
  packet.append("//");
  serial->writeDevice(packet);

  QThread::sleep(1);
}

void MainWindow::on_servoInitBtn_clicked()
{
  QByteArray packet;
  packet.append("##");
  packet.append("0x03");  // servo_init
  packet.append("//");
  serial->writeDevice(packet);

  QThread::sleep(1);
}

void MainWindow::on_timerSetBtn_clicked()
{
  QDialog dialog(this);
  dialog.setWindowTitle("타이머 설정");

  QVBoxLayout * mainLayout = new QVBoxLayout(&dialog);
  QHBoxLayout * inputLayout = new QHBoxLayout();

  QSpinBox * valueSpinBox = new QSpinBox();
  valueSpinBox->setRange(0, 999);

  QComboBox * unitComboBox = new QComboBox();
  unitComboBox->addItems({"밀리초", "초", "분"});

  QDialogButtonBox * buttonBox =
    new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

  inputLayout->addWidget(valueSpinBox);
  inputLayout->addWidget(unitComboBox);
  mainLayout->addLayout(inputLayout);
  mainLayout->addWidget(buttonBox);

  connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
  connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

  if (dialog.exec() == QDialog::Accepted) {
    int value = valueSpinBox->value();
    int unit = unitComboBox->currentIndex();

    int milliseconds = value;
    switch (unit) {
      case 1:
        milliseconds *= 1000;
        break;
      case 2:
        milliseconds *= 60000;
        break;
    }

    min = milliseconds / 60000;
    sec = (milliseconds % 60000) / 1000;
    msec = milliseconds % 1000;

    last_min = min;
    last_sec = sec;
    last_msec = msec;

    timerType = TimerType::timer;
    ui->modeLabel->setText("Timer");
    timerSetDialog(msec, sec, min);
  }
}

void MainWindow::on_stopwatchBtn_clicked()
{
  timerType = TimerType::stopwatch;
  ui->modeLabel->setText("Stopwatch");

  msec = 0;
  sec = 0;
  min = 0;
  timerSetDialog(msec, sec, min);
}

void MainWindow::on_startBtn_clicked()
{
  if (onCount) {
    ui->startBtn->setText("Start");
    msTimer->stop();
    disconnect(msTimer, &QTimer::timeout, this, &MainWindow::timerUpdate);
    onCount = false;
  } else {
    ui->startBtn->setText("Stop");
    onCount = true;
    msTimer->start(1);
    connect(msTimer, &QTimer::timeout, this, &MainWindow::timerUpdate);
  }
}

void MainWindow::timerUpdate()
{
  if (timerType == TimerType::timer) {
    msec--;
    if (msec < 0) {
      msec = 999;
      sec--;
      if (sec < 0) {
        sec = 59;
        min--;
        if (min < 0) {
          msec = 0;
          sec = 0;
          min = 0;
          ui->startBtn->setText("Start");
          msTimer->stop();
          disconnect(msTimer, &QTimer::timeout, this, &MainWindow::timerUpdate);
          onCount = false;
          QMessageBox::information(this, "타이머 종료", "설정한 시간이 완료되었습니다!");
        }
      }
    }
    timerSetDialog(msec, sec, min);
  } else if (timerType == TimerType::stopwatch) {
    msec++;
    if (msec >= 1000) {
      msec = 0;
      sec++;
      if (sec >= 60) {
        sec = 0;
        min++;
      }
    }
    timerSetDialog(msec, sec, min);
  }
}

void MainWindow::on_resetBtn_clicked()
{
  ui->startBtn->setText("Start");
  msTimer->stop();
  disconnect(msTimer, &QTimer::timeout, this, &MainWindow::timerUpdate);
  onCount = false;

  msec = 0;
  sec = 0;
  min = 0;

  timerSetDialog(msec, sec, min);

  if (timerType == TimerType::timer) {
    QMessageBox::StandardButton reply = QMessageBox::question(
      this, "타이머 리셋", "마지막으로 설정한 시간으로 복원하시겠습니까?",
      QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
      min = last_min;
      sec = last_sec;
      msec = last_msec;
      timerSetDialog(msec, sec, min);
    }
  }
}