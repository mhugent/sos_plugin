/***************************************************************************
                          qgssossourceselect.cpp  -  description
                          ------------------------------------
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

#include "qgssossourceselect.h"
#include "qgssosconnectiondialog.h"
#include "qgsowsconnection.h"
#include "qgssoscapabilities.h"
#include "qgisinterface.h"
#include <QMessageBox>
#include <QUrl>

QgsSOSSourceSelect::QgsSOSSourceSelect( QgisInterface* iface, QWidget* parent, Qt::WindowFlags fl ):
    QDialog( parent, fl ), mCapabilities( 0 ), mIface( iface )
{
  setupUi( this );
  populateConnectionList();

  QPushButton* mAddButton = new QPushButton( tr( "&Add" ) );
  mButtonBox->addButton( mAddButton, QDialogButtonBox::ActionRole );
  connect( mAddButton, SIGNAL( clicked() ), this, SLOT( addLayer() ) );

  mFilterModel.setSourceModel( &mOfferingsModel );
  mOfferingsView->setModel( &mFilterModel );
}

QgsSOSSourceSelect::~QgsSOSSourceSelect()
{
  delete mCapabilities;
}

void QgsSOSSourceSelect::on_mConnectButton_clicked()
{
  QgsOwsConnection connection( "SOS", mConnectionsComboBox->currentText() );
  delete mCapabilities;
  mCapabilities = new QgsSOSCapabilities( connection.uri().encodedUri() );
  connect( mCapabilities, SIGNAL( gotCapabilities() ), this, SLOT( gotCapabilities() ) );
  mCapabilities->requestCapabilities();
}

void QgsSOSSourceSelect::on_mNewButton_clicked()
{
    showConnectionDialog();
}

void QgsSOSSourceSelect::on_mEditButton_clicked()
{
    QSettings s;
    QString connectionName = mConnectionsComboBox->currentText();
    QString url = s.value( "qgis/connections-sos/" + connectionName + "/url" ).toString();
    showConnectionDialog( connectionName, url );
}

void QgsSOSSourceSelect::showConnectionDialog( const QString& connectionName, const QString& connectionUrl )
{
    QgsSOSConnectionDialog d( connectionName, connectionUrl, this );
    if( d.exec() == QDialog::Accepted )
    {
        QSettings s;
        s.setValue( QString( "qgis/connections-sos/" ) + d.name() + "/url", d.url() );
        populateConnectionList();
    }
}

void QgsSOSSourceSelect::on_mDeleteButton_clicked()
{
  QString msg = tr( "Are you sure you want to remove the %1 connection and all associated settings?" )
                .arg( mConnectionsComboBox->currentText() );
  QMessageBox::StandardButton result = QMessageBox::information( this, tr( "Confirm Delete" ), msg, QMessageBox::Ok | QMessageBox::Cancel );
  if ( result == QMessageBox::Ok )
  {
    QgsOwsConnection::deleteConnection( "SOS", mConnectionsComboBox->currentText() );
    mConnectionsComboBox->removeItem( mConnectionsComboBox->currentIndex() );
  }
}

void QgsSOSSourceSelect::on_mFilterLineEdit_textChanged( const QString& text )
{
    mFilterModel.setFilterWildcard( text );
}

void QgsSOSSourceSelect::populateConnectionList()
{
  mConnectionsComboBox->clear();

  QStringList keys = QgsOwsConnection::connectionList( "SOS" );
  QStringList::const_iterator it = keys.constBegin();
  for ( ; it != keys.constEnd(); ++it )
  {
    mConnectionsComboBox->addItem( *it );
  }

  bool buttonsEnabled = ( keys.constBegin() != keys.constEnd() );
  mConnectButton->setEnabled( buttonsEnabled );
  mEditButton->setEnabled( buttonsEnabled );
  mDeleteButton->setEnabled( buttonsEnabled );

  //set last used connection
  QString selectedConnection = QgsOwsConnection::selectedConnection( "SOS" );
  int index = mConnectionsComboBox->findText( selectedConnection );
  if ( index != -1 )
  {
    mConnectionsComboBox->setCurrentIndex( index );
  }
}

void QgsSOSSourceSelect::gotCapabilities()
{
  mOfferingsModel.clear();
  if ( !mCapabilities )
  {
    return;
  }

  //check network error
  const QString& networkError = mCapabilities->networkError();
  if ( !networkError.isEmpty() )
  {
    QMessageBox::critical( this, tr( "Network error" ), networkError );
  }

  //check xml error
  const QString& xmlError = mCapabilities->xmlError();
  if ( !xmlError.isEmpty() )
  {

  }

  const QStringList* observablePropertyList = mCapabilities->observableProperties();
  if ( observablePropertyList )
  {
    QStringList::const_iterator pIt = observablePropertyList->constBegin();
    for ( ; pIt != observablePropertyList->constEnd(); ++pIt )
    {
        mOfferingsModel.appendRow( new QStandardItem( *pIt ) );
    }
  }
}

void QgsSOSSourceSelect::addLayer()
{
  QgsOwsConnection connection( "SOS", mConnectionsComboBox->currentText() );
  QString urlString = connection.uri().param( "url" );
  if( !urlString.endsWith( "?" ) && !urlString.endsWith( "&" ) )
  {
      urlString.append( urlString.contains( "?" ) ? "&" : "?" );
  }
  QString getFeatureOfInterestUrl = urlString + "SERVICE=SOS&request=GetFeatureOfInterest&Version=2.0.0";

  QString observedPropertiesString;
  QModelIndexList selectedIndexList = mOfferingsView->selectionModel()->selectedRows();
  for ( int i = 0; i < selectedIndexList.size(); ++i )
  {
    if ( i > 0 )
    {
      observedPropertiesString.append( "," );
    }

    QStandardItem* item = mOfferingsModel.itemFromIndex( mFilterModel.mapToSource( selectedIndexList.at( i ) ) );
    if( item )
    {
        observedPropertiesString.append( item->text() );
    }
  }

  if ( !selectedIndexList.isEmpty() )
  {
    getFeatureOfInterestUrl += ( QString( "&observedProperty=" ) + QString( QUrl::toPercentEncoding( observedPropertiesString.toLocal8Bit() ) ) );
  }

  QString sosLayerName = observedPropertiesString.split( "/" ).last();

  if ( mIface )
  {
    mIface->addVectorLayer( getFeatureOfInterestUrl, sosLayerName, "SOS" );
  }
  return;
}

