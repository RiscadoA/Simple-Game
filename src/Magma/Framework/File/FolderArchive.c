#include "FolderArchive.h"
#include "Path.h"
#include "Config.h"

#include "../Memory/Allocator.h"
#include "../String/StringStream.h"
#include "../Thread/Mutex.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#if defined(MAGMA_FRAMEWORK_USE_WINDOWS_FILESYSTEM)
#include <Windows.h>
#else
#error MAGMA_FRAMEWORK_USE_WINDOWS_FILESYSTEM must be defined. File system library still only supports Windows.
#endif

typedef struct mffFolderFile mffFolderFile;

struct mffFolderFile
{
	mffFile base;
	mfsUTF8CodeUnit path[MFF_MAX_FILE_PATH_SIZE];
	mffFolderFile* parent;
	mffFolderFile* next;
	mffFolderFile* first;
	mffEnum type;
};

typedef struct
{
	mffArchive base;
	mfsUTF8CodeUnit path[MFF_MAX_FILE_PATH_SIZE];
	void* allocator;
	mffFolderFile* firstFile;
	mftMutex* mutex;
} mffFolderArchive;

static void mffDestroyFile(void* file)
{
	if (file == NULL)
		abort();

	mfError err;
	mffFolderFile* folderFile = file;

	err = mfmDeinitObject(&folderFile->base.object);
	if (err != MF_ERROR_OKAY)
		abort();

	err = mfmDeallocate(((mffFolderArchive*)folderFile->base.archive)->allocator, folderFile);
	if (err != MF_ERROR_OKAY)
		abort();
}

static mfError mffArchiveGetFileUnsafe(mffArchive* archive, mffFile** outFile, const mfsUTF8CodeUnit* path)
{
	mffFolderArchive* folderArchive = archive;
	mfError err;

	if (strcmp(path, u8"/") == 0)
	{
		*outFile = NULL;
		return MF_ERROR_OKAY;
	}

	mffFolderFile* currentFile = folderArchive->firstFile;
	while (currentFile != NULL)
	{
		if (strcmp(currentFile->path, path) == 0)
		{
			*outFile = currentFile;
			return MF_ERROR_OKAY;
		}

		// Next file
		if (currentFile->first != NULL)
			currentFile = currentFile->first;
		else
		{
			if (currentFile->next == NULL)
			{
				if (currentFile->parent == NULL)
					break;
				mffFolderFile* nextParent = currentFile;
				do
				{
					nextParent = nextParent->parent;
					if (nextParent == NULL)
						break;
				} while (nextParent->next == NULL);
				if (nextParent == NULL)
					currentFile = NULL;
				else
					currentFile = nextParent->next;
			}
			else
				currentFile = currentFile->next;
		}
	}

	return MFF_ERROR_FILE_NOT_FOUND;
}

static mfError mffArchiveGetFile(mffArchive* archive, mffFile** outFile, const mfsUTF8CodeUnit* path)
{
	mffFolderArchive* folderArchive = archive;
	mfError err;

	err = mftLockMutex(folderArchive->mutex, 0);
	if (err != MF_ERROR_OKAY)
		return err;

	err = mffArchiveGetFileUnsafe(archive, outFile, path);
	if (err != MF_ERROR_OKAY)
	{
		mftUnlockMutex(folderArchive->mutex);
		return err;
	}

	err = mftUnlockMutex(folderArchive->mutex);
	if (err != MF_ERROR_OKAY)
		return err;

	return MF_ERROR_OKAY;
}

static mfError mffArchiveGetFileType(mffArchive* archive, mffFile* file, mffEnum* outType)
{
	mffFolderFile* folderFile = file;
	*outType = folderFile->type;
	return MF_ERROR_OKAY;
}

static mfError mffArchiveGetFirstFile(mffArchive* archive, mffFile** outFile, mffDirectory* directory)
{
	mffFolderFile* folderFile = directory;
	if (directory == NULL)
		*outFile = ((mffFolderArchive*)archive)->firstFile;
	else
		*outFile = folderFile->first;
	return MF_ERROR_OKAY;
}

static mfError mffArchiveGetNextFile(mffArchive* archive, mffFile** outFile, mffFile* file)
{
	mffFolderFile* folderFile = file;
	*outFile = folderFile->next;
	return MF_ERROR_OKAY;
}

