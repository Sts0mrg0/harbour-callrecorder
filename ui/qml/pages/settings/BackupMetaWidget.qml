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

import QtQuick 2.0
import Sailfish.Silica 1.0

Column
{
    property BackupMetaObject backupMeta

    width: parent.width

    DetailItem
    {
        //: Backup file details
        //% "Program version"
        label: qsTrId("id_program_version")
        value:  backupMeta.producerVersion
    }

    DetailItem
    {
        //: Backup file details
        //% "Date and time"
        label: qsTrId("id_date_and_time")
        value: Format.formatDate(backupMeta.timeStamp, Format.Timepoint)
    }

    DetailItem
    {
        //: Backup file details
        //% "Unpacked size"
        label: qsTrId("id_unpacked_size")
        value: Format.formatFileSize(backupMeta.restoreSize)
    }

    DetailItem
    {
        //: Backup file details
        //% "Files"
        label: qsTrId('id_files')
        value: backupMeta.totalCount
    }
}
