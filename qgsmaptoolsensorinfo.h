/***************************************************************************
                          qgsmaptoolsensorinfo.h  -  description
                          --------------------------------------
    begin                : June 2013
    copyright            : (C) 2013 by Marco Hugentobler
    email                : marco dot hugentobler at sourcepole dot ch
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef QGSMAPTOOLSENSORINFO_H
#define QGSMAPTOOLSENSORINFO_H

#ifndef M_PI
#define M_PI 3.1415926535897931159979634685
#endif

#include "qgsmaptool.h"

class QgsRubberBand;
class QgsSensorInfoDialog;
class QgsVectorLayer;
class QDateTime;

class QgsMapToolSensorInfo: public QgsMapTool
{
    Q_OBJECT
  public:
    QgsMapToolSensorInfo( QgsMapCanvas* canvas );
    ~QgsMapToolSensorInfo();

    void canvasMoveEvent( QgsMapMouseEvent* e ) override;

    void canvasPressEvent( QgsMapMouseEvent* e ) override;

    void canvasReleaseEvent( QgsMapMouseEvent* e ) override;

  private:
    /**Returns sensor layers in the project*/
    QList< QgsMapLayer* > sensorLayers();
    /**Does GetDataAvailability request for station id*/
    int getDataAvailability( const QString& serviceUrl, const QString& station_id,
                             QStringList& observedPropertyList, QList< QDateTime >& begin,
                             QList< QDateTime >& end );

    bool mDataAvailabilityRequestFinished;

    QgsSensorInfoDialog* mSensorInfoDialog;

    QgsRubberBand* mRubberBand;

    bool mDragging;

    QRect mSelectRect;

  private slots:
    void dataAvailabilityRequestFinished();
    void showSensorInfoDialog();
};

#endif // QGSMAPTOOLSENSORINFO_H