static mfError mffArchiveGetParent(mffArchive* archive, mffDirectory** outParent, mffFile* file)
{
	mffFolderFile* folderFile = file;
	*outParent = folderFile->parent;
	return MF_ERROR_OKAY;
}

static mfError mffArchiveCreateDirectoryUnsafe(mffArchive* archive, mffDirectory** outDir, const mfsUTF8CodeUnit* path)
{
	mfError err;
	mffFolderArchive* folderArchive = archive;

	err = mffArchiveGetFile(archive, outDir, path);
	if (err == MF_ERROR_OKAY)
		return MFF_ERROR_ALREADY_EXISTS;
	else if (err != MFF_ERROR_FILE_NOT_FOUND)
		return err;

	// Get parent path
	// Example: /test.txt -> / ; /Test/test.txt -> /Test/

	// Get last '/'
	const mfsUTF8CodeUnit* lastSep = path;
	const mfsUTF8CodeUnit* it = path;
	while (*it != '\0')
	{
		if (*it == '/')
			lastSep = it;
		++it;
	}

	// Get separator offset
	mfmU64 lastSepOff = lastSep - path;
	if (lastSepOff >= MFF_MAX_FILE_PATH_SIZE - 1)
		return MFF_ERROR_PATH_TOO_BIG;

	// Get parent file
	mffFolderFile* parentFile;
	{
		mfsUTF8CodeUnit parentPath[MFF_MAX_FILE_PATH_SIZE];
		memcpy(parentPath, path, lastSepOff);
		parentPath[lastSepOff] = '\0';

		if (lastSepOff == 0)
			parentFile = NULL;
		else
		{
			err = mffArchiveGetFileUnsafe(archive, &parentFile, parentPath);
			if (err != MF_ERROR_OKAY)
				return err;
		}
	}

	mffFolderFile* file;

	// Create real file
	{
		mfsUTF8CodeUnit realPath[256];
		{
			mfsStringStream ss;
			err = mfsCreateLocalStringStream(&ss, realPath, sizeof(realPath));
			if (err != MF_ERROR_OKAY)
				return err;
			err = mfsPutString(&ss, folderArchive->path);
			if (err != MF_ERROR_OKAY)
				return err;
			err = mfsPutString(&ss, path);
			if (err != MF_ERROR_OKAY)
				return err;
			mfsDestroyLocalStringStream(&ss);
		}
	
#if defined(MAGMA_FRAMEWORK_USE_WINDOWS_FILESYSTEM)
		if (!CreateDirectory(realPath, NULL))
			return MFF_ERROR_INTERNAL;
#endif
	}

	// Create file
	{
		err = mfmAllocate(folderArchive->allocator, &file, sizeof(mffFolderFile));
		if (err != MF_ERROR_OKAY)
			return err;

		err = mfmInitObject(&file->base.object);
		if (err != MF_ERROR_OKAY)
			return err;

		err = mfmAcquireObject(&file->base.object);
		if (err != MF_ERROR_OKAY)
			return err;

		file->base.object.destructorFunc = &mffDestroyFile;
		file->base.archive = archive;
		file->first = NULL;
		file->next = NULL;
		file->parent = parentFile;
		file->type = MFF_DIRECTORY;

		mfsStringStream ss;
		err = mfsCreateLocalStringStream(&ss, file->path, sizeof(file->path));
		if (err != MF_ERROR_OKAY)
			return err;
		err = mfsPrintFormat(&ss, u8"%s", path);
		if (err != MF_ERROR_OKAY)
			return err;
		mfsDestroyLocalStringStream(&ss);
	}

	// Add new file to parent file
	if (parentFile == NULL)
	{
		if (folderArchive->firstFile == NULL)
			folderArchive->firstFile = file;
		else
		{
			mffFolderFile* prevFile = folderArchive->firstFile;
			while (prevFile->next != NULL)
				prevFile = prevFile->next;
			prevFile->next = file;
		}
	}
	else
	{
		if (parentFile->first == NULL)
			parentFile->first = file;
		else
		{
			mffFolderFile* prevFile = parentFile->first;
			while (prevFile->next != NULL)
				prevFile = prevFile->next;
			prevFile->next = file;
		}
	}

	if (outDir != NULL)
		*outDir = file;

	return MF_ERROR_OKAY;
}

