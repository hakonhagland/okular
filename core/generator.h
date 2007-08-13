/***************************************************************************
 *   Copyright (C) 2004-5 by Enrico Ros <eros.kde@email.it>                *
 *   Copyright (C) 2005   by Piotr Szymanski <niedakh@gmail.com>           *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _OKULAR_GENERATOR_H_
#define _OKULAR_GENERATOR_H_

#include <okular/core/okular_export.h>
#include <okular/core/fontinfo.h>
#include <okular/core/global.h>
#include <okular/core/pagesize.h>

#include <QtCore/QList>
#include <QtCore/QObject>
#include <QtCore/QSharedDataPointer>
#include <QtCore/QString>
#include <QtCore/QVariant>
#include <QtCore/QVector>

#include <kmimetype.h>

// KDE_EXPORT is correct here - the function needs to be exported every time
#define OKULAR_EXPORT_PLUGIN( classname ) \
    extern "C" { \
         KDE_EXPORT Okular::Generator* create_plugin() { return new classname(); } \
    }

class KAboutData;
class KComponentData;
class KIcon;
class KPrinter;

namespace Okular {

class Document;
class DocumentFonts;
class DocumentInfo;
class DocumentSynopsis;
class EmbeddedFile;
class ExportFormatPrivate;
class GeneratorPrivate;
class Page;
class PixmapRequest;
class TextPage;

/* Note: on contents generation and asynchronous queries.
 * Many observers may want to request data syncronously or asynchronously.
 * - Sync requests. These should be done in-place.
 * - Async request must be done in real background. That usually means a
 *   thread, such as QThread derived classes.
 * Once contents are available, they must be immediately stored in the
 * Page they refer to, and a signal is emitted as soon as storing
 * (even for sync or async queries) has been done.
 */

/**
 * @short Defines an entry for the export menu
 *
 * This class encapsulates information about an export format.
 * Every Generator can support 0 or more export formats which can be
 * queried with @ref Generator::exportFormats().
 */
class OKULAR_EXPORT ExportFormat
{
    public:
        typedef QList<ExportFormat> List;

        /**
         * Creates an empty export format.
         *
         * @see isNull()
         */
        ExportFormat();

        /**
         * Creates a new export format.
         *
         * @param description The i18n'ed description of the format.
         * @param mimeType The supported mime type of the format.
         */
        ExportFormat( const QString &description, const KMimeType::Ptr &mimeType );

        /**
         * Creates a new export format.
         *
         * @param icon The icon used in the GUI for this format.
         * @param description The i18n'ed description of the format.
         * @param mimeType The supported mime type of the format.
         */
        ExportFormat( const KIcon &icon, const QString &description, const KMimeType::Ptr &mimeType );

        /**
         * Destroys the export format.
         */
        ~ExportFormat();

        /**
         * @internal
         */
        ExportFormat( const ExportFormat &other );

        /**
         * @internal
         */
        ExportFormat& operator=( const ExportFormat &other );

        /**
         * Returns the description of the format.
         */
        QString description() const;

        /**
         * Returns the mime type of the format.
         */
        KMimeType::Ptr mimeType() const;

        /**
         * Returns the icon for GUI representations of the format.
         */
        KIcon icon() const;

        /**
         * Returns whether the export format is null/valid.
         *
         * An ExportFormat is null if the mimetype is not valid or the
         * description is empty, or both.
         */
        bool isNull() const;

        /**
         * Type of standard export format.
         */
        enum StandardExportFormat
        {
            PlainText,         ///< Plain text
            PDF                ///< PDF, aka Portable Document Format
        };

        /**
         * Builds a standard format for the specified @p type .
         */
        static ExportFormat standardFormat( StandardExportFormat type );

        bool operator==( const ExportFormat &other ) const;

        bool operator!=( const ExportFormat &other ) const;

    private:
        friend class ExportFormatPrivate;
        QSharedDataPointer<ExportFormatPrivate> d;
};

/**
 * @short [Abstract Class] The information generator.
 *
 * Most of class members are virtuals and some of them pure virtual. The pure
 * virtuals provide the minimal functionalities for a Generator, that is being
 * able to generate QPixmap for the Page 's of the Document.
 *
 * Implementing the other functions will make the Generator able to provide
 * more contents and/or functionalities (like text extraction).
 *
 * Generation/query is requested by the Document class only, and that
 * class stores the resulting data into Page s. The data will then be
 * displayed by the GUI components (PageView, ThumbnailList, etc..).
 *
 * @see PrintInterface, ConfigInterface, GuiInterface
 */
class OKULAR_EXPORT Generator : public QObject
{
    friend class PixmapGenerationThread;
    friend class TextPageGenerationThread;

