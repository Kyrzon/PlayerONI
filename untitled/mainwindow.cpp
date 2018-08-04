#include "mainwindow.h"
#include "ui_mainwindow.h"




using namespace openni;

int MainWindow::openStream(openni::Device& device, openni::SensorType sensorType,
               openni::VideoStream& stream, const openni::SensorInfo** ppSensorInfo, bool* pbIsStreamOn)
{
    *ppSensorInfo = device.getSensorInfo(sensorType);
    *pbIsStreamOn = false;

    if (*ppSensorInfo == NULL)
    {
        return 0;
    }

    openni::Status nRetVal = stream.create(device, sensorType);
    if (nRetVal != openni::STATUS_OK)
    {
        return 0;
    }

    nRetVal = stream.start();
    if (nRetVal != openni::STATUS_OK)
    {
        stream.destroy();
        return 0;
    }

    *pbIsStreamOn = true;

    return 0;
}

int MainWindow::openCommon(openni::Device& device)
{
    g_pPlaybackControl = g_device.getPlaybackControl();

    int ret;

    ret = openStream(device, openni::SENSOR_DEPTH, g_depthStream, &g_depthSensorInfo, &g_bIsDepthOn);
    if (ret != 0)
    {
        return ret;
    }

    ret = openStream(device, openni::SENSOR_COLOR, g_colorStream, &g_colorSensorInfo, &g_bIsColorOn);
    if (ret != 0)
    {
        return ret;
    }

    ret = openStream(device, openni::SENSOR_IR, g_irStream, &g_irSensorInfo, &g_bIsIROn);
    if (ret != 0)
    {
        return ret;
    }

    readFrame();

    return 0;
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

    if (0 != openCommon(g_device))
    {
        return openni::STATUS_ERROR;
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

void MainWindow::readFrame()
{
    openni::Status rc = openni::STATUS_ERROR;

    openni::VideoStream* streams[] = {&g_depthStream, &g_colorStream, &g_irStream};

    int changedIndex = -1;
    while (rc != openni::STATUS_OK)
    {
        rc = openni::OpenNI::waitForAnyStream(streams, 3, &changedIndex, 0);
        if (rc == openni::STATUS_OK)
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
                printf("Error in wait\n");
            }
        }
    }
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

        //displayMessage("Current frame: %u/%u", frameId, numberOfFrames);
    }
    else if ((rc == openni::STATUS_NOT_IMPLEMENTED) || (rc == openni::STATUS_NOT_SUPPORTED) || (rc == openni::STATUS_BAD_PARAMETER) || (rc == openni::STATUS_NO_DEVICE))
    {
        return;
        //displayError("Seeking is not supported");
        //QMessageBox::information(this, tr("Couldn't create depth stream"), OpenNI::getExtendedError());
    }
    else
    {
        return;
        //displayError("Error seeking to frame:\n%s", openni::OpenNI::getExtendedError());
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

void MainWindow::OldTry(QString fileName)
{
    Device glDevice;
    openni::VideoStream depth_stream;
    openni::VideoStream color_stream;
    openni::Status rc;
    openni::VideoFrameRef depth_frame_ref;
    openni::VideoFrameRef color_frame_ref;
    QVideoFrame *pFrame;

    pFrame = new QVideoFrame();

    rc = glDevice.open(fileName.toStdString().c_str());
    if(rc != openni::STATUS_OK)
    {
        QMessageBox::information(this, tr("Error open file"), OpenNI::getExtendedError());
    }

    if (glDevice.getSensorInfo(SENSOR_DEPTH) != NULL)
    {
        rc = depth_stream.create(glDevice, SENSOR_DEPTH);
        if (rc != STATUS_OK)
        {
            QMessageBox::information(this, tr("Couldn't create depth stream"), OpenNI::getExtendedError());
        }
    }

    // set resolution
    // depth mode
    /*const openni::SensorInfo* sinfo = glDevice.getSensorInfo(openni::SENSOR_DEPTH);
    const openni::Array< openni::VideoMode>& modesDepth = sinfo->getSupportedVideoModes();
    rc = depth_stream.setVideoMode(modesDepth[0]);
    if (openni::STATUS_OK != rc)
    {
        QMessageBox::information(this, tr("error: depth fromat not supprted..."), OpenNI::getExtendedError());
    }

    rc = depth_stream.start();
    if (rc != STATUS_OK)
    {
        QMessageBox::information(this, tr("Couldn't start the depth stream"), OpenNI::getExtendedError());
    }
*/
    rc = depth_stream.create (glDevice, openni::SENSOR_DEPTH);
    if(rc != openni::STATUS_OK)
    {
        QMessageBox::information(this, tr("Error create stream"), OpenNI::getExtendedError());
    }



    rc = color_stream.create (glDevice, openni::SENSOR_COLOR);
    if(rc != openni::STATUS_OK)
    {
        QMessageBox::information(this, tr("Error create stream"), OpenNI::getExtendedError());
    }

    /*openni::VideoMode depth_video_mode;
    depth_video_mode.setFps(30);
    depth_video_mode.setPixelFormat(openni::PIXEL_FORMAT_DEPTH_1_MM);
    depth_video_mode.setResolution(640,480);
    rc = depth_stream.setVideoMode(depth_video_mode);
    if(rc == openni::STATUS_ERROR)
    {
        QMessageBox::information(this, tr("Error Set Video Mode"), OpenNI::getExtendedError());
        std::cout << "Could not set video mode." << std::endl;
    }*/

    //device.setImageRegistrationMode (openni::IMAGE_REGISTRATION_DEPTH_TO_COLOR);
    depth_stream.start ();
    color_stream.start ();


    //depth_stream.readFrame (&depth_frame_ref);
    //color_stream.readFrame (&color_frame_ref);

    //depth_stream.stop();
    //color_stream.stop();

    int changedStreamDummy;
    VideoStream* pStream = &depth_stream;
    while (1)
    {
        rc = OpenNI::waitForAnyStream(&pStream, 1, &changedStreamDummy, 20000);//SAMPLE_READ_WAIT_TIMEOUT);
        if (rc != STATUS_OK)
        {
            QMessageBox::information(this, tr("Frame ID"), "Error wait...");
            //continue;
        }

        rc = depth_stream.readFrame(&depth_frame_ref);
        if (rc != STATUS_OK)
        {
            //cout << "Read failed!" << endl << OpenNI::getExtendedError() << endl;
            //continue;
        }

        //if (color_frame_ref.getVideoMode().getPixelFormat() != PIXEL_FORMAT_DEPTH_1_MM && color_frame_ref.getVideoMode().getPixelFormat() != PIXEL_FORMAT_DEPTH_100_UM)
        {
            //cout << "Unexpected frame format" << endl;
            //continue;
        }

        uchar *data = (uchar *)depth_frame_ref.getData();

            int imageWidth = 640;
            int imageHeight = 480;
            //int bytesPerPixel = 1; // 4 for RGBA, 3 for RGB
            QImage image(data, imageWidth, imageHeight, QImage::Format_RGB888);
            ui->label->setPixmap(QPixmap::fromImage(image).scaled(200,200));
            setUpdatesEnabled(true);
            repaint();
            setUpdatesEnabled(false);
            QMessageBox::information(this, tr("Frame ID"), QString::number(depth_frame_ref.getFrameIndex()));
    }
}

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{


    Status rc = OpenNI::initialize();
    if (rc != STATUS_OK)
    {
        QMessageBox::information(this, tr("Error Init"), OpenNI::getExtendedError());
    }



    ui->setupUi(this);

    pMediaPlayer = new QMediaPlayer(this);
    pVideoWidget = new QVideoWidget(this);
    //pMediaPlayer->setVideoOutput(pVideoWidget);
    //this->setCentralWidget(pVideoWidget);
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

MainWindow::~MainWindow()
{
    OpenNI::shutdown();
    delete ui;
}

void MainWindow::on_pushButton_openFile_clicked()
{

}


void MainWindow::on_pushButton_play_clicked()
{

}

void MainWindow::on_action_openFile_triggered()
{
    QString fileName = "F:\\a.oni";//QFileDialog::getOpenFileName(this, tr("Open File"), "C:\\", "ONI files (*.oni)");
    if (fileName.isEmpty())
    {
        return;
    }

    openDevice(fileName.toStdString().c_str());
    if(openDevice(fileName.toStdString().c_str()) == openni::STATUS_OK)
    {
        if (openCommon(g_device) == 0)
        {
            while(1)
            {
                uchar *data = (uchar *)(g_colorFrame.getData());
                int imageWidth = 640;
                int imageHeight = 480;
                QImage image(data, imageWidth, imageHeight, QImage::Format_RGB888);
                ui->label->setPixmap(QPixmap::fromImage(image).scaled(200,200));
                QVideoFrame fra(image);
                //fra.
                //pMediaPlayer->setMedia()


                data = (uchar *)(g_depthFrame.getData());
                image = QImage(data, imageWidth, imageHeight, QImage::Format_RGB16);
                ui->label_2->setPixmap(QPixmap::fromImage(image).scaled(200,200));

                setUpdatesEnabled(true);
                repaint();
                setUpdatesEnabled(false);
                readFrame();
            }
        }
        else
        {
            QMessageBox::information(this, tr("Error open Streams"), OpenNI::getExtendedError());
        }
    }
    else
    {
        QMessageBox::information(this, tr("Error open Device"), OpenNI::getExtendedError());
    }

    //on_actionStop_triggered();
    //pMediaPlayer->setMedia(QUrl(fileName));
    //on_actionPlay_triggered();
}

void MainWindow::on_actionPlay_triggered()
{
    pMediaPlayer->play();
    ui->statusBar->showMessage("Playing");
}

void MainWindow::on_actionPause_triggered()
{
    pMediaPlayer->pause();
    ui->statusBar->showMessage("Pause");
}

void MainWindow::on_actionStop_triggered()
{
    pMediaPlayer->stop();
    ui->statusBar->showMessage("Stoping");
}
