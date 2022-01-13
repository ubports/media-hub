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

#ifndef TEST_DBUSMOCK_H
#define TEST_DBUSMOCK_H

#include <QDebug>
#include <libqtdbusmock/DBusMock.h>

namespace QtDBusMock {

/* Workaround for libqtdbusmock deserialization bug
 */
static void transform(QVariantList &list);

static void transform(QVariantMap &map);

static QVariant transform(const QDBusArgument & value) {
	switch (value.currentType()) {
	case QDBusArgument::ArrayType: {
		value.beginArray();
		QVariantList list = transform(value).toList();
		value.endArray();
		return list;
	}
	case QDBusArgument::MapType: {
        if (value.currentSignature() == "a{sv}") {
            QVariantMap map = qdbus_cast<QVariantMap>(value);
            transform(map);
            return map;
        }
        return value.asVariant();
	}
	case QDBusArgument::StructureType: {
		value.beginStructure();
		QVariantList list;
		while (!value.atEnd()) {
			list << value.asVariant();
		}
		value.endStructure();
		return list;
	}
	default:
		qDebug() << "Unhandled type" << value.currentType()
				<< value.currentSignature();
	}
	return QVariant();
}

static void transform(QVariant &variant) {
	if (variant.canConvert<QVariantList>()) {
		QVariantList list = variant.toList();
		transform(list);
		variant = list;
	} else if (variant.canConvert<QDBusArgument>()) {
		QDBusArgument value(variant.value<QDBusArgument>());
		variant = transform(value);
	}
}

static void transform(QVariantMap &map) {
	for (auto it(map.begin()); it != map.end(); ++it) {
		transform(*it);
	}
}

static void transform(QVariantList &list) {
	for (auto it(list.begin()); it != list.end(); ++it) {
		transform(*it);
	}
}

const QDBusArgument readMethod(const QDBusArgument &argument,
                               MethodCall &methodCall) {
	quint64 timestamp;
	QVariantList args;

	argument.beginStructure();
	argument >> timestamp >> args;
	argument.endStructure();

	transform(args);

	methodCall.setTimestamp(timestamp);
	methodCall.setArgs(args);

	return argument;
}

QList<MethodCall> parseCalls(const QDBusMessage &reply)
{
    QList<MethodCall> calls;
    const QDBusArgument arg = reply.arguments().at(0).value<QDBusArgument>();
    arg.beginArray();
    while (!arg.atEnd()) {
        MethodCall call;
        readMethod(arg, call);
        calls.append(call);
    }
    return calls;
}

} // namespace

#endif // TEST_DBUSMOCK_H
