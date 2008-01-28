
#include <QIcon>
#include <QXmlStreamReader>
#include <QtDebug>
#include <QFileInfo>
#include <QMessageBox>

#include "QXmlPlaylistHandler.hpp"
#include "QPlaylistModel.hpp"

//#include "libprojectM/projectM.hpp"

class XmlReadFunctor {
	public:
		XmlReadFunctor(QPlaylistModel & model) : m_model(model) {}

		inline void setPlaylistName(const QString & name) {
			m_model.setPlaylistName(name);
		}

		inline void setPlaylistDesc(const QString & desc) {
			m_model.setPlaylistDesc(desc);
		}

		inline void appendItem(const QString & url, const QString & name, int rating) {
			m_model.appendRow(url, name, rating);
		}
	private:
		QPlaylistModel & m_model;

};


class XmlWriteFunctor {
	public:
		XmlWriteFunctor(QPlaylistModel & model) : m_model(model), m_index(0) {}

		inline const QString & playlistName() const {
			return m_model.playlistName();
		}

		inline const QString & playlistDesc() const {
			return m_model.playlistDesc();
		}

		inline bool nextItem(QString & url, int & rating) {

			if (m_index >= m_model.rowCount())
				return false;

			QModelIndex modelIndex = m_model.index(m_index, 0);
			url = m_model.data(modelIndex, QPlaylistModel::URLInfoRole).toString();
			rating = m_model.data(modelIndex, QPlaylistModel::RatingRole).toInt();
			m_index++;
			return true;
		}
	private:
		QPlaylistModel & m_model;
		int m_index;

};

QPlaylistModel::QPlaylistModel ( projectM & _projectM, QObject * parent ) :
		QAbstractTableModel ( parent ), m_projectM ( _projectM )
{
	m_ratings = QVector<int> ( rowCount(), 3 );
}


void QPlaylistModel::updateItemHighlights()
{
	if ( rowCount() == 0 )
		return;

	emit ( dataChanged ( this->index ( 0,0 ), this->index ( rowCount()-1,columnCount()-1 ) ) )
	;
}

bool QPlaylistModel::setData ( const QModelIndex & index, const QVariant & value, int role )
{
	if ( role == QPlaylistModel::RatingRole )
	{
		//QAbstractTableModel::setData(index, ratingToIcon(value.toInt()), Qt::DecorationRole);
		//std::cerr << "here" << std::endl;
		m_ratings[index.row() ] = value.toInt();
		emit ( dataChanged ( index, index ) );
		return true;
	}
	else
		return QAbstractTableModel::setData ( index, value, role );

}

QVariant QPlaylistModel::ratingToIcon ( int rating )  const
{
	switch ( rating )
	{
		case 0:
			return QVariant ( QIcon ( ":/images/icons/face0.png" ) );
		case 1:
			return QVariant ( QIcon ( ":/images/icons/face1.png" ) );
		case 2:
			return QVariant ( QIcon ( ":/images/icons/face2.png" ) );
		case 3:
			return QVariant ( QIcon ( ":/images/icons/face3.png" ) );
		case 4:
			return QVariant ( QIcon ( ":/images/icons/face4.png" ) );
		case 5:
			return QVariant ( QIcon ( ":/images/icons/face5.png" ) );
		default:
			return QVariant ( QIcon ( ":/images/icons/face0.png" ) );
	}
}

