/************************************************************
 * 文件名称： main.cpp
 * 内容摘要： 系统入口程序，负责启动多媒体动画与挂载主界面
 * 其它说明： 使用了 QVideoWidget 和透明浮层机制实现现代 UI 效果
 **********************************************************/
#include "mainwindow.h"
#include <QApplication>
#include <QMediaPlayer>
#include <QVideoWidget>
#include <QAudioOutput>
#include <QTimer>
#include <QCoreApplication>
#include <QLabel>
#include <QGraphicsDropShadowEffect>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QVideoWidget *videoWidget = new QVideoWidget;
    videoWidget->setWindowFlags(Qt::SplashScreen | Qt::WindowStaysOnTopHint | Qt::FramelessWindowHint);
    videoWidget->resize(1280, 720);

    QLabel *titleLabel = new QLabel;
    titleLabel->setWindowFlags(Qt::SplashScreen | Qt::WindowStaysOnTopHint | Qt::FramelessWindowHint | Qt::WindowTransparentForInput);
    titleLabel->setAttribute(Qt::WA_TranslucentBackground);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet("color: white; font-size: 90px; font-family: 'STXingkai', '华文行楷', 'Microsoft YaHei'; font-weight: bold;");
    titleLabel->setText("智能停车场管理系统");

    QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect;
    shadow->setOffset(4, 4);
    shadow->setColor(QColor(0, 0, 0, 200));
    shadow->setBlurRadius(15);
    titleLabel->setGraphicsEffect(shadow);

    QMediaPlayer *player = new QMediaPlayer;
    QAudioOutput *audioOutput = new QAudioOutput;
    player->setAudioOutput(audioOutput);
    player->setVideoOutput(videoWidget);

    QString videoPath = QCoreApplication::applicationDirPath() + "/startup.mp4";
    player->setSource(QUrl::fromLocalFile(videoPath));

    videoWidget->show();
    player->play();

    MainWindow *w = new MainWindow;
    w->setWindowTitle("智能停车场收费调度系统");

    QTimer::singleShot(3500, [=]() {
        titleLabel->setGeometry(videoWidget->geometry());
        titleLabel->show();
        titleLabel->raise();
    });

    QTimer::singleShot(5000, [=]() {
        player->stop();
        videoWidget->close();
        titleLabel->close();
        w->show();

        player->deleteLater();
        audioOutput->deleteLater();
        videoWidget->deleteLater();
        titleLabel->deleteLater();
    });

    return a.exec();
}
