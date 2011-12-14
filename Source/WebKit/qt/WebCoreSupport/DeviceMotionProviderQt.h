/*
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */
#ifndef DeviceMotionProviderQt_h
#define DeviceMotionProviderQt_h

#include "DeviceMotionData.h"
#include "RefPtr.h"

#include <QAccelerometerFilter>
#include <QObject>

namespace WebCore {

class DeviceOrientationProviderQt;

class DeviceMotionProviderQt
    : public QObject
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
    , public QTM_NAMESPACE::QAccelerometerFilter {
#else
    , public QAccelerometerFilter {
#endif
    Q_OBJECT
public:
    DeviceMotionProviderQt();
    ~DeviceMotionProviderQt();

#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
    bool filter(QTM_NAMESPACE::QAccelerometerReading*);
#else
    bool filter(QAccelerometerReading*);
#endif

    void start();
    void stop();
    DeviceMotionData* currentDeviceMotion() const { return m_motion.get(); }

Q_SIGNALS:
    void deviceMotionChanged();

private:
    RefPtr<DeviceMotionData> m_motion;
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
    QTM_NAMESPACE::QAccelerometer m_acceleration;
#else
    QAccelerometer m_acceleration;
#endif
    DeviceOrientationProviderQt* m_deviceOrientation;
};

} // namespace WebCore

#endif // DeviceMotionProviderQt_h
