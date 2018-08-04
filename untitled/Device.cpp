int openStream(openni::Device& device, openni::SensorType sensorType,
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

int openCommon(openni::Device& device)
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

openni::Status openDevice(const char* uri)
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

void closeDevice()
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

void readFrame()
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

openni::VideoStream* getSeekingStream(openni::VideoFrameRef*& pCurFrame)
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

void seekStream(openni::VideoStream* pStream, openni::VideoFrameRef* pCurFrame, int frameId)
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

void seekFrame(int nDiff)
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

void seekFrameAbs(int frameId)
{
    openni::VideoStream* pStream = NULL;
    openni::VideoFrameRef* pCurFrame = NULL;

    pStream = getSeekingStream(pCurFrame);
    if (pStream == NULL)
        return;

    seekStream(pStream, pCurFrame, frameId);
}
