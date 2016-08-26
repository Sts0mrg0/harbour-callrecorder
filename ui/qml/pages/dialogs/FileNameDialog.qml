/*
    Call Recorder for SailfishOS
    Copyright (C) 2014-2015 Dmitriy Purgin <dpurgin@gmail.com>

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

Dialog {
    property alias fileName: textField.text

    allowedOrientations: Orientation.All

    SilicaFlickable {
        anchors.fill: parent

        DialogHeader {
            id: header

            //: Dialog button. This should be the same as the corresponding Sailfish string
            //% "Accept"
            acceptText: qsTrId('id_do_accept')
            //: Dialog button. This should be the same as the corresponding Sailfish string
            //% "Cancel"
            cancelText: qsTrId('id_do_cancel')
        }

        TextField {
            id: textField

            anchors.top: header.bottom

            width: parent.width            

            placeholderText: qsTr('New name')
            label: qsTr('New name')

            focus: true

            inputMethodHints: Qt.ImhNoPredictiveText
            validator: RegExpValidator {
                regExp: /[^\/]+/
            }
        }
    }
}
