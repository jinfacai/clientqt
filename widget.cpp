#include "widget.h"
#include "ui_widget.h"
#include <QMessageBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QDir>
#include <QtEndian>
#include <QTimer>
#include <QDebug>
#include <boost/crc.hpp>
#include <QRegularExpression>

// CRC32工具函数声明
quint32 calculateHeaderCRC32(const PacketHeader& header, const QByteArray& data);
bool verifyHeaderCRC32(const PacketHeader& header, const QByteArray& data);

// 构造函数：初始化UI、TCP套接字和信号槽连接
Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
    , socket(new QTcpSocket(this))
{
    ui->setupUi(this);
    setWindowTitle("Chat Client (Not Connected)");

    // 连接按钮点击信号到对应槽函数
    connect(ui->serveButton, &QPushButton::clicked, this, &Widget::connectToServer);
    connect(socket, &QTcpSocket::connected, this, &Widget::onConnected);
    connect(socket, &QTcpSocket::errorOccurred, this, &Widget::onError);
    connect(socket, &QTcpSocket::readyRead, this, &Widget::onSocketReadyRead);
    connect(ui->sendButton, &QPushButton::clicked, this, &Widget::onSendButtonClicked);
    connect(ui->selectfileButton, &QPushButton::clicked, this, &Widget::selectFile);
    connect(ui->servefileButton, &QPushButton::clicked, this, &Widget::serveFile);

    // 文件接收按钮的信号连接
    connect(ui->acceptButton1, &QPushButton::clicked, this, &Widget::onacceptButton1clicked);
    connect(ui->acceptButton2, &QPushButton::clicked, this, &Widget::onacceptButton2clicked);
    connect(ui->acceptButton3, &QPushButton::clicked, this, &Widget::onacceptButton3clicked);
    connect(ui->refuseButton1, &QPushButton::clicked, this, &Widget::onrefuseButton1clicked);
    connect(ui->refuseButton2, &QPushButton::clicked, this, &Widget::onrefuseButton2clicked);
    connect(ui->refuseButton3, &QPushButton::clicked, this, &Widget::onrefuseButton3clicked);
    connect(ui->saveasButton1, &QPushButton::clicked, this, &Widget::onsaveasButton1clicked);
    connect(ui->saveasButton2, &QPushButton::clicked, this, &Widget::onsaveasButton2clicked);
    connect(ui->saveasButton3, &QPushButton::clicked, this, &Widget::onsaveasButton3clicked);

    //设置文件和窗口图标
    QIcon icon(":/icons/Iconka-Cat-Commerce-Client.ico");
    this->setWindowIcon(icon);
    QPixmap iconPix(":/icons/Treetog-I-Documents.ico");

    //设置隐藏滑动条，超出时显示
    ui->filescrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    ui->serveBrowser->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    ui->clientTextEdit->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);


    slotMsgIds.resize(3, 0);
    // 初始化隐藏所有文件槽相关UI组件
    QLabel* nameLabel[3] = {ui->filenamelabel1, ui->filenamelabel2, ui->filenamelabel3};
    QLabel* sizeLabel[3] = {ui->filesizelabel1, ui->filesizelabel2, ui->filesizelabel3};
    QProgressBar* progressbar[3] = {ui->progressBar1, ui->progressBar2, ui->progressBar3};
    QPushButton* acceptBtn[3] = {ui->acceptButton1, ui->acceptButton2, ui->acceptButton3};
    QPushButton* saveasBtn[3] = {ui->saveasButton1, ui->saveasButton2, ui->saveasButton3};
    QPushButton* refuseBtn[3] = {ui->refuseButton1, ui->refuseButton2, ui->refuseButton3};
    QLabel* fileicolabel[3] = {ui->fileicolabel1,ui->fileicolabel2,ui->fileicolabel3};

    for (int i = 0; i < 3; ++i) {
        nameLabel[i]->setVisible(false);
        sizeLabel[i]->setVisible(false);
        progressbar[i]->setVisible(false);
        acceptBtn[i]->setVisible(false);
        saveasBtn[i]->setVisible(false);
        refuseBtn[i]->setVisible(false);
        fileicolabel[i]->setVisible(false);
        fileicolabel[i]->setPixmap(iconPix);

    }
}

// 析构函数：清理UI
Widget::~Widget()
{
    delete ui;
}

// 连接服务器
void Widget::connectToServer()
{
    // 获取输入的IP地址和端口
    QString ip = ui->serveipLineEdit->text().trimmed();//trimmed去除两端空白
    QString portStr = ui->portLineEdit->text().trimmed();

    // 验证IP地址是否为空
    if (ip.isEmpty()) {
        QMessageBox::critical(this, "输入错误", "服务器IP地址不能为空！");
        ui->serveipLineEdit->setFocus();//聚焦输入框
        return;
    }

    // 验证端口号是否为空
    if (portStr.isEmpty()) {
        QMessageBox::critical(this, "输入错误", "端口号不能为空！");
        ui->portLineEdit->setFocus();
        return;
    }

    // 验证端口号是否为有效数字
    bool ok;
    quint16 port = portStr.toUShort(&ok);
    if (!ok) {
        QMessageBox::critical(this, "输入错误", "端口号必须是0-65535之间的数字！");
        ui->portLineEdit->setFocus();
        return;
    }

    // 验证端口号范围
    if (port == 0) {
        QMessageBox::critical(this, "输入错误", "无效的端口号！");
        ui->portLineEdit->setFocus();
        return;
    }

    // 尝试连接服务器
    socket->connectToHost(ip, port);

}

// 连接成功处理
void Widget::onConnected()
{
    QMessageBox::information(this, "连接成功", "已成功连接到服务器");
    setWindowTitle("Chat Client (Waiting for ID)");
}

// 连接错误处理
void Widget::onError(QAbstractSocket::SocketError socketError)
{
    Q_UNUSED(socketError)
    QMessageBox::critical(this, "连接失败", socket->errorString());
}

// 发送按钮点击处理
void Widget::onSendButtonClicked()
{
    QString text = ui->clientTextEdit->toPlainText().trimmed();
    if (text.isEmpty()) {
        QMessageBox::information(this, "发送失败", "文本内容为空");
        return;
    }
    sendTextMessage(text);
    ui->clientTextEdit->clear();
    // 只显示"我"
    //ui->serveBrowser->append("[我] " + text);
    //添加时间显示
    QString currentTime = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
    ui->serveBrowser->append(QString("[%1]\n[我] %2").arg(currentTime, text));
}

