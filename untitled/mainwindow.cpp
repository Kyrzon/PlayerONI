#include "mainwindow.h"
#include "ui_mainwindow.h"




using namespace openni;

openni::Status MainWindow::openStream(openni::Device& device, openni::SensorType sensorType,
               openni::VideoStream& stream, const openni::SensorInfo** ppSensorInfo, bool* pbIsStreamOn,
                                      openni::VideoFrameRef* frame)
{
    *ppSensorInfo = device.getSensorInfo(sensorType);
    *pbIsStreamOn = false;

    if (*ppSensorInfo == NULL)
    {
        return openni::STATUS_ERROR;
    }

    openni::Status nRetVal = stream.create(device, sensorType);
    if (nRetVal != openni::STATUS_OK)
    {
        return nRetVal;
    }

    nRetVal = stream.start();
    if (nRetVal != openni::STATUS_OK)
    {
        stream.destroy();
        return nRetVal;
    }

    stream.readFrame(frame);

    *pbIsStreamOn = true;

    return openni::STATUS_OK;
}

openni::Status MainWindow::openCommon(openni::Device& device)
{
    g_pPlaybackControl = device.getPlaybackControl();

    openStream(device, openni::SENSOR_DEPTH, g_depthStream, &g_depthSensorInfo, &g_bIsDepthOn, &g_depthFrame);

    openStream(device, openni::SENSOR_COLOR, g_colorStream, &g_colorSensorInfo, &g_bIsColorOn, &g_colorFrame);

    openStream(device, openni::SENSOR_IR, g_irStream, &g_irSensorInfo, &g_bIsIROn, &g_irFrame);

    if (!(g_bIsDepthOn || g_bIsColorOn || g_bIsIROn))
    {
        return openni::STATUS_ERROR;
    }

    return openni::STATUS_OK;
}

openni::Status MainWindow::openDevice(const char* uri)
{
    openni::Status nRetVal = openni::OpenNI::initialize();
    if (nRetVal != openni::STATUS_OK)
    {
        return nRetVal;
    }

    // Open the requested device.
    nRetVal = g_device.open(uri);
    if (nRetVal != openni::STATUS_OK)
    {
        return nRetVal;
    }

    nRetVal = openCommon(g_device);
    if (nRetVal != openni::STATUS_OK)
    {
        return nRetVal;
    }

    return openni::STATUS_OK;
}

void MainWindow::closeDevice()
{
    g_depthStream.stop();
    g_colorStream.stop();
    g_irStream.stop();

    g_depthStream.destroy();
    g_colorStream.destroy();
    g_irStream.destroy();

    g_device.close();

    openni::OpenNI::shutdown();
}

openni::Status MainWindow::readFrame()
{
    openni::Status nRetVal = openni::STATUS_ERROR;

    openni::VideoStream* streams[] = {&g_depthStream, &g_colorStream, &g_irStream};

    int changedIndex = -1;
    while (nRetVal != openni::STATUS_OK)
    {
        nRetVal = openni::OpenNI::waitForAnyStream(streams, 3, &changedIndex, 0);
        if (nRetVal == openni::STATUS_OK)
        {
            switch (changedIndex)
            {
            case 0:
                g_depthStream.readFrame(&g_depthFrame); break;
            case 1:
                g_colorStream.readFrame(&g_colorFrame); break;
            case 2:
                g_irStream.readFrame(&g_irFrame); break;
            default:
                return openni::STATUS_ERROR;
            }
        }
    }

    return nRetVal;
}

openni::VideoStream* MainWindow::getSeekingStream(openni::VideoFrameRef*& pCurFrame)
{
    if (g_pPlaybackControl == NULL)
    {
        return NULL;
    }

    if (g_bIsDepthOn)
    {
        pCurFrame = &g_depthFrame;
        return &g_depthStream;
    }
    else if (g_bIsColorOn)
    {
        pCurFrame = &g_colorFrame;
        return &g_colorStream;
    }
    else if (g_bIsIROn)
    {
        pCurFrame = &g_irFrame;
        return &g_irStream;
    }
    else
    {
        return NULL;
    }
}

void MainWindow::seekStream(openni::VideoStream* pStream, openni::VideoFrameRef* pCurFrame, int frameId)
{
    int numberOfFrames = 0;

    // Get number of frames
    numberOfFrames = g_pPlaybackControl->getNumberOfFrames(*pStream);

    // Seek
    openni::Status rc = g_pPlaybackControl->seek(*pStream, frameId);
    if (rc == openni::STATUS_OK)
    {
        // Read next frame from all streams.
        if (g_bIsDepthOn)
        {
            g_depthStream.readFrame(&g_depthFrame);
        }
        if (g_bIsColorOn)
        {
            g_colorStream.readFrame(&g_colorFrame);
        }
        if (g_bIsIROn)
        {
            g_irStream.readFrame(&g_irFrame);
        }

        // the new frameId might be different than expected (due to clipping to edges)
        frameId = pCurFrame->getFrameIndex();

    }
    else if ((rc == openni::STATUS_NOT_IMPLEMENTED) || (rc == openni::STATUS_NOT_SUPPORTED) || (rc == openni::STATUS_BAD_PARAMETER) || (rc == openni::STATUS_NO_DEVICE))
    {
        return;
        //QMessageBox::information(this, tr("Couldn't create depth stream"), OpenNI::getExtendedError());
    }
    else
    {
        return;
    }
}

