/*
 * Copyright © 2021 UBports Foundation.
 *
 * Contact: Alberto Mardegan <mardy@users.sourceforge.net>
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "MediaHub/error_p.h"

#include "MediaHub/dbus_constants.h"

#include <QObject>
#include <QTest>

using namespace lomiri::MediaHub;

class TestError: public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void testErrorFromApi_data();
    void testErrorFromApi();
};

void TestError::testErrorFromApi_data()
{
    QTest::addColumn<qint16>("apiCode");
    QTest::addColumn<int>("expectedCode");
    QTest::addColumn<QString>("expectedMessage");

    QTest::newRow("no error") <<
        qint16(DBusConstants::NoError) <<
        int(Error::NoError) <<
        QString();

    QTest::newRow("resource error") <<
        qint16(DBusConstants::ResourceError) <<
        int(Error::ResourceError) <<
        "A media resource couldn't be resolved.";

    QTest::newRow("format error") <<
        qint16(DBusConstants::FormatError) <<
        int(Error::FormatError) <<
        "The media format type is not playable due to a missing codec.";

    QTest::newRow("network error") <<
        qint16(DBusConstants::NetworkError) <<
        int(Error::NetworkError) <<
        "A network error occurred.";

    QTest::newRow("permission error") <<
        qint16(DBusConstants::AccessDeniedError) <<
        int(Error::AccessDeniedError) <<
        "Insufficient privileges to play that media.";

    QTest::newRow("service error") <<
        qint16(DBusConstants::ServiceMissingError) <<
        int(Error::ServiceMissingError) <<
        "A valid playback service was not found, "
        "playback cannot proceed.";

    QTest::newRow("unknown error") <<
        qint16(5353) <<
        int(Error::InternalError) <<
        "The backend emitted an unrecognized error code";
}

void TestError::testErrorFromApi()
{
    QFETCH(qint16, apiCode);
    QFETCH(int, expectedCode);
    QFETCH(QString, expectedMessage);

    Error error = errorFromApiCode(apiCode);
    QCOMPARE(error.code(), expectedCode);
    QCOMPARE(error.message(), expectedMessage);
}

QTEST_GUILESS_MAIN(TestError)

#include "test_error.moc"