// 发送文本消息
void Widget::sendTextMessage(const QString& text)
{
    QByteArray data = text.toUtf8();
    PacketHeader header = {};
    header.version = MY_PROTOCOOL_VERSION;
    header.msg_type = MSG_TYPE_TEXT;
    header.datalen = qToBigEndian<quint32>(data.size());
    header.filename_len = 0;
    header.file_size = 0;
    header.msg_id = qToBigEndian<quint32>(nextMsgId());
    header.chunk_index = 0;
    header.chunk_count = qToBigEndian<quint32>(1);
    header.sender_id = qToBigEndian<quint32>(clientId);
    // 计算CRC32
    header.crc32 = qToBigEndian<quint32>(calculateHeaderCRC32(header, data));
    // 发送header+data
    qint64 headerWritten = socket->write(reinterpret_cast<const char*>(&header), sizeof(header));
    if (headerWritten == -1) {
        // 写入失败，获取错误信息
        QString errorMsg = QString("发送协议头失败: %1\n错误代码: %2")
                               .arg(socket->errorString())
                               .arg(socket->error());

        // 显示错误对话框
        QMessageBox::critical(this, "网络错误", errorMsg);

        // 记录错误日志
        qCritical() << "Header write error:" << errorMsg;

        // 关闭连接并清理资源
        socket->disconnectFromHost();
        if (socket->state() != QAbstractSocket::UnconnectedState) {
            socket->waitForDisconnected(1000);
        }
        return;
    }
    // 检查是否完整写入
    else if (headerWritten != sizeof(header)) {
        QString errorMsg = QString("协议头未完整发送\n已发送: %1 字节/应发送: %2 字节")
                               .arg(headerWritten)
                               .arg(sizeof(header));

        QMessageBox::warning(this, "网络警告", errorMsg);
        qWarning() << "Incomplete header write:" << errorMsg;
        //考虑是否重发
        return;
    }
    qint64 dataWritten = socket->write(data);
    if (dataWritten == -1) {
        QString errorMsg = QString("发送数据失败: %1\n错误代码: %2")
                               .arg(socket->errorString())
                               .arg(socket->error());

        QMessageBox::critical(this, "网络错误", errorMsg);
        qCritical() << "Data write error:" << errorMsg;

        socket->disconnectFromHost();
        if (socket->state() != QAbstractSocket::UnconnectedState) {
            socket->waitForDisconnected(1000);
        }
        return;
    }
    // 检查数据是否完整写入
    else if (dataWritten != data.size()) {
        QString errorMsg = QString("数据未完整发送\n已发送: %1 字节/应发送: %2 字节")
                               .arg(dataWritten)
                               .arg(data.size());

        QMessageBox::warning(this, "网络警告", errorMsg);
        qWarning() << "Incomplete data write:" << errorMsg;

        // 对于未完整发送的情况，可以尝试重发或记录状态

    }
}

// 选择文件
void Widget::selectFile()
{
    QString fileName = QFileDialog::getOpenFileName(this, "请选择要发送的文件");
    if (fileName.isEmpty()) {
        QMessageBox::information(this, "发送失败", "选择文件为空");
        return;
    }
    selectedFilePath = fileName;
    ui->fileBrowser->setText(QFileInfo(fileName).fileName());
}

// 发送文件按钮处理
void Widget::serveFile()
{
    if (selectedFilePath.isEmpty()) {
        QMessageBox::information(this, "提示", "请先选择文件");
        return;
    }
    sendFile(selectedFilePath);
}

// 发送文件主逻辑
void Widget::sendFile(const QString& path)
{
    QFile* file = new QFile(path);
    if (!file->open(QIODevice::ReadOnly)) {
        QMessageBox::critical(this, "错误", "无法打开文件");
        delete file;
        return;
    }

    // 文件名合法性校验
    QString safeFileName = QFileInfo(*file).fileName();
    if (safeFileName.isEmpty() || safeFileName.length() > 100) {
        QMessageBox::critical(this, "错误", "文件名为空或过长！");
        file->close();
        delete file;
        return;
    }

    // 使用QRegularExpression进行校验
    QRegularExpression invalidPattern("[\\\\/:*?\"<>|]");
    QRegularExpressionMatch match = invalidPattern.match(safeFileName);
    if (match.hasMatch()) {
        QMessageBox::critical(this, "错误", "文件名包含非法字符！");
        file->close();
        delete file;
        return;
    }
    quint64 fileSize = file->size();
    quint32 msg_id = nextMsgId();
    quint32 chunk_count = (fileSize + CHUNK_SIZE - 1) / CHUNK_SIZE;
    // 发送文件开始
    PacketHeader startHeader = {};
    startHeader.version = MY_PROTOCOOL_VERSION;
    startHeader.msg_type = MSG_TYPE_FILE_START;
    startHeader.datalen = 0;
    startHeader.filename_len = qToBigEndian<quint32>(safeFileName.size());
    startHeader.file_size = qToBigEndian<quint64>(fileSize);
    startHeader.msg_id = qToBigEndian<quint32>(msg_id);
    startHeader.chunk_index = 0;
    startHeader.chunk_count = qToBigEndian<quint32>(chunk_count);
    startHeader.sender_id = qToBigEndian<quint32>(clientId);
    // 计算CRC32（header+文件名）
    startHeader.crc32 = qToBigEndian<quint32>(calculateHeaderCRC32(startHeader, safeFileName.toUtf8()));
    // 发送header+文件名
    qint64 headerWritten = socket->write(reinterpret_cast<const char*>(&startHeader), sizeof(startHeader));
    if (headerWritten == -1) {
        // 写入失败，获取错误信息
        QString errorMsg = QString("发送文件开始协议头失败: %1\n错误代码: %2")
                               .arg(socket->errorString())
                               .arg(socket->error());

        // 显示错误对话框
        QMessageBox::critical(this, "文件传输错误", errorMsg);

        // 记录错误日志
        qCritical() << "File start header write error:" << errorMsg;

        // 清理文件资源
        file->close();
        delete file;

        // 关闭连接
        socket->disconnectFromHost();
        if (socket->state() != QAbstractSocket::UnconnectedState) {
            socket->waitForDisconnected(1000);
        }
        return;
    }
    // 检查是否完整写入
    else if (headerWritten != sizeof(startHeader)) {
        QString errorMsg = QString("文件开始协议头未完整发送\n已发送: %1 字节/应发送: %2 字节")
                               .arg(headerWritten)
                               .arg(sizeof(startHeader));

        QMessageBox::warning(this, "文件传输警告", errorMsg);
        qWarning() << "Incomplete file start header write:" << errorMsg;

        // 清理文件资源
        file->close();
        delete file;
        return;
    }
    // 尝试写入文件名
    qint64 nameWritten = socket->write(safeFileName.toUtf8());
    if (nameWritten == -1) {
        QString errorMsg = QString("发送文件名失败: %1\n错误代码: %2")
                               .arg(socket->errorString())
                               .arg(socket->error());

        QMessageBox::critical(this, "文件传输错误", errorMsg);
        qCritical() << "File name write error:" << errorMsg;

        // 清理文件资源
        file->close();
        delete file;

        // 关闭连接
        socket->disconnectFromHost();
        if (socket->state() != QAbstractSocket::UnconnectedState) {
            socket->waitForDisconnected(1000);
        }
        return;
    }
    // 检查文件名是否完整写入
    else if (nameWritten != safeFileName.size()) {
        QString errorMsg = QString("文件名未完整发送\n已发送: %1 字节/应发送: %2 字节")
                               .arg(nameWritten)
                               .arg(safeFileName.size());

        QMessageBox::warning(this, "文件传输警告", errorMsg);
        qWarning() << "Incomplete file name write:" << errorMsg;

        // 清理文件资源
        file->close();
        delete file;
        return;
    }
    // 初始化发送任务
    FileSendTask task;
    task.file = file;
    task.msg_id = msg_id;
    task.chunk_count = chunk_count;
    task.current_chunk = 0;
    task.acked = QVector<bool>(chunk_count, false);
    task.timer = nullptr;
    task.filename = safeFileName.toUtf8();
    task.filesize = fileSize;
    sendTasks[msg_id] = task;
    sendNextChunk(msg_id);
}

