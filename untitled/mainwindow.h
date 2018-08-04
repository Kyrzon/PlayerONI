#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QFileDialog>
#include <QMessageBox>
#include <QMediaPlayer>
#include <QVideoWidget>
#include <QVideoFrame>
#include <QImage>
#include <QProgressBar>
#include <QSlider>
#include <iostream>
#include "OpenNI.h"


namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_pushButton_openFile_clicked();

    void on_action_openFile_triggered();

    void on_pushButton_play_clicked();

    void on_actionPlay_triggered();

    void on_actionPause_triggered();

    void on_actionStop_triggered();

    void OldTry(QString fileName);

    int openStream(openni::Device& device, openni::SensorType sensorType,
                   openni::VideoStream& stream, const openni::SensorInfo** ppSensorInfo, bool* pbIsStreamOn);

    int openCommon(openni::Device& device);

    openni::Status openDevice(const char* uri);

    void closeDevice();

    void readFrame();

    openni::VideoStream* getSeekingStream(openni::VideoFrameRef*& pCurFrame);

    void seekStream(openni::VideoStream* pStream, openni::VideoFrameRef* pCurFrame, int frameId);

    void seekFrame(int nDiff);

    void seekFrameAbs(int frameId);

private:
    Ui::MainWindow *ui;
    QMediaPlayer* pMediaPlayer;
    QProgressBar* pProgressBar;
    QVideoWidget* pVideoWidget;
    QSlider* pSlider;

    bool g_bIsDepthOn = false;
    bool g_bIsColorOn = false;
    bool g_bIsIROn = false;

    openni::Device g_device;

    openni::PlaybackControl* g_pPlaybackControl;

    openni::VideoStream g_depthStream;
    openni::VideoStream g_colorStream;
    openni::VideoStream g_irStream;

    openni::VideoFrameRef g_depthFrame;
    openni::VideoFrameRef g_colorFrame;
    openni::VideoFrameRef g_irFrame;

    const openni::SensorInfo* g_depthSensorInfo = NULL;
    const openni::SensorInfo* g_colorSensorInfo = NULL;
    const openni::SensorInfo* g_irSensorInfo = NULL;

};

#endif // MAINWINDOW_H
