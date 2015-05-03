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

import kz.dpurgin.nemomobile.contacts 1.0

Dialog {
    property var selectedPhoneNumbers: []
    property bool multiSelect: true

    SilicaFlickable {
        anchors.fill: parent

        DialogHeader {
            id: header

            acceptText: qsTr('Accept')
            cancelText: qsTr('Cancel')
        }

        SilicaListView {
            anchors {
                top: header.bottom
                bottom: parent.bottom
                left: parent.left
                right: parent.right
            }

            model: PeopleModel {
                filterType: PeopleModel.FilterAll
                requiredProperty: PeopleModel.PhoneNumberRequired
            }

            delegate: ContactPickerDelegate { }

            VerticalScrollDecorator { }
        }
    }

    function addToSelection(phoneNumber)
    {
        if (multiSelect)
        {
            if (selectedPhoneNumbers.indexOf(phoneNumber) === -1)
                selectedPhoneNumbers.push(phoneNumber);
        }
        else
            selectedPhoneNumbers = [ phoneNumber ];
    }

    function isSelected(phoneNumber)
    {
        return (selectedPhoneNumbers.indexOf(phoneNumber) !== -1);
    }

    function removeFromSelection(phoneNumber)
    {
        var idx = selectedPhoneNumbers.indexOf(phoneNumber);

        if (idx !== -1)
            selectedPhoneNumbers.splice(idx, 1);
    }
}