//发送下一个文件块数据
void Widget::sendNextChunk(quint32 msg_id)
{
    if (!sendTasks.contains(msg_id)) {
        QMessageBox::information(this, "发送失败", "消息ID为空");
        return;
    }
    FileSendTask& task = sendTasks[msg_id];
    if (task.current_chunk >= task.chunk_count) {
        // 文件结束
        PacketHeader endHeader = {};
        endHeader.version = MY_PROTOCOOL_VERSION;
        endHeader.msg_type = MSG_TYPE_FILE_END;
        endHeader.datalen = 0;
        endHeader.filename_len = 0;
        endHeader.file_size = 0;
        endHeader.msg_id = qToBigEndian<quint32>(msg_id);
        endHeader.chunk_index = 0;
        endHeader.chunk_count = 0;
        endHeader.sender_id = qToBigEndian<quint32>(clientId);
        // 计算CRC32（header本身）
        endHeader.crc32 = qToBigEndian<quint32>(calculateHeaderCRC32(endHeader));
        qint64 endHeaderWritten = socket->write(reinterpret_cast<const char*>(&endHeader), sizeof(endHeader));
        // 错误处理
        if (endHeaderWritten == -1) {
            QString errorMsg = QString("发送文件结束协议头失败: %1\n错误代码: %2")
                                   .arg(socket->errorString())
                                   .arg(socket->error());

            QMessageBox::critical(this, "文件传输错误", errorMsg);
            qCritical() << "File end header write error:" << errorMsg;
        }
        else if (endHeaderWritten != sizeof(endHeader)) {
            QString errorMsg = QString("文件结束协议头未完整发送\n已发送: %1 字节/应发送: %2 字节")
                                   .arg(endHeaderWritten)
                                   .arg(sizeof(endHeader));

            QMessageBox::warning(this, "文件传输警告", errorMsg);
            qWarning() << "Incomplete file end header write:" << errorMsg;
            //后续加上重传
        }
        task.file->close();
        delete task.file;
        if (task.timer) { task.timer->stop(); delete task.timer; }
        sendTasks.remove(msg_id);

        // 只显示"我"
        QString currentTime = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
        //ui->serveBrowser->append(QString("[我] 发送了文件：%1").arg(task.filename));
        ui->serveBrowser->append(QString("[%1]\n[我] 发送了文件：%2").arg(currentTime, task.filename));
        ui->fileBrowser->clear();
        return;
    }
    if (task.acked[task.current_chunk]) {
        task.current_chunk++;
        sendNextChunk(msg_id);
        return;
    }
    task.file->seek(task.current_chunk * CHUNK_SIZE);
    QByteArray chunk = task.file->read(CHUNK_SIZE);
    PacketHeader chunkHeader = {};
    chunkHeader.version = MY_PROTOCOOL_VERSION;
    chunkHeader.msg_type = MSG_TYPE_FILE_CHUNK;
    chunkHeader.datalen = qToBigEndian<quint32>(chunk.size());
    chunkHeader.filename_len = 0;
    chunkHeader.file_size = 0;
    chunkHeader.msg_id = qToBigEndian<quint32>(msg_id);
    chunkHeader.chunk_index = qToBigEndian<quint32>(task.current_chunk);
    chunkHeader.chunk_count = qToBigEndian<quint32>(task.chunk_count);
    chunkHeader.sender_id = qToBigEndian<quint32>(clientId);
    // 计算CRC32（header+chunk）
    chunkHeader.crc32 = qToBigEndian<quint32>(calculateHeaderCRC32(chunkHeader, chunk));
    // 发送header+chunk
    qint64 headerWritten = socket->write(reinterpret_cast<const char*>(&chunkHeader), sizeof(chunkHeader));
    // 错误处理 - 协议头写入
    if (headerWritten == -1) {
        // 写入失败
        QString errorMsg = QString("发送文件块协议头失败: %1\n错误代码: %2")
                               .arg(socket->errorString())
                               .arg(socket->error());

        QMessageBox::critical(this, "文件传输错误", errorMsg);
        qCritical() << "File chunk header write error:" << errorMsg;

        // 清理资源
        if (task.file) {
            task.file->close();
            delete task.file;
        }
        if (task.timer) {
            task.timer->stop();
            delete task.timer;
        }
        sendTasks.remove(msg_id);
        return;
    }
    else if (headerWritten != sizeof(chunkHeader)) {
        // 部分写入
        QString errorMsg = QString("文件块协议头未完整发送\n已发送: %1 字节/应发送: %2 字节")
                               .arg(headerWritten)
                               .arg(sizeof(chunkHeader));

        QMessageBox::warning(this, "文件传输警告", errorMsg);
        qWarning() << "Incomplete file chunk header write:" << errorMsg;

        // 清理资源
        if (task.file) {
            task.file->close();
            delete task.file;
        }
        if (task.timer) {
            task.timer->stop();
            delete task.timer;
        }
        sendTasks.remove(msg_id);
        return;
    }
    // 尝试写入块数据
    qint64 chunkWritten = socket->write(chunk);
    // 错误处理 - 块数据写入
    if (chunkWritten == -1) {
        // 写入失败
        QString errorMsg = QString("发送文件块数据失败: %1\n错误代码: %2")
                               .arg(socket->errorString())
                               .arg(socket->error());

        QMessageBox::critical(this, "文件传输错误", errorMsg);
        qCritical() << "File chunk data write error:" << errorMsg;

        // 清理资源
        if (task.file) {
            task.file->close();
            delete task.file;
        }
        if (task.timer) {
            task.timer->stop();
            delete task.timer;
        }
        sendTasks.remove(msg_id);
        return;
    }
    else if (chunkWritten != chunk.size()) {
        // 部分写入
        QString errorMsg = QString("文件块数据未完整发送\n已发送: %1 字节/应发送: %2 字节")
                               .arg(chunkWritten)
                               .arg(chunk.size());

        QMessageBox::warning(this, "文件传输警告", errorMsg);
        qWarning() << "Incomplete file chunk data write:" << errorMsg;

        // 清理资源
        if (task.file) {
            task.file->close();
            delete task.file;
        }
        if (task.timer) {
            task.timer->stop();
            delete task.timer;
        }
        sendTasks.remove(msg_id);
        return;
    }
    // 启动定时器
    if (!task.timer) {
        task.timer = new QTimer(this);
        connect(task.timer, &QTimer::timeout, this, [this, msg_id]() {
            resendCurrentChunk(msg_id);
        });
    }
    task.timer->start(3000); // 3秒超时
}

