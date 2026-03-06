#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>
#include <QDateTime>
#include <QFile>
#include <QTextStream>
#include <QList>
#include <QThread>
#include <QCoreApplication>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // 锁定大小防全屏错乱，并确保触发析构函数
    this->setFixedSize(this->width(), this->height());
    this->setAttribute(Qt::WA_DeleteOnClose);

    loadFromFile();
    updateUI();
    ui->textLog->append("系统初始化完成。当前停车场最大车位：" + QString::number(maxCapacity));
}

MainWindow::~MainWindow()
{
    saveToFile();
    delete ui;
}

/**********************************************************
 * 功能描述：处理车辆入场请求，分配车位或进入便道
 * 输入参数：无（从UI控件获取输入）
 * 输出参数：无
 * 返回值：无
 * 其它说明：利用散列表(Hash)进行 O(1) 复杂度的极速查重拦截
 ************************************************************/
void MainWindow::on_btnEnter_clicked()
{
    QString plate = ui->inputPlate->text().trimmed();

    if(plate.isEmpty()) {
        QMessageBox::warning(this, "警告", "车牌号不能为空！");
        return;
    }
    if(plate.length() < 5 || plate.contains(" ")) {
        QMessageBox::warning(this, "警告", "车牌号格式错误（过短或包含空格）！");
        ui->inputPlate->clear();
        return;
    }

    // 利用哈希表进行极速查重，替代原先 O(N) 的栈/队列遍历
    if (carHashMap.find(plate) != carHashMap.end()) {
        QMessageBox::warning(this, "拦截", QString("车牌号 %1 已在系统中，请勿重复录入！").arg(plate));
        ui->inputPlate->clear();
        return;
    }

    Car newCar;
    newCar.plate = plate;
    newCar.brand = ui->comboBrand->currentText();
    newCar.color = ui->comboColor->currentText();
    newCar.exhaust = ui->comboExhaust->currentText();
    newCar.type = ui->comboType->currentText();
    newCar.entryTime = QDateTime::currentDateTime();

    if (parkingStack.size() < maxCapacity) {
        newCar.status = 1; // 场内状态
        parkingStack.push(newCar);
        ui->textLog->append(QString("【入场】%1 进入车位。").arg(plate));
    } else {
        newCar.status = 2; // 便道排队状态
        waitingQueue.push(newCar);
        ui->textLog->append(QString("【排队】车位已满，%1 进入便道。").arg(plate));
    }

    // 同步更新全局哈希索引
    carHashMap[plate] = newCar;

    ui->inputPlate->clear();
    updateUI();
}

/**********************************************************
 * 功能描述：处理目标车辆出场，支持场内调度与便道排队中途离场
 * 输入参数：无
 * 输出参数：无
 * 返回值：无
 * 其它说明：利用哈希索引 O(1) 预判车辆位置，执行不同出库逻辑
 ************************************************************/
void MainWindow::on_btnExit_clicked()
{
    QString targetPlate = ui->inputPlate->text().trimmed();
    if(targetPlate.isEmpty()) {
        QMessageBox::warning(this, "警告", "请输入要出场的车牌号！");
        return;
    }

    // 1. 利用哈希表 O(1) 极速拦截并获取车辆状态
    if (carHashMap.find(targetPlate) == carHashMap.end()) {
        QMessageBox::information(this, "提示", "系统中未找到该车！");
        ui->inputPlate->clear();
        return;
    }

    Car targetCar = carHashMap[targetPlate];
    ui->textLog->append("------ 开始调度 ------");

    // 2. 场景A：车辆在停车场内（栈中），执行标准的出栈挪车计费逻辑
    if (targetCar.status == 1) {
        bool found = false;
        while (!parkingStack.empty()) {
            Car topCar = parkingStack.top();
            parkingStack.pop();

            if (topCar.plate == targetPlate) {
                found = true;
                break;
            } else {
                tempStack.push(topCar);
                ui->textLog->append(QString("【让路】挡路车 %1 暂移出...").arg(topCar.plate));
                updateUI();
                QCoreApplication::processEvents();
                QThread::msleep(800);
            }
        }

        if (found) {
            carHashMap.erase(targetPlate);

            // 动态阶梯计费
            qint64 seconds = targetCar.entryTime.secsTo(QDateTime::currentDateTime());
            double hours = std::ceil(seconds / 3600.0);
            if (hours <= 0) hours = 1;

            double baseRate = 15.0;
            if (targetCar.exhaust == "纯电") baseRate = 10.0;
            else if (targetCar.exhaust == "1.5L" || targetCar.exhaust == "1.6L") baseRate = 12.0;
            else if (targetCar.exhaust == "1.5T" || targetCar.exhaust == "2.0L") baseRate = 15.0;
            else if (targetCar.exhaust == "2.0T") baseRate = 18.0;

            if (targetCar.type == "大型车") baseRate *= 2.0;
            double fee = hours * baseRate;

            ui->textLog->append(QString("【出场】%1 驶出！").arg(targetPlate));
            updateUI();
            QCoreApplication::processEvents();
            QThread::msleep(1000);

            // 挡路车辆归位
            while (!tempStack.empty()) {
                Car returnCar = tempStack.top();
                parkingStack.push(returnCar);
                tempStack.pop();
                ui->textLog->append(QString("【归位】挡路车 %1 停回。").arg(returnCar.plate));
                updateUI();
                QCoreApplication::processEvents();
                QThread::msleep(800);
            }

            // 便道车辆递补
            if (!waitingQueue.empty()) {
                Car nextCar = waitingQueue.front();
                waitingQueue.pop();
                nextCar.entryTime = QDateTime::currentDateTime();
                nextCar.status = 1;
                parkingStack.push(nextCar);

                carHashMap[nextCar.plate] = nextCar;

                ui->textLog->append(QString("【调度】便道车 %1 驶入！").arg(nextCar.plate));
                updateUI();
                QCoreApplication::processEvents();
                QThread::msleep(800);
            }

            ui->textLog->append(QString("调度完毕。计费时长：%1小时，收费：%2元。").arg(hours).arg(fee));
        }
    }
    // 3. 场景B：车辆在便道排队中（队列中），中途放弃排队离场
    else if (targetCar.status == 2) {
        int qSize = waitingQueue.size();
        for (int i = 0; i < qSize; ++i) {
            Car c = waitingQueue.front();
            waitingQueue.pop();

            if (c.plate == targetPlate) {
                // 找到排队车辆，销毁哈希索引，不放回队列
                carHashMap.erase(targetPlate);
                ui->textLog->append(QString("【离开】便道车辆 %1 放弃排队驶离！").arg(targetPlate));
            } else {
                // 其他无辜排队的车辆，原封不动排回队尾
                waitingQueue.push(c);
            }
        }
        ui->textLog->append("调度完毕。排队未入场，不产生停车费。");
    }

    ui->textLog->append("--------------------------");
    ui->inputPlate->clear();
    updateUI();
}

