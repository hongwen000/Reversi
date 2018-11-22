#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <map>
#include <QThread>
#include <QClipboard>

int currentPlayer;
QString playerIP[2] = {"127.0.0.1", "127.0.0.1"};
int playerListenPorts[2] = {16666, 16666};
int playerType[2] = {HUMAN, COMPUTER};
atomic_bool gamming(false);
std::map<QString, int> mp = {{"人类", HUMAN},
                             {"电脑", COMPUTER},
                             {"远程玩家", REMOTE}};

void sendMove(int move)
{
    QByteArray datagram = QByteArray::number(move);
    auto sender = new QUdpSocket(nullptr);
    QHostAddress host1(playerIP[0]);
    sender->writeDatagram(datagram, host1, playerListenPorts[0]);
    if(playerIP[0] != playerIP[1] || playerListenPorts[0] != playerListenPorts[1])
    {
        QHostAddress host2(playerIP[1]);
        auto sender = new QUdpSocket(nullptr);
        sender->writeDatagram(datagram, host2, playerListenPorts[1]);
    }
}

void panicAI(Grid<ChessBlock> * grid)
{
    srand(time(nullptr));
    while(true)
    {
        auto r = rand() % grid->row;
        auto c = rand() % grid->col;
        if(grid->pBlocks[r][c]->isOccupied || !grid->pBlocks[r][c]->isAvailable)
            continue;
        QThread::sleep(2);
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
    receiver->bind(0, QAbstractSocket::ShareAddress);
    auto port = receiver->localPort();
    playerListenPorts[0] = port;
    ui->statusBar->showMessage("监听端口" + QString::number(port));
    control_thread = new std::thread(&MainWindow::control_func, this);
    ui->lcdNumber->setAutoFillBackground(true);
    ui->lineEdit->setPlaceholderText("远程玩家 IP:Port");
    connect(this, SIGNAL(remoteChallengeEvent(QString)), this, SLOT(on_remoteChallengeEvent(QString)));
    connect(this, &MainWindow::reqRepaint, this, &MainWindow::on_reqRepaint);
}

MainWindow::~MainWindow()
{
    exit = true;
    control_thread->join();
    delete ui;
}

void MainWindow::control_func()
{
    while (!exit) {
        QThread::msleep(50);
        QByteArray datagram;
        auto size = receiver->pendingDatagramSize();
        if(!size) continue;
        QHostAddress addr;
        quint16 port;
        datagram.resize(size);
        receiver->readDatagram(datagram.data(), datagram.size(), &addr, &port);
        if(!gamming)
        {
            ui->checkBox->setChecked(false);
            startGame();
        }
        auto block = datagram.toInt();
        auto this_row = block/col;
        auto this_col = (block - (block / col) * row);
        processMove(this_row, this_col);
        QApplication::processEvents();

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
            grid->pBlocks[i][j]->color = EMPTY;
        }
    }
}

void MainWindow::setCurrentPlayer(int player)
{
    currentPlayer = player;
    if(currentPlayer) ui->label_3->setText("请等待玩家2落子");
    else ui->label_3->setText("请等待玩家1落子");
    if(pTimer == nullptr)
    {
        emit remoteChallengeEvent("");
    }
    QPalette pal = palette();
    pal.setColor(QPalette::Background, currentPlayer ? Qt::GlobalColor::red : Qt::GlobalColor::green);
    ui->lcdNumber->setPalette(pal);
    ui->lcdNumber->display(0);
}


void MainWindow::startGame()
{
//    processGrid(GAMESCALE / 2 - 1, GAMESCALE / 2 - 1, WHITE);
//    processGrid(GAMESCALE / 2 - 1, GAMESCALE / 2, BLACK);
//    processGrid(GAMESCALE / 2, GAMESCALE / 2 - 1, BLACK);
//    processGrid(GAMESCALE / 2, GAMESCALE / 2, WHITE);
    processGrid(GAMESCALE / 2 - 1, GAMESCALE / 2 - 1, BLACK);
    processGrid(GAMESCALE / 2 - 1, GAMESCALE / 2, WHITE);
    processGrid(GAMESCALE / 2, GAMESCALE / 2 - 1, WHITE);
    processGrid(GAMESCALE / 2, GAMESCALE / 2, BLACK);
    qDebug() << "I am player " << getFirstPlayer();
    setCurrentPlayer(getFirstPlayer());
    auto ret = getAvail(toState(), getCurrentPlayerChessColor());
    drawAvailNum(ret);
    reqRepaint();
    playerType[0] = mp[ui->comboBox_p1->currentText()];
    playerType[1] = mp[ui->comboBox_p2->currentText()];
    if(playerType[1] == REMOTE)
    {
        QStringList lst = ui->lineEdit->text().split(':', QString::SkipEmptyParts);
        playerIP[1] = lst[0];
        playerListenPorts[1] = lst[1].toInt();
    }
    gamming = true;

}