//重传文件块数据
void Widget::resendCurrentChunk(quint32 msg_id)
{
    if (!sendTasks.contains(msg_id)){
        qWarning() << "重传失败: 无效的消息ID" << msg_id;
        return;
    }
    FileSendTask& task = sendTasks[msg_id];
    task.file->seek(task.current_chunk * CHUNK_SIZE);
    QByteArray chunk = task.file->read(CHUNK_SIZE);
    PacketHeader chunkHeader = {};
    chunkHeader.version = MY_PROTOCOOL_VERSION;
    chunkHeader.msg_type = MSG_TYPE_FILE_CHUNK;
    chunkHeader.datalen = qToBigEndian<quint32>(chunk.size());
    chunkHeader.filename_len = 0;
    chunkHeader.file_size = 0;
    chunkHeader.msg_id = qToBigEndian<quint32>(msg_id);
    chunkHeader.chunk_index = qToBigEndian<quint32>(task.current_chunk);
    chunkHeader.chunk_count = qToBigEndian<quint32>(task.chunk_count);
    chunkHeader.sender_id = qToBigEndian<quint32>(clientId);
    // 计算CRC32（header+chunk）
    chunkHeader.crc32 = qToBigEndian<quint32>(calculateHeaderCRC32(chunkHeader, chunk));
    // 发送header+chunk
    qint64 headerWritten = socket->write(reinterpret_cast<const char*>(&chunkHeader), sizeof(chunkHeader));
    // 错误处理 - 协议头写入
    if (headerWritten == -1) {
        QString errorMsg = QString("重传文件块协议头失败: %1\n错误代码: %2")
                               .arg(socket->errorString())
                               .arg(socket->error());

        qWarning() << "Retry chunk header write error:" << errorMsg;
    }
    else if (headerWritten != sizeof(chunkHeader)) {
        QString errorMsg = QString("重传文件块协议头未完整发送\n已发送: %1 字节/应发送: %2 字节")
                               .arg(headerWritten)
                               .arg(sizeof(chunkHeader));

        qWarning() << "Incomplete retry chunk header write:" << errorMsg;
    }
    // 尝试写入块数据
    qint64 chunkWritten = socket->write(chunk);
    // 错误处理 - 块数据写入
    if (chunkWritten == -1) {
        QString errorMsg = QString("重传文件块数据失败: %1\n错误代码: %2")
                               .arg(socket->errorString())
                               .arg(socket->error());

        qWarning() << "Retry chunk data write error:" << errorMsg;
    }
    else if (chunkWritten != chunk.size()) {
        QString errorMsg = QString("重传文件块数据未完整发送\n已发送: %1 字节/应发送: %2 字节")
                               .arg(chunkWritten)
                               .arg(chunk.size());

        qWarning() << "Incomplete retry chunk data write:" << errorMsg;
    }

    // 重新启动定时器
    if (task.timer) task.timer->start(3000);
}

//处理ACK
void Widget::handleAck(const PacketHeader& header)
{
    if (!verifyHeaderCRC32(header)) {
        qCritical() << "[CRC32校验失败] ACK包被丢弃, msg_id:" << qFromBigEndian(header.msg_id);
        return;
    }
    quint32 msg_id = qFromBigEndian(header.msg_id);
    quint32 chunk_index = qFromBigEndian(header.chunk_index);
    if (!sendTasks.contains(msg_id)){
        qWarning() << "ACK失败: 无效的消息ID" << msg_id;
        return;
    }
    FileSendTask& task = sendTasks[msg_id];
    if (chunk_index < task.acked.size()) task.acked[chunk_index] = true;
    if (chunk_index == task.current_chunk) {
        if (task.timer) task.timer->stop();
        task.current_chunk++;
        sendNextChunk(msg_id);
    }
}