static mfError mffArchiveCreateDirectory(mffArchive* archive, mffDirectory** outDir, const mfsUTF8CodeUnit* path)
{
	mffFolderArchive* folderArchive = archive;
	mfError err;

	err = mftLockMutex(folderArchive->mutex, 0);
	if (err != MF_ERROR_OKAY)
		return err;

	err = mffArchiveCreateDirectoryUnsafe(archive, outDir, path);
	if (err != MF_ERROR_OKAY)
	{
		mftUnlockMutex(folderArchive->mutex);
		return err;
	}

	err = mftUnlockMutex(folderArchive->mutex);
	if (err != MF_ERROR_OKAY)
		return err;

	return MF_ERROR_OKAY;
}

static mfError mffArchiveDeleteDirectoryUnsafe(mffArchive* archive, mffDirectory* dir)
{
	mfError err;
	mffFolderArchive* folderArchive = archive;
	mffFolderFile* folderFile = dir;

	if (folderFile->type != MFF_DIRECTORY)
		return MFF_ERROR_NOT_A_DIRECTORY;

	if (folderFile->first != NULL)
		return MFF_ERROR_MUST_BE_EMPTY;

	{
		mfmI32 refCount;
		mfmGetObjectRefCount(&dir->object, &refCount);

		if (refCount != 1)return MFM_ERROR_STILL_HAS_REFERENCES;

		err = mfmReleaseObject(&dir->object);
		if (err != MF_ERROR_OKAY)
			return err;
	}

	// Delete file
	mfsUTF8CodeUnit realPath[256];
	{
		mfsStringStream ss;
		err = mfsCreateLocalStringStream(&ss, realPath, sizeof(realPath));
		if (err != MF_ERROR_OKAY)
			return err;
		err = mfsPutString(&ss, folderArchive->path);
		if (err != MF_ERROR_OKAY)
			return err;
		err = mfsPutString(&ss, folderFile->path);
		if (err != MF_ERROR_OKAY)
			return err;
		mfsDestroyLocalStringStream(&ss);
	}

#if defined(MAGMA_FRAMEWORK_USE_WINDOWS_FILESYSTEM)
	if (!RemoveDirectory(realPath))
		return MFF_ERROR_INTERNAL;
#endif

	// Remove file from tree
	if (folderFile->parent == NULL)
	{
		mffFolderFile* prev = folderArchive->firstFile;
		if (prev == folderFile)
			folderArchive->firstFile = folderFile->next;
		else
		{
			while (prev->next != folderFile)
				prev = prev->next;
			prev->next = folderFile->next;
		}
	}
	else
	{
		mffFolderFile* prev = folderFile->parent->first;
		if (prev == folderFile)
			folderFile->parent->first = folderFile->next;
		else
		{
			while (prev->next != folderFile)
				prev = prev->next;
			prev->next = folderFile->next;
		}
	}

	// Call file destructor
	folderFile->base.object.destructorFunc(folderFile);

	return MF_ERROR_OKAY;
}

static mfError mffArchiveDeleteDirectory(mffArchive* archive, mffDirectory* dir)
{
	mffFolderArchive* folderArchive = archive;
	mfError err;

	err = mftLockMutex(folderArchive->mutex, 0);
	if (err != MF_ERROR_OKAY)
		return err;

	err = mffArchiveDeleteDirectoryUnsafe(archive, dir);
	if (err != MF_ERROR_OKAY)
	{
		mftUnlockMutex(folderArchive->mutex);
		return err;
	}

	err = mftUnlockMutex(folderArchive->mutex);
	if (err != MF_ERROR_OKAY)
		return err;

	return MF_ERROR_OKAY;
}

