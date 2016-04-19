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

#include "voicecallrecorder.h"

#include <QAudioInput>
#include <QFileInfo>
#include <QScopedPointer>

#include <FLAC/stream_encoder.h>

#include <qofono-qt5/qofonovoicecall.h>

#include <libcallrecorder/blacklisttablemodel.h>
#include <libcallrecorder/eventstablemodel.h>
#include <libcallrecorder/phonenumberstablemodel.h>
#include <libcallrecorder/settings.h>
#include <libcallrecorder/whitelisttablemodel.h>

#include "application.h"
#include "dbusadaptor.h"
#include "model.h"

class VoiceCallRecorder::VoiceCallRecorderPrivate
{
    friend class VoiceCallRecorder;

    VoiceCallRecorderPrivate()
        : audioInputDevice(NULL),
          callType(VoiceCallRecorder::Incoming),
          duration(0),
          eventId(-1),
          flacEncoder(NULL),
          state(VoiceCallRecorder::Inactive)
    {
    }

    QScopedPointer< QAudioInput > audioInput;
    QIODevice* audioInputDevice;

    VoiceCallRecorder::CallType callType;

    QString dbusObjectPath;

    quint64 duration; // recording duration

    int eventId;
    QString lineIdentification;

    FLAC__StreamEncoder* flacEncoder;

    qint64 lastRecordStart; // stores the last time the recording was started. Need to calculate duration

    QString outputLocation;

    QScopedPointer< QOfonoVoiceCall > qofonoVoiceCall;

    VoiceCallRecorder::State state;

    QDateTime timeStamp;    
};

VoiceCallRecorder::VoiceCallRecorder(const QString& dbusObjectPath)
    : d(new VoiceCallRecorderPrivate())
{
    qDebug() << dbusObjectPath;

    d->dbusObjectPath = dbusObjectPath;

    d->qofonoVoiceCall.reset(new QOfonoVoiceCall());
    d->qofonoVoiceCall->setVoiceCallPath(dbusObjectPath);

    d->lineIdentification = d->qofonoVoiceCall->lineIdentification();

    connect(d->qofonoVoiceCall.data(), &QOfonoVoiceCall::lineIdentificationChanged,
            this, &VoiceCallRecorder::onVoiceCallLineIdentificationChanged);

    connect(d->qofonoVoiceCall.data(), &QOfonoVoiceCall::stateChanged,
            this, &VoiceCallRecorder::processOfonoState);

#ifdef QT_DEBUG
    connect(daemon->dbusAdaptor(), &DBusAdaptor::EmulatedLineIdentificationChanged,
            this, &VoiceCallRecorder::onVoiceCallLineIdentificationChanged);
    connect(daemon->dbusAdaptor(), &DBusAdaptor::EmulatedVoiceCallStateChanged,
            this, &VoiceCallRecorder::processOfonoState);
#endif // QT_DEBUG
}

VoiceCallRecorder::~VoiceCallRecorder()
{
    qDebug() << d->dbusObjectPath;

    // if the call was recorded, the processor for "disconnected" signal should have told us
    // to finalize FLAC encoder, as readyRead() could still have been called after audioInput->stop()
    if (state() == WaitingForFinish)
    {
        FLAC__stream_encoder_finish(d->flacEncoder);
        FLAC__stream_encoder_delete(d->flacEncoder);

        // update recording state to Waiting for Approval or Done
        if (d->eventId != -1)
        {
            QFileInfo fi(d->outputLocation);

            QVariantMap params;

            EventsTableModel::RecordingState recordingState =
                    daemon->settings()->requireApproval()?
                        EventsTableModel::WaitingForApproval:
                        EventsTableModel::Done;

            params.insert(QLatin1String("RecordingStateID"), static_cast< int >(recordingState));
            params.insert(QLatin1String("FileSize"), fi.size());
            params.insert(QLatin1String("Duration"), d->duration / 1000); // duration is stored in seconds

            daemon->model()->events()->update(d->eventId, params);
        }
        // If call was armed, FLAC has started the output file. Need to remove it
        else
            QFile(d->outputLocation).remove();

        setState(Inactive);
    }

    // the state() should be Inactive. If it isn't, something went wrong
    if (state() != Inactive)
        qWarning() << __PRETTY_FUNCTION__ <<
                      QLatin1String(": dbusObjectPath = ") << d->dbusObjectPath <<
                      QLatin1String(", qofonoVoiceCall->state() = ") << d->qofonoVoiceCall->state() <<
                      QLatin1String(", state() = ") << state();
}

