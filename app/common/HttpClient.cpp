#include "HttpClient.h"

#include <QDebug>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSslConfiguration>
#include <QSslError>
#include <QSslSocket>
#include <QTimer>

HttpClient::HttpClient(QObject* parent/* = nullptr*/) :
	QObject(parent),
	m_httpManager(new QNetworkAccessManager(this)),
	m_timer(new QTimer(this))
{
	connect(m_httpManager, &QNetworkAccessManager::finished, this,
		&HttpClient::OnReceiveReply);

	m_timer->callOnTimeout([=]()
		{
			qDebug() << "HttpClient::callOnTimeout:1" << "\n";
			m_httpCallback(QByteArray(), HttpErrorCode::Timeout);
			//内部触发QNetworkAccessManager::finished，调用OnReceiveReply
			StopRequest();
		});
}

HttpClient::~HttpClient()
{
	Clear();
}

void HttpClient::InitRequest(QNetworkRequest& networkRequest)
{
	auto sslConfiguration = networkRequest.sslConfiguration();
	sslConfiguration.setPeerVerifyMode(QSslSocket::VerifyNone);
	networkRequest.setSslConfiguration(sslConfiguration);
}

void HttpClient::InitReply(QNetworkReply* reply)
{
	if (!reply)
		return;

	connect(reply, &QNetworkReply::sslErrors, this,
		[reply](const QList<QSslError>& errors)
		{
			Q_UNUSED(errors);
			reply->ignoreSslErrors();
		}
	);
}

void HttpClient::Post(
	const QString& url,
	const QString& postData,
	HTTP_CALLBACK callback,
	int mSecond /*= 10000 */
)
{
	m_httpCallback = callback;
	m_netRequestType = NetRequestType::HttpRequest;

	Clear();

	QNetworkRequest networkRequest(url);
	InitRequest(networkRequest);
	networkRequest.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

	m_reply = m_httpManager->post(networkRequest, postData.toUtf8());
	InitReply(m_reply);

	m_timer->setSingleShot(true);
	m_timer->start(mSecond);
}

void HttpClient::Get(
	const QString& url,
	HTTP_CALLBACK callback,
	int mSecond /*= 10000*/
)
{
	m_httpCallback = callback;
	m_netRequestType = NetRequestType::HttpRequest;

	Clear();

	QNetworkRequest networkRequest(url);
	InitRequest(networkRequest);

	m_reply = m_httpManager->get(networkRequest);
	InitReply(m_reply);

	m_timer->setSingleShot(true);
	m_timer->start(mSecond);
}

bool HttpClient::IsRunning() const
{
	if (m_reply && m_reply->isRunning())
		return true;
	return false;
}

void HttpClient::OnReceiveReply(QNetworkReply* reply)
{
	if (m_timer->isActive())
		m_timer->stop();

	if (m_netRequestType != NetRequestType::HttpRequest)
		return;

	auto replyErr = reply->error();
	if (replyErr == QNetworkReply::OperationCanceledError)
		return;

	if (replyErr != QNetworkReply::NoError)
	{
		m_httpCallback(QByteArray(), HttpErrorCode::Fail);
	}
	else
	{
		m_httpCallback(reply->readAll(), HttpErrorCode::Success);
	}
}

void HttpClient::StopRequest()
{
	if (m_reply)
	{
		if (m_reply->isRunning())
			m_reply->abort();
		m_reply->deleteLater();
		m_reply = nullptr;
	}
}

void HttpClient::Clear()
{
	StopRequest();
	if (m_timer->isActive())
		m_timer->stop();
}

bool HttpClient::Download(
	const QString& url,
	const QString& saveFilePath,
	DOWNLOAD_CALLBACK callback
)
{
	StopRequest();

	m_netRequestType = NetRequestType::Download;
	m_downloadCallback = callback;
	m_downFile.setFileName(saveFilePath);
	m_stopDownload = false;
	if (!m_downFile.open(QIODevice::ReadWrite | QIODevice::Append))
	{
		m_downloadCallback(0, 0, 0, HttpErrorCode::OpenFileFial);
		return false;
	}

	QNetworkRequest networkRequest(url);
	InitRequest(networkRequest);

	QString range = QString("bytes=%1-").arg(m_downFile.size());
	networkRequest.setRawHeader("Range", range.toUtf8());

	m_reply = m_httpManager->get(networkRequest);
	InitReply(m_reply);

	m_totalSize = 0;
	m_writeSize = 0;

	connect(m_reply, &QNetworkReply::readyRead, this, [this]
		{
			auto reply = qobject_cast<QNetworkReply*>(sender());
			if (!reply)
				return;

			const QByteArray data = reply->readAll();
			m_writeSize += data.size();
			m_downFile.write(data);
		}
	);

	connect(m_reply, &QNetworkReply::finished, this, [this]
		{
			m_downFile.close();
			if (m_stopDownload)
			{
				//说明是手动停止的
				return;
			}

			if (m_reply && m_reply->error() != QNetworkReply::NoError)
			{
				m_downloadCallback(0, m_writeSize, m_totalSize, HttpErrorCode::Fail);
				return;
			}

			m_downloadCallback(m_totalSize, m_writeSize, m_totalSize, HttpErrorCode::Success);
		}
	);

	connect(m_reply, &QNetworkReply::errorOccurred, this, [this]
	(QNetworkReply::NetworkError code)
		{
			Q_UNUSED(code);
			m_downFile.close();
			if (m_stopDownload)
			{
				//说明是手动停止的
				return;
			}
			m_downloadCallback(0, m_writeSize, m_totalSize, HttpErrorCode::Fail);
		}
	);

	connect(m_reply, &QNetworkReply::downloadProgress, this, [this]
	(qint64 bytesReceived, qint64 bytesTotal)
		{
			m_totalSize = bytesTotal;
			m_downloadCallback(bytesReceived, m_writeSize, m_totalSize, HttpErrorCode::Downloading);
		}
	);

	return true;
}

void HttpClient::Stop()
{
	m_stopDownload = true;
	Clear();
}