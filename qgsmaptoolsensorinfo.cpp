/***************************************************************************
                          qgsmaptoolsensorinfo.cpp  -  description
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

#include "qgsmaptoolsensorinfo.h"
#include "qgsmapcanvas.h"
#include "qgsmapmouseevent.h"
#include "qgsmaptoolidentify.h"
#include "qgsnetworkaccessmanager.h"
#include "qgsrubberband.h"
#include "qgssensorinfodialog.h"
#include "qgsvectordataprovider.h"
#include "qgsvectorlayer.h"
#include <QDateTime>
#include <QMouseEvent>
#include <QNetworkReply>
#include <QNetworkRequest>

QgsMapToolSensorInfo::QgsMapToolSensorInfo( QgsMapCanvas* canvas ): QgsMapTool( canvas ),
    mDataAvailabilityRequestFinished( true ), mSensorInfoDialog( 0 ), mRubberBand( 0 ), mDragging( false )
{

}

QgsMapToolSensorInfo::~QgsMapToolSensorInfo()
{
  delete mSensorInfoDialog;
  delete mRubberBand;
}

void QgsMapToolSensorInfo::canvasMoveEvent( QgsMapMouseEvent* e )
{
    if ( e->buttons() != Qt::LeftButton )
      return;

    if ( !mDragging )
    {
      mDragging = true;
      mSelectRect.setTopLeft( e->pos() );
    }
    mSelectRect.setBottomRight( e->pos() );

    if( mRubberBand )
    {
        mRubberBand->setToCanvasRectangle( mSelectRect );
    }
}

void QgsMapToolSensorInfo::canvasPressEvent( QgsMapMouseEvent* e )
{
    mSelectRect.setRect( 0, 0, 0, 0 );
    delete mRubberBand;
    mRubberBand = new QgsRubberBand( mCanvas, QgsWkbTypes::PolygonGeometry );
    mRubberBand->setFillColor( QColor( 255, 0, 0, 127 ) );
}

void QgsMapToolSensorInfo::canvasReleaseEvent( QgsMapMouseEvent* e )
{
    delete mRubberBand;
    mRubberBand = 0;

    QList< QgsMapLayer* > sensorLayerList = sensorLayers();
    if ( sensorLayerList.isEmpty() )
    {
      return;
    }

    showSensorInfoDialog();
    mSensorInfoDialog->clearObservables();

    QList< QgsMapLayer* >::const_iterator  layerIt = sensorLayerList.begin();
    for(; layerIt != sensorLayerList.end(); ++layerIt )
    {
        QgsVectorLayer* vl = dynamic_cast<QgsVectorLayer*>( *layerIt );
        if( !vl )
        {
            return;
        }
        QgsDataProvider* dp = vl->dataProvider();
        if( !dp )
        {
            return;
        }

        //rectangle in layer CRS
        QgsPointXY llRect;
        QgsPointXY urRect;
        if( mDragging )
        {
            llRect = toMapCoordinates( mSelectRect.bottomLeft () );
            urRect = toMapCoordinates( mSelectRect.topRight() );
        }
        else //click
        {
            QPoint clickPoint = e->originalPixelPoint();
            //+- 5 pixel tolerance
            llRect = toMapCoordinates( QPoint( clickPoint.x() - 5, clickPoint.y() + 5 ) );
            urRect = toMapCoordinates( QPoint( clickPoint.x() + 5, clickPoint.y() - 5 ) );
        }
        QgsRectangle selectRect = toLayerCoordinates( *layerIt, QgsRectangle( llRect.x(), llRect.y(), urRect.x(), urRect.y() ) );

        QgsFeatureRequest fReq( selectRect );
        QgsFeatureIterator fIt = vl->getFeatures( fReq );

        QgsFeature f;
        while( fIt.nextFeature( f ) )
        {
            QString name = f.attribute( "name" ).toString();
            QString id = f.attribute( "identifier" ).toString();

            //data structure with id / name / list< observable, start time, end time>
            QStringList observedProperties;
            QList< QDateTime > beginList;
            QList< QDateTime > endList;
            QStringList filteredProperties;
            QList< QDateTime > filteredBegin;
            QList< QDateTime > filteredEnd;
            getDataAvailability( dp->dataSourceUri(), id, observedProperties, beginList, endList, filteredProperties, filteredBegin, filteredEnd );

            mSensorInfoDialog->addObservables( dp->dataSourceUri(), id, name, observedProperties, beginList, endList );
            mSensorInfoDialog->addHiddenObservables( dp->dataSourceUri(), id, name, filteredProperties, filteredBegin, filteredEnd );
        }
    }

    mDragging = false;
}

QList< QgsMapLayer* > QgsMapToolSensorInfo::sensorLayers()
{
  QList< QgsMapLayer* > sensorLayerList;

  if ( !mCanvas )
  {
    return sensorLayerList;
  }

  QList<QgsMapLayer*> layerList = mCanvas->layers();
  QList<QgsMapLayer*>::iterator layerIt = layerList.begin();
  for ( ; layerIt != layerList.end(); ++layerIt )
  {
    QgsVectorLayer* vl = qobject_cast<QgsVectorLayer*>( *layerIt );
    if ( vl && vl->dataProvider() )
    {
      if ( vl->dataProvider()->name() == "SOS" )
      {
        sensorLayerList.append( vl );
      }
    }
  }
  return sensorLayerList;
}

int QgsMapToolSensorInfo::getDataAvailability( const QString& serviceUrl, const QString& station_id,
    QStringList& observedPropertyList, QList< QDateTime >& beginList, QList< QDateTime >& endList,
    QStringList& filteredPropertyList, QList< QDateTime >& filteredBeginList, QList< QDateTime >& filteredEndList )
{
  if ( !mDataAvailabilityRequestFinished ) //another request is still in progress
  {
    return 1;
  }

  QUrl url( serviceUrl );
  QUrlQuery query(url);

  //filter list to only get the observables selected for the sos layer
  QSet<QString> selectedObservables = QUrl::fromPercentEncoding( query.queryItemValue( "observedProperty" )\
                                      .toLocal8Bit() ).split( ",", QString::SkipEmptyParts ).toSet();

  query.removeQueryItem( "observedProperty" );
  query.removeQueryItem( "request" );
  query.addQueryItem( "request", "GetDataAvailability" );
  query.addQueryItem( "featureofinterest", station_id );
  url.setQuery( query );
  QString debug = url.toString();

  mDataAvailabilityRequestFinished = false;
  QUrl u = QUrl::fromEncoded( url.toString().toLocal8Bit() );
  QNetworkRequest request( u );
  QNetworkReply* reply = QgsNetworkAccessManager::instance()->get( request );
  connect( reply, SIGNAL( finished() ), this, SLOT( dataAvailabilityRequestFinished() ) );
  while ( !mDataAvailabilityRequestFinished )
  {
    QCoreApplication::processEvents( QEventLoop::ExcludeUserInputEvents, 200 );
  }

  //read results from xml
  QByteArray response = reply->readAll();
  reply->deleteLater();

  QDomDocument dataAvailabilityDocument;
  if ( !dataAvailabilityDocument.setContent( response, true ) )
  {
    return 2;
  }

  QSet<QString> observedPropertySet; //keep track of duplicates

  QDomNodeList dataAvailabilityList = dataAvailabilityDocument.elementsByTagNameNS( "http://www.opengis.net/sosgda/1.0", "dataAvailabilityMember" );
  for ( int i = 0; i < dataAvailabilityList.size(); ++i )
  {
    QDomElement observedPropertyElem = dataAvailabilityList.at( i ).firstChildElement( "observedProperty" );
    if ( observedPropertyElem.isNull() )
    {
      continue;
    }

    //property was not requested
    QString property = observedPropertyElem.attribute( "href" );
    bool propertyRequested = true;
    if( !selectedObservables.isEmpty() && !selectedObservables.contains( property ) )
    {
        propertyRequested = false;
    }

    QDateTime beginTime;
    QDateTime endTime;

    QDomElement phenomenonTimeElem = dataAvailabilityList.at( i ).firstChildElement( "phenomenonTime" );
    if ( !phenomenonTimeElem.isNull() )
    {
      QDomElement timePeriodElem = phenomenonTimeElem.firstChildElement( "TimePeriod" );
      if ( !timePeriodElem.isNull() )
      {
        QDomElement beginElem = timePeriodElem.firstChildElement( "beginPosition" );
        if ( !beginElem.isNull() )
        {
          beginTime = QDateTime::fromString( beginElem.text(), "yyyy'-'MM'-'dd'T'HH':'mm':'ss'.000+00:00'" );
        }
        QDomElement endElem = timePeriodElem.firstChildElement( "endPosition" );
        if ( !endElem.isNull() )
        {
          endTime = QDateTime::fromString( endElem.text(), "yyyy'-'MM'-'dd'T'HH':'mm':'ss'.000+00:00'" );
        }
      }
    }

    if ( observedPropertySet.contains( property ) )
    {
        continue;
    }

    observedPropertySet.insert( property );
    if( propertyRequested )
    {
        observedPropertyList.push_back( property );
        beginList.push_back( beginTime );
        endList.push_back( endTime );
    }
    else
    {
        filteredPropertyList.push_back( property );
        filteredBeginList.push_back( beginTime );
        filteredEndList.push_back( endTime );
    }
  }

  return 0;
}

void QgsMapToolSensorInfo::dataAvailabilityRequestFinished()
{
  mDataAvailabilityRequestFinished = true;
}

void QgsMapToolSensorInfo::showSensorInfoDialog()
{
  if ( !mSensorInfoDialog )
  {
    mSensorInfoDialog = new QgsSensorInfoDialog();
  }
  mSensorInfoDialog->openObservablesTab();
  mSensorInfoDialog->show();
  mSensorInfoDialog->raise();
}