void VoiceCallRecorder::arm()
{
    qDebug();

    const QAudioFormat audioFormat = daemon->settings()->audioFormat();

    qDebug() << QLatin1String("Audio format is: ") << audioFormat;

    // create audio input device
    if (d->audioInput.isNull())
    {
        d->audioInput.reset(new QAudioInput(daemon->settings()->inputDevice(), audioFormat));
        connect(d->audioInput.data(), SIGNAL(stateChanged(QAudio::State)),
                this, SLOT(onAudioInputStateChanged(QAudio::State)));
    }
    else
        qWarning() << QLatin1String(": d->audioInput expected to be NULL but it wasn't!");

    // initialize FLAC encoder
    if (d->flacEncoder == NULL)
    {
        d->flacEncoder = FLAC__stream_encoder_new();

        if (d->flacEncoder == NULL)
            qCritical() << QLatin1String("unable to create FLAC encoder!");

        if (!FLAC__stream_encoder_set_channels(d->flacEncoder, audioFormat.channelCount()))
            qCritical() << QLatin1String(": unable to set FLAC channels: ") <<
                           FLAC__stream_encoder_get_state(d->flacEncoder);

        if (!FLAC__stream_encoder_set_bits_per_sample(d->flacEncoder, audioFormat.sampleSize()))
            qCritical() << QLatin1String(": unable to set FLAC bits per sample: ") <<
                           FLAC__stream_encoder_get_state(d->flacEncoder);

        if (!FLAC__stream_encoder_set_sample_rate(d->flacEncoder, audioFormat.sampleRate()))
            qCritical() << QLatin1String(": unable to set FLAC sample rate: ") <<
                           FLAC__stream_encoder_get_state(d->flacEncoder);

        if (!FLAC__stream_encoder_set_compression_level(d->flacEncoder, daemon->settings()->compression()))
            qCritical() << QLatin1String(": unable to set FLAC compression level: ") <<
                           FLAC__stream_encoder_get_state(d->flacEncoder);

        QString callTypeSuffix;

        if (callType() == Incoming)
            callTypeSuffix = QLatin1String("in");
        else if (callType() == Outgoing)
            callTypeSuffix = QLatin1String("out");
        else
            callTypeSuffix = QLatin1String("part");

        // form output location with file name based on current date, phone number, call direction (in/out).
        // File name is set to "{timestamp}_{phoneNumber}_{type}.flac"
        d->outputLocation = (daemon->settings()->outputLocation() %
                             QLatin1Char('/') %
                             Application::getIsoTimeStamp(timeStamp()).replace(QChar(':'), QChar('_')) % QLatin1Char('_') %
                             d->lineIdentification % QLatin1Char('_') %
                             callTypeSuffix %
                             QLatin1String(".flac"));

        qDebug() << QLatin1String("Writing to ") << d->outputLocation;

        FLAC__StreamEncoderInitStatus status =
                FLAC__stream_encoder_init_file(d->flacEncoder,
                                               d->outputLocation.toLatin1().data(),
                                               NULL,
                                               NULL);

        if (status != FLAC__STREAM_ENCODER_INIT_STATUS_OK)
        {
            qCritical() << QLatin1String(": unable to init FLAC file: ") <<
                           status <<
                           QLatin1String(", current encoder status is ") <<
                           FLAC__stream_encoder_get_state(d->flacEncoder);
        }
    }
    else
        qWarning() << QLatin1String(": d->flacEncoder expected to be NULL but it wasn't!");

    // add new event to events table
    if (d->eventId == -1)
    {
        EventsTableModel::EventType eventType;

        if (callType() == Incoming)
            eventType = EventsTableModel::Incoming;
        else if (callType() == Outgoing)
            eventType = EventsTableModel::Outgoing;
        else
            eventType = EventsTableModel::Partial;

        d->eventId =
                daemon->model()->events()->add(
                    timeStamp(),                                   // time stamp of recording
                    daemon->model()->phoneNumbers()->getIdByLineIdentification(
                        d->lineIdentification),                    // phone number ref
                    eventType,
                    QFileInfo(d->outputLocation).fileName(),
                    EventsTableModel::Armed);                      // initial recording state
    }
    else
        qWarning() << QLatin1String(": d->eventId expected to be -1 but it wasn't!");

    setState(Armed);
}

