#include "TaskPump.h"
#include "Defines.h"
#include "Config.h"
#include "BusinessFun.h"
#include "SoLogger.h"
#include "Common.h"

#include <iostream>
#include <QJsonParseError>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QProcess>
#include <QCoreApplication>
#include <QUuid>

using namespace std;

TaskPump::TaskPump()
{
    this->moveToThread(&m_workThread);
    connect(this, &TaskPump::TaskIncoming, this, &TaskPump::OnTaskIncoming);
    connect(this, &TaskPump::PushVideoMessageIncoming, this, &TaskPump::OnPushVideoMessageIncoming);

    m_config = Config::getInstance();
}

TaskPump::~TaskPump()
{
    Stop();
}

void TaskPump::Start()
{
    m_workThread.start();
}

void TaskPump::Stop()
{
    m_workThread.quit();
    m_workThread.wait();
    StopPushVideoProcesses();
}

void TaskPump::OnTaskIncoming(const QByteArray& data)
{
    QJsonParseError jsonError;
    QJsonDocument doucment = QJsonDocument::fromJson(data, &jsonError);
    if (jsonError.error == QJsonParseError::NoError && doucment.isObject())
    {
        const QJsonObject& obj = doucment.object();
        auto act = (Action)obj.find("action")->toInt();
        switch (act)
        {
        case Action::ActPushVideo:
        {
			OnActPushVideo(obj);
        }
            break;
        case Action::ActExitPushVideo:
        {
            StopPushVideoProcesses();
        }
            break;
        default:
            break;
        }
    }
}

void TaskPump::OnPushVideoMessageIncoming(MediaMessage mediaMessage, const QString& roomId,
                                          const QString& cameraName)
{
    const QSet<QString>& listeners = m_camsListeners.take(cameraName);
    QString data;
    switch (mediaMessage)
    {
    case MediaMessage::NoError:
    {
        emit FinishPushingVideo(cameraName);
        foreach(QString listener , listeners)
        {
            data = QString(
                        "{"
                        "\"action\":%1,"
                        "\"car_id\":\"%2\","
                        "\"room_id\":\"%3\","
                        "\"success_cameras\":[\"%4\"],"
                        "\"driver_id\":\"%5\","
                        "\"videoWidth\":%6,"
                        "\"videoHeight\":%7,"
                        "\"videoFPS\":%8"
                        "}"
                        ).
                    arg((int)Action::ActPushVideoNotice).
                    arg(m_config->m_clientId,
                        roomId,
                        cameraName,
                        listener,
                        m_config->m_videoWidth,
                        m_config->m_videoHeight,
                        m_config->m_frameRate);
            log_info << "MediaMessage::NoError:data=" << data.toStdString() << std::endl;
            if(m_pushVideo)
            {
                emit DataOut(data);
            }
            else
            {
                m_camsListeners.clear();
                break;
            }
        }
    }
        break;
    case MediaMessage::Eof:
    {
        data = QString(
                    "{"
                    "\"action\":%1,"
                    "\"car_id\":\"%2\","
                    "\"room_id\":\"%3\","
                    "\"camera\":\"%4\""
                    "}"
                    ).
                arg((int)Action::ActCameraExitNotice).
                arg(m_config->m_clientId, roomId, cameraName);
        m_camsListeners.clear();
        emit DataOut(data);
        log_info << "MediaMessage::Eof:data=" << data.toStdString() << std::endl;
    }
        break;
    case MediaMessage::OpenFail:
    {
        foreach(QString listener , listeners)
        {
            data = QString(
                        "{"
                        "\"action\":%1,"
                        "\"car_id\":\"%2\","
                        "\"room_id\":\"%3\","
                        "\"fail_cameras\":[\"%4\"],"
                        "\"driver_id\":\"%5\","
                        "\"videoWidth\":%6,"
                        "\"videoHeight\":%7,"
                        "\"videoFPS\":%8"
                        "}"
                        ).
                    arg((int)Action::ActPushVideoNotice).
                    arg(m_config->m_clientId,
                        roomId,
                        cameraName,
                        listener,
                        m_config->m_videoWidth,
                        m_config->m_videoHeight,
                        m_config->m_frameRate);
            log_info << "MediaMessage::OpenFail:data=" << data.toStdString() << std::endl;
            if(m_pushVideo)
            {
                emit DataOut(data);
            }
            else
            {
                m_camsListeners.clear();
                break;
            }
        }
    }
        break;
    default:
        break;
    }
}