QVariant QPlaylistModel::data ( const QModelIndex & index, int role = Qt::DisplayRole ) const
{

	switch ( role )
	{
		case Qt::DisplayRole:
			if ( index.column() == 0 )
				return QVariant ( QString ( m_projectM.getPresetName ( index.row() ).c_str() ) );
			else
				return ratingToIcon ( m_ratings[index.row() ] );
		case Qt::ToolTip:
			if ( index.column() == 0 )
				return QVariant ( QString ( m_projectM.getPresetName ( index.row() ).c_str() ) );
			else
				return QString ( "Current rating is %1 / 5" ).arg ( m_ratings[index.row() ] );
		case Qt::DecorationRole:
			if ( index.column() == 1 )
				return ratingToIcon ( m_ratings[index.row() ] );
			else
				return QVariant();
		case QPlaylistModel::RatingRole:
			return QVariant ( m_ratings[index.row() ] );

		case Qt::BackgroundRole:

			if ( m_projectM.isPresetLocked() && ( index.row() == m_projectM.selectedPresetIndex() ) )
				return Qt::red;
			if ( !m_projectM.isPresetLocked() && ( index.row() == m_projectM.selectedPresetIndex() ) )
				return Qt::green;

			return Qt::white;

		case QPlaylistModel::URLInfoRole:
			return QVariant ( QString ( m_projectM.getPresetURL ( index.row() ).c_str() ) );
		default:
			return QVariant();
	}
}

QVariant QPlaylistModel::headerData ( int section, Qt::Orientation orientation, int role ) const
{

	if ( orientation == Qt::Vertical )
		return QAbstractTableModel::headerData ( section, orientation, role );

	if ( ( section == 0 ) && ( role == Qt::SizeHintRole ) )
		return QVariant ( 500 );
//	 if ((section == 1) && (role == Qt::SizeHintRole))
//		return QVariant(60);
	if ( ( section == 0 ) && ( role == Qt::DisplayRole ) )
		return QString ( tr ( "Preset" ) );
	if ( ( section == 1 ) && ( role == Qt::DisplayRole ) )
		return QString ( tr ( "Rating" ) );

	return QAbstractTableModel::headerData ( section, orientation, role );
}

int QPlaylistModel::rowCount ( const QModelIndex & parent ) const
{
	return m_projectM.getPlaylistSize();
}


int QPlaylistModel::columnCount ( const QModelIndex & parent ) const
{

	// eventually add ratings here so size should be 2
	if ( rowCount() > 0 )
		return 2;
	else
		return 0;
}

void QPlaylistModel::appendRow ( const QString & presetURL, const QString & presetName, int rating )
{
	beginInsertRows ( QModelIndex(), rowCount(), rowCount() );
	m_projectM.addPresetURL ( presetURL.toStdString(), presetName.toStdString() );
	m_ratings.push_back ( rating );
	endInsertRows();
}


void QPlaylistModel::removeRow ( int index )
{
	beginRemoveRows ( QModelIndex(), index, index );
	m_projectM.removePreset ( index );
	m_ratings.remove ( index );
	endRemoveRows();
}

void QPlaylistModel::clear()
{
	clearItems();
	m_playlistName = "";
	m_playlistDesc = "";
}

void QPlaylistModel::clearItems()
{
	beginRemoveRows ( QModelIndex(), 0, rowCount()-1 );
	m_projectM.clearPlaylist();
	m_ratings.clear();	
	endRemoveRows();
}


bool QPlaylistModel::writePlaylist ( const QString & file ) {

	QFile qfile(file);

 	if (!qfile.open(QIODevice::WriteOnly)) {
		QMessageBox::warning (0, "Playlist Save Error", QString("There was a problem trying to save the playlist \"%1\".  You may not have permission to modify this file.").arg(file));				
		return false;
	}


	XmlWriteFunctor writeFunctor(*this);

	QXmlPlaylistHandler::writePlaylist(&qfile, writeFunctor);
	return true;
}

bool QPlaylistModel::readPlaylist ( const QString & file )
{
	
        QFile qfile(file);
 	if (!qfile.open(QIODevice::ReadOnly)) {
		QMessageBox::warning (0, "Playlist File Error", QString("There was a problem trying to open the playlist \"%1\".  You may not have permission to read the file.").arg(file));				
		return false;
	}

	XmlReadFunctor readFunctor(*this);


	if (QXmlPlaylistHandler::readPlaylist(&qfile, readFunctor) != QXmlStreamReader::NoError) {
		QMessageBox::warning ( 0, "Playlist Parse Error", QString(tr("There was a problem trying to parse the playlist \"%1\". Some of your playlist items may have loaded correctly, but don't expect miracles.")).arg(file));		
	}

	return true;
}
