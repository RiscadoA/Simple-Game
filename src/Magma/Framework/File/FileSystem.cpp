#include "FileSystem.hpp"
#include "Exception.hpp"
#include "../ErrorString.h"

void Magma::Framework::File::RegisterArchive(Archive archive, const mfsUTF8CodeUnit* name)
{
	mfError err;
	err = mffRegisterArchive((mffArchive*)&archive.Get(), name);
	if (err != MF_ERROR_OKAY)
		throw FileSystemError(mfErrorToString(err));
}

void Magma::Framework::File::UnregisterArchive(Archive archive)
{
	mfError err;
	err = mffUnregisterArchive((mffArchive*)&archive.Get());
	if (err != MF_ERROR_OKAY)
		throw FileSystemError(mfErrorToString(err));
}

Magma::Framework::File::FileType Magma::Framework::File::File::GetType()
{
	mfError err;
	mffEnum type;
	err = mffGetFileType((mffFile*)&this->Get(), &type);
	if (err != MF_ERROR_OKAY)
		throw FileSystemError(mfErrorToString(err));
	return static_cast<FileType>(type);
}

Magma::Framework::File::File Magma::Framework::File::File::GetFirst()
{
	mfError err;
	mffFile* file;
	err = mffGetFirstFile(&file, (mffFile*)&this->Get());
	if (err != MF_ERROR_OKAY)
		throw FileSystemError(mfErrorToString(err));
	return file;
}

Magma::Framework::File::File Magma::Framework::File::File::GetNext()
{
	mfError err;
	mffFile* file;
	err = mffGetNextFile(&file, (mffFile*)&this->Get());
	if (err != MF_ERROR_OKAY)
		throw FileSystemError(mfErrorToString(err));
	return file;
}

Magma::Framework::File::File Magma::Framework::File::File::GetParent()
{
	mfError err;
	mffFile* file;
	err = mffGetParent(&file, (mffFile*)&this->Get());
	if (err != MF_ERROR_OKAY)
		throw FileSystemError(mfErrorToString(err));
	return file;
}

Magma::Framework::File::File Magma::Framework::File::GetFile(const mfsUTF8CodeUnit * path)
{
	mfError err;
	mffFile* file;
	err = mffGetFile(&file, path);
	if (err == MFF_ERROR_FILE_NOT_FOUND)
		return NULL;
	if (err != MF_ERROR_OKAY)
		throw FileSystemError(mfErrorToString(err));
	return file;
}

Magma::Framework::File::File Magma::Framework::File::CreateDirectory(const mfsUTF8CodeUnit * path)
{
	mfError err;
	mffDirectory* dir;
	err = mffCreateDirectory(&dir, path);
	if (err != MF_ERROR_OKAY && err != MFF_ERROR_ALREADY_EXISTS)
		throw FileSystemError(mfErrorToString(err));
	return dir;
}

void Magma::Framework::File::DeleteDirectory(File & dir)
{
	mfError err;
	mffDirectory* d = (mffDirectory*)dir.GetNoChecks();
	dir.Release();
	err = mffDeleteDirectory(d);
	if (err != MF_ERROR_OKAY)
		throw FileSystemError(mfErrorToString(err));
}

Magma::Framework::File::File Magma::Framework::File::CreateFile(const mfsUTF8CodeUnit * path)
{
	mfError err;
	mffFile* f;
	err = mffCreateFile(&f, path);
	if (err != MF_ERROR_OKAY && err != MFF_ERROR_ALREADY_EXISTS)
		throw FileSystemError(mfErrorToString(err));
	return f;
}

void Magma::Framework::File::DeleteFile(File & file)
{
	mfError err;
	mffFile* f = (mffFile*)file.GetNoChecks();
	file.Release();
	err = mffDeleteFile(f);
	if (err != MF_ERROR_OKAY)
		throw FileSystemError(mfErrorToString(err));
}

Magma::Framework::String::Stream Magma::Framework::File::OpenFile(File file, FileMode mode)
{
	mfError err;
	mfsStream* stream;
	err = mffOpenFile(&stream, (mffFile*)&file.Get(), static_cast<mffEnum>(mode));
	if (err != MF_ERROR_OKAY)
		throw FileSystemError(mfErrorToString(err));
	return stream;
}