VoiceCallRecorder::CallType VoiceCallRecorder::callType() const
{
    return d->callType;
}

void VoiceCallRecorder::onAudioInputDeviceReadyRead()
{
//    qDebug() << __PRETTY_FUNCTION__;

    QByteArray data = d->audioInputDevice->readAll();

//    qDebug() << __PRETTY_FUNCTION__ << ": read bytes: " << data.size();

    const qint16* sampleData = reinterpret_cast< const qint16* >(data.constData());

    quint64 sampleCount = data.size() / (daemon->settings()->audioFormat().sampleSize() / 8);

//    qDebug() << __PRETTY_FUNCTION__ << ": sample count: " << sampleCount;

    FLAC__StreamEncoderState state = FLAC__stream_encoder_get_state(d->flacEncoder);

    if (state != FLAC__STREAM_ENCODER_OK)
        qDebug() << ": FLAC encoder state is not ok: " << state;
    else
    {
        for (quint64 sampleIdx = 0; sampleIdx < sampleCount; sampleIdx++)
        {
            FLAC__int32 sample = static_cast< FLAC__int32 >(sampleData[sampleIdx]);

            if (!FLAC__stream_encoder_process_interleaved(d->flacEncoder, &sample, 1))
            {
                qDebug() << "Unable to FLAC__stream_encoder_process(): " <<
                            FLAC__stream_encoder_get_state(d->flacEncoder);
                break;
            }
        }
    }
}

void VoiceCallRecorder::onAudioInputStateChanged(QAudio::State state)
{
    qDebug() << state;
}

void VoiceCallRecorder::onVoiceCallLineIdentificationChanged(const QString& lineIdentification)
{
    qDebug() << lineIdentification;

    d->lineIdentification = lineIdentification;

    // if eventId exists, it means that the recorder was armed with another phoneNumberId.
    // update it with the current one

    if (d->eventId != -1)
    {
        QVariantMap params;

        params.insert(
            QLatin1String("PhoneNumberID"),
            daemon->model()->phoneNumbers()->getIdByLineIdentification(lineIdentification));

        daemon->model()->events()->update(d->eventId, params);
    }
}

