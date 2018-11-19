#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <map>

int currentPlayer;
QString playerIP[2] = {"127.0.0.1", "127.0.0.1"};
int playerType[2] = {HUMAN, COMPUTER};
atomic_bool gamming(false);

void sendMove(int move)
{
    QByteArray datagram = QByteArray::number(move);
    auto sender = new QUdpSocket(nullptr);
    QHostAddress host1(playerIP[0]);
    sender->writeDatagram(datagram, host1, 16666);
    if(playerIP[0] != playerIP[1])
    {
        QHostAddress host2(playerIP[1]);
        auto sender = new QUdpSocket(nullptr);
        sender->writeDatagram(datagram, host2, 16666);
    }
}

void panicAI(Grid<ChessBlock> * grid)
{
    srand(time(nullptr));
    while(true)
    {
        auto r = rand() % grid->row;
        auto c = rand() % grid->col;
        if(grid->pBlocks[r][c]->isOccupied)
            continue;
        sendMove(r * grid->col + c);
        break;
    }
}


MainWindow::MainWindow(QWidget *parent, size_t row, size_t col) :
    QMainWindow(parent), ui(new Ui::MainWindow), row(row), col(col), exit(false)
{
    ui->setupUi(this);
    grid = new Grid<ChessBlock>(this, ui->gridLayout_2, row, col);
    resetBoard();
    receiver = new QUdpSocket(this);
    receiver->bind(16666, QAbstractSocket::ShareAddress);
    ui->statusBar->showMessage("状态：端口已打开");
    control_thread = new std::thread(&MainWindow::control_func, this);
    ui->lcdNumber->setAutoFillBackground(true);
}

MainWindow::~MainWindow()
{
    exit = true;
    delete ui;
}

void MainWindow::control_func()
{
    while (!exit) {
        if(gamming)
        {
            QByteArray datagram;
            auto size = receiver->pendingDatagramSize();
            if(!size) continue;
            datagram.resize(size);
            receiver->readDatagram(datagram.data(), datagram.size());
            auto block = datagram.toInt();
            auto this_row = block/col;
            auto this_col = (block - (block / col) * row);
            processMove(this_row, this_col);
        }
    }
}

void MainWindow::resetBoard()
{
    for(size_t i = 0; i < row; ++i)
    {
        for(size_t j = 0; j < col; ++j)
        {
            bool ii = i % 2;
            bool jj = j % 2;
            if(ii && jj)
                grid->setPic(i, j, ODDFN);
            else if(ii && !jj)
                grid->setPic(i, j, EVENFN);
            else if (!ii && jj)
                grid->setPic(i, j, EVENFN);
            else
                grid->setPic(i, j, ODDFN);
        }
    }
}

void MainWindow::setCurrentPlayer(int player)
{
    currentPlayer = player;
    if(pTimer == nullptr)
    {
        pTimer = new QTimer(this);
        pTimer->setInterval(1000);
        pTimer->start();
        connect(pTimer, &QTimer::timeout, [this](){/*qDebug() << "In func";*/ ui->lcdNumber->display(ui->lcdNumber->intValue() + 1);});
    }
    QPalette pal = palette();
    pal.setColor(QPalette::Background, currentPlayer ? Qt::GlobalColor::red : Qt::GlobalColor::green);
    ui->lcdNumber->setPalette(pal);
    ui->lcdNumber->display(0);
}


void ChessBlock::mousePressEvent(QMouseEvent *e)
{
    Q_UNUSED(e);
    if(!gamming || playerType[currentPlayer] != HUMAN || isOccupied) return;
    sendMove(id);
}
std::map<QString, int> mp = {{"人类", HUMAN},
                             {"电脑", COMPUTER},
                             {"远程玩家", REMOTE}};

void MainWindow::on_pushButton_2_clicked()
{
    setCurrentPlayer(!ui->checkBox->isChecked());
    playerType[0] = mp[ui->comboBox_p1->currentText()];
    playerType[1] = mp[ui->comboBox_p2->currentText()];
    gamming = true;
}

void MainWindow::processMove(size_t this_row, size_t this_col)
{
    setCurrentPlayer(!currentPlayer);
    grid->setPic(this_row, this_col, currentPlayer ? WCFN : BCFN);
    grid->pBlocks[this_row][this_col]->isOccupied = true;
    if(playerType[currentPlayer] == COMPUTER)
        std::thread(panicAI, grid).detach();
}

void MainWindow::on_pushButton_3_clicked()
{
    pTimer->stop();
    delete pTimer;
    pTimer = nullptr;
    QPalette pal = palette();
    pal.setColor(QPalette::Background, Qt::GlobalColor::white);
    ui->lcdNumber->setPalette(pal);
    ui->lcdNumber->display(0);
    grid->reDraw();
    resetBoard();

}
