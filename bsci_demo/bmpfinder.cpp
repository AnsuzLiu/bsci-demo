#include "bmpfinder.h"

BmpFinder::BmpFinder( const QString &path, int intervalMs, QObject * parent )
    : QObject( parent ), m_dirPath( path )
{

    m_ScanTimer = new QTimer( this );

    connect( m_ScanTimer, &QTimer::timeout, this, &BmpFinder::Slot_Scan_Update );

    m_ScanTimer->start( intervalMs );

    printf( "[QCAP DEBUG] Scan bmp file folder: %s\n", m_dirPath.toUtf8().data() );

    Func_Scan_InitFullScan();

}

void BmpFinder::Func_Scan_InitFullScan()
{

    QDir dir( m_dirPath, "*.bmp", QDir::Name, QDir::Files );

    if( dir.exists() == FALSE ) {

        printf( "[QCAP DEBUG] Outputfolder does not exist: %s\n", m_dirPath.toUtf8().data() );

        return;

    }

    QStringList allFiles = dir.entryList();

    m_knownFileSet = QSet< QString >( allFiles.begin(), allFiles.end() );

    if( allFiles.isEmpty() == FALSE ) {

        QString lastFile = allFiles.last();

        QString fullPath = dir.absoluteFilePath( lastFile );

        emit Signal_Bmp_LatestFound( fullPath );

    }

}

void BmpFinder::Slot_Scan_Update()
{

    QtConcurrent::run( [ this ]() {

        QDir dir( m_dirPath, "*.bmp", QDir::Name, QDir::Files );

        if( dir.exists() == FALSE ) return;

        QStringList currentFiles = dir.entryList();

        if( currentFiles.isEmpty() == TRUE ) {

            QMetaObject::invokeMethod( this, [ this ]() {

                m_knownFileSet.clear();

            }, Qt::QueuedConnection );

            return;

        }

        QSet< QString > currentSet( currentFiles.begin(), currentFiles.end() );

        QStringList newFiles;

        QStringList deletedFiles;

        ////// Find new files

        for( const QString &file : currentFiles ) {

            if( m_knownFileSet.contains( file ) == FALSE ) newFiles.append( file );

        }


        ////// Find deleted files

        for( const QString &oldFile : m_knownFileSet ) {

            if( currentSet.contains( oldFile ) == FALSE ) deletedFiles.append( oldFile );

        }

        QString lastNewFile;

        if( newFiles.isEmpty() == FALSE ) lastNewFile = dir.absoluteFilePath( newFiles.last() );

        QMetaObject::invokeMethod( this, [ this, currentSet, lastNewFile, deletedFiles ]() {

            m_knownFileSet = currentSet;

//            for( const QString &f : deletedFiles )
//                emit Signal_Bmp_Removed( f );

            if( lastNewFile.isEmpty() == FALSE ) emit Signal_Bmp_LatestFound( lastNewFile );

        }, Qt::QueuedConnection );

    } );

}