static mfError mffArchiveCreateFileUnsafe(mffArchive* archive, mffFile** outFile, const mfsUTF8CodeUnit* path)
{
	mfError err;
	mffFolderArchive* folderArchive = archive;

	err = mffArchiveGetFile(archive, outFile, path);
	if (err == MF_ERROR_OKAY)
		return MFF_ERROR_ALREADY_EXISTS;
	else if (err != MFF_ERROR_FILE_NOT_FOUND)
		return err;

	// Get parent path
	// Example: /test.txt -> / ; /Test/test.txt -> /Test/

	// Get last '/'
	const mfsUTF8CodeUnit* lastSep = path;
	const mfsUTF8CodeUnit* it = path;
	while (*it != '\0')
	{
		if (*it == '/')
			lastSep = it;
		++it;
	}

	// Get separator offset
	mfmU64 lastSepOff = lastSep - path;
	if (lastSepOff >= MFF_MAX_FILE_PATH_SIZE - 1)
		return MFF_ERROR_PATH_TOO_BIG;

	// Get parent file
	mffFolderFile* parentFile;
	{
		mfsUTF8CodeUnit parentPath[MFF_MAX_FILE_PATH_SIZE];
		memcpy(parentPath, path, lastSepOff);
		parentPath[lastSepOff] = '\0';

		if (lastSepOff == 0)
			parentFile = NULL;
		else
		{
			err = mffArchiveGetFileUnsafe(archive, &parentFile, parentPath);
			if (err != MF_ERROR_OKAY)
				return err;
		}
	}

	mffFolderFile* file;

	// Create real file
	{
		mfsUTF8CodeUnit realPath[256];
		{
			mfsStringStream ss;
			err = mfsCreateLocalStringStream(&ss, realPath, sizeof(realPath));
			if (err != MF_ERROR_OKAY)
				return err;
			err = mfsPutString(&ss, folderArchive->path);
			if (err != MF_ERROR_OKAY)
				return err;
			err = mfsPutString(&ss, path);
			if (err != MF_ERROR_OKAY)
				return err;
			mfsDestroyLocalStringStream(&ss);
		}

#if defined(MAGMA_FRAMEWORK_USE_WINDOWS_FILESYSTEM)
		HANDLE f = CreateFile(realPath, 0, 0, NULL, CREATE_NEW, 0, NULL);
		if (f == INVALID_HANDLE_VALUE)
			return MFF_ERROR_INTERNAL;
		GetLastError();
		CloseHandle(f);
#endif
	}

	// Create file
	{
		err = mfmAllocate(folderArchive->allocator, &file, sizeof(mffFolderFile));
		if (err != MF_ERROR_OKAY)
			return err;

		err = mfmInitObject(&file->base.object);
		if (err != MF_ERROR_OKAY)
			return err;

		err = mfmAcquireObject(&file->base.object);
		if (err != MF_ERROR_OKAY)
			return err;

		file->base.object.destructorFunc = &mffDestroyFile;
		file->base.archive = archive;
		file->first = NULL;
		file->next = NULL;
		file->parent = parentFile;
		file->type = MFF_FILE;

		mfsStringStream ss;
		err = mfsCreateLocalStringStream(&ss, file->path, sizeof(file->path));
		if (err != MF_ERROR_OKAY)
			return err;
		err = mfsPrintFormat(&ss, u8"%s", path);
		if (err != MF_ERROR_OKAY)
			return err;
		mfsDestroyLocalStringStream(&ss);
	}

	// Add new file to parent file
	if (parentFile == NULL)
	{
		if (folderArchive->firstFile == NULL)
			folderArchive->firstFile = file;
		else
		{
			mffFolderFile* prevFile = folderArchive->firstFile;
			while (prevFile->next != NULL)
				prevFile = prevFile->next;
			prevFile->next = file;
		}
	}
	else
	{
		if (parentFile->first == NULL)
			parentFile->first = file;
		else
		{
			mffFolderFile* prevFile = parentFile->first;
			while (prevFile->next != NULL)
				prevFile = prevFile->next;
			prevFile->next = file;
		}
	}

	if (outFile != NULL)
		*outFile = file;

	return MF_ERROR_OKAY;
}

static mfError mffArchiveCreateFile(mffArchive* archive, mffFile** outFile, const mfsUTF8CodeUnit* path)
{
	mffFolderArchive* folderArchive = archive;
	mfError err;

	err = mftLockMutex(folderArchive->mutex, 0);
	if (err != MF_ERROR_OKAY)
		return err;

	err = mffArchiveCreateFileUnsafe(archive, outFile, path);
	if (err != MF_ERROR_OKAY)
	{
		mftUnlockMutex(folderArchive->mutex);
		return err;
	}

	err = mftUnlockMutex(folderArchive->mutex);
	if (err != MF_ERROR_OKAY)
		return err;

	return MF_ERROR_OKAY;
}