//处理接收信息
void Widget::onSocketReadyRead()
{
    while (socket->bytesAvailable() >= qint64(sizeof(PacketHeader))) {
        PacketHeader header;
        qint64 headerPeeked = socket->peek(reinterpret_cast<char*>(&header), sizeof(header));
        // 头部预览错误处理
        if (headerPeeked != sizeof(header)) {
            qWarning() << "头部预览失败: 期望" << sizeof(header) << "字节, 实际预览"
                       << headerPeeked << "字节";
            qWarning() << "可用字节数:" << socket->bytesAvailable();
            return;
        }
        quint32 datalen = qFromBigEndian(header.datalen);
        quint32 filename_len = qFromBigEndian(header.filename_len);

        if (socket->bytesAvailable() < qint64(sizeof(PacketHeader) + datalen + filename_len))
            return; // 等待更多数据

        // 读取头部
        qint64 headerRead = socket->read(reinterpret_cast<char*>(&header), sizeof(header));
        // 头部读取错误处理
        if (headerRead != sizeof(header)) {
            qCritical() << "头部读取失败: 期望" << sizeof(header) << "字节, 实际读取"
                        << headerRead << "字节";
            qCritical() << "错误信息:" << socket->errorString();
            return;
        }
        switch (header.msg_type) {
        case MSG_TYPE_TEXT:
            handleTextMessage(header);
            break;
        case MSG_TYPE_FILE_START:
            handleFileStart(header);
            break;
        case MSG_TYPE_FILE_CHUNK:
            handleFileChunk(header);
            break;
        case MSG_TYPE_FILE_END:
            handleFileEnd(header);
            break;
        case MSG_TYPE_ACK:
            handleAck(header);
            break;
        case MSG_TYPE_ID_ASSIGN:
            handleIdAssign(header);
            break;
        default:
            socket->read(datalen + filename_len); // 跳过未知数据
            break;
        }
    }
}

//处理文本消息
void Widget::handleTextMessage(const PacketHeader& header)
{
    quint32 datalen = qFromBigEndian(header.datalen);
    QByteArray data = socket->read(datalen);
    if (!verifyHeaderCRC32(header, data)) {
        qCritical() << "[CRC32校验失败] 文本消息被丢弃, msg_id:" << qFromBigEndian(header.msg_id);
        return;
    }
    if (data.size() != datalen) {
        qCritical() << "文本消息数据读取失败: 期望" << datalen << "字节, 实际读取"
                    << data.size() << "字节";
        qCritical() << "错误信息:" << socket->errorString();
        return;
    }
    QString text = QString::fromUtf8(data);
    quint32 sender_id = qFromBigEndian(header.sender_id);
    // 无论是谁发的消息，都显示[客户端X]
    //QString senderText = QString("[客户端%1] ").arg(sender_id);
    //QString currentTime = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
    //ui->serveBrowser->append(senderText + text);
    QString currentTime = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
    QString senderText = QString("[客户端%1] ").arg(sender_id);
    ui->serveBrowser->append(QString("[%1]\n%2%3").arg(currentTime, senderText, text));
    sendAck(qFromBigEndian(header.msg_id), 0);
}

//处理文件开始包
void Widget::handleFileStart(const PacketHeader& header)
{
    quint32 filename_len = qFromBigEndian(header.filename_len);
    quint64 filesize = qFromBigEndian(header.file_size);
    quint32 msg_id = qFromBigEndian(header.msg_id);
    QByteArray filenameData = socket->read(filename_len);
    if (!verifyHeaderCRC32(header, filenameData)) {
        qCritical() << "[CRC32校验失败] 文件开始包被丢弃, msg_id:" << msg_id;
        return;
    }
    if (filenameData.size() != filename_len) {
        qCritical() << "文件名数据读取失败: 期望" << filename_len << "字节, 实际读取"
                    << filenameData.size() << "字节";
        qCritical() << "错误信息:" << socket->errorString();
        return;
    }
    QString filename = QString::fromUtf8(filenameData);
    FileRecvInfo info;
    info.filename = filename;
    info.filesize = filesize;
    info.chunk_count = qFromBigEndian(header.chunk_count);
    info.received.resize(info.chunk_count, false);
    info.received_chunks = 0;
    info.accepted = false;
    info.saveasPath = "";
    info.file = nullptr;
    info.end_received = false;
    info.acceptProgressInitialized = false;
    // 创建临时文件
    QString tempPath = QDir::temp().filePath(filename + ".part");
    info.tempFilePath = tempPath;
    info.file = new QFile(tempPath);
    if (!info.file->open(QIODevice::WriteOnly)) {
        QMessageBox::critical(this, "错误", "无法创建临时文件: " + tempPath);
        delete info.file;
        info.file = nullptr;
        return;
    }
    recvFiles[msg_id] = info;
    int slot = findFreeSlot();
    if (slot != -1) {
        slotMsgIds[slot] = msg_id;
        updateFileSlot(slot, filename, filesize, 0);
        if (slot < 0 || slot > 2) {
            qWarning() << "updateFileSlot: invalid slot" << slot;
            return;
        }
        // 只显示文件名、大小和操作按钮，进度条保持隐藏
        QLabel* nameLabel[3] = {ui->filenamelabel1, ui->filenamelabel2, ui->filenamelabel3};
        QLabel* sizeLabel[3] = {ui->filesizelabel1, ui->filesizelabel2, ui->filesizelabel3};
        QProgressBar* bar[3] = {ui->progressBar1, ui->progressBar2, ui->progressBar3};
        QPushButton* acceptBtn[3] = {ui->acceptButton1, ui->acceptButton2, ui->acceptButton3};
        QPushButton* saveasBtn[3] = {ui->saveasButton1, ui->saveasButton2, ui->saveasButton3};
        QPushButton* refuseBtn[3] = {ui->refuseButton1, ui->refuseButton2, ui->refuseButton3};
        QLabel* fileicolabel[3] = {ui->fileicolabel1,ui->fileicolabel2,ui->fileicolabel3};
        nameLabel[slot]->setVisible(true);
        sizeLabel[slot]->setVisible(true);
        bar[slot]->setVisible(false); // 进度条隐藏
        acceptBtn[slot]->setVisible(true);
        saveasBtn[slot]->setVisible(true);
        refuseBtn[slot]->setVisible(true);
        fileicolabel[slot]->setVisible(true);

        // 显示文件接收信息
        //quint32 sender_id = qFromBigEndian(header.sender_id);
        //QString senderText = QString("[客户端%1] ").arg(sender_id);
        //ui->serveBrowser->append(QString("%1发送文件：%2").arg(senderText, filename));
        quint32 sender_id = qFromBigEndian(header.sender_id);
        QString currentTime = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
        QString senderText = QString("[客户端%1] ").arg(sender_id);
        ui->serveBrowser->append(QString("[%1]\n%2发送文件：%3").arg(currentTime, senderText, filename));
    }
    sendAck(msg_id, 0);
}