void MainWindow::drawAvailNum(const array<int, GAMESCALE * GAMESCALE> &avi)
{
    for(int i = 0; i < GAMESCALE * GAMESCALE; ++i)
    {
        auto [x,y] = getXY(i);
        grid->pBlocks[x][y]->drawNum(avi[i]);
    }
}

State MainWindow::toState()
{
    State ret;
    for(int i = 0; i < GAMESCALE; ++i)
    {
        for (int j = 0; j < GAMESCALE; ++j)
        {
            ret[getID(i,j)] = grid->pBlocks[i][j]->color;
        }
    }
    return ret;
}

pair<int, int> MainWindow::fromState(const State &s)
{
    int b = 0;
    int w = 0;
    for(int i = 0; i < GAMESCALE; ++i)
    {
        for (int j = 0; j < GAMESCALE; ++j)
        {
            auto c = s[getID(i,j)];
            if(c == BLACK) b++;
            if(c == WHITE) w++;
            if(grid->pBlocks[i][j]->color != c)
            {
                processGrid(i, j, c);
            }
        }
    }
    return {b,w};
}

void MainWindow::processGrid(int x, int y, int color)
{
    grid->setOverlayPic(x, y, color == WHITE ? WCFN : BCFN);
    grid->pBlocks[x][y]->color = color;
    grid->pBlocks[x][y]->isOccupied = true;
    emit reqRepaint();
}

void MainWindow::on_reqRepaint()
{
//    update();
    repaint();
}

void MainWindow::on_remoteChallengeEvent(const QString &str)
{
    Q_UNUSED(str);
    if(!pTimer)
    {
        pTimer = new QTimer(nullptr);
        pTimer->setInterval(1000);
        pTimer->start();
        qDebug() << "Timer stared";
        connect(pTimer, &QTimer::timeout, [this](){ui->lcdNumber->display(ui->lcdNumber->intValue() + 1);});
    }
}


void ChessBlock::drawNum(int n)
{
    if(n == 0 && !isOccupied)
    {
        isAvailable = false;
        pic = pic_ori;
        return;
    }
    else if(n != 0 && !isOccupied)
    {
        isAvailable = true;
        QPainter p;
        pic = pic_ori;
        p.begin(&pic);
        p.setPen(QPen(Qt::GlobalColor::black));
        p.drawText(pic.rect(), Qt::AlignCenter, QString::number(n));
        p.end();
    }
    else if(isOccupied)
        return;
}

void ChessBlock::mousePressEvent(QMouseEvent *e)
{
    Q_UNUSED(e);
    if(!gamming || playerType[currentPlayer] != HUMAN || isOccupied || !isAvailable) return;
    sendMove(id);
}

void MainWindow::on_pushButton_2_clicked()
{
    startGame();
    if(playerType[currentPlayer] == COMPUTER)
    {
        ai_thread = new std::thread(panicAI, grid);
    }
}

void MainWindow::processMove(size_t x, size_t y)
{
    auto color = getCurrentPlayerChessColor();
    processGrid(x, y, color);
    auto n = getNextState(toState(), getID(x,y), color);
    auto [b,w] = fromState(n);
    ui->label_4->setText("黑子数量：" + QString::number(b));
    ui->label_5->setText("白子数量：" + QString::number(w));
    setCurrentPlayer(!currentPlayer);
    auto avi = getAvail(n, getCurrentPlayerChessColor());
    drawAvailNum(avi);
    emit reqRepaint();
    if(playerType[currentPlayer] == COMPUTER)
    {
        ai_thread = new std::thread(panicAI, grid);
    }
}

int MainWindow::getFirstPlayer()
{
    return !ui->checkBox->isChecked();
}

int MainWindow::getCurrentPlayerChessColor()
{
    int color;
    auto chk = ui->checkBox->isChecked();
    if(currentPlayer && chk) color = WHITE;
    else if(currentPlayer && !chk) color = BLACK;
    else if(!currentPlayer && chk) color = BLACK;
    else color = WHITE;
    return color;
}

void MainWindow::on_pushButton_3_clicked()
{
    if(ai_thread && ai_thread->joinable())
        ai_thread->join();
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

void MainWindow::on_comboBox_p2_currentTextChanged(const QString &arg1)
{
    if(mp[arg1] == REMOTE)
        ui->lineEdit->setEnabled(true);
    else
        ui->lineEdit->setEnabled(false);
}

void MainWindow::on_pushButton_clicked()
{
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(QString::number(playerListenPorts[0]));
}
