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

Item {
    property string mode: 'dialpad';
    property string value: '';

    Row {
        width: parent.width

        TextField {
            id: inputField

            width: parent.width - iconsRow.width

            placeholderText: qsTr('Search or add')

            inputMethodHints: mode == 'dialpad'? Qt.ImhDialableCharactersOnly: Qt.ImhNoPredictiveText

            onTextChanged: {
                if (/^[0-9p*#+]+$/.test(text))
                    value = text;
                else
                    value = '';
            }
        }

        Row {
            id: iconsRow

            IconButton {
                icon.source: mode == 'dialpad'?
                                 'image://theme/icon-m-keyboard':
                                 'image://theme/icon-m-dialpad'

                onClicked: {
                    mode = (mode == 'dialpad'? 'keyboard': 'dialpad')

                    inputField.forceActiveFocus()
                }
            }

            IconButton {
                icon.source: 'image://theme/icon-m-add'

                visible: value.length > 0
            }
        }
    }
}