static mfError mffArchiveDeleteFileUnsafe(mffArchive* archive, mffFile* file)
{
	mfError err;
	mffFolderArchive* folderArchive = archive;
	mffFolderFile* folderFile = file;

	if (folderFile->type != MFF_FILE)
		return MFF_ERROR_NOT_A_FILE;

	{
		mfmI32 refCount;
		mfmGetObjectRefCount(&file->object, &refCount);

		if (refCount != 1)return MFM_ERROR_STILL_HAS_REFERENCES;

		err = mfmReleaseObject(&file->object);
		if (err != MF_ERROR_OKAY)
			return err;
	}

	// Delete file
	mfsUTF8CodeUnit realPath[256];
	{
		mfsStringStream ss;
		err = mfsCreateLocalStringStream(&ss, realPath, sizeof(realPath));
		if (err != MF_ERROR_OKAY)
			return err;
		err = mfsPutString(&ss, folderArchive->path);
		if (err != MF_ERROR_OKAY)
			return err;
		err = mfsPutString(&ss, folderFile->path);
		if (err != MF_ERROR_OKAY)
			return err;
		mfsDestroyLocalStringStream(&ss);
	}

#if defined(MAGMA_FRAMEWORK_USE_WINDOWS_FILESYSTEM)
	if (!DeleteFile(realPath))
		return MFF_ERROR_INTERNAL;
#endif

	// Remove file from tree
	if (folderFile->parent == NULL)
	{
		mffFolderFile* prev = folderArchive->firstFile;
		if (prev == folderFile)
			folderArchive->firstFile = folderFile->next;
		else
		{
			while (prev->next != folderFile)
				prev = prev->next;
			prev->next = folderFile->next;
		}
	}
	else
	{
		mffFolderFile* prev = folderFile->parent->first;
		if (prev == folderFile)
			folderFile->parent->first = folderFile->next;
		else
		{
			while (prev->next != folderFile)
				prev = prev->next;
			prev->next = folderFile->next;
		}
	}

	// Call file destructor
	folderFile->base.object.destructorFunc(folderFile);

	return MF_ERROR_OKAY;
}

static mfError mffArchiveDeleteFile(mffArchive* archive, mffFile* file)
{
	mffFolderArchive* folderArchive = archive;
	mfError err;

	err = mftLockMutex(folderArchive->mutex, 0);
	if (err != MF_ERROR_OKAY)
		return err;

	err = mffArchiveDeleteFileUnsafe(archive, file);
	if (err != MF_ERROR_OKAY)
	{
		mftUnlockMutex(folderArchive->mutex);
		return err;
	}

	err = mftUnlockMutex(folderArchive->mutex);
	if (err != MF_ERROR_OKAY)
		return err;

	return MF_ERROR_OKAY;
}

typedef struct
{
	mfsStream base;
	mffFolderArchive* archive;
	mffFolderFile* file;
	FILE* handle;
} mffFolderFileStream;

static void mffArchiveCloseFileUnsafe(mffFolderFileStream* stream)
{
	mfError err;

	err = mfmDeinitObject(&stream->base.object);
	if (err != MF_ERROR_OKAY)
		abort();

	err = mfmReleaseObject(&stream->file->base.object);
	if (err != MF_ERROR_OKAY)
		abort();

	fclose(stream->handle);

	err = mfmDeallocate(stream->archive->allocator, stream);
	if (err != MF_ERROR_OKAY)
		abort();

	return MF_ERROR_OKAY;
}

static mfError mffFileStreamWrite(void* stream, const mfmU8* memory, mfmU64 memorySize, mfmU64* writtenSize)
{
	mffFolderFileStream* folderStream = stream;

	mfmU64 size = fwrite(memory, sizeof(mfmU8), memorySize, folderStream->handle);
	if (writtenSize != NULL)
		*writtenSize = size;

	errno_t e = errno;
	if (e != 0)
		return MFS_ERROR_INTERNAL;

	return MF_ERROR_OKAY;
}


static mfError mffFileStreamRead(void* stream, mfmU8* memory, mfmU64 memorySize, mfmU64* readSize)
{
	mffFolderFileStream* folderStream = stream;

	mfmU64 size = fread(memory, sizeof(mfmU8), memorySize, folderStream->handle);
	if (readSize != NULL)
		*readSize = size;

	errno_t e = errno;
	if (e != 0)
		return MFS_ERROR_INTERNAL;

	return MF_ERROR_OKAY;
}

