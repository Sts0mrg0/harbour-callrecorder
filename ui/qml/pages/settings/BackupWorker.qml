/*
    Call Recorder for SailfishOS
    Copyright (C) 2016 Dmitriy Purgin <dpurgin@gmail.com>

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

import QtQuick 2.2
import Sailfish.Silica 1.0

import kz.dpurgin.callrecorder.BackupHelper 1.0

import "../../widgets"

Page
{
    property string fileName
    property bool compress: false
    property bool overwrite: true
    property string outputLocation
    property bool removeExisting: true

    property bool isBackup: true
    readonly property bool isRestore: !isBackup

    readonly property bool restoreComplete:
        isRestore && errorCode === BackupHelper.None && operation === BackupHelper.Complete

    onRestoreCompleteChanged:
    {
        console.log('restore complete: '+ restoreComplete);
    }

    property alias busy: backupHelper.busy
    property alias errorCode: backupHelper.errorCode
    property alias progress: backupHelper.progress
    property alias operation: backupHelper.operation        

    backNavigation: !busy && !restoreComplete

    states: [
        State
        {
            when: busy && operation !== BackupHelper.NotStarted

            PropertyChanges
            {
                target: progressBar

                label:
                {
                    switch (operation)
                    {
                        case BackupHelper.Preparing: return qsTr('Preparing...');
                        case BackupHelper.BackingUp: return qsTr('Making backup...');
                        case BackupHelper.RemovingOldData: return qsTr('Removing old data...');
                        case BackupHelper.Restoring: return qsTr('Restoring...');
                        case BackupHelper.Complete: return qsTr('Complete!')
                        default:
                    }

                    return qsTr('Not started');
                }
            }
        },

        State
        {
            when: !busy && errorCode === BackupHelper.None && progress > 0

            PropertyChanges
            {
                target: progressBar

                label: qsTr('Complete!')
            }
        },

        State
        {
            when: errorCode !== BackupHelper.None

            PropertyChanges
            {
                target: progressBar

                label: qsTr('Error')
            }

            PropertyChanges
            {
                target: progressBar

                indeterminate: false
                value: 100
                maximumValue: 100

                label:
                {
                    switch (errorCode)
                    {
                        case BackupHelper.UnableToWrite: return qsTr('Unable to write archive');
                        case BackupHelper.UnableToStart: return qsTr('Unable to start thread');
                        case BackupHelper.FileExists: return qsTr('Backup file already exists');
                        case BackupHelper.FileNotExists: return qsTr('Backup file doesn\'t exist');
                    }

                    return qsTr('Unknown error');
                }
            }
        }

    ]

    BackupHelper
    {
        id: backupHelper
    }

    Column
    {
        anchors.fill: parent

        PageHeader
        {
            title: isBackup? qsTr('Backup'): qsTr('Restore')
        }

        StyledLabel
        {
            text: isBackup?
                      qsTr('Performing backup. Please do not close the application until the operation is complete'):
                      qsTr('Performing restore. Please do not close the application until the operation is complete. Doing so may damage the data completely and lead to unpredictable behavior.')


            height: implicitHeight + Theme.paddingLarge

            color: Theme.highlightColor
            font.pixelSize: Theme.fontSizeSmall
        }

        ProgressBar
        {
            id: progressBar

            width: parent.width

            indeterminate: progress <= 1

            minimumValue: 0
            maximumValue: backupHelper.totalCount
            value: backupHelper.progress
        }

        Item
        {
            width: parent.width
            height: Theme.paddingLarge * 2
        }

        StyledLabel
        {
            text: qsTr('Please restart the application')

            anchors.horizontalCenter: parent.horizontalCenter
            horizontalAlignment: Text.AlignHCenter

            font.pixelSize: Theme.fontSizeSmall

            visible: restoreComplete
            opacity: visible? 1: 0

            Behavior on opacity
            {
                FadeAnimator { }
            }
        }

        StyledLabel
        {
            text: qsTr('Database, settings and recordings were successfully restored. ' +
                       'The Call Recorder needs to be restarted to apply the changes. ' +
                       'If you chose to merge the existing recording files, you should run the ' +
                       'database repair tool from Utilities after restart.');

            color: Theme.secondaryColor

            height: implicitHeight + Theme.paddingMedium

            font.pixelSize: Theme.fontSizeExtraSmall

            visible: restoreComplete
            opacity: visible? 1: 0

            Behavior on opacity {
                FadeAnimator { }
            }
        }
    }

    onStatusChanged:
    {
        if (status === PageStatus.Activating)
        {
            if (isBackup)
                backupHelper.backup(fileName, compress, overwrite);
            else if (isRestore)
                backupHelper.restore(fileName, outputLocation, removeExisting);
        }
    }
}
