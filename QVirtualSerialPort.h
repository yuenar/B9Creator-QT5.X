#pragma once
#include "qextserialport-1.2beta2/src/qextserialport.h"
#include <QTimer>
class QVirtualSerialPort :
	public QextSerialPort
{
	Q_OBJECT
public:
	QVirtualSerialPort(QextSerialPort::QueryMode mode, QObject *parent);
	QByteArray readAll();
	qint64 write(const QByteArray &data);
	bool open(OpenMode mode);
	void startWatchDogTimer();
	bool isOpen(){return true;}
;
	~QVirtualSerialPort(void);
private:
	QByteArray m_Bufbytes;
	QTimer *timer;
	QString projectorPower;
public slots:
	void emitReadReady();
	void watchDog();
};