//处理文件数据块
void Widget::handleFileChunk(const PacketHeader& header)
{
    quint32 msg_id = qFromBigEndian(header.msg_id);
    quint32 chunk_index = qFromBigEndian(header.chunk_index);
    quint32 datalen = qFromBigEndian(header.datalen);
    QByteArray chunk = socket->read(datalen);
    if (!verifyHeaderCRC32(header, chunk)) {
        qCritical() << "[CRC32校验失败] 文件分片被丢弃, msg_id:" << msg_id << ", chunk_index:" << chunk_index;
        return;
    }
    if (chunk.size() != datalen) {
        QString errorMsg = QString("文件块数据读取不完整\n期望: %1 字节\n实际: %2 字节\n块索引: %3/%4")
                               .arg(datalen)
                               .arg(chunk.size())
                               .arg(chunk_index)
                               .arg(qFromBigEndian(header.chunk_count));
        qCritical() << errorMsg;
        QMessageBox::critical(this, "文件传输错误", errorMsg);
        return;
    }
    if (!recvFiles.contains(msg_id)) {
        QString errorMsg = QString("无效的文件传输ID: %1\n块索引: %2")
                               .arg(msg_id)
                               .arg(chunk_index);
        qWarning() << errorMsg;
        return;
    }
    FileRecvInfo& info = recvFiles[msg_id];
    if (!info.file) {
        qWarning() << "未打开临时文件，丢弃分片";
        return;
    }
    if (chunk_index >= info.chunk_count) {
        QString errorMsg = QString("无效的块索引\n接收: %1\n最大: %2\n文件: %3")
                               .arg(chunk_index)
                               .arg(info.chunk_count - 1)
                               .arg(info.filename);
        qCritical() << errorMsg;
        QMessageBox::critical(this, "文件传输错误", errorMsg);
        return;
    }
    qint64 offset = qint64(chunk_index) * CHUNK_SIZE;
    if (!info.file->seek(offset)) {
        QString errorMsg = QStringLiteral("文件定位失败\n位置: %1\n文件: %2\n错误: %3")
                               .arg(QString::number(offset), info.filename, info.file->errorString());
        qCritical() << errorMsg;
        QMessageBox::critical(this, "文件传输错误", errorMsg);
        return;
    }
    qint64 written = info.file->write(chunk);
    if (written != chunk.size()) {
        QString errorMsg = QStringLiteral("文件写入失败\n期望: %1 字节\n实际: %2 字节\n文件: %3\n错误: %4")
                               .arg(QString::number(chunk.size()), QString::number(written), info.filename, info.file->errorString());
        qCritical() << errorMsg;
        QMessageBox::critical(this, "文件传输错误", errorMsg);
        return;
    }
    if (!info.received[chunk_index]) {
        info.received[chunk_index] = true;
        info.received_chunks++;
    }
    int slot = findSlotByMsgId(msg_id);
    if (slot != -1) {
        // 只有已接受时才更新进度条
        if (info.accepted && info.acceptProgressInitialized) {
            int progress = int(double(info.received_chunks) / info.chunk_count * 100);
            updateFileSlot(slot, info.filename, info.filesize, progress);
        }
    }
    checkForCompletion(msg_id);
    sendAck(msg_id, chunk_index);
}

//处理文件结束块
void Widget::handleFileEnd(const PacketHeader& header)
{
    quint32 msg_id = qFromBigEndian(header.msg_id);
    if (!verifyHeaderCRC32(header)) {
        qCritical() << "[CRC32校验失败] 文件结束包被丢弃, msg_id:" << msg_id;
        return;
    }
    if (!recvFiles.contains(msg_id)) {
        qWarning() << "文件结束块失败: 无效的消息ID" << msg_id;
        return;
    }
    FileRecvInfo& info = recvFiles[msg_id];
    info.end_received = true; // 标记收到文件结束包

    // 不再直接更新UI，而是检查是否完成
    checkForCompletion(msg_id);

    sendAck(msg_id, 0);
}

//发送ACK
void Widget::sendAck(quint32 msg_id, quint32 chunk_index)
{
    PacketHeader ack = {};
    ack.version = MY_PROTOCOOL_VERSION;
    ack.msg_type = MSG_TYPE_ACK;
    ack.datalen = 0;
    ack.filename_len = 0;
    ack.file_size = 0;
    ack.msg_id = qToBigEndian<quint32>(msg_id);
    ack.chunk_index = qToBigEndian<quint32>(chunk_index);
    ack.chunk_count = 0;
    ack.sender_id = qToBigEndian<quint32>(clientId);
    // 计算CRC32（header本身）
    ack.crc32 = qToBigEndian<quint32>(calculateHeaderCRC32(ack));
    // 发送ACK
    qint64 ackWritten = socket->write(reinterpret_cast<const char*>(&ack), sizeof(ack));
    // 错误处理
    if (ackWritten == -1) {
        // 写入失败
        QString errorMsg = QStringLiteral("发送ACK包失败: %1\n错误代码: %2\n消息ID: %3\n块索引: %4")
                               .arg(socket->errorString())
                               .arg(socket->error())
                               .arg(qFromBigEndian(ack.msg_id))
                               .arg(qFromBigEndian(ack.chunk_index));

        qCritical().noquote() << errorMsg;
        QMessageBox::warning(this, "网络错误", errorMsg);
    }
    else if (ackWritten != sizeof(ack)) {
        // 部分写入
        QString errorMsg = QStringLiteral("ACK包未完整发送\n已发送: %1 字节/应发送: %2 字节\n消息ID: %3\n块索引: %4")
                               .arg(ackWritten)
                               .arg(sizeof(ack))
                               .arg(qFromBigEndian(ack.msg_id))
                               .arg(qFromBigEndian(ack.chunk_index));

        qWarning().noquote() << errorMsg;
        QMessageBox::warning(this, "网络警告", errorMsg);
    } else {
        // 成功发送
        qDebug() << "ACK包发送成功"
                 << "msg_id:" << qFromBigEndian(ack.msg_id)
                 << "chunk_index:" << qFromBigEndian(ack.chunk_index);
    }
}

//查找空闲槽
int Widget::findFreeSlot()
{
    for (int i = 0; i < slotMsgIds.size(); ++i)
        if (slotMsgIds[i] == 0) return i;
    return -1;
}

//查找槽msgid
int Widget::findSlotByMsgId(quint32 msg_id)
{
    for (int i = 0; i < slotMsgIds.size(); ++i)
        if (slotMsgIds[i] == msg_id) return i;
    return -1;
}

