/***************************************************************************
                          qgssensorinfodialog.h  -  description
                          -------------------------------------
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

#ifndef QGSSENSORINFODIALOG_H
#define QGSSENSORINFODIALOG_H

#include "ui_qgssensorinfodialogbase.h"
#include "qwt_compat.h"
#include <QDateTime>

class QwtPlot;
class QwtPlotMarker;

class QgsSensorInfoDialog: public QDialog, private Ui::QgsSensorInfoDialogBase
{
    Q_OBJECT
  public:

    struct ObservableEntry
    {
        QString serviceUrl;
        QString stationId;
        QString stationName;
        QStringList observables;
        QList< QDateTime > beginList;
        QList< QDateTime > endList;
    };

    QgsSensorInfoDialog( QWidget * parent = 0, Qt::WindowFlags f = 0 );
    ~QgsSensorInfoDialog();

    void clearObservables();
    void addObservables( const QString& serviceUrl, const QString& stationId, const QString& stationName, const QStringList& observables, const QList< QDateTime >& begin, const QList< QDateTime >& end );
    /**Adds observables which are hidden at the moment but may be shown later (after mShowAllObservableButton has been clicked)*/
    void addHiddenObservables( const QString& serviceUrl, const QString& stationId, const QString& stationName, const QStringList& observables, const QList< QDateTime >& begin, const QList< QDateTime >& end );

    void openObservablesTab();

  private slots:
    void showDiagram();
    int convertTimeToInt( const QDateTime& dt ) const;
    QDateTime convertIntToTime( int t ) const;
    void onDiagramSelected( const QwtDoublePoint &pt );
    void on_mTabWidget_tabCloseRequested( int index );
    void showAllObservables();
    void exportToCSV();

  private:
    QStringList plotList() const;
    QwtPlot* plot( const QString& name );
    QwtPlot* currentPlot();
    QwtPlotMarker* plotMarker( QwtPlot* plot );
    /**Removes characters which might be roblematic e.g. on windows*/
    static QString removeProblematicFileCharacters( const QString& fileName );

    QMap< QwtPlot*, QwtPlotMarker* > mPlotMarkers;

    QList< ObservableEntry > mHiddenObservables;

    QPushButton* mShowAllButton;
};

#endif // QGSSENSORINFODIALOG_H