/**********************************************************
 * 功能描述：根据车牌号快速检索车辆当前状态信息
 * 输入参数：无
 * 输出参数：无
 * 返回值：无
 * 其它说明：核心算法优化，完全摒弃遍历，依赖哈希实现 O(1) 查询
 ************************************************************/
void MainWindow::on_btnQuery_clicked()
{
    QString targetPlate = ui->inputPlate->text().trimmed();
    if(targetPlate.isEmpty()) {
        QMessageBox::warning(this, "警告", "请输入查询车牌号！");
        return;
    }

    // O(1) 哈希查询
    if (carHashMap.find(targetPlate) != carHashMap.end()) {
        Car c = carHashMap[targetPlate];
        QString resultMsg;

        if (c.status == 1) { // 场内
            qint64 seconds = c.entryTime.secsTo(QDateTime::currentDateTime());

            // 费用计算同步逻辑
            double hours = std::ceil(seconds / 3600.0);
            if (hours <= 0) hours = 1;

            double baseRate = 15.0;
            if (c.exhaust == "纯电") baseRate = 10.0;
            else if (c.exhaust == "1.5L" || c.exhaust == "1.6L") baseRate = 12.0;
            else if (c.exhaust == "1.5T" || c.exhaust == "2.0L") baseRate = 15.0;
            else if (c.exhaust == "2.0T") baseRate = 18.0;

            if (c.type == "大型车") baseRate *= 2.0;
            double fee = hours * baseRate;

            // 优化了显示文案，把计费的小时数也显示出来，让老师看得更明白
            resultMsg = QString("【场内车辆】\n车牌：%1\n品牌：%2\n入场：%3\n已停时长：%4 秒\n计费时长：%5 小时\n预估费用：%6 元")
                            .arg(c.plate).arg(c.brand).arg(c.entryTime.toString("yyyy-MM-dd HH:mm:ss"))
                            .arg(seconds).arg(hours).arg(fee);
        } else { // 排队中
            resultMsg = QString("【便道排队中】\n车牌：%1\n品牌：%2\n状态：等待入场\n预估费用：0 元")
                            .arg(c.plate).arg(c.brand);
        }
        QMessageBox::information(this, "查询结果 (基于哈希索引)", resultMsg);
    } else {
        QMessageBox::warning(this, "查询失败", "系统中未找到该车辆！");
    }

    ui->inputPlate->clear();
}

/**********************************************************
 * 功能描述：同步更新底层容器数据至可视化UI界面
 * 输入参数：无
 * 输出参数：无
 * 返回值：无
 * 其它说明：无
 ************************************************************/