static mfError mffFileStreamFlush(void* stream)
{
	mffFolderFileStream* folderStream = stream;
	int ret = fflush(folderStream->handle);
	if (ret == EOF)
		return MFS_ERROR_EOF;
	return MF_ERROR_OKAY;
}

static mfError mffFileStreamSetBuffer(void* stream, mfmU8* buffer, mfmU64 bufferSize)
{
	mffFolderFileStream* folderStream = stream;

	if (buffer != NULL)
	{
		int ret = setvbuf(folderStream->handle, buffer, _IOFBF, bufferSize);
		if (ret != 0)
			return MFS_ERROR_INTERNAL;
	}
	else
	{
		int ret = setvbuf(folderStream->handle, NULL, _IONBF, 0);
		if (ret != 0)
			return MFS_ERROR_INTERNAL;
	}

	return MF_ERROR_OKAY;
}

static mfError mffFileStreamSeekBegin(void* stream, mfmU64 offset)
{
	mffFolderFileStream* folderStream = stream;
	int ret = fseek(folderStream->handle, offset, SEEK_SET);
	if (ret != 0)
		return MFS_ERROR_INTERNAL;
	return MF_ERROR_OKAY;
}

static mfError mffFileStreamSeekEnd(void* stream, mfmU64 offset)
{
	mffFolderFileStream* folderStream = stream;
	int ret = fseek(folderStream->handle, -(mfmI64)offset, SEEK_END);
	if (ret != 0)
		return MFS_ERROR_INTERNAL;
	return MF_ERROR_OKAY;
}

static mfError mffFileStreamSeekHead(void* stream, mfmI64 offset)
{
	mffFolderFileStream* folderStream = stream;
	int ret = fseek(folderStream->handle, offset, SEEK_CUR);
	if (ret != 0)
		return MFS_ERROR_INTERNAL;
	return MF_ERROR_OKAY;
}

static mfError mffFileStreamTell(void* stream, mfmU64* position)
{
	mffFolderFileStream* folderStream = stream;
	mfmI64 ret = ftell(folderStream->handle);
	if (ret == -1)
		return MFS_ERROR_INTERNAL;
	*position = ret;
	return MF_ERROR_OKAY;
}

static mfError mffFileStreamEOF(void* stream, mfmBool* eof)
{
	mffFolderFileStream* folderStream = stream;
	int ret = feof(folderStream->handle);
	if (ret != 0)
		*eof = MFM_TRUE;
	else
		*eof = MFM_FALSE;
	return MF_ERROR_OKAY;
}

static void mffArchiveCloseFile(void* stream)
{
	mffFolderFileStream* folderStream = stream;
	mffFolderArchive* archive = folderStream->archive;
	mfError err;

	err = mftLockMutex(archive->mutex, 0);
	if (err != MF_ERROR_OKAY)
		abort();

	mffArchiveCloseFileUnsafe(folderStream);

	err = mftUnlockMutex(archive->mutex);
	if (err != MF_ERROR_OKAY)
		abort();
}

static mfError mffArchiveOpenFileUnsafe(mffArchive* archive, mfsStream** outStream, mffFile* file, mffEnum mode)
{
	mfError err;

	mffFolderArchive* folderArchive = archive;
	mffFolderFile* folderFile = file;
	mffFolderFileStream* stream;

	mfsUTF8CodeUnit realPath[256];
	{
		mfsStringStream ss;
		err = mfsCreateLocalStringStream(&ss, realPath, sizeof(realPath));
		if (err != MF_ERROR_OKAY)
			return err;
		err = mfsPutString(&ss, folderArchive->path);
		if (err != MF_ERROR_OKAY)
			return err;
		err = mfsPutString(&ss, folderFile->path);
		if (err != MF_ERROR_OKAY)
			return err;
		mfsDestroyLocalStringStream(&ss);
	}

	FILE* handle = NULL;
	if (mode == MFF_FILE_READ)
	{
		errno_t e = fopen_s(&handle, realPath, u8"rb");
		if (e != 0)
			return MFF_ERROR_INTERNAL;
	}
	else if (mode == MFF_FILE_WRITE)
	{
		errno_t e = fopen_s(&handle, realPath, u8"wb");
		if (e != 0)
			return MFF_ERROR_INTERNAL;
	}
	else
		return MFF_ERROR_INVALID_MODE;

	err = mfmAllocate(folderArchive->allocator, &stream, sizeof(mffFolderFileStream));
	if (err != MF_ERROR_OKAY)
		return err;

	err = mfmInitObject(&stream->base.object);
	if (err != MF_ERROR_OKAY)
		return err;
	stream->base.object.destructorFunc = &mffArchiveCloseFile;

	err = mfmAcquireObject(file);
	if (err != MF_ERROR_OKAY)
		return err;

	stream->archive = archive;
	stream->file = file;
	stream->handle = handle;

	stream->base.read = &mffFileStreamRead;
	stream->base.write = &mffFileStreamWrite;
	stream->base.flush = &mffFileStreamFlush;
	stream->base.setBuffer = &mffFileStreamSetBuffer;
	stream->base.seekBegin = &mffFileStreamSeekBegin;
	stream->base.seekEnd = &mffFileStreamSeekEnd;
	stream->base.seekHead = &mffFileStreamSeekHead;
	stream->base.tell = &mffFileStreamTell;
	stream->base.buffer = NULL;
	stream->base.bufferSize = 0;
	stream->base.eof = &mffFileStreamEOF;

	*outStream = stream;

	return MF_ERROR_OKAY;
}

