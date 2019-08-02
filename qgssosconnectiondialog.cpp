#include "qgssosconnectiondialog.h"

QgsSOSConnectionDialog::QgsSOSConnectionDialog( QString name, QString url, QWidget* parent, Qt::WindowFlags fl ): QDialog( parent, fl )
{
    setupUi( this );
    mNameLineEdit->setText( name );
    mUrlLineEdit->setText( url );
}

QgsSOSConnectionDialog::QgsSOSConnectionDialog( QWidget* parent, Qt::WindowFlags fl ): QDialog( parent, fl )
{
    setupUi( this );
}

QgsSOSConnectionDialog::~QgsSOSConnectionDialog()
{
}

QString QgsSOSConnectionDialog::url() const
{
    return mUrlLineEdit->text();
}

QString QgsSOSConnectionDialog::name() const
{
    return mNameLineEdit->text();
}
