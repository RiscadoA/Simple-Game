#include "DefaultArchive.h"
#include "Config.h"

#include "../String/StringStream.h"
#include "../Memory/Allocator.h"

#ifdef MAGMA_FRAMEWORK_USE_WINDOWS_FILESYSTEM
#include <Windows.h>
#else
#error MAGMA_FRAMEWORK_USE_WINDOWS_FILESYSTEM is required for now
#endif

typedef struct
{
	mffFile base;
} mffDefaultFile;

typedef struct
{
	mffArchive base;
} mffDefaultArchive;

typedef struct
{
	mfsStream base;
	void* allocator;
} mffDefaultFileStream;

static void mffDefaultArchiveDestroyFile(void* file)
{

}

static mfError mffDefaultArchiveOpenFile(mfsStream** stream, mffFile* file, mfmU32 mode)
{
	if (stream == NULL || file == NULL || (mode != MFF_READ && mode != MFF_WRITE))
		return MFF_ERROR_INVALID_ARGUMENTS;

	/* /test/text.txt */

	/*mffDefaultArchive* arc = archive;
	mfError err;
	mffDefaultFileStream* fs;

	mfsUTF8CodeUnit absPath[MFF_PATH_MAX_SIZE];
	strcpy(absPath, arc->base.path);
	const mfsUTF8CodeUnit* it = path;
	while (*it != '\0')
	{
	if (*it == '/')

	++it;
	}

	err = mfmAllocate(arc->base.base.allocator, &fs, sizeof(mffDefaultFileStream));
	if (err != MF_ERROR_OKAY)
	return err;

	err = mfmInitObject(&fs->base.object);
	if (err != MF_ERROR_OKAY)
	return err;

	fs->allocator = arc->base.base.allocator;
	fs->base.buffer = NULL;
	fs->base.bufferSize = 0;
	fs->base.object;*/

	// TO DO

	return MF_ERROR_OKAY;
}


static mfError mffDefaultArchiveCreateFile(mffArchive* archive, mffFile** outFile, const mfsUTF8CodeUnit* name)
{
	if (archive == NULL || name == NULL)
		return MFF_ERROR_INVALID_ARGUMENTS;

	mffDefaultArchive* arc = archive;
	mfError err;

	// Create file
#if defined(MAGMA_FRAMEWORK_USE_WINDOWS_FILESYSTEM)
	{
		mfsUTF8CodeUnit path[256];
		{
			mfsStringStream ss;
			err = mfsCreateLocalStringStream(&ss, path, sizeof(path));
			if (err != MF_ERROR_OKAY)
				return err;
			err = mfsPutString(&ss, arc->base.path);
			if (err != MF_ERROR_OKAY)
				return err;
			err = mfsPutString(&ss, name);
			if (err != MF_ERROR_OKAY)
				return err;
			mfsDestroyLocalStringStream(&ss);
		}

		HANDLE f = CreateFile(path, 0, 0, NULL, CREATE_NEW, 0, NULL);
		if (f == INVALID_HANDLE_VALUE)
			return MFF_ERROR_INTERNAL_ERROR;
		GetLastError();
		CloseHandle(f);
	}
#endif

	mffFile* file;

	// Create archive file
	err = mfmAllocate(arc->base.base.allocator, &file, sizeof(mffFile));
	if (err != MF_ERROR_OKAY)
		return err;

	err = mfmInitObject(&file->object);
	if (err != MF_ERROR_OKAY)
		return err;
	file->object.destructorFunc = &mffDefaultArchiveDestroyFile;

	file->allocator = arc->base.base.allocator;
	file->archive = arc;

	if (strlen(name) >= MFF_FILE_NAME_MAX_SIZE)
	{
		memcpy(file->name, name, MFF_FILE_NAME_MAX_SIZE - 1);
		file->name[MFF_FILE_NAME_MAX_SIZE - 1] = '\0';
	}
	else
		strcpy(file->name, name);

	file->nextFile = NULL;
	file->type = MFF_FILE;

	// Add new file to archive
	if (arc->base.firstFile == NULL)
		arc->base.firstFile = file;
	else
	{
		mffFile* prevFile = arc->base.firstFile;
		while (prevFile->nextFile != NULL)
			prevFile = prevFile->nextFile;
		prevFile->nextFile = file;
	}

	return MF_ERROR_OKAY;
}

static void mffDefaultArchiveCloseFile(void* stream)
{
	return MF_ERROR_OKAY;
}

void mffDestroyDefaultArchive(void * archive)
{
	if (archive == NULL)
		abort();

	mffDefaultArchive* arc = archive;
	mfError err;

	mffFile* file = arc->base.firstFile;
	while (file != NULL)
	{
		mffFile* next = file->nextFile;
		file->object.destructorFunc(file);
		file = next;
	}

	err = mfmDestroyObject(&arc->base.base.object);
	if (err != MF_ERROR_OKAY)
		abort();

	err = mfmDeallocate(arc->base.base.allocator, arc);
	if (err != MF_ERROR_OKAY)
		abort();
}

