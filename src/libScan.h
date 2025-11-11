#ifndef LIBSCAN_H
#define LIBSCAN_H

#include <QString>
#include <sqlite3.h>

class LibScan 
{
    public:
        static void scanMusicLibrary(const QString &directoryPath, const QString &dbPath); //called after filedialog prompt
        
        static bool tableExists(sqlite3 *db, const QString &tableName);  //compiler having a fit because this wasn't static
    };
#endif // LIBSCAN_H