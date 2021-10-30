//
// Created by olivier on 29/10/2021.
//

#ifndef ONELIFE_COMPONENT_ASYNCFILETHREAD_H
#define ONELIFE_COMPONENT_ASYNCFILETHREAD_H

#include "minorGems/system/StopSignalThread.h"
#include "minorGems/system/BinarySemaphore.h"
#include "minorGems/util/SimpleVector.h"
#include "minorGems/system/MutexLock.h"
#include "minorGems/io/file/File.h"
#include "OneLife/gameSource/application.h"
#include "OneLife/gameSource/components/asyncFileRecord.h"

extern BinarySemaphore newFileToReadSem;
extern BinarySemaphore newFileDoneReadingSem;
extern SimpleVector<AsyncFileRecord> asyncFiles;
extern MutexLock asyncLock;


class AsyncFileThread : public StopSignalThread {

	public:

		AsyncFileThread() {
			start();
		}

		~AsyncFileThread() {
			stop();
			newFileToReadSem.signal();
			join();

			for( int i=0; i<asyncFiles.size(); i++ ) {

				AsyncFileRecord *r = asyncFiles.getElement( i );

				if( r->filePath != NULL ) {
					delete [] r->filePath;
				}

				if( r->data != NULL ) {
					delete [] r->data;
				}
			}
			asyncFiles.deleteAll();
		}


		virtual void run() {
			while( ! isStopped() ) {

				int handleToRead = -1;
				char *pathToRead = NULL;

				asyncLock.lock();

				for( int i=0; i<asyncFiles.size(); i++ ) {
					AsyncFileRecord *r = asyncFiles.getElement( i );

					if( ! r->doneReading ) {
						// can't trust pointer to record in vector
						// outside of lock, because vector storage may
						// change
						handleToRead = r->handle;
						pathToRead = r->filePath;
						break;
					}
				}

				asyncLock.unlock();

				if( handleToRead != -1 ) {
					// read file data

					File f( NULL, pathToRead );

					int dataLength;
					unsigned char *data = f.readFileContents( &dataLength );

					// re-lock vector, search for handle, and add it
					// cannot count on vector order or pointers to records
					// when we don't have it locked

					asyncLock.lock();

					for( int i=0; i<asyncFiles.size(); i++ ) {
						AsyncFileRecord *r = asyncFiles.getElement( i );

						if( r->handle == handleToRead ) {
							r->dataLength = dataLength;
							r->data = data;
							r->doneReading = true;
							break;
						}
					}

					asyncLock.unlock();

					// let anyone waiting for a new file to finish
					// reading (only matters in the case of playback, where
					// the file-done must happen on a specific frame)
					newFileDoneReadingSem.signal();
				}
				else {
					// wait on binary semaphore until something else added
					// for us to read
					newFileToReadSem.wait();
				}
			}
		};
};

int startAsyncFileRead( const char *inFilePath );
char checkAsyncFileReadDone( int inHandle );
unsigned char *getAsyncFileData( int inHandle, int *outDataLength );

#endif //ONELIFE_COMPONENT_ASYNCFILETHREAD_H
