/*
    Call Recorder for SailfishOS
    Copyright (C) 2014-2016 Dmitriy Purgin <dpurgin@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef HARBOUR_CALLRECORDERD_VOICECALLRECORDER_H
#define HARBOUR_CALLRECORDERD_VOICECALLRECORDER_H

#include <QAudio>
#include <QDateTime>
#include <QObject>

class VoiceCallRecorder : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(VoiceCallRecorder)

public:
    enum CallType
    {
        Incoming,
        Outgoing,
        Partial
    };

    enum State
    {
        Inactive,
        Armed,
        Active,
        Suspended,
        WaitingForFinish,
        Finish
    };

public:
    explicit VoiceCallRecorder(const QString& dbusObjectPath);
    virtual ~VoiceCallRecorder();

    CallType callType() const;
    State state() const;
    QDateTime timeStamp() const;

signals:
    void stateChanged(VoiceCallRecorder::State state);

public slots:

private slots:
    void onAudioInputDeviceReadyRead();
    void onAudioInputStateChanged(QAudio::State state);
    void onVoiceCallLineIdentificationChanged(const QString& lineIdentification);

    void processOfonoState(const QString& state);

private:
    void arm();

    void setCallType(CallType callType);
    void setState(State state);
    void setTimeStamp(const QDateTime& timeStamp);

private:
    class VoiceCallRecorderPrivate;
    QScopedPointer< VoiceCallRecorderPrivate > d;
};

#endif // HARBOUR_CALLRECORDERD_VOICECALLRECORDER_H