void MainWindow::seekFrame(int nDiff)
{
    // Make sure seek is required.
    if (nDiff == 0)
    {
        return;
    }

    openni::VideoStream* pStream = NULL;
    openni::VideoFrameRef* pCurFrame = NULL;

    pStream = getSeekingStream(pCurFrame);
    if (pStream == NULL)
        return;

    int frameId = pCurFrame->getFrameIndex();
    // Calculate the new frame ID
    frameId = (frameId + nDiff < 1) ? 1 : frameId + nDiff;

    seekStream(pStream, pCurFrame, frameId);
}

void MainWindow::seekFrameAbs(int frameId)
{
    openni::VideoStream* pStream = NULL;
    openni::VideoFrameRef* pCurFrame = NULL;

    pStream = getSeekingStream(pCurFrame);
    if (pStream == NULL)
        return;

    seekStream(pStream, pCurFrame, frameId);
}

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{

    ui->setupUi(this);

    if (ONIMode)
    {
        Status rc = OpenNI::initialize();
        if (rc != STATUS_OK)
        {
            QMessageBox::information(this, tr("Error Init"), OpenNI::getExtendedError());
        }
    }
    else // Standart player
    {
        pMediaPlayer = new QMediaPlayer(this);
        pVideoWidget = new QVideoWidget(this);
        pMediaPlayer->setVideoOutput(pVideoWidget);
        this->setCentralWidget(pVideoWidget);
        pSlider = new QSlider(this);
        pProgressBar = new QProgressBar(this);

        pSlider->setOrientation(Qt::Horizontal);
        ui->statusBar->addPermanentWidget(pSlider);
        ui->statusBar->addPermanentWidget(pProgressBar);

        connect(pMediaPlayer, &QMediaPlayer::durationChanged, pSlider, &QSlider::setMaximum);
        connect(pMediaPlayer, &QMediaPlayer::positionChanged, pSlider, &QSlider::setValue);

        connect(pSlider, &QSlider::sliderMoved, pMediaPlayer, &QMediaPlayer::setPosition);

        connect(pMediaPlayer, &QMediaPlayer::durationChanged, pProgressBar, &QProgressBar::setMaximum);
        connect(pMediaPlayer, &QMediaPlayer::positionChanged, pProgressBar, &QProgressBar::setValue);
    }

}

MainWindow::~MainWindow()
{
    if (ONIMode)
    {
        OpenNI::shutdown();
    }
    delete ui;
}


void MainWindow::on_action_openFile_triggered()
{
    if(ONIMode)
    {
        QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"), "C:\\", "ONI files (*.oni)");
        if (fileName.isEmpty())
        {
            return;
        }

        openni::Status nRetVal = openDevice(fileName.toStdString().c_str());
        if(nRetVal != openni::STATUS_OK)
        {
            QMessageBox::information(this, tr("Error open Device"), OpenNI::getExtendedError());
            return;
        }

        int curCountOfFrames = 0;
        int numberOfFrames = g_pPlaybackControl->getNumberOfFrames(g_depthStream);
        while(curCountOfFrames < numberOfFrames)
        {

            openni::Status rc = g_pPlaybackControl->seek(g_colorStream, curCountOfFrames);
            if (rc != openni::STATUS_OK)
            {
                return;
            }
            g_colorStream.readFrame(&g_colorFrame);
            uchar *data = (uchar *)(g_colorFrame.getData());
            int imageWidth = 640;
            int imageHeight = 480;
            QImage image(data, imageWidth, imageHeight, QImage::Format_RGB888);
            ui->label->setPixmap(QPixmap::fromImage(image).scaled(ui->label->width(), ui->label->height(), Qt::KeepAspectRatio));

            rc = g_pPlaybackControl->seek(g_depthStream, curCountOfFrames);
            if (rc != openni::STATUS_OK)
            {
                return;
            }
            g_depthStream.readFrame(&g_depthFrame);
            data = (uchar *)(g_depthFrame.getData());
            image = QImage(data, imageWidth, imageHeight, QImage::Format_RGB16);
            ui->label_2->setPixmap(QPixmap::fromImage(image).scaled(ui->label_2->width(), ui->label->height(), Qt::KeepAspectRatio));

            setUpdatesEnabled(true);
            repaint();
            setUpdatesEnabled(false);
            curCountOfFrames++;
        }
    }
    else // Standart player
    {
        QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"), "C:\\", "Video files (*.*)");
        if (fileName.isEmpty())
        {
            return;
        }
        on_actionStop_triggered();
        pMediaPlayer->setMedia(QUrl(fileName));
        on_actionPlay_triggered();
    }
}

void MainWindow::on_actionPlay_triggered()
{
    if(ONIMode)
    {
        //TODO
    }
    else
    {
        pMediaPlayer->play();
        ui->statusBar->showMessage("Playing");
    }

}

void MainWindow::on_actionPause_triggered()
{
    if(ONIMode)
    {
        //TODO
    }
    else
    {
        pMediaPlayer->pause();
        ui->statusBar->showMessage("Pause");
    }
}

void MainWindow::on_actionStop_triggered()
{
    if(ONIMode)
    {
        //TODO
    }
    else
    {
        pMediaPlayer->stop();
        ui->statusBar->showMessage("Stoping");
    }
}
