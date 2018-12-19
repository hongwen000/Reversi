#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <map>
#include <QThread>
#include <QMessageBox>
#include <QClipboard>
#include <QTime>
#include <cstdint>
#include <QProcess>

using namespace std;

QVariant convertToJSON(const QByteArray& datagram)
{
    QJsonParseError jsonError;
    QJsonDocument document;
    document = QJsonDocument::fromJson(datagram, &jsonError);
    if(document.isNull() || !(jsonError.error == QJsonParseError::NoError) || !document.isObject())
        return QVariant();
    QJsonObject object = document.object();
    if(!object.contains("magic") || !object.value("magic").isString())
        return QVariant();
    auto magic = object.value("magic").toString();
    if(magic != MAGIC_NUM)
        return QVariant();
    return QVariant(object);
}
int currentPlayer;
QString serverIP = "127.0.0.1";
int serverPort;
QString localIP = "127.0.0.1";
int localPort = -1;
int playerType[2] = {HUMAN, COMPUTER};
atomic_bool gamming(false);
std::map<QString, int> mp = {{"人类", HUMAN},
                             {"电脑", COMPUTER},
                             {"远程玩家", REMOTE}};

void sendMove(int move)
{
    QJsonObject j;
    j.insert("magic", MAGIC_NUM);
    j.insert("id", move);
    QJsonDocument document;
    document.setObject(j);
    QByteArray datagram = document.toJson(QJsonDocument::Compact);
    auto sender = new QUdpSocket(nullptr);
    QHostAddress host(serverIP);
    sender->writeDatagram(datagram, host, serverPort);
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
int totalTime = 0;
#ifdef USE_PYTHON_AGENT
void MainWindow::AI(const State& s, int thinkingLevel = THINKINGLEVEL)
{
    QJsonObject j;
    j.insert("magic", MAGIC_NUM);
    j.insert("state", QJsonValue(QString::fromUtf8(s.data(), GAMESCALE * GAMESCALE)));
    j.insert("chesscolor", getCurrentPlayerChessColor());
    QJsonDocument document;
    document.setObject(j);
    QByteArray datagram = document.toJson(QJsonDocument::Compact);
    python_connector->writeDatagram(datagram, QHostAddress("127.0.0.1"), PYTHON_PORT);
}
#else
void MainWindow::AI(const State& s, int thinkingLevel = THINKINGLEVEL)
{
//    QThread::sleep(1);
    int bestMove = -1;
    QTime time;
    time.start();
    //qDebug() << "AI搜索层数" << thinkingLevel << "层";
    qDebug() << "最佳效用值" << AlphaBeta(s, getCurrentPlayerChessColor(), thinkingLevel, positionalValue, INT_MIN, INT_MAX, bestMove);
    //qDebug() << "搜索耗时" << time.elapsed() << "(ms)";
    totalTime += time.elapsed();
    qDebug() << "Total Time" << totalTime << "(ms)";
    if(bestMove == -1)
    {
        gamming = false;
    }
    else
    {
        sendMove(bestMove);
    }
}
#endif


MainWindow::MainWindow(QWidget *parent, size_t row, size_t col) :
    QMainWindow(parent), ui(new Ui::MainWindow), row(row), col(col), exit(false)
{
    ui->setupUi(this);
    grid = new Grid<ChessBlock>(this, ui->gridLayout_2, row, col);
    resetBoard();
    ui->gridLayout_2->setContentsMargins(0,0,0,this->height() - (this->width() - ui->lcdNumber->width()));


    control_thread = new std::thread(&MainWindow::control_func, this);
    room_thread = new std::thread(&MainWindow::room_func, this);
    ui->lcdNumber->setAutoFillBackground(true);
    ui->lineEdit->setPlaceholderText("远程玩家 IP:Port");

    connect(this, &MainWindow::roomCreate, [this](int port){
        ui->lineEdit->setText(serverIP + ":" + QString::number(serverPort));
        ui->statusBar->showMessage("房间端口" + QString::number(port));
    });
    connect(this, SIGNAL(remoteChallengeEvent(QString)), this, SLOT(on_remoteChallengeEvent(QString)));
    connect(this, &MainWindow::reqRepaint, this, &MainWindow::on_reqRepaint);
    connect(this, &MainWindow::gameOverEvent, this, &MainWindow::on_GameOver);
    connect(this, &MainWindow::startGameEvent, this, &MainWindow::on_GameStart);
}

MainWindow::~MainWindow()
{
    exit = true;
    control_thread->join();
    delete ui;
}

void MainWindow::control_func()
{
    // UDP随机监听一个端口
    receiver = new QUdpSocket(0);
    receiver->bind(0, QAbstractSocket::ShareAddress);
    // 获取监听的端口号
    auto port = receiver->localPort();
    localPort = port;
    while (!exit) {
        QThread::msleep(50);
        QByteArray datagram;
        // 查看有没有收到包
        auto size = receiver->pendingDatagramSize();
        if(!size) continue;
        QHostAddress addr;
        quint16 port;
        datagram.resize(size);
        int id;
        QJsonParseError jsonError;
        QJsonDocument document;
        // 如果收到就读取，放到一个datagram里面
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
        if(object.contains("gamestatus"))
        {
            if(object.value("gamestatus").toBool())
            {
                emit startGameEvent();
            }
            else {
                emit gameOverEvent();
            }
        }
        else if(object.contains("id"))
        {
            id = object.value("id").toInt();
#ifdef USE_LETTER_LABEL
            qDebug() << "玩家" << (currentPlayer ? "二" : "一") << "落子在" << (id / GAMESCALE) + 1 << "行" << (char)((id % GAMESCALE) + 'A') << "列";
#else
            qDebug() << "玩家" << (currentPlayer ? "二" : "一") << "落子在" << (id / GAMESCALE)  << "行" << (id % GAMESCALE)  << "列";
#endif
            auto block = id;
            auto this_row = block/col;
            auto this_col = (block - (block / col) * row);
            processMove(this_row, this_col);
        }
        QApplication::processEvents();
    }
}


void MainWindow::room_func()
{
    int askStartCnt = 0;
    room = new QUdpSocket(0);
    room->bind(0, QAbstractSocket::ShareAddress);
    auto port = room->localPort();
    python_connector = new QUdpSocket(0);
    python_connector->bind(0, QAbstractSocket::ShareAddress);
    auto python_conn_port = python_connector->localPort();
    serverPort = port;
    emit roomCreate(port);
    vector<QHostAddress> playerAddr;
    vector<int> playerPort;
    bool start = false;
//    QFile f("/tmp/room_type");
//    f.open(QIODevice::WriteOnly);
//    QTextStream out(&f);
//    out << serverPort;
//    f.close();
#ifdef USE_PYTHON_AGENT
    bool have_responce = false;
    QJsonObject j;
    j.insert("magic", MAGIC_NUM);
    j.insert("roomport", serverPort);
    while(localPort == -1);
    j.insert("localport", localPort);
    j.insert("connectport", python_conn_port);
    QJsonDocument document;
    document.setObject(j);
    QByteArray datagram = document.toJson(QJsonDocument::Compact);
    auto sender = new QUdpSocket(nullptr);
    while(!have_responce)
    {
        sender->writeDatagram(datagram, QHostAddress("127.0.0.1"), PYTHON_PORT);
        QThread::msleep(50);
        auto size = room->pendingDatagramSize();
        if(!size) continue;
        qDebug() << size;
        QByteArray datagram2;
        datagram2.resize(size);
        room->readDatagram(datagram2.data(), datagram2.size());
        qDebug() << datagram2;
        auto var = convertToJSON(datagram2);
        if(var.isNull()) continue;
        auto object = var.toJsonObject();
        if(object.contains("responce"))
            have_responce = true;
        qDebug() << "GOOD";
    }
#endif
    while (!exit) {
        QThread::msleep(50);
        auto size = room->pendingDatagramSize();
        if(!size) continue;
        QByteArray datagram;
        QHostAddress addr;
        quint16 port;
        datagram.resize(size);
        int id;
        room->readDatagram(datagram.data(), datagram.size(), &addr, &port);
        auto var = convertToJSON(datagram);
        if(var.isNull()) continue;
        auto object = var.toJsonObject();
        if(!start)
        {
            if(!object.contains("addr") || !object.value("addr").isString())
                continue;
            if(!object.contains("port"))
                continue;
            auto addr = QHostAddress(object.value("addr").toString());
            auto port = object.value("port").toInt();
            if(std::find(playerAddr.begin(), playerAddr.end(), addr) == playerAddr.end() || std::find(playerPort.begin(), playerPort.end(), port) == playerPort.end())
            {
                playerAddr.push_back(addr);
                playerPort.push_back(port);
            }
            if(++askStartCnt == 2)
            {
                start = true;
                sendGameStatus(playerAddr, playerPort, true);
                askStartCnt = 0;
            }
        }
        else if(object.contains("id"))
        {
            id = object.value("id").toInt();
#ifdef USE_LETTER_LABEL
            qDebug() << "玩家" << (currentPlayer ? "二" : "一") << "落子在" << (id / GAMESCALE) + 1 << "行" << (char)((id % GAMESCALE) + 'A') << "列";
#else
//            qDebug() << "玩家" << (currentPlayer ? "二" : "一") << "落子在" << (id / GAMESCALE) + 1 << "行" << (id % GAMESCALE) + 1 << "列";
#endif
            sendBackMove(playerAddr, playerPort,id);
        }
        else if(object.contains("gamestatus") && object.value("gamestatus").isBool() && object.value("gamestatus").toBool() == false)
        {
            sendGameStatus(playerAddr, playerPort, false);
            start = false;
            playerAddr.clear();
            playerPort.clear();
        }
    }
}

void MainWindow::sendGameStatus(const vector<QHostAddress>& addrs, const vector<int> ports, bool stat)
{
    for(size_t i = 0; i < addrs.size(); ++i)
    {
        QJsonObject j;
        j.insert("magic", MAGIC_NUM);
        j.insert("gamestatus", stat);
        QJsonDocument document;
        document.setObject(j);
        QByteArray datagram = document.toJson(QJsonDocument::Compact);
        auto sender = new QUdpSocket(nullptr);
        sender->writeDatagram(datagram, addrs[i], ports[i]);
    }
}

void MainWindow::sendBackMove(const vector<QHostAddress> &addrs, const vector<int> ports, int id)
{

    for(size_t i = 0; i < addrs.size(); ++i)
    {
        QJsonObject j;
        j.insert("magic", MAGIC_NUM);
        j.insert("id", id);
        QJsonDocument document;
        document.setObject(j);
        QByteArray datagram = document.toJson(QJsonDocument::Compact);
        auto sender = new QUdpSocket(nullptr);
        sender->writeDatagram(datagram, addrs[i], ports[i]);
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
void MainWindow::applyState(const State& s)
{
    for(size_t i = 0; i < GAMESCALE * GAMESCALE; ++i)
    {
        auto row = i / GAMESCALE;
        auto col = i % GAMESCALE;
        processGrid(row, col, s[i]);
    }
}

#define START_RL
void MainWindow::startGameConfirmed()
{
    ui->pushButton_2->setEnabled(false);
    ui->pushButton_3->setEnabled(true);
    grid->reDraw();
    resetBoard();
    playerType[0] = mp[ui->comboBox_p1->currentText()];
    playerType[1] = mp[ui->comboBox_p2->currentText()];
#ifdef START_RL
    processGrid(GAMESCALE / 2 - 1, GAMESCALE / 2 - 1, WHITE);
    processGrid(GAMESCALE / 2 - 1, GAMESCALE / 2, BLACK);
    processGrid(GAMESCALE / 2, GAMESCALE / 2 - 1, BLACK);
    processGrid(GAMESCALE / 2, GAMESCALE / 2, WHITE);
#else
    processGrid(GAMESCALE / 2 - 1, GAMESCALE / 2 - 1, BLACK);
    processGrid(GAMESCALE / 2 - 1, GAMESCALE / 2, WHITE);
    processGrid(GAMESCALE / 2, GAMESCALE / 2 - 1, WHITE);
    processGrid(GAMESCALE / 2, GAMESCALE / 2, BLACK);
#endif
    qDebug() << "Player " << getFirstPlayer() << " uses black chess";
    setCurrentPlayer(getFirstPlayer());
    auto ret = getAvail(toState(), getCurrentPlayerChessColor());
    drawAvailNum(ret);
    reqRepaint();
    gamming = true;
    if(playerType[currentPlayer] == COMPUTER)
    {
        ai_thread = new std::thread(&MainWindow::AI, this, toState(), 10);
    }
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
    if(color == EMPTY) return;
    grid->setOverlayPic(x, y, color == WHITE ? WCFN : BCFN);
    grid->pBlocks[x][y]->color = color;
    grid->pBlocks[x][y]->isOccupied = true;
//    emit reqRepaint();
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
    int numB =  ui->label_4->text().split(":",QString::SkipEmptyParts)[1].toInt();
    int numW =  ui->label_5->text().split(":",QString::SkipEmptyParts)[1].toInt();
    qDebug() << "Game Over " << numB << ":" << numW;
    QMessageBox::information(this, "游戏结束", numB > numW ? "黑子胜" : "白子胜");
    ui->pushButton_2->setEnabled(true);
    if(ai_thread && ai_thread->joinable())
        ai_thread->join();
    gamming = false;
    resetTimer();
    ui->pushButton_3->setEnabled(false);
}

void MainWindow::on_GameStart()
{
    startGameConfirmed();
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
    QStringList lst = ui->lineEdit->text().split(':', QString::SkipEmptyParts);
    serverIP = lst[0];
    serverPort = lst[1].toInt();
    for(int i = 0;i < 2; ++i)
    {
        if(playerType[i] != REMOTE)
        {
            QJsonObject j;
            j.insert("magic", MAGIC_NUM);
            j.insert("addr", localIP);
            j.insert("port", localPort);
            QJsonDocument document;
            document.setObject(j);
            QByteArray datagram = document.toJson(QJsonDocument::Compact);
            auto sender = new QUdpSocket(nullptr);
            QHostAddress host(serverIP);
            sender->writeDatagram(datagram, host, serverPort);
        }
    }
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
    ui->label_4->setText("黑子数量:" + QString::number(b));
    ui->label_5->setText("白子数量:" + QString::number(w));
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
            QJsonObject j;
            j.insert("magic", MAGIC_NUM);
            j.insert("gamestatus", false);
            QJsonDocument document;
            document.setObject(j);
            QByteArray datagram = document.toJson(QJsonDocument::Compact);
            auto sender = new QUdpSocket(nullptr);
            QHostAddress host(serverIP);
            sender->writeDatagram(datagram, host, serverPort);
            return;
        }
    }
    if(playerType[currentPlayer] == COMPUTER)
    {
        ai_thread = new std::thread(&MainWindow::AI, this, toState(), 10);
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
//    if(color == WHITE) qDebug() << "Now WHITE";
//    if(color == BLACK) qDebug() << "Now BLACK";
    return color;
}

void MainWindow::on_pushButton_3_clicked()
{

    QJsonObject j;
    j.insert("magic", MAGIC_NUM);
    j.insert("gamestatus", false);
    QJsonDocument document;
    document.setObject(j);
    QByteArray datagram = document.toJson(QJsonDocument::Compact);
    auto sender = new QUdpSocket(nullptr);
    QHostAddress host(serverIP);
    sender->writeDatagram(datagram, host, serverPort);


}

void MainWindow::on_comboBox_p2_currentTextChanged(const QString &arg1)
{
}

void MainWindow::on_pushButton_clicked()
{
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(QString::number(serverPort));
}
