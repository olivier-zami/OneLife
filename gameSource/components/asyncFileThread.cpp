//
// Created by olivier on 29/10/2021.
//

#include "asyncFileThread.h"

//#include "minorGems/graphics/openGL/ScreenGL.h"
#include "OneLife/gameSource/components/asyncFileRecord.h"

extern OneLife::game::Application *gameApplication;

//TODO: newFileToReadSem, newFileDoneReadingSem, asyncFiles, asyncLock must be declared before fileReadThread => improve modularity
BinarySemaphore newFileToReadSem;
BinarySemaphore newFileDoneReadingSem;
SimpleVector<AsyncFileRecord> asyncFiles;
MutexLock asyncLock;
static AsyncFileThread fileReadThread;

static int nextAsyncFileHandle = 0;

/*
AsyncFileThread::AsyncFileThread()
{
	start();
}

AsyncFileThread::~AsyncFileThread()
{
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

void AsyncFileThread::run()
{
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
}
*/
int startAsyncFileRead( const char *inFilePath ) {

	int handle = nextAsyncFileHandle;
	nextAsyncFileHandle ++;

	AsyncFileRecord r = {
			handle,
			stringDuplicate( inFilePath ),
			-1,
			NULL,
			false };

	asyncLock.lock();
	asyncFiles.push_back( r );
	asyncLock.unlock();

	newFileToReadSem.signal();

	return handle;
}

char checkAsyncFileReadDone( int inHandle ) {

	char ready = false;

	asyncLock.lock();

	for( int i=0; i<asyncFiles.size(); i++ ) {
		AsyncFileRecord *r = asyncFiles.getElement( i );

		if( r->handle == inHandle &&
			r->doneReading ) {

			ready = true;
			break;
		}
	}
	asyncLock.unlock();


	if( gameApplication->isPlayingBack() ) {
		char playbackSaysReady = gameApplication->getAsyncFileDone( inHandle );

		if( ready && playbackSaysReady ) {
			return true;
		}
		else if( ready && !playbackSaysReady ) {
			return false;
		}
		else if( ! ready && playbackSaysReady ) {
			// need to return ready before end of this frame
			// so behavior matches recording behavior

			// wait for read to finish, synchronously
			while( !ready ) {
				newFileDoneReadingSem.wait();

				asyncLock.lock();

				for( int i=0; i<asyncFiles.size(); i++ ) {
					AsyncFileRecord *r = asyncFiles.getElement( i );

					if( r->handle == inHandle &&
						r->doneReading ) {

						ready = true;
						break;
					}
				}
				asyncLock.unlock();
			}

			return true;
		}
	}
	else {
		if( ready ) {
			gameApplication->registerAsyncFileDone( inHandle );
		}
	}


	return ready;
}

unsigned char *getAsyncFileData( int inHandle, int *outDataLength ) {

	unsigned char *data = NULL;

	asyncLock.lock();

	for( int i=0; i<asyncFiles.size(); i++ ) {
		AsyncFileRecord *r = asyncFiles.getElement( i );

		if( r->handle == inHandle ) {

			data = r->data;
			*outDataLength = r->dataLength;

			if( r->filePath != NULL ) {
				delete [] r->filePath;
			}

			asyncFiles.deleteElement( i );
			break;
		}
	}
	asyncLock.unlock();

	return data;
}