void TaskPump::OnActPushVideo(const QJsonObject& obj)
{
    //推流状态:1、打开;2、推流中;3、停止
    //每个方位摄像头以hash保存FFmpegKits
    //每个方位摄像头以hash保存待通知的拉流端,通知完毕后删除被通知拉流端
    //访问摄像头时,FFmpegKits状态：
    //  打开:拉流端id加入到等待该方位摄像头推流结果的hash中
    //  推流中:直接通知拉流端可以拉流了
    //  停止:调用FFmpegKits推流,拉流端id加入到等待该方位摄像头推流结果的hash中
    //推流回调中:以qt信号的形式，通过该类所在线程通知拉流端结果
    if(!m_pushVideo)
    {
        cout << "TaskPump::OnActPushVideo:exit:m_pushVideo==false" << endl;
        return;
    }

    auto cameras = obj.find("cameras")->toArray();
    auto driverId = obj.find("driver_id")->toString();
    MediaState mediaState;
    for(auto jsonCameraName : cameras)
    {
        auto cameraName = jsonCameraName.toString();
        if(m_camsFFmpegKits.contains(cameraName))
        {
            auto& ffmpegKitsInfo = m_camsFFmpegKits[cameraName];
            auto ffmpegKits = ffmpegKitsInfo.ffmpegKits;
            mediaState = ffmpegKits->GetMediaState();
            switch (mediaState) {
            case MediaState::Opening:
                log_info << "MediaState::Opening:cameraName=" << cameraName.toStdString() << std::endl;
                m_camsListeners[cameraName].insert(driverId);
                break;
            case MediaState::Playing:
            {
                if(m_config->m_bitRate == "0")
                {
                    if(m_pushVideo && !ffmpegKits->IsValid(ffmpegKits->GetOutAddr(), "tcp", 3000, 3, true))
                    {
                        log_info << "MediaState::Playing:ffmpegKits not IsValid:cameraName=" << cameraName.toStdString() << std::endl;
                        ffmpegKitsInfo.roomId = QUuid::createUuid().toString();
                        if(m_pushVideo)
                            PushVideo(ffmpegKits, cameraName, driverId, ffmpegKitsInfo.roomId);
                        break;
                    }
                }
                QString data =  QString(
                            "{"
                            "\"action\":%1,"
                            "\"car_id\":\"%2\","
                            "\"room_id\":\"%3\","
                            "\"success_cameras\":[\"%4\"],"
                            "\"driver_id\":\"%5\","
                            "\"videoWidth\":%6,"
                            "\"videoHeight\":%7,"
                            "\"videoFPS\":%8"
                            "}"
                            ).
                        arg((int)Action::ActPushVideoNotice).
                        arg(m_config->m_clientId,
                            ffmpegKitsInfo.roomId,
                            cameraName,
                            driverId,
                            m_config->m_videoWidth,
                            m_config->m_videoHeight,
                            m_config->m_frameRate);
                if(m_pushVideo)
                {
                    emit DataOut(data);
                    log_info << "MediaState::Playing:data=" << data.toStdString() << std::endl;
                }
                else
                {
                    log_info << "TaskPump::OnActPushVideo:exit push video, data="<< data.toStdString() << endl;
                }
            }
                break;
            case MediaState::Stopped:
            {
                log_info << "MediaState::Stopped:cameraName=" << cameraName.toStdString() << std::endl;
                ffmpegKitsInfo.roomId = QUuid::createUuid().toString();
                PushVideo(ffmpegKits, cameraName, driverId, ffmpegKitsInfo.roomId);
            }
                break;
            default:
                break;
            }
        }
        else
        {
            log_info << "m_camsFFmpegKits not contains cameraName=" << cameraName.toStdString() << std::endl;
            FFMPEGKITS_PTR ffmpegKits(new FFmpegKits());
            QString roomId = QUuid::createUuid().toString();
            m_camsFFmpegKits[cameraName] = FFmpegKitsVideoInfo{roomId, ffmpegKits};
            PushVideo(ffmpegKits, cameraName, driverId, roomId);
        }
    }
}

void TaskPump::StopPushVideoProcesses()
{
    for(auto it = m_camsFFmpegKits.begin(); it != m_camsFFmpegKits.end(); ++it )
    {
        if(m_pushVideo)
        {
            log_info << "TaskPump::StopPushVideoProcesses:m_pushVideo==true" << ",cameraName=" << it.key().toStdString() << endl;
            return;
        }
        it->ffmpegKits->StopPushing();
        log_info << "cameraName=" << it.key().toStdString() << std::endl;
    }
    emit StopPushingVideo();
}

