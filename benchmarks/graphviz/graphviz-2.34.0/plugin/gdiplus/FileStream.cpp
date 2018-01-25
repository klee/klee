/* $Id$ $Revision$ */
/* vim:set shiftwidth=4 ts=8: */

/*************************************************************************
 * Copyright (c) 2011 AT&T Intellectual Property 
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors: See CVS logs. Details at http://www.graphviz.org/
 *************************************************************************/

#include "FileStream.h"
#include "gvplugin_device.h"

#include "gvplugin_gdiplus.h"



IStream *FileStream::Create(char *name, FILE *file)
{
	return new FileStream(name, file);
}
	
/* IUnknown methods */

HRESULT FileStream::QueryInterface( 
    REFIID riid,
    void **ppvObject)
{
	if (riid == IID_IUnknown)
	{
		*ppvObject = (IUnknown*)this;
		++_ref;
		return S_OK;
	}
	else if (riid == IID_ISequentialStream)
	{
		*ppvObject = (ISequentialStream*)this;
		++_ref;
		return S_OK;
	}
	else if (riid == IID_IStream)
	{
		*ppvObject = (IStream*)this;
		++_ref;
		return S_OK;
	}
	else
	{
		*ppvObject = NULL;
		return E_NOINTERFACE;
	}
}

ULONG FileStream::AddRef()
{
	return ++_ref;
}

ULONG FileStream::Release()
{
	ULONG ref = --_ref;
	if (ref == 0)
		delete this;
	return ref;
}
        
/* ISequentialStream methods */

HRESULT FileStream::Read( 
    void *pv,
    ULONG cb,
    ULONG *pcbRead)
{
	ULONG read = fread(pv, 1, cb, _file);
	if (pcbRead)
		*pcbRead = read;
	return S_OK;
}

HRESULT FileStream::Write( 
    const void *pv,
    ULONG cb,
    ULONG *pcbWritten)
{
	ULONG written = fwrite(pv, 1, cb, _file);
	if (pcbWritten)
		*pcbWritten = written;
	return S_OK;
}

/* IStream methods */

HRESULT FileStream::Seek( 
    LARGE_INTEGER dlibMove,
    DWORD dwOrigin,
    ULARGE_INTEGER *plibNewPosition)
{
	long whence;
	switch (dwOrigin)
	{
	case STREAM_SEEK_SET:
	default:
		whence = SEEK_SET;
		break;
	case STREAM_SEEK_CUR:
		whence = SEEK_CUR;
		break;
	case STREAM_SEEK_END:
		whence = SEEK_END;
		break;
	}
	if (dlibMove.HighPart > 0 || fseek(_file, dlibMove.LowPart, whence) != 0)
		return S_FALSE;
	if (plibNewPosition)
	{
		long pos = ftell(_file);
		if (pos == -1L)
			return S_FALSE;
		
		plibNewPosition->HighPart = 0;
		plibNewPosition->LowPart = pos;
	}
	
	return S_OK;
}

HRESULT FileStream::SetSize( 
    ULARGE_INTEGER libNewSize)
{
	return E_NOTIMPL;
}

HRESULT FileStream::CopyTo( 
    IStream *pstm,
	ULARGE_INTEGER cb,
    ULARGE_INTEGER *pcbRead,
    ULARGE_INTEGER *pcbWritten)
{
	return E_NOTIMPL;
}

HRESULT FileStream::Commit( 
    DWORD grfCommitFlags)
{
	return E_NOTIMPL;
}

HRESULT FileStream::Revert()
{
	return 0;
}

HRESULT FileStream::LockRegion( 
    ULARGE_INTEGER libOffset,
    ULARGE_INTEGER cb,
    DWORD dwLockType)
{
	return E_NOTIMPL;
}

HRESULT FileStream::UnlockRegion( 
    ULARGE_INTEGER libOffset,
    ULARGE_INTEGER cb,
    DWORD dwLockType)
{
	return E_NOTIMPL;
}

HRESULT FileStream::Stat( 
    STATSTG *pstatstg,
    DWORD grfStatFlag)
{
	/* call UNIX fstat on underlying descriptor */
	struct _stat file_stat;
	if (_fstat(_fileno(_file), &file_stat) != 0)
		return S_FALSE;
	
	/* fill in filename, if needed */
	if (grfStatFlag != STATFLAG_NONAME)
	{
		int wide_count = MultiByteToWideChar(CP_UTF8, 0, _name, -1, NULL, 0);
		if (wide_count > 0)
		{
			pstatstg->pwcsName = (LPOLESTR)CoTaskMemAlloc(wide_count * 2);
			MultiByteToWideChar(CP_UTF8, 0, _name, -1, pstatstg->pwcsName, wide_count);
		}
		else
			pstatstg->pwcsName = NULL;
	}
	
	/* fill out rest of STATSTG */
	pstatstg->type = STGTY_STREAM;
	pstatstg->cbSize.QuadPart = file_stat.st_size;
	UnixTimeToFileTime(file_stat.st_mtime, pstatstg->mtime);
	UnixTimeToFileTime(file_stat.st_ctime, pstatstg->ctime);
	UnixTimeToFileTime(file_stat.st_atime, pstatstg->atime);
	pstatstg->grfLocksSupported = 0;
	pstatstg->grfMode = STGM_READWRITE;
	pstatstg->clsid = CLSID_NULL;
	pstatstg->grfStateBits = 0;
	pstatstg->reserved = 0;
	
	return S_OK;
}

HRESULT FileStream::Clone( 
    IStream **ppstm)
{
	return E_NOTIMPL;
}
    
FileStream::FileStream(char *name, FILE *file): _ref(1), _name(name), _file(file)
{
}

void FileStream::UnixTimeToFileTime(time_t unixTime, FILETIME &fileTime)
{
	/* convert Unix time (seconds since 1/1/1970) to Windows filetime (100 nanoseconds since 1/1/1601) */
	LONGLONG ft = (LONGLONG)unixTime * 10000000LL + 116444736000000000LL;
	fileTime.dwLowDateTime = (DWORD)ft;
	fileTime.dwHighDateTime = ft >> 32;
}