static mfError mffArchiveOpenFile(mffArchive* archive, mfsStream** outStream, mffFile* file, mffEnum mode)
{
	mffFolderArchive* folderArchive = archive;
	mfError err;

	err = mftLockMutex(folderArchive->mutex, 0);
	if (err != MF_ERROR_OKAY)
		return err;

	err = mffArchiveOpenFileUnsafe(archive, outStream, file, mode);
	if (err != MF_ERROR_OKAY)
	{
		mftUnlockMutex(folderArchive->mutex);
		return err;
	}

	err = mftUnlockMutex(folderArchive->mutex);
	if (err != MF_ERROR_OKAY)
		return err;

	return MF_ERROR_OKAY;
}

#if defined(MAGMA_FRAMEWORK_USE_WINDOWS_FILESYSTEM)
static mfError mffFillWindowsFiles(mffFolderArchive * archive, void* allocator, const mfsUTF8CodeUnit* path, const mfsUTF8CodeUnit* internalPath, mffFolderFile* parent, mffFolderFile** first)
{
	

	mfError err;
	WIN32_FIND_DATA data;
	HANDLE hFind;

	{
		mfsUTF8CodeUnit searchPath[MFF_PATH_MAX_SIZE];

		mfsStringStream ss;
		err = mfsCreateLocalStringStream(&ss, searchPath, sizeof(searchPath));
		if (err != MF_ERROR_OKAY)
			return err;
		err = mfsPrintFormat(&ss, u8"%s/*.*", path);
		if (err != MF_ERROR_OKAY)
			return err;
		mfsDestroyLocalStringStream(&ss);

		hFind = FindFirstFile(searchPath, &data);
	}


	if (hFind != INVALID_HANDLE_VALUE)
	{
		// Iterate over files in directory
		do
		{
			if (strcmp(data.cFileName, u8".") == 0 ||
				strcmp(data.cFileName, u8"..") == 0)
				continue;

			// If is directory
			if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				mffFolderFile* file;

				err = mfmAllocate(allocator, &file, sizeof(mffFolderFile));
				if (err != MF_ERROR_OKAY)
					return err;

				err = mfmInitObject(&file->base.object);
				if (err != MF_ERROR_OKAY)
					return err;

				err = mfmAcquireObject(&file->base.object);
				if (err != MF_ERROR_OKAY)
					return err;

				file->base.object.destructorFunc = &mffDestroyFile;
				file->base.archive = archive;
				file->first = NULL;
				file->next = NULL;
				file->parent = parent;
				file->type = MFF_DIRECTORY;

				mfsStringStream ss;
				
				// Get internal path
				err = mfsCreateLocalStringStream(&ss, file->path, sizeof(file->path));
				if (err != MF_ERROR_OKAY)
					return err;
				err = mfsPrintFormat(&ss, u8"%s/%s", internalPath, data.cFileName);
				if (err != MF_ERROR_OKAY)
					return err;
				mfsDestroyLocalStringStream(&ss);

				// Get real path
				mfsUTF8CodeUnit realPath[MFF_MAX_FILE_PATH_SIZE];
				err = mfsCreateLocalStringStream(&ss, realPath, sizeof(realPath));
				if (err != MF_ERROR_OKAY)
					return err;
				err = mfsPrintFormat(&ss, u8"%s/%s", path, data.cFileName);
				if (err != MF_ERROR_OKAY)
					return err;
				mfsDestroyLocalStringStream(&ss);

				*first = file;
				first = &(*first)->next;

				err = mffFillWindowsFiles(archive, allocator, realPath, file->path, file, &file->first);
				if (err != MF_ERROR_OKAY)
					return err;
			}
			// If is file
			else
			{
				mffFolderFile* file;

				err = mfmAllocate(allocator, &file, sizeof(mffFolderFile));
				if (err != MF_ERROR_OKAY)
					return err;

				err = mfmInitObject(&file->base.object);
				if (err != MF_ERROR_OKAY)
					return err;

				err = mfmAcquireObject(&file->base.object);
				if (err != MF_ERROR_OKAY)
					return err;

				file->base.object.destructorFunc = &mffDestroyFile;
				file->base.archive = archive;
				file->first = NULL;
				file->next = NULL;
				file->parent = parent;
				file->type = MFF_FILE;

				mfsStringStream ss;
				err = mfsCreateLocalStringStream(&ss, file->path, sizeof(file->path));
				if (err != MF_ERROR_OKAY)
					return err;
				err = mfsPrintFormat(&ss, u8"%s/%s", internalPath, data.cFileName);
				if (err != MF_ERROR_OKAY)
					return err;
				mfsDestroyLocalStringStream(&ss);

				*first = file;
				first = &(*first)->next;
			}
		}
		while (FindNextFile(hFind, &data));
	}
}
#endif