void TaskPump::PushTask(Action act, const QByteArray& data)
{
    switch (act)
    {
    case Action::ActPushVideo:
        m_pushVideo = true;
        break;
    case Action::ActExitPushVideo:
        cout << "TaskPump::PushTask:Action::ActExitPushVideo" << endl;
        m_pushVideo = false;
        break;
    default:
        break;
    }
    emit TaskIncoming(data);
}

void TaskPump::PushVideo(FFMPEGKITS_PTR ffmpegKits, const QString& cameraName, const QString& driverId,
                         const QString& roomId)
{
    //FFmpegKits StopPush彻底停止推流线程
    ffmpegKits->StopPushing();

    //增加驾驶端到m_camsListeners
    m_camsListeners[cameraName].insert(driverId);

    //运行FFmpegKits推流
    std::unordered_map<int, QString> videoResolutions {
        {1, "ultrafast"},
        {2, "superfast"},
        {3, "veryfast"},
        {4, "faster"},
        {5, "fast"},
        {6, "medium"},
        {7, "slow"},
        {8, "slower"},
        {9, "veryslow"}
    };  
    QString cameraDevPath = GetCameraDevicePath(cameraName);
    if(cameraDevPath.isEmpty())
    {
        QString data =  QString(
                    "{"
                    "\"action\":%1,"
                    "\"car_id\":\"%2\","
                    "\"room_id\":\"%3\","
                    "\"fail_cameras\":[\"%4\"],"
                    "\"driver_id\":\"%5\","
                    "\"videoWidth\":%6,"
                    "\"videoHeight\":%7,"
                    "\"videoFPS\":%8"
                    "}"
                    ).
                arg((int)Action::ActPushVideoNotice).
                arg(m_config->m_clientId,
                    roomId,
                    cameraName,
                    driverId,
                    m_config->m_videoWidth,
                    m_config->m_videoHeight,
                    m_config->m_frameRate);
        if(m_pushVideo)
        {
            emit DataOut(data);
        }
        log_info << "GetCameraDevicePath return null,data=" << data.toStdString() << std::endl;
        return;
    }

    if(!cameraDevPath.contains("rtsp"))
    {
        QString tmpImage = QUuid::createUuid().toString() + ".jpg";
        QString initCamCmd = QString("%3/ffmpeg -f video4linux2 -s 640x480 -i %1 -ss 0:0:2 -frames 1 ./%2 && rm ./%2").
                arg(cameraDevPath, tmpImage, QCoreApplication::applicationDirPath());
        QProcess initCameProc;
        initCameProc.startCommand(initCamCmd);
        if(!initCameProc.waitForStarted())
        {
            log_info << "no start:initCamCmd=" << initCamCmd.toStdString() << ",cameraName=" << cameraName.toStdString() << endl;
        }
        else
        {
            log_info << "start:initCamCmd=" << initCamCmd.toStdString() << ",cameraName=" << cameraName.toStdString() << endl;
        }
        initCameProc.waitForFinished();
        initCameProc.close();
    }

    QString mediaTransUrl = GetPushMediaUrl(roomId, cameraName, m_config->m_mediaAddr, m_config->m_mediaTransProtocols);
    auto callback = [=](MediaMessage message){
        emit PushVideoMessageIncoming(message, roomId, cameraName);
    };
    VideoPushParam videoPushParam;
    videoPushParam.bitRate = m_config->m_bitRate.toInt() * 1024;
    videoPushParam.frameRate = m_config->m_frameRate.toInt();
    videoPushParam.frameGroup = m_config->m_frameGroup.toInt();
    videoPushParam.keyIntMin = m_config->m_keyIntMin.toInt();
    videoPushParam.width = m_config->m_videoWidth.toInt();
    videoPushParam.height = m_config->m_videoHeight.toInt();
    videoPushParam.preset = videoResolutions[m_config->m_videoPreset].toStdString();
    videoPushParam.protocol = m_config->m_rtspTransProtocol.toStdString();
    videoPushParam.mediaType = cameraDevPath.contains("rtsp") ? MediaType::NetStream : MediaType::Camera;
    ffmpegKits->PushVideo(cameraDevPath.toStdString(), mediaTransUrl.toStdString(),
                            videoPushParam, callback);
}