void VoiceCallRecorder::processOfonoState(const QString& ofonoState)
{
    qDebug() << d->dbusObjectPath << ofonoState;

    // if a call is not disconnecting, arm the recorder if not yet
    if (ofonoState != QLatin1String("disconnected") && state() == Inactive)
    {
        if (ofonoState == QLatin1String("incoming"))
            setCallType(Incoming);
        else if (ofonoState == QLatin1String("dialing"))
            setCallType(Outgoing);
        else
            setCallType(Partial); // recorder has been turned on while call is in progress

        setTimeStamp(QDateTime::currentDateTime());

        // if line identification received, arm the recording if operation mode allows.
        // All calls from private numbers are recorded.

        if (!d->qofonoVoiceCall->lineIdentification().isEmpty())
            d->lineIdentification = d->qofonoVoiceCall->lineIdentification();

        if (!d->lineIdentification.isEmpty())
        {
            // check the operation mode
            // If operating in BlackList mode, the call is recorded if it doesn't belong to
            // the black list.
            // If operating in WhiteList mode, the call is recorded only if it belongs to the
            // white list.

            Settings::OperationMode opMode = daemon->settings()->operationMode();

            if ((opMode == Settings::BlackList &&
                    !daemon->model()->blackList()->contains(d->lineIdentification)) ||
                (opMode == Settings::WhiteList &&
                    daemon->model()->whiteList()->contains(d->lineIdentification)))
            {
                arm();
            }
            else
                qWarning() << "Not arming for " << d->lineIdentification
                           << " due to operation mode " << opMode;
        }
        else
        {
            qWarning() << "call from private number, arming recorder";
            arm();
        }
    }

    // the call has been armed just now or is being recorded already
    if (state() != Inactive)
    {
        // when the call goes into active state from ringing, the sound card's profile is set to
        // voicecall-record. Recording is started or resumed
        if (ofonoState == QLatin1String("active"))
        {
            // if the recorder was armed, recording was not started yet. start recording and connect
            // to readyRead() signal to retrieve and encode data
            if (state() == Armed)
            {
                d->audioInputDevice = d->audioInput->start();
                connect(d->audioInputDevice, SIGNAL(readyRead()),
                        this, SLOT(onAudioInputDeviceReadyRead()));
            }
            // if the recorder was suspended, just resume the recording
            else if (state() == Suspended)
                d->audioInput->resume();

            // update event state to InProgress
            if (d->eventId != -1)
            {
                QVariantMap params;
                params.insert(QLatin1String("RecordingStateID"),
                              QVariant(static_cast< int >(EventsTableModel::InProgress)));

                daemon->model()->events()->update(d->eventId, params);
            }

            // update 'last record start' timestamp to calculate duration when the recording ents
            d->lastRecordStart = QDateTime::currentMSecsSinceEpoch();

            setState(Active);
        }
        // stop recording if the call was disconnected
        else if (ofonoState == QLatin1String("disconnected"))
        {
            // stop recording if it was ever started
            if (state() == Active || state() == Suspended)
            {
                d->audioInput->stop();

                // if the call was active, calculate and add duration
                // if the call was suspended, the duration has already been added
                if (state() == Active)
                    d->duration += QDateTime::currentMSecsSinceEpoch() - d->lastRecordStart;

            }
            // if the call was never active, remove the record from Events
            else
            {
                daemon->model()->events()->remove(d->eventId);
                d->eventId = -1;
            }

            // We do not call FLAC__stream_encode_finish at this point, as the d->audioInput can still some data.
            // To actually cleanup FLAC, we will process WaitingForFinish in destructor.
            // In case if the call was never active, FLAC still has to be cleaned up.

            setState(WaitingForFinish);
        }
        // if the call is on hold or waiting, suspend recording if it was active
        else if (ofonoState == QLatin1String("held") || ofonoState == QLatin1String("waiting"))
        {
            if (state() == Active)
            {
                d->audioInput->suspend();

                setState(Suspended);

                if (d->eventId != -1)
                {
                    QVariantMap params;
                    params.insert(QLatin1String("RecordingStateID"), QVariant(static_cast< int >(EventsTableModel::Suspended)));

                    daemon->model()->events()->update(d->eventId, params);
                }

                // calculate and add duration now
                d->duration += QDateTime::currentMSecsSinceEpoch() - d->lastRecordStart;
            }
        }
    }
}

void VoiceCallRecorder::setCallType(CallType callType)
{
    d->callType = callType;
}

void VoiceCallRecorder::setState(State state)
{
    d->state = state;

    emit stateChanged(state);
}

void VoiceCallRecorder::setTimeStamp(const QDateTime& timeStamp)
{
    d->timeStamp = timeStamp;
}

VoiceCallRecorder::State VoiceCallRecorder::state() const
{
    return d->state;
}

QDateTime VoiceCallRecorder::timeStamp() const
{
    return d->timeStamp;
}
