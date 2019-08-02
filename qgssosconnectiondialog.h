#ifndef QGSSOSCONNECTIONDIALOG_H
#define QGSSOSCONNECTIONDIALOG_H

#include "ui_qgssosconnectiondialogbase.h"

class QgsSOSConnectionDialog: public QDialog, private Ui::QgsSOSConnectionDialogBase
{
    public:
        QgsSOSConnectionDialog( QString name, QString url, QWidget* parent = 0, Qt::WindowFlags fl = 0 );
        QgsSOSConnectionDialog( QWidget* parent = 0, Qt::WindowFlags fl = 0 );
        ~QgsSOSConnectionDialog();

        QString url() const;
        QString name() const;
};

#endif // QGSSOSCONNECTIONDIALOG_H