mfError mffCreateDefaultArchive(mffArchive ** archive, const mfsUTF8CodeUnit * path, void * allocator)
{
	if (archive == NULL || path == NULL)
		return MFF_ERROR_INVALID_ARGUMENTS;

	mfError err;

	mffDefaultArchive* arc = NULL;
	err = mfmAllocate(allocator, &arc, sizeof(mffDefaultArchive));
	if (err != MF_ERROR_OKAY)
		return err;

	err = mfmInitObject(&arc->base.base.object);
	if (err != MF_ERROR_OKAY)
		return err;
	arc->base.base.object.destructorFunc = &mffDestroyDefaultArchive;

	arc->base.base.allocator = allocator;
	arc->base.base.archive = NULL;

	const mfsUTF8CodeUnit* name = mffGetPathName(path);
	if (name != NULL)
	{
		strcpy(arc->base.base.name, name);
		for (mfmU64 i = 0; i < MFF_FILE_NAME_MAX_SIZE; ++i)
			if (arc->base.base.name[i] == '/')
				arc->base.base.name[i] = '\0';
			
	}
	else
		arc->base.base.name[0] = '\0';
	strcpy(arc->base.path, path);

	arc->base.base.nextFile = NULL;
	arc->base.firstFile = NULL;
	arc->base.openFile = &mffDefaultArchiveOpenFile;
	arc->base.createFile = &mffDefaultArchiveCreateFile;
	arc->base.base.type = MFF_ARCHIVE;

	*archive = arc;

	// Get all files and archives in this archive
#if defined(MAGMA_FRAMEWORK_USE_WINDOWS_FILESYSTEM)
	{

		WIN32_FIND_DATA data;
		HANDLE hFind = INVALID_HANDLE_VALUE;
		
		// Get search path
		{
			mfsUTF8CodeUnit searchPath[MFF_PATH_MAX_SIZE];
			mfsStringStream ss;
			err = mfsCreateLocalStringStream(&ss, searchPath, sizeof(searchPath));
			if (err != MF_ERROR_OKAY)
				return err;
			err = mfsPutString(&ss, arc->base.path);
			if (err != MF_ERROR_OKAY)
				return err;
			err = mfsPutString(&ss, u8"*.*");
			if (err != MF_ERROR_OKAY)
				return err;
			mfsDestroyLocalStringStream(&ss);
			hFind = FindFirstFile(searchPath, &data);
		}

		if (hFind != INVALID_HANDLE_VALUE)
		{
			do
			{
				if (strcmp(data.cFileName, u8".") == 0 ||
					strcmp(data.cFileName, u8"..") == 0)
					continue;

				mffFile* file = NULL;

				// Archive
				if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				{
					mfsUTF8CodeUnit archivePath[MFF_PATH_MAX_SIZE];

					// Get path
					{
						mfsStringStream ss;
						err = mfsCreateLocalStringStream(&ss, archivePath, sizeof(archivePath));
						if (err != MF_ERROR_OKAY)
							return err;
						err = mfsPutString(&ss, arc->base.path);
						if (err != MF_ERROR_OKAY)
							return err;
						err = mfsPutString(&ss, data.cFileName);
						if (err != MF_ERROR_OKAY)
							return err;
						err = mfsPutString(&ss, u8"/");
						if (err != MF_ERROR_OKAY)
							return err;
						mfsDestroyLocalStringStream(&ss);
					}

					// Create archive
					err = mffCreateArchive(&file, archivePath, allocator);
					if (err != MF_ERROR_OKAY)
						return err;
				}
				// File
				else
				{
					// Create file
					err = mfmAllocate(allocator, &file, sizeof(mffFile));
					if (err != MF_ERROR_OKAY)
						return err;

					err = mfmInitObject(&file->object);
					if (err != MF_ERROR_OKAY)
						return err;
					file->object.destructorFunc = &mffDefaultArchiveDestroyFile;

					file->allocator = allocator;
					file->archive = arc;

					if (strlen(data.cFileName) >= MFF_FILE_NAME_MAX_SIZE)
					{
						memcpy(file->name, data.cFileName, MFF_FILE_NAME_MAX_SIZE - 1);
						file->name[MFF_FILE_NAME_MAX_SIZE - 1] = '\0';
					}
					else
						strcpy(file->name, data.cFileName);

					file->nextFile = NULL;
					file->type = MFF_FILE;
				}

				// Add new file to archive
				if (arc->base.firstFile == NULL)
					arc->base.firstFile = file;
				else
				{
					mffFile* prevFile = arc->base.firstFile;
					while (prevFile->nextFile != NULL)
						prevFile = prevFile->nextFile;
					prevFile->nextFile = file;
				}
			} while (FindNextFile(hFind, &data));
			FindClose(hFind);
		}
	}
#endif

	return MF_ERROR_OKAY;
}
