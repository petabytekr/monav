/*
Copyright 2010  Christian Vetter veaac.fdirct@gmail.com

This file is part of MoNav.

MoNav is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

MoNav is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with MoNav.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "oisettingsdialog.h"
#include "ui_oisettingsdialog.h"
#include <QFileDialog>
#include <QTextStream>
#include <QSettings>
#include <QtDebug>
#include "utils/qthelpers.h"

OISettingsDialog::OISettingsDialog(QWidget *parent) :
    QDialog(parent),
	 m_ui(new Ui::OISettingsDialog)
{
	m_ui->setupUi(this);
	m_ui->speedTable->resizeColumnsToContents();

	connectSlots();

	QSettings settings( "MoNav" );
	settings.beginGroup( "OSM Importer" );
	m_ui->inputEdit->setText( settings.value( "inputFile" ).toString() );
	m_ui->trafficLightPenalty->setValue( settings.value( "trafficLightPenalty", 1 ).toInt() );
	m_ui->setDefaultCitySpeed->setChecked( settings.value( "defaultCitySpeed", true ).toBool() );
	m_ui->ignoreOneway->setChecked( settings.value( "ignoreOneway", false ).toBool() );
	m_ui->ignoreMaxspeed->setChecked( settings.value( "ignoreMaxspeed", false ).toBool() );
	m_lastFilename = settings.value( "speedProfile" ).toString();
	QString accessType = settings.value( "accessType", "motorcar" ).toString();

	QList< QTreeWidgetItem* > items = m_ui->accessTree->findItems( accessType, Qt::MatchFixedString | Qt::MatchRecursive );
	if ( items.size() > 0 )
		items.first()->setSelected( true );
	m_ui->accessTree->expandAll();

	if ( QFile::exists( m_lastFilename ) )
		load( m_lastFilename );
	else
		setDefaultSpeed();
}

void OISettingsDialog::connectSlots()
{
	connect( m_ui->browseButton, SIGNAL(clicked()), this, SLOT(browse()) );
	connect( m_ui->addRowButton, SIGNAL(clicked()), this, SLOT(addSpeed()) );
	connect( m_ui->deleteEntryButton, SIGNAL(clicked()), this, SLOT(removeSpeed()) );
	connect( m_ui->defaultButton, SIGNAL(clicked()), this, SLOT(setDefaultSpeed()) );
	connect( m_ui->saveButton, SIGNAL(clicked()), this, SLOT(saveSpeed()) );
	connect( m_ui->loadButton, SIGNAL(clicked()), this, SLOT(loadSpeed()) );
}

OISettingsDialog::~OISettingsDialog()
{
	QSettings settings( "MoNav" );
	settings.beginGroup( "OSM Importer" );
	settings.setValue( "inputFile", m_ui->inputEdit->text()  );
	settings.setValue( "trafficLightPenalty", m_ui->trafficLightPenalty->value() );
	settings.setValue( "defaultCitySpeed", m_ui->setDefaultCitySpeed->isChecked() );
	settings.setValue( "ignoreOneway", m_ui->ignoreOneway->isChecked() );
	settings.setValue( "ignoreMaxspeed", m_ui->ignoreMaxspeed->isChecked() );
	settings.setValue( "speedProfile", m_lastFilename );
	QList< QTreeWidgetItem* > items = m_ui->accessTree->selectedItems();
	if ( items.size() == 1 )
		settings.setValue( "accessType", items.first()->text( 0 ) );
	delete m_ui;
}

void OISettingsDialog::changeEvent(QEvent *e)
{
    QDialog::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
		  m_ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

void OISettingsDialog::setDefaultSpeed() {
	load( ":/speedprofiles/default.spp" );
}

void OISettingsDialog::addSpeed() {
	int rows = m_ui->speedTable->rowCount();
	m_ui->speedTable->setRowCount( rows + 1 );
}

void OISettingsDialog::removeSpeed() {
	if ( m_ui->speedTable->rowCount() == 0 )
		return;

	int row = m_ui->speedTable->currentRow();
	m_ui->speedTable->removeRow( row );
}

void OISettingsDialog::save( const QString& filename )
{
	QFile file( filename );
	if ( !openQFile( &file, QIODevice::WriteOnly ) )
		return;

	QTextStream out( &file );
	int rowCount = m_ui->speedTable->rowCount();
	int colCount = m_ui->speedTable->columnCount();
	for ( int row = 0; row < rowCount; ++row ) {
		bool valid = true;
		for ( int i = 0; i < colCount; i++ )
		{
			if ( m_ui->speedTable->item( row, i ) == NULL )
				valid = false;
		}
		if ( !valid )
			continue;
		for ( int i = 0; i < colCount - 1; i++ )
			out << m_ui->speedTable->item( row, i )->text() << "\t";

		out << m_ui->speedTable->item( row, colCount - 1 )->text() << "\n";
	}
	m_lastFilename = filename;
}

void OISettingsDialog::load( const QString& filename )
{
	QFile file( filename );
	if ( !openQFile( &file, QIODevice::ReadOnly ) )
		return;

	QTextStream in( &file );
	int rowCount = 0;
	int colCount = m_ui->speedTable->columnCount();
	while ( !in.atEnd() ) {
		rowCount++;
		m_ui->speedTable->setRowCount( rowCount );
		QString text = in.readLine();
		QStringList entries = text.split( '\t' );
		if ( entries.size() != colCount )
			continue;
		for ( int i = 0; i < colCount; i++ )
		{
			if ( m_ui->speedTable->item( rowCount - 1, i ) == NULL )
				m_ui->speedTable->setItem( rowCount - 1, i, new QTableWidgetItem );
		}
		for ( int i = 0; i < colCount; i++ )
		{
			m_ui->speedTable->item( rowCount - 1, i )->setText( entries[i] );
		}
	}
	qDebug() << "OSM Importer: read speed profile from:" << filename;
	m_lastFilename = filename;
}

void OISettingsDialog::saveSpeed()
{
	QString filename = QFileDialog::getSaveFileName( this, tr( "Enter Speed Filename" ), "", "*.spp" );
	save( filename );
}

void OISettingsDialog::loadSpeed()
{
	QString filename = QFileDialog::getOpenFileName( this, tr( "Enter Speed Filename" ), "", "*.spp" );
	load( filename );
}

void OISettingsDialog::browse() {
	QString file = m_ui->inputEdit->text();
	file = QFileDialog::getOpenFileName( this, tr("Enter OSM XML Filename"), file, "*.osm *osm.bz2" );
	if ( file != "" )
		m_ui->inputEdit->setText( file );
}

bool OISettingsDialog::getSettings( Settings* settings )
{
	if ( settings == NULL )
		return false;
	settings->accessList.clear();
	settings->defaultCitySpeed = m_ui->setDefaultCitySpeed->isChecked();
	settings->trafficLightPenalty = m_ui->trafficLightPenalty->value();
	settings->input = m_ui->inputEdit->text();
	settings->ignoreOneway = m_ui->ignoreOneway->isChecked();
	settings->ignoreMaxspeed = m_ui->ignoreMaxspeed->isChecked();

	int rowCount = m_ui->speedTable->rowCount();
	int colCount = m_ui->speedTable->columnCount();

	if ( colCount != 4 )
		return false;

	settings->speedProfile.names.clear();
	settings->speedProfile.speed.clear();
	settings->speedProfile.speedInCity.clear();
	settings->speedProfile.averagePercentage.clear();

	for ( int row = 0; row < rowCount; ++row ) {
		for ( int i = 0; i < colCount; i++ )
		{
			if ( m_ui->speedTable->item( row, i ) == NULL ) {
				qCritical() << tr( "Missing entry in speed profile table" );
				return false;
			}
		}
		settings->speedProfile.names.push_back( m_ui->speedTable->item( row, 0 )->text() );
		settings->speedProfile.speed.push_back( m_ui->speedTable->item( row, 1 )->text().toInt() );
		settings->speedProfile.speedInCity.push_back( m_ui->speedTable->item( row, 2 )->text().toInt() );
		settings->speedProfile.averagePercentage.push_back( m_ui->speedTable->item( row, 3 )->text().toInt() );
	}

	QList< QTreeWidgetItem* > items = m_ui->accessTree->selectedItems();
	if ( items.size() != 1 ) {
		qCritical() << tr( "No Access Type selected" );
		return false;
	}
	QTreeWidgetItem* item = items.first();
	do {
		qDebug() << "OSM Importer: access list:" << settings->accessList.size() << ":" << item->text( 0 );
		settings->accessList.push_back( item->text( 0 ) );
		item = item->parent();
	} while ( item != NULL );

	return true;
}
