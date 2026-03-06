#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QString>
#include <QDateTime>
#include <stack>
#include <queue>
#include <unordered_map>
#include <QListWidgetItem>

// 车辆信息结构体 (ADT Car)
struct Car {
    QString brand;
    QString color;
    QString exhaust;
    QString plate;
    QString type;
    QDateTime entryTime;
    int status; // 新增状态标识：1代表停在车位，2代表在便道排队
};

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_btnEnter_clicked();
    void on_btnExit_clicked();
    void on_btnQuery_clicked();

    void on_inputExhaust_cursorPositionChanged(int arg1, int arg2);
    void on_listStack_itemClicked(QListWidgetItem *item);
    void on_listQueue_itemClicked(QListWidgetItem *item);

private:
    Ui::MainWindow *ui;

    // 核心数据结构
    std::stack<Car> parkingStack;
    std::stack<Car> tempStack;
    std::queue<Car> waitingQueue;

    // 复杂非线性数据结构：建立车牌号到车辆实体的映射，实现 O(1) 查询
    std::unordered_map<QString, Car> carHashMap;

    int maxCapacity = 30; // 满足验收 30 条数据的要求

    // 内部调用函数
    void updateUI();
    void saveToFile();
    void loadFromFile();
};

#endif // MAINWINDOW_H