void MainWindow::updateUI()
{
    ui->listStack->clear();
    ui->listQueue->clear();
    ui->listTemp->clear();

    auto getCarIcon = [](const QString& color) -> QString {
        if(color == "红") return "🔴🚗";
        if(color == "蓝") return "🔵🚙";
        if(color == "黄") return "🟡🚕";
        if(color == "绿") return "🟢🚗";
        if(color == "黑") return "⚫🚗";
        if(color == "白") return "⚪🚗";
        if(color == "紫") return "🟣🚗";
        if(color == "青") return "💠🚗";
        if(color == "橙") return "🟠🚗";
        // 新增的冷门高级颜色
        if(color == "灰") return "🔘🚗";
        if(color == "银") return "💿🚗";
        if(color == "棕") return "🟤🚗";
        if(color == "双拼") return "☯️🚗";
        // 如果以后还有不认识的颜色，默认给个灰色圆圈，保持队形！
        return "🔘🚗";
    };

    std::stack<Car> pStack = parkingStack;
    QList<QString> stackItems;
    int spotNum = pStack.size();
    while (!pStack.empty()) {
        Car c = pStack.top();
        stackItems.prepend(QString("车位%1: %2 %3").arg(spotNum).arg(getCarIcon(c.color)).arg(c.plate));
        pStack.pop();
        spotNum--;
    }
    for(const QString& p : stackItems) ui->listStack->addItem(p);

    std::stack<Car> tStack = tempStack;
    QList<QString> tempItems;
    int tempSpot = tStack.size();
    while (!tStack.empty()) {
        Car c = tStack.top();
        tempItems.prepend(QString("挪位%1: %2 %3").arg(tempSpot).arg(getCarIcon(c.color)).arg(c.plate));
        tStack.pop();
        tempSpot--;
    }
    for(const QString& p : tempItems) ui->listTemp->addItem(p);

    std::queue<Car> wQueue = waitingQueue;
    int queueNum = 1;
    while (!wQueue.empty()) {
        Car c = wQueue.front();
        ui->listQueue->addItem(QString("排队%1: %2 %3").arg(queueNum).arg(getCarIcon(c.color)).arg(c.plate));
        wQueue.pop();
        queueNum++;
    }

    QString statusMsg = QString("🅿️ 状态：已停 %1 辆 | 剩余空位 %2 个 | 排队 %3 辆")
                            .arg(parkingStack.size())
                            .arg(maxCapacity - parkingStack.size())
                            .arg(waitingQueue.size());
    ui->labelStatus->setText(statusMsg);
}

/**********************************************************
 * 功能描述：持久化保存数据到本地文件
 * 输入参数：无
 * 输出参数：无
 * 返回值：无
 * 其它说明：确保软件重启后车辆信息不丢失
 ************************************************************/
void MainWindow::saveToFile()
{
    QString path = QCoreApplication::applicationDirPath() + "/parking_data.txt";
    QFile file(path);
    if(file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);

        std::stack<Car> tS = parkingStack;
        while(!tS.empty()) {
            Car c = tS.top();
            out << "S," << c.plate << "," << c.entryTime.toSecsSinceEpoch() << ","
                << c.brand << "," << c.color << "," << c.exhaust << "," << c.type << "\n";
            tS.pop();
        }

        std::queue<Car> tQ = waitingQueue;
        while(!tQ.empty()) {
            Car c = tQ.front();
            out << "Q," << c.plate << "," << c.entryTime.toSecsSinceEpoch() << ","
                << c.brand << "," << c.color << "," << c.exhaust << "," << c.type << "\n";
            tQ.pop();
        }
        file.close();
    }
}

/**********************************************************
 * 功能描述：从本地文件加载历史数据并重建运行时结构
 * 输入参数：无
 * 输出参数：无
 * 返回值：无
 * 其它说明：数据读取的同时，同步构建哈希索引表
 ************************************************************/
void MainWindow::loadFromFile()
{
    QString path = QCoreApplication::applicationDirPath() + "/parking_data.txt";
    QFile file(path);
    if(file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        QList<Car> loadedStack;

        while(!in.atEnd()) {
            QStringList parts = in.readLine().split(",");
            if(parts.size() >= 7) {
                Car c;
                c.plate = parts[1];
                c.entryTime = QDateTime::fromSecsSinceEpoch(parts[2].toLongLong());
                c.brand = parts[3];
                c.color = parts[4];
                c.exhaust = parts[5];
                c.type = parts[6];

                if(parts[0] == "S") {
                    c.status = 1;
                    loadedStack.append(c);
                } else {
                    c.status = 2;
                    waitingQueue.push(c);
                }
                // 核心：启动时立刻重建哈希索引
                carHashMap[c.plate] = c;
            }
        }
        for(int i = loadedStack.size() - 1; i >= 0; --i) {
            parkingStack.push(loadedStack[i]);
        }
        file.close();
    }
}

// 自动生成的 UI 槽函数，保持默认
void MainWindow::on_listStack_itemClicked(QListWidgetItem *item)
{
    QString text = item->text();
    QStringList parts = text.split(" ");
    if (!parts.isEmpty()) {
        ui->inputPlate->setText(parts.last());
    }
}

void MainWindow::on_listQueue_itemClicked(QListWidgetItem *item)
{
    QString text = item->text();
    QStringList parts = text.split(" ");
    if (!parts.isEmpty()) {
        ui->inputPlate->setText(parts.last());
    }
}

void MainWindow::on_inputExhaust_cursorPositionChanged(int arg1, int arg2) {}
