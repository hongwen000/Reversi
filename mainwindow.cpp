#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <map>
#include <QThread>
#include <QMessageBox>
#include <QClipboard>
#include <QTime>
#include <cstdint>
using namespace std;

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
    QJsonObject j;
//    json j;
    j.insert("magic", MAGIC_NUM);
//    j["magic"] = MAGIC_NUM;
    j.insert("id", move);
//    j["id"] = move;
    QJsonDocument document;
    document.setObject(j);
    QByteArray datagram = document.toJson(QJsonDocument::Compact);
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

void MainWindow::panicAI(Grid<ChessBlock> * grid)
{
    srand(time(nullptr));
    while(true)
    {
        vector<pair<int,int>> possibleMove;
        for(int i = 0; i < GAMESCALE; ++i)
        {
            for(int j = 0; j < GAMESCALE; ++j)
            {
                if(!grid->pBlocks[i][j]->isOccupied && grid->pBlocks[i][j]->isAvailable)
                    possibleMove.push_back({i,j});
            }
        }
        if(possibleMove.size() == 0) break;
        auto n = rand() % possibleMove.size();
        sendMove(possibleMove[n].first * grid->row + possibleMove[n].second);
        break;
    }
}

void MainWindow::AI(const State& s)
{
//    QThread::sleep(1);
    int bestMove = -1;
    QTime time;
    time.start();
    qDebug() << "Best estimation value: " << AlphaBeta(s, getCurrentPlayerChessColor(), THINKINGLEVEL, positionalValue, INT_MIN, INT_MAX, bestMove);
    qDebug() << "Search Spend: " << time.elapsed() << "(ms)";
    if(bestMove == -1)
    {
        gamming = false;
    }
    else
    {
        sendMove(bestMove);
    }
}


MainWindow::MainWindow(QWidget *parent, size_t row, size_t col) :
    QMainWindow(parent), ui(new Ui::MainWindow), row(row), col(col), exit(false)
{
    ui->setupUi(this);
    grid = new Grid<ChessBlock>(this, ui->gridLayout_2, row, col);
    resetBoard();
//    const int ratio = 6;
//    ui->gridLayout->setColumnStretch(0,ratio);
//    ui->gridLayout->setColumnStretch(1,1);
    ui->gridLayout_2->setContentsMargins(0,0,0,this->height() - (this->width() - ui->lcdNumber->width()));
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
    connect(this, &MainWindow::gameOverEvent, this, &MainWindow::on_GameOver);
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
        int id;
        QJsonParseError jsonError;
        QJsonDocument document;
        receiver->readDatagram(datagram.data(), datagram.size(), &addr, &port);
        document = QJsonDocument::fromJson(datagram, &jsonError);
        if(document.isNull() || !(jsonError.error == QJsonParseError::NoError) || !document.isObject())
            continue;
        QJsonObject object = document.object();
        if(!object.contains("magic") || !object.value("magic").isString())
            continue;
        auto magic = object.value("magic").toString();
        if(magic != MAGIC_NUM)
            continue;
        id = object.value("id").toInt();
        qDebug() << "Player " << currentPlayer << " down at row:" << (id / GAMESCALE) + 1 << ", col:" << (char)((id % GAMESCALE) + 'A');
        if(!gamming)
        {
            ui->checkBox->setChecked(false);
            startGame();
        }
        auto block = id;
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
    grid->reDraw();
    resetBoard();
    processGrid(GAMESCALE / 2 - 1, GAMESCALE / 2 - 1, BLACK);
    processGrid(GAMESCALE / 2 - 1, GAMESCALE / 2, WHITE);
    processGrid(GAMESCALE / 2, GAMESCALE / 2 - 1, WHITE);
    processGrid(GAMESCALE / 2, GAMESCALE / 2, BLACK);
    qDebug() << "Player " << getFirstPlayer() << " uses black chess";
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
        auto ret = getXY(i);
        int x = ret.first;
        int y = ret.second;
//        auto [x,y] = getXY(i);
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

void MainWindow::resetTimer()
{
    pTimer->stop();
    delete pTimer;
    pTimer = nullptr;
    QPalette pal = palette();
    pal.setColor(QPalette::Background, Qt::GlobalColor::white);
    ui->lcdNumber->setPalette(pal);
    ui->lcdNumber->display(0);
}

void MainWindow::on_reqRepaint()
{
//    update();
    repaint();
}

void MainWindow::on_GameOver()
{
    qDebug() << "Game Over";
    QMessageBox::information(this, "游戏结束", ui->label_4->text().toInt() > ui->label_4->text().toInt() ? "黑子胜" : "白子胜");
    ui->pushButton_3->clicked();
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
    ui->pushButton_2->setEnabled(false);
    startGame();
    if(playerType[currentPlayer] == COMPUTER)
    {
        ai_thread = new std::thread(&MainWindow::AI, this, toState());
    }
    ui->pushButton_3->setEnabled(true);
}

bool noAvailMove(const std::array<int, GAMESCALE * GAMESCALE>& avi)
{
    for(auto&i:avi) if(i!=0) return false;
    return true;
}

void MainWindow::processMove(size_t x, size_t y)
{
    auto color = getCurrentPlayerChessColor();
    processGrid(x, y, color);
    auto n = getNextState(toState(), getID(x,y), color);
    auto ret = fromState(n);
    int b = ret.first;
    int w = ret.second;
    ui->label_4->setText("黑子数量：" + QString::number(b));
    ui->label_5->setText("白子数量：" + QString::number(w));
    setCurrentPlayer(!currentPlayer);
    auto avi = getAvail(n, getCurrentPlayerChessColor());
    drawAvailNum(avi);
    emit reqRepaint();
    if(noAvailMove(avi))
    {
        setCurrentPlayer(!currentPlayer);
        auto avi = getAvail(n, getCurrentPlayerChessColor());
        drawAvailNum(avi);
        emit reqRepaint();
        if(noAvailMove(avi))
        {
            emit gameOverEvent();
            return;
        }
    }
    if(playerType[currentPlayer] == COMPUTER)
    {
        ai_thread = new std::thread(&MainWindow::AI, this, toState());
    }
//    if(currentPlayer == PLAYER1 && playerType[currentPlayer] == COMPUTER)
//    {
//        ai_thread = new std::thread(&MainWindow::AI, this, toState());
//    }
//    if(currentPlayer == PLAYER2 && playerType[currentPlayer] == COMPUTER)
//    {
//        panic_ai_thread = new std::thread(&MainWindow::panicAI, this, grid);
//    }
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
    ui->pushButton_2->setEnabled(true);
    if(ai_thread && ai_thread->joinable())
        ai_thread->join();
    gamming = false;
    resetTimer();
    ui->pushButton_3->setEnabled(false);

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