//更新槽
void Widget::updateFileSlot(int slot, const QString& filename, quint64 filesize, int progress)
{
    if (slot < 0 || slot > 2) {
        qWarning() << "updateFileSlot: invalid slot" << slot;
        return;
    }
    QLabel* nameLabel[3] = {ui->filenamelabel1, ui->filenamelabel2, ui->filenamelabel3};
    QLabel* sizeLabel[3] = {ui->filesizelabel1, ui->filesizelabel2, ui->filesizelabel3};
    QProgressBar* bar[3] = {ui->progressBar1, ui->progressBar2, ui->progressBar3};
    nameLabel[slot]->setText(elideFileName(filename, 20));
    sizeLabel[slot]->setText(QString("大小：%1").arg(formatFileSize(filesize)));
    bar[slot]->setValue(progress);
    nameLabel[slot]->setVisible(true);
    sizeLabel[slot]->setVisible(true);
    bar[slot]->setVisible(true);
}

//清楚文件槽
void Widget::clearFileSlot(int slot)
{
    // 1) 范围检查，防止 slot 越界
    if (slot < 0 || slot >= 3) {
        qWarning() << "clearFileSlot: invalid slot" << slot;
        return;
    }

    slotMsgIds[slot] = 0;
    QLabel* nameLabel[3] = {ui->filenamelabel1, ui->filenamelabel2, ui->filenamelabel3};
    QLabel* sizeLabel[3] = {ui->filesizelabel1, ui->filesizelabel2, ui->filesizelabel3};
    QProgressBar* bar[3] = {ui->progressBar1, ui->progressBar2, ui->progressBar3};
    QLabel* fileicolabel[3] = {ui->fileicolabel1,ui->fileicolabel2,ui->fileicolabel3};
    QPushButton* acceptBtn[3] = {ui->acceptButton1, ui->acceptButton2, ui->acceptButton3};
    QPushButton* saveasBtn[3] = {ui->saveasButton1, ui->saveasButton2, ui->saveasButton3};
    QPushButton* refuseBtn[3] = {ui->refuseButton1, ui->refuseButton2, ui->refuseButton3};

    nameLabel[slot]->clear();
    sizeLabel[slot]->clear();
    bar[slot]->setValue(0);
    nameLabel[slot]->setVisible(false);
    sizeLabel[slot]->setVisible(false);
    bar[slot]->setVisible(false);
    fileicolabel[slot]->setVisible(false);
    acceptBtn[slot]->setVisible(false);
    saveasBtn[slot]->setVisible(false);
    refuseBtn[slot]->setVisible(false);
}

//msgid增加处理
quint32 Widget::nextMsgId()
{
    return ++lastMsgId;
}

// 其它按钮槽函数可直接调用 clearFileSlot(slot) 并弹窗提示
void Widget::onacceptButton1clicked() { handleAccept(0); }
void Widget::onacceptButton2clicked() { handleAccept(1); }
void Widget::onacceptButton3clicked() { handleAccept(2); }
void Widget::onrefuseButton1clicked() { handleRefuse(0); }
void Widget::onrefuseButton2clicked() { handleRefuse(1); }
void Widget::onrefuseButton3clicked() { handleRefuse(2); }
void Widget::onsaveasButton1clicked() { handleSaveAs(0); }
void Widget::onsaveasButton2clicked() { handleSaveAs(1); }
void Widget::onsaveasButton3clicked() { handleSaveAs(2); }

//处理接收按键
void Widget::handleAccept(int slotIndex)
{
    int msg_id = slotMsgIds[slotIndex];
    if (!recvFiles.contains(msg_id)) return;
    FileRecvInfo& info = recvFiles[msg_id];
    info.accepted = true;
    QString defaultDir = "D:/git/serverqt/serverqt/download";
    QDir().mkpath(defaultDir);
    QString savePath = defaultDir + "/" + info.filename;
    info.saveasPath = savePath;
    // 不再立即 close/rename，等 checkForCompletion
    // if (info.file) info.file->close();
    // QFile::rename(info.tempFilePath, savePath);
    // UI相关
    QLabel* nameLabel[3] = {ui->filenamelabel1, ui->filenamelabel2, ui->filenamelabel3};
    QLabel* sizeLabel[3] = {ui->filesizelabel1, ui->filesizelabel2, ui->filesizelabel3};
    QProgressBar* bar[3] = {ui->progressBar1, ui->progressBar2, ui->progressBar3};
    QPushButton* acceptBtn[3] = {ui->acceptButton1, ui->acceptButton2, ui->acceptButton3};
    QPushButton* saveasBtn[3] = {ui->saveasButton1, ui->saveasButton2, ui->saveasButton3};
    QPushButton* refuseBtn[3] = {ui->refuseButton1, ui->refuseButton2, ui->refuseButton3};
    QLabel* fileicolabel[3] = {ui->fileicolabel1,ui->fileicolabel2,ui->fileicolabel3};
    acceptBtn[slotIndex]->setVisible(false);
    saveasBtn[slotIndex]->setVisible(false);
    refuseBtn[slotIndex]->setVisible(false);
    nameLabel[slotIndex]->setVisible(true);
    sizeLabel[slotIndex]->setVisible(true);
    bar[slotIndex]->setVisible(true); // 只有点击接受后才显示进度条
    fileicolabel[slotIndex]->setVisible(true);
    // 新增：初始化进度条
    initAcceptProgress(slotIndex, info);
    checkForCompletion(msg_id);
    qDebug() << "handleAccept后状态: accepted=" << info.accepted
             << "end_received=" << info.end_received
             << "received_chunks=" << info.received_chunks
             << "chunk_count=" << info.chunk_count;
}