    Q_OBJECT

    public:
        /**
         * Describe the possible optional features that a Generator can
         * provide.
         */
        enum GeneratorFeature
        {
            Threaded,
            TextExtraction,    ///< Whether the Generator can extract text from the document in the form of TextPage's
            ReadRawData,       ///< Whether the Generator can read a document directly from its raw data.
            FontInfo,          ///< Whether the Generator can provide information about the fonts used in the document
            PageSizes          ///< Whether the Generator can change the size of the document pages.
        };

        /**
         * Creates a new generator.
         */
        Generator();

        /**
         * Destroys the generator.
         */
        virtual ~Generator();

        /**
         * Loads the document with the given @p fileName and fills the
         * @p pagesVector with the parsed pages.
         *
         * @returns true on success, false otherwise.
         */
        virtual bool loadDocument( const QString & fileName, QVector< Page * > & pagesVector ) = 0;

        /**
         * Loads the document from the raw data @p fileData and fills the
         * @p pagesVector with the parsed pages.
         *
         * @note the Generator has to have the feature @ref ReadRawData enabled
         *
         * @returns true on success, false otherwise.
         */
        virtual bool loadDocumentFromData( const QByteArray & fileData, QVector< Page * > & pagesVector );

        /**
         * This method is called when the document is closed and not used
         * any longer.
         *
         * @returns true on success, false otherwise.
         */
        virtual bool closeDocument() = 0;

        /**
         * This method returns whether the generator is ready to
         * handle a new pixmap request.
         */
        virtual bool canGeneratePixmap() const;

        /**
         * This method can be called to trigger the generation of
         * a new pixmap as described by @p request.
         */
        virtual void generatePixmap( PixmapRequest * request );

        /**
         * This method returns whether the generator is ready to
         * handle a new text page request.
         */
        virtual bool canGenerateTextPage() const;

        /**
         * This method can be called to trigger the generation of
         * a text page for the given @p page.
         *
         * The generation is done synchronous or asynchronous, depending
         * on the @p type parameter and the capabilities of the
         * generator (e.g. multithreading).
         *
         * @see TextPage
         */
        virtual void generateTextPage( Page * page );

        /**
         * Returns the general information object of the document or 0 if
         * no information are available.
         */
        virtual const DocumentInfo * generateDocumentInfo();

        /**
         * Returns the 'table of content' object of the document or 0 if
         * no table of content is available.
         */
        virtual const DocumentSynopsis * generateDocumentSynopsis();

        /**
         * Returns the 'list of embedded fonts' object of the specified \page
         * of the document.
         *
         * \param page a page of the document, starting from 1 - 0 indicates all
         * the other fonts
         */
        virtual FontInfo::List fontsForPage( int page );

        /**
         * Returns the 'list of embedded files' object of the document or 0 if
         * no list of embedded files is available.
         */
        virtual const QList<EmbeddedFile*> * embeddedFiles() const;

        /**
         * This enum identifies the metric of the page size.
         */
        enum PageSizeMetric
        {
          None,   ///< The page size is not defined in a physical metric.
          Points  ///< The page size is given in 1/72 inches.
        };

        /**
         * This method returns the metric of the page size. Default is @ref None.
         */
        virtual PageSizeMetric pagesSizeMetric() const;

        /**
         * This method returns whether given @p action (@ref Permission) is
         * allowed in this document.
         */
        virtual bool isAllowed( Permission action ) const;

        /**
         * This method is called when the orientation has been changed by the user.
         */
        virtual void rotationChanged( Rotation orientation, Rotation oldOrientation );

        /**
         * Returns the list of supported page sizes.
         */
        virtual PageSize::List pageSizes() const;

        /**
         * This method is called when the page size has been changed by the user.
         */
        virtual void pageSizeChanged( const PageSize &pageSize, const PageSize &oldPageSize );

        /**
         * This method is called to print the document to the given @p printer.
         */
        virtual bool print( KPrinter &printer );

        /**
         * This method returns the meta data of the given @p key with the given @p option
         * of the document.
         */
        virtual QVariant metaData( const QString &key, const QVariant &option ) const;

        /**
         * Returns the list of additional supported export formats.
         */
        virtual ExportFormat::List exportFormats() const;

        /**
         * This method is called to export the document in the given @p format and save it
         * under the given @p fileName. The format must be one of the supported export formats.
         */
        virtual bool exportTo( const QString &fileName, const ExportFormat &format );

        /**
         * Query for the specified @p feature.
         */
        bool hasFeature( GeneratorFeature feature ) const;