mfError mffCreateFolderArchive(mffArchive ** outArchive, void* allocator, const mfsUTF8CodeUnit* path)
{
	mfError err;
	mffFolderArchive* folderArchive;

	err = mfmAllocate(allocator, &folderArchive, sizeof(mffFolderArchive));
	if (err != MF_ERROR_OKAY)
		return err;

	err = mfmInitObject(&folderArchive->base.object);
	if (err != MF_ERROR_OKAY)
		return err;
	folderArchive->base.object.destructorFunc = &mffDestroyFolderArchive;
	folderArchive->allocator = allocator;
	strcpy(folderArchive->path, path);
	strcpy(folderArchive->base.name, mffGetPathName(path));

	folderArchive->base.getFile = &mffArchiveGetFile;
	folderArchive->base.getFileType = &mffArchiveGetFileType;
	folderArchive->base.getFirstFile = &mffArchiveGetFirstFile;
	folderArchive->base.getNextFile = &mffArchiveGetNextFile;
	folderArchive->base.getParent = &mffArchiveGetParent;
	folderArchive->base.createDirectory = &mffArchiveCreateDirectory;
	folderArchive->base.deleteDirectory = &mffArchiveDeleteDirectory;
	folderArchive->base.createFile = &mffArchiveCreateFile;
	folderArchive->base.deleteFile = &mffArchiveDeleteFile;
	folderArchive->base.openFile = &mffArchiveOpenFile;

	folderArchive->firstFile = NULL;

	// Create mutex
	err = mftCreateMutex(&folderArchive->mutex, allocator);
	if (err != MF_ERROR_OKAY)
		return err;

	// Get files in folder
#if defined(MAGMA_FRAMEWORK_USE_WINDOWS_FILESYSTEM)
	err = mffFillWindowsFiles(folderArchive, allocator, path, u8"", NULL, &folderArchive->firstFile);
	if (err != MF_ERROR_OKAY)
		return err;
#endif

	*outArchive = folderArchive;
	return MF_ERROR_OKAY;
}

void mffDestroyFolderArchive(void * archive)
{
	if (archive == NULL)
		abort();

	mfError err;
	mffFolderArchive* folderArchive = archive;

	err = mftDestroyMutex(folderArchive->mutex);
	if (err != MF_ERROR_OKAY)
		abort();

	err = mfmDeinitObject(&folderArchive->base.object);
	if (err != MF_ERROR_OKAY)
		abort();
	
	err = mfmDeallocate(folderArchive->allocator, folderArchive);
	if (err != MF_ERROR_OKAY)
		abort();
}
