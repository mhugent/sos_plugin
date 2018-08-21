/***************************************************************************
                          qgssensorinfodialog.cpp  -  description
                          ---------------------------------------
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

#include "qgssensorinfodialog.h"
#include "qgsmessagelog.h"
#include "qgswatermldata.h"
#include <QCheckBox>
#include <QCursor>
#include <QDateTimeEdit>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QInputDialog>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <QTextStream>
#include <QUrl>
#include <QToolTip>
#include <qwt_picker_machine.h>
#include <qwt_plot.h>
#include <qwt_plot_curve.h>
#include <qwt_plot_marker.h>
#include <qwt_plot_picker.h>
#include <qwt_scale_draw.h>
#include <qwt_symbol.h>

class TimeScaleDraw: public QwtScaleDraw
{
  public:
    TimeScaleDraw() {}
    virtual QwtText label( double v ) const
    {
      if ( v >= 0 )
      {
        return QDateTime::fromTime_t( v ).toString();
      }
      else
      {
        return QDateTime::fromTime_t( 0 ).addSecs( v ).toString();
      }
    }
};

QgsSensorInfoDialog::QgsSensorInfoDialog( QWidget* parent, Qt::WindowFlags f ): QDialog( parent, f )
{
  setupUi( this );

  QPushButton* displayButton = new QPushButton( tr( "Display" ), this );
  connect( displayButton, SIGNAL( clicked() ), this, SLOT( showDiagram() ) );
  mButtonBox->addButton( displayButton, QDialogButtonBox::ActionRole );

  mShowAllButton = new QPushButton( tr( "Show all observables" ), this );
  mShowAllButton->setEnabled( false );
  connect( mShowAllButton, SIGNAL( clicked() ), this, SLOT( showAllObservables() ) );
  mButtonBox->addButton( mShowAllButton, QDialogButtonBox::ActionRole );

  QPushButton* exportToCSVButton = new QPushButton( tr( "Export CSV..." ), this );
  connect( exportToCSVButton, SIGNAL( clicked() ), this, SLOT( exportToCSV() ) );
  mButtonBox->addButton( exportToCSVButton, QDialogButtonBox::ActionRole );

  mTabWidget->setTabsClosable( true );
}

QgsSensorInfoDialog::~QgsSensorInfoDialog()
{
}

void QgsSensorInfoDialog::clearObservables()
{
  mObservableTreeWidget->clear();
  mHiddenObservables.clear();
}

void QgsSensorInfoDialog::addObservables( const QString& serviceUrl, const QString stationId, const QStringList& observables, const QList< QDateTime >& begin,
    const QList< QDateTime >& end )
{
  QTreeWidgetItem* stationIdItem = 0;
  int stationItemCount = mObservableTreeWidget->topLevelItemCount();
  for( int i = 0; i < stationItemCount; ++i )
  {
    QTreeWidgetItem* currentItem = mObservableTreeWidget->topLevelItem( i );
    if( currentItem->text( 0 ) == stationId )
    {
        stationIdItem = currentItem;
        break;
    }
  }

  if( !stationIdItem ) //item does not yet exist, create a new one
  {
      stationIdItem = new QTreeWidgetItem( QStringList() << stationId );
      mObservableTreeWidget->addTopLevelItem( stationIdItem );
      stationIdItem->setData( 0, Qt::UserRole, serviceUrl );
      stationIdItem->setFlags( Qt::ItemIsEnabled );
  }

  QStringList::const_iterator obsIt = observables.constBegin();
  QList< QDateTime >::const_iterator bIt = begin.constBegin();
  QList< QDateTime >::const_iterator eIt = end.constBegin();
  for ( ; obsIt != observables.constEnd() && bIt != begin.constEnd() && eIt != end.constEnd(); ++obsIt )
  {

    QTreeWidgetItem* observableItem = new QTreeWidgetItem( stationIdItem, QStringList() << "" << *obsIt );
    QCheckBox* cb = new QCheckBox();
    mObservableTreeWidget->setItemWidget( observableItem, 2, cb );
    cb->setChecked( true );
    //begin
    QDateTime beginDateTime = bIt->isValid() ? *bIt : QDateTime( QDate( 1900, 1, 1 ), QTime( 0, 0, 0 ), Qt::UTC );
    QDateTimeEdit* beginDateTimeEdit = new QDateTimeEdit( beginDateTime );
    beginDateTimeEdit->setDisplayFormat( "dd.MM.yyyy HH:mm" );
    mObservableTreeWidget->setItemWidget( observableItem, 3, beginDateTimeEdit );
    //end
    QDateTime endDateTime;
    if ( eIt->isValid() )
    {
      endDateTime = *eIt;
    }
    else
    {
      endDateTime =  QDateTime::currentDateTime().toTimeSpec( Qt::UTC );
      endDateTime.setTime( QTime( 0, 0, 0 ) );
    }
    QDateTimeEdit* endDateTimeEdit = new QDateTimeEdit( endDateTime );
    endDateTimeEdit->setDisplayFormat( "dd.MM.yyyy HH:mm" );
    mObservableTreeWidget->setItemWidget( observableItem, 4, endDateTimeEdit );
    if ( bIt->isValid() && eIt->isValid() )
    {
      cb->setCheckState( Qt::Checked );
    }
    mObservableTreeWidget->setCurrentItem( observableItem );
  }
  mObservableTreeWidget->expandAll();
}

void QgsSensorInfoDialog::addHiddenObservables( const QString& serviceUrl, const QString stationId, const QStringList& observables, const QList< QDateTime >& begin, const QList< QDateTime >& end )
{
    ObservableEntry entry;
    entry.serviceUrl = serviceUrl;
    entry.stationId = stationId;
    entry.observables = observables;
    entry.beginList = begin;
    entry.endList = end;

    mHiddenObservables.append( entry );
    mShowAllButton->setEnabled( true );
}

void QgsSensorInfoDialog::openObservablesTab()
{
    mTabWidget->setCurrentIndex( 0 );
}

void QgsSensorInfoDialog::showDiagram()
{
  QList<QTreeWidgetItem*> selectedItems = mObservableTreeWidget->selectedItems();
  if ( selectedItems.size() < 1 )
  {
    return;
  }

  QTreeWidgetItem* item  = selectedItems.at( 0 );
  QTreeWidgetItem* parent = item->parent();
  if ( !parent )
  {
    return;
  }

  QwtPlot* targetPlot = 0;
  QStringList currentPlots = plotList();
  if( currentPlots.size() > 0 )
  {
    currentPlots.prepend( tr( "Create new plot" ) );
    QString targetPlotName = QInputDialog::getItem( this, tr( "Plot" ), tr( "Select target plot for data" ), currentPlots );
    if ( targetPlotName != tr( "Create new plot" ) )
    {
        targetPlot = plot( targetPlotName );
    }
  }

  QString featureOfInterest = parent->text( 0 );
  QString serviceUrl = parent->data( 0, Qt::UserRole ).toString();
  QString observedProperty = item->text( 1 );

  bool useTemporalFilter = qobject_cast<QCheckBox*>( mObservableTreeWidget->itemWidget( item, 2 ) )->isChecked();
  QString temporalFilter;

  if ( useTemporalFilter )
  {
    QDateTimeEdit* beginEdit = qobject_cast<QDateTimeEdit*>( mObservableTreeWidget->itemWidget( item, 3 ) );
    QDateTimeEdit* endEdit = qobject_cast<QDateTimeEdit*>( mObservableTreeWidget->itemWidget( item, 4 ) );
    if ( !beginEdit || ! endEdit )
    {
      return;
    }

    temporalFilter = "om:phenomenonTime," + beginEdit->dateTime().toString( Qt::ISODate )
                     + "/" + endEdit->dateTime().toString( Qt::ISODate );
  }

  QUrl url( serviceUrl );
  url.removeQueryItem( "request" );
  url.addQueryItem( "request", "GetObservation" );
  url.addQueryItem( "featureOfInterest", featureOfInterest );
  url.removeQueryItem( "observedProperty" );
  url.addQueryItem( "observedProperty", observedProperty );
  url.addQueryItem( "responseFormat", "http://www.opengis.net/waterml/2.0" );
  if ( useTemporalFilter )
  {
    url.addQueryItem( "temporalFilter", temporalFilter );
  }

  QgsWaterMLData data( url.toString().toLocal8Bit().data() );

  QMap< QDateTime, double > timeValueMap;
  data.getData( &timeValueMap );

  //cancel diagram if result contains no data
  if ( timeValueMap.size() < 1 )
  {
    QMessageBox::information( this, tr( "No data" ), tr( "The GetObservation request returned no observation" ) );
    return;
  }

  QVector<double> timeVector( timeValueMap.size() );
  QVector<double> valueVector( timeValueMap.size() );
  int i = 0;
  QMap< QDateTime, double >::const_iterator it = timeValueMap.constBegin();
  for ( ; it != timeValueMap.constEnd(); ++it )
  {
    timeVector[i] = convertTimeToInt( it.key() );
    valueVector[i] = it.value();
    ++i;
  }

  //create new QWtPlot
  if( !targetPlot )
  {
    targetPlot = new QwtPlot( featureOfInterest, this );
    targetPlot->setAxisScaleDraw( QwtPlot::xBottom, new TimeScaleDraw() );

    //plot picker to show tooltips
    QwtPlotPicker* plotPicker = new QwtPlotPicker( targetPlot->canvas() );
    plotPicker->setSelectionFlags( QwtPicker::PointSelection | QwtPicker::ClickSelection );
    plotPicker->setRubberBand( QwtPlotPicker::RectRubberBand );
    connect( plotPicker, SIGNAL( selected( const QwtDoublePoint& ) ), this, SLOT( onDiagramSelected( const QwtDoublePoint& ) ) );

    int nPlots = plotList().size();
    mTabWidget->addTab( targetPlot, tr( "Plot %1" ).arg( nPlots + 1 ) );
  }

  //random color
  QColor curveColor = QColor::fromHsv( qrand() % 360, 64 + qrand() % 192, 128 + qrand() % 128 );

  QwtPlotCurve* curve = new QwtPlotCurve( featureOfInterest + "_" + observedProperty );
  curve->setPen( QPen( curveColor ) );
  curve->setData( timeVector.constData(), valueVector.constData(), timeValueMap.size() );

  QwtSymbol symbol( QwtSymbol::Rect, QBrush( Qt::NoBrush ), QPen( Qt::red ), QSize( 5, 5 ) );
  curve->setSymbol( symbol );
  curve->attach( targetPlot );



  targetPlot->replot();
  raise();
}

int QgsSensorInfoDialog::convertTimeToInt( const QDateTime& dt ) const
{
  int i = dt.toTime_t();
  if ( i < 0 )
  {
    i = - dt.secsTo( QDateTime::fromTime_t( 0 ) );
  }
  return i;
}

QDateTime QgsSensorInfoDialog::convertIntToTime( int t ) const
{
  if ( t > 0 )
  {
    return QDateTime::fromTime_t( t );
  }
  else
  {
    return QDateTime::fromTime_t( 0 ).addSecs( t );
  }
}

void QgsSensorInfoDialog::onDiagramSelected( const QwtDoublePoint &pt )
{
  QwtPlot* cPlot = currentPlot();
  if( !cPlot )
  {
      return;
  }

  int xPixel = cPlot->transform( QwtPlot::xBottom, pt.x() );
  int yPixel = cPlot->transform( QwtPlot::yLeft , pt.y() );

  double minDist = std::numeric_limits<double>::max();
  QwtPlotCurve* closestCurve = 0;
  int snapPoint = -1;

  QwtPlotItemList items = cPlot->itemList();
  for( int i = 0; i < items.size(); ++i )
  {
      QwtPlotCurve* curve = dynamic_cast<QwtPlotCurve*>( cPlot->itemList()[i] );
      if( !curve )
      {
          continue;
      }

      double currentDist;
      int currentSnapPoint = curve->closestPoint( QPoint( xPixel, yPixel ), &currentDist );
      if( currentDist < minDist && currentSnapPoint >= 0 )
      {
        minDist = currentDist;
        snapPoint = currentSnapPoint;
        closestCurve = curve;
      }
  }

  if( !closestCurve )
  {
      return;
  }

  QString snappedTime = convertIntToTime( closestCurve->data().x( snapPoint ) ).toString();
  double snappedValue = closestCurve->data().y( snapPoint );

  QwtPlotMarker* mark = plotMarker( cPlot );
  mark->setValue( closestCurve->data().x( snapPoint ), closestCurve->data().y( snapPoint ) );
  mark->show();

  QwtText  labelText;
  labelText.setText( "Value: " + QString::number( snappedValue ) + "\n" + snappedTime );
  labelText.setColor( QColor( Qt::black ) );
  QBrush labelBrush( QColor( Qt::white ), Qt::SolidPattern );
  labelText.setBackgroundBrush( labelBrush );
  mark->setLabel( labelText );
  mark->setLabelAlignment( Qt::AlignRight | Qt::AlignTop );
  cPlot->replot();
}

QStringList QgsSensorInfoDialog::plotList() const
{
    QStringList list;
    int nTabs = mTabWidget->count();
    for( int i = 0; i < nTabs; ++i )
    {
        QwtPlot* plot = qobject_cast<QwtPlot*>( mTabWidget->widget( i ) );
        if( plot )
        {
            list.append( mTabWidget->tabText( i ) );
        }
    }
    return list;
}

QwtPlot* QgsSensorInfoDialog::plot( const QString& name )
{
    int nTabs = mTabWidget->count();
    for( int i = 0; i < nTabs; ++i )
    {
        if( mTabWidget->tabText( i ) == name )
        {
            return qobject_cast<QwtPlot*>( mTabWidget->widget( i ) );
        }
    }

    return 0;
}

QwtPlot* QgsSensorInfoDialog::currentPlot()
{
  return qobject_cast<QwtPlot*>( mTabWidget->currentWidget() );
}

QwtPlotMarker* QgsSensorInfoDialog::plotMarker( QwtPlot* plot )
{
  if ( !plot )
  {
    return 0;
  }

  QMap< QwtPlot*, QwtPlotMarker* >::const_iterator plotIt = mPlotMarkers.find( plot );
  if ( plotIt != mPlotMarkers.constEnd() )
  {
    return plotIt.value();
  }

  QwtPlotMarker* marker = new QwtPlotMarker();
  marker->setXAxis( QwtPlot::xBottom );
  marker->setYAxis( QwtPlot::yLeft );
  marker->setSymbol( QwtSymbol( QwtSymbol::Rect, QBrush( Qt::blue ), QPen( Qt::green ), QSize( 5, 5 ) ) );
  marker->attach( plot );
  mPlotMarkers.insert( plot, marker );
  return marker;
}

void QgsSensorInfoDialog::on_mTabWidget_tabCloseRequested( int index )
{
  if ( index < 1 )
  {
    return;
  }
  mTabWidget->removeTab( index );
}

void QgsSensorInfoDialog::showAllObservables()
{
    QList< ObservableEntry >::const_iterator it = mHiddenObservables.constBegin();
    for(; it != mHiddenObservables.constEnd(); ++it )
    {
        addObservables( it->serviceUrl, it->stationId, it->observables, it->beginList, it->endList );
    }
    mHiddenObservables.clear();
    mShowAllButton->setEnabled( false );
}

void QgsSensorInfoDialog::exportToCSV()
{
    QSettings s;
    QString saveDir = QFileDialog::getExistingDirectory( this, tr( "Select output directory" ), s.value( "/NIWA/sos/exportCsvDir" ).toString() );
    if( saveDir.isEmpty() )
    {
        return;
    }

    //current tab
    QwtPlot* plot = currentPlot();
    if( !plot )
    {
        return;
    }

    QwtPlotItemList items = plot->itemList();
    if( items.size() < 1 )
    {
        return;
    }

    for( int i = 0; i < items.size(); ++i )
    {
        QwtPlotCurve* curve = dynamic_cast<QwtPlotCurve*>( items.at( i ) );
        if( !curve )
        {
            continue;
        }

        QFile outFile( saveDir + "/" + curve->title().text() + ".csv" );
        if( !outFile.open( QIODevice::WriteOnly ) )
        {
            QgsMessageLog::logMessage( QString( "Creating output file %1 failed" ).arg( saveDir + "/" + curve->title().text() + ".csv" ), "SOS Plugin", QgsMessageLog::CRITICAL );
            QgsMessageLog::logMessage( outFile.errorString(), "SOS Plugin", QgsMessageLog::CRITICAL );
            return;
        }

        QTextStream outStream( &outFile );
        outStream << "time,value" << "\n";

        const QwtData& curveData = curve->data();
        int curveDataSize = curveData.size();
        for( int i = 0; i < curveDataSize; ++i )
        {
            QDateTime t = convertIntToTime( curveData.x( i ) );
            outStream << t.toString();
            outStream << ",";
            double value = curveData.y( i );
            outStream << QString::number( value );
            outStream << "\n";
        }
    }

    //
    s.setValue( "/NIWA/sos/exportCsvDir", saveDir );
}