        /**
         * Returns the component data associated with the generator. May be null.
         */
        const KComponentData* componentData() const;

    Q_SIGNALS:
        /**
         * This signal should be emitted whenever an error occurred in the generator.
         *
         * @param message The message which should be shown to the user.
         * @param duration The time that the message should be shown to the user.
         */
        void error( const QString &message, int duration );

        /**
         * This signal should be emitted whenever the user should be warned.
         *
         * @param message The message which should be shown to the user.
         * @param duration The time that the message should be shown to the user.
         */
        void warning( const QString &message, int duration );

        /**
         * This signal should be emitted whenever the user should be noticed.
         *
         * @param message The message which should be shown to the user.
         * @param duration The time that the message should be shown to the user.
         */
        void notice( const QString &message, int duration );

    protected:
        /**
         * This method must be called when the pixmap request triggered by generatePixmap()
         * has been finished.
         */
        void signalPixmapRequestDone( PixmapRequest * request );

        /**
         * Returns the image of the page as specified in
         * the passed pixmap @p request.
         *
         * @warning this method may be executed in its own separated thread if the
         * @ref Threaded is enabled!
         */
        virtual QImage image( PixmapRequest *page );

        /**
         * Returns the text page for the given @p page.
         *
         * @warning this method may be executed in its own separated thread if the
         * @ref Threaded is enabled!
         */
        virtual TextPage* textPage( Page *page );

        /**
         * Returns a pointer to the document.
         */
        const Document * document() const;

        /**
         * Toggle the @p feature .
         */
        void setFeature( GeneratorFeature feature, bool on = true );

        /**
         * Sets a new about @p data for the generator. The base generator
         * class will take ownership of the data.
         *
         * Create it on the heap (\b never on the stack!), and fill it with
         * data like:
         * @code
KAboutData *about = new KAboutData(
    "generator_foo", // internal name (notes below)
    "generator_foo",  // i18n catalog (notes below)
    ki18n( "Foo Backend" ),
    "0.1",
    ki18n( "A foo backend" ),
    KAboutData::License_GPL,
    ki18n( "© 2007 Developer" )
);
about->addAuthor( ki18n( "Joe Developer" ), ki18n( "Developer" ), " joe@kde.org" );
setAboutData( about );
         * @endcode
         *
         * @note both "internal name" and "i18n catalog" are reccomended to be
         * set like "okular_foo" (where foo is the name of your generator).
         * The first is important for loading some metadata of the generator
         * itself, while the second is used for loading the .mo catalog with
         * the translation.
         */
        void setAboutData( KAboutData* data );

    private:
        friend class GeneratorPrivate;
        GeneratorPrivate* const d;

        friend class Document;

        Q_PRIVATE_SLOT( d, void pixmapGenerationFinished() )
        Q_PRIVATE_SLOT( d, void textpageGenerationFinished() )
};

/**
 * @short Describes a pixmap type request.
 */
class OKULAR_EXPORT PixmapRequest
{
    friend class Document;

    public:
        /**
         * Creates a new pixmap request.
         *
         * @param id The observer id.
         * @param pageNumber The page number.
         * @param width The width of the page.
         * @param height The height of the page.
         * @param priority The priority of the request.
         * @param asynchronous The mode of generation.
         */
        PixmapRequest( int id, int pageNumber, int width, int height, int priority, bool asynchronous );

        /**
         * Destroys the pixmap request.
         */
        ~PixmapRequest();

        /**
         * Returns the observer id of the request.
         */
        int id() const;

        /**
         * Returns the page number of the request.
         */
        int pageNumber() const;

        /**
         * Returns the page width of the requested pixmap.
         */
        int width() const;

        /**
         * Returns the page height of the requested pixmap.
         */
        int height() const;

        /**
         * Returns the priority (less it better, 0 is maximum) of the
         * request.
         */
        int priority() const;

        /**
         * Returns whether the generation should be done synchronous or
         * asynchronous.
         *
         * If asynchronous, the pixmap is created in a thread and the observer
         * is notified when the job is done.
         */
        bool asynchronous() const;

        /**
         * Returns a pointer to the page where the pixmap shall be generated for.
         */
        Page *page() const;

        /**
         * Internal usage.
         */
        void swap();

    protected:
        /**
         * Internal usage.
         */
        void setPriority( int priority );

        /**
         * Internal usage.
         */
        void setAsynchronous( bool asynchronous );

        /**
         * Internal usage.
         */
        void setPage( Page *page );

    private:
        Q_DISABLE_COPY( PixmapRequest )

        class Private;
        Private* const d;
};

}

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<( QDebug str, const Okular::PixmapRequest &req );
#endif

#endif