//处理另存为按键
void Widget::handleSaveAs(int slotIndex)
{
    int msg_id = slotMsgIds[slotIndex];
    if (!recvFiles.contains(msg_id)) return;
    FileRecvInfo& info = recvFiles[msg_id];
    QString savePath = QFileDialog::getSaveFileName(this, "另存为", info.filename);
    if (savePath.isEmpty()) return;
    info.saveasPath = savePath;
    info.accepted = true;
    // 不再立即 close/rename，等 checkForCompletion
    // if (info.file) info.file->close();
    // QFile::rename(info.tempFilePath, savePath);
    // UI更新，隐藏操作按钮，显示进度条
    QLabel* nameLabel[3] = {ui->filenamelabel1, ui->filenamelabel2, ui->filenamelabel3};
    QLabel* sizeLabel[3] = {ui->filesizelabel1, ui->filesizelabel2, ui->filesizelabel3};
    QProgressBar* bar[3] = {ui->progressBar1, ui->progressBar2, ui->progressBar3};
    QPushButton* acceptBtn[3] = {ui->acceptButton1, ui->acceptButton2, ui->acceptButton3};
    QPushButton* saveasBtn[3] = {ui->saveasButton1, ui->saveasButton2, ui->saveasButton3};
    QPushButton* refuseBtn[3] = {ui->refuseButton1, ui->refuseButton2, ui->refuseButton3};
    QLabel* fileicolabel[3] = {ui->fileicolabel1,ui->fileicolabel2,ui->fileicolabel3};
    acceptBtn[slotIndex]->setVisible(false);
    saveasBtn[slotIndex]->setVisible(false);
    refuseBtn[slotIndex]->setVisible(false);
    nameLabel[slotIndex]->setVisible(true);
    sizeLabel[slotIndex]->setVisible(true);
    bar[slotIndex]->setVisible(true);
    fileicolabel[slotIndex]->setVisible(true);
    checkForCompletion(msg_id);
    QMessageBox::information(this, "另存为", QString("文件将保存到: %1").arg(savePath));
    qDebug() << "handleSaveAs: 用户另存为" << savePath;
}

void Widget::handleRefuse(int slotIndex)
{
    int msg_id = slotMsgIds[slotIndex];
    if (recvFiles.contains(msg_id)) {
        FileRecvInfo& info = recvFiles[msg_id];
        if (info.file) {
            info.file->close();
            delete info.file;
            info.file = nullptr;
        }
        if (!info.tempFilePath.isEmpty()) {
            QFile::remove(info.tempFilePath);
        }
        recvFiles.remove(msg_id);
    }
    clearFileSlot(slotIndex);
    QMessageBox::information(this, "拒绝文件", "文件已被拒绝");
}

void Widget::checkForCompletion(quint32 msg_id)
{
    if (!recvFiles.contains(msg_id)) return;

    FileRecvInfo& info = recvFiles[msg_id];

    // 只有在文件被接受、所有块都收到、结束包也收到的情况下，才算完成
    if (info.accepted && info.end_received && (info.received_chunks == info.chunk_count)) {
        if (info.file) {
            info.file->close();
            delete info.file;
            info.file = nullptr;
        }
        // 完成后再重命名
        if (!info.tempFilePath.isEmpty() && !info.saveasPath.isEmpty()) {
            QFile::rename(info.tempFilePath, info.saveasPath);
        }
        int slot = findSlotByMsgId(msg_id);
        if (slot != -1) {
            // 确保进度条是100%
            updateFileSlot(slot, info.filename, info.filesize, 100);
            QMessageBox::information(this, "文件接收完成", QString("文件 '%1' 已成功接收。").arg(info.filename));
            // 可以在这里延迟后清理槽位
            // clearFileSlot(slot);
            clearFileSlot(slot);
            recvFiles.remove(msg_id);
        }
    }
}

//显示文件大小
QString Widget::formatFileSize(quint64 bytes)
{
    const qint64 kb = 1024;
    const qint64 mb = 1024 * kb;
    const qint64 gb = 1024 * mb;

    if (bytes >= gb) {
        return QString::number(static_cast<double>(bytes) / gb, 'f', 2) + " GB";
    } else if (bytes >= mb) {
        return QString::number(static_cast<double>(bytes) / mb, 'f', 2) + " MB";
    } else if (bytes >= kb) {
        return QString::number(static_cast<double>(bytes) / kb, 'f', 2) + " KB";
    } else {
        return QString::number(bytes) + " B";
    }
}

//处理ID分配
void Widget::handleIdAssign(const PacketHeader& header)
{
    quint32 assignedId = qFromBigEndian(header.sender_id);
    this->clientId = assignedId;
    setWindowTitle(QString("Chat Client - My ID: %1").arg(this->clientId));
    qDebug() << "Client ID has been assigned:" << this->clientId;
}

void Widget::initAcceptProgress(int slotIndex, FileRecvInfo& info)
{
    QProgressBar* bar[3] = {ui->progressBar1, ui->progressBar2, ui->progressBar3};
    if (info.received_chunks == info.chunk_count) {
        // 所有分片已到达，直接100%
        bar[slotIndex]->setValue(100);
        info.acceptProgressInitialized = true;
    } else {
        // 还未全部到达，进度条从0开始
        bar[slotIndex]->setValue(0);
        info.acceptProgressInitialized = true;
    }
}

// CRC32计算函数实现
quint32 calculateHeaderCRC32(const PacketHeader& header, const QByteArray& data) {
    boost::crc_32_type crc;
    crc.process_bytes(&header, offsetof(PacketHeader, crc32));
    if (!data.isEmpty()) {
        crc.process_bytes(data.constData(), data.size());
    }
    return crc.checksum();
}

// CRC32校验函数实现
bool verifyHeaderCRC32(const PacketHeader& header, const QByteArray& data) {
    quint32 expectedCRC32 = qFromBigEndian(header.crc32);
    quint32 calculatedCRC32 = calculateHeaderCRC32(header, data);
    QString typeStr;
    switch(header.msg_type) {
    case MSG_TYPE_TEXT: typeStr = "TEXT"; break;
    case MSG_TYPE_FILE_START: typeStr = "FILE_START"; break;
    case MSG_TYPE_FILE_CHUNK: typeStr = "FILE_CHUNK"; break;
    case MSG_TYPE_FILE_END: typeStr = "FILE_END"; break;
    case MSG_TYPE_ACK: typeStr = "ACK"; break;
    case MSG_TYPE_ID_ASSIGN: typeStr = "ID_ASSIGN"; break;
    default: typeStr = QString::number(header.msg_type); break;
    }
    quint32 msg_id = qFromBigEndian(header.msg_id);
    bool ok = (calculatedCRC32 == expectedCRC32);
    qDebug().noquote() << QString("[CRC32校验] type:%1 msg_id:%2 收到:%3 计算:%4 结果:%5")
                              .arg(typeStr,
                                   QString::number(msg_id),  // 显式转换整数类型
                                   QString::number(expectedCRC32, 16).rightJustified(8, '0'),
                                   QString::number(calculatedCRC32, 16).rightJustified(8, '0'),
                                   ok ? "成功" : "失败");
    return ok;
}

// 文件名缩略显示
QString Widget::elideFileName(const QString& name, int maxLen) {
    if (name.length() <= maxLen) return name;
    int left = maxLen / 2;
    int right = maxLen - left - 3; // 3 for ...
    return name.left(left) + "..." + name.right(right);
}
