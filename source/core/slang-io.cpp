#include "slang-io.h"
#include "exception.h"

#ifndef __STDC__
#   define __STDC__ 1
#endif

#include <sys/stat.h>

#ifdef _WIN32
#   include <direct.h>

#   define WIN32_LEAN_AND_MEAN
#   define VC_EXTRALEAN
#   include <Windows.h>
#endif

#if defined(__linux__) || defined(__CYGWIN__)
#   include <unistd.h>
#endif

#if SLANG_APPLE_FAMILY
#   include <mach-o/dyld.h>
#endif

#include <limits.h> /* PATH_MAX */
#include <stdio.h>
#include <stdlib.h>

namespace Slang
{
	bool File::exists(const String& fileName)
	{
#ifdef _WIN32
		struct _stat32 statVar;
		return ::_wstat32(((String)fileName).ToWString(), &statVar) != -1;
#else
		struct stat statVar;
		return ::stat(fileName.Buffer(), &statVar) == 0;
#endif
	}

	String Path::truncateExt(const String& path)
	{
		UInt dotPos = path.LastIndexOf('.');
		if (dotPos != -1)
			return path.SubString(0, dotPos);
		else
			return path;
	}
	String Path::replaceExt(const String& path, const char* newExt)
	{
		StringBuilder sb(path.Length()+10);
		UInt dotPos = path.LastIndexOf('.');
		if (dotPos == -1)
			dotPos = path.Length();
		sb.Append(path.Buffer(), dotPos);
		sb.Append('.');
		sb.Append(newExt);
		return sb.ProduceString();
	}

    static UInt findLastSeparator(String const& path)
    {
		UInt slashPos = path.LastIndexOf('/');
        UInt backslashPos = path.LastIndexOf('\\');

        if (slashPos == -1) return backslashPos;
        if (backslashPos == -1) return slashPos;

        UInt pos = slashPos;
        if (backslashPos > slashPos)
            pos = backslashPos;

        return pos;
    }

	String Path::getFileName(const String& path)
	{
        UInt pos = findLastSeparator(path);
        if (pos != -1)
        {
            pos = pos + 1;
            return path.SubString(pos, path.Length() - pos);
        }
        else
        {
            return path;
        }
	}
	String Path::getFileNameWithoutExt(const String& path)
	{
        String fileName = getFileName(path);
		UInt dotPos = fileName.LastIndexOf('.');
		if (dotPos == -1)
            return fileName;
		return fileName.SubString(0, dotPos);
	}
	String Path::getFileExt(const String& path)
	{
		UInt dotPos = path.LastIndexOf('.');
		if (dotPos != -1)
			return path.SubString(dotPos+1, path.Length()-dotPos-1);
		else
			return "";
	}
	String Path::getParentDirectory(const String& path)
	{
        UInt pos = findLastSeparator(path);
		if (pos != -1)
			return path.SubString(0, pos);
		else
			return "";
	}
	String Path::combine(const String& path1, const String& path2)
	{
		if (path1.Length() == 0) return path2;
		StringBuilder sb(path1.Length()+path2.Length()+2);
		sb.Append(path1);
		if (!path1.EndsWith('\\') && !path1.EndsWith('/'))
			sb.Append(kPathDelimiter);
		sb.Append(path2);
		return sb.ProduceString();
	}
	String Path::combine(const String& path1, const String& path2, const String& path3)
	{
		StringBuilder sb(path1.Length()+path2.Length()+path3.Length()+3);
		sb.Append(path1);
		if (!path1.EndsWith('\\') && !path1.EndsWith('/'))
			sb.Append(kPathDelimiter);
		sb.Append(path2);
		if (!path2.EndsWith('\\') && !path2.EndsWith('/'))
			sb.Append(kPathDelimiter);
		sb.Append(path3);
		return sb.ProduceString();
	}

    /* static */ bool Path::isDriveSpecification(const UnownedStringSlice& element)
    {
        switch (element.size())
        {
            case 0:     
            {
                // We'll just assume it is
                return true;
            }
            case 2:
            {
                // Look for a windows like drive spec
                const char firstChar = element[0]; 
                return element[1] == ':' && ((firstChar >= 'a' && firstChar <= 'z') || (firstChar >= 'A' && firstChar <= 'Z'));
            }
            default:    return false;
        }
        
    }

    /* static */void Path::split(const UnownedStringSlice& path, List<UnownedStringSlice>& splitOut)
    {
        splitOut.Clear();

        const char* start = path.begin();
        const char* end = path.end();

        while (start < end)
        {
            const char* cur = start;
            // Find the split
            while (cur < end && !isDelimiter(*cur)) cur++;

            splitOut.Add(UnownedStringSlice(start, cur));
           
            // Next
            start = cur + 1;
        }

        // Okay if the end is empty. And we aren't with a spec like // or c:/ , then drop the final slash 
        if (splitOut.Count() > 1 && splitOut.Last().size() == 0)
        {
            if (splitOut.Count() == 2 && isDriveSpecification(splitOut[0]))
            {
                return;
            }
            // Remove the last 
            splitOut.RemoveLast();
        }
    }

    /* static */bool Path::isRelative(const UnownedStringSlice& path)
    {
        List<UnownedStringSlice> splitPath;
        split(path, splitPath);

        for (const auto& cur : splitPath)
        {
            if (cur == "." || cur == "..")
            {
                return true;
            }
        }
        return false;
    }

    /* static */String Path::simplify(const UnownedStringSlice& path)
    {
        List<UnownedStringSlice> splitPath;
        split(path, splitPath);

        // Strictly speaking we could do something about case on platforms like window, but here we won't worry about that
        for (int i = 0; i < int(splitPath.Count()); i++)
        {
            const UnownedStringSlice& cur = splitPath[i];
            if (cur == "." && splitPath.Count() > 1)
            {
                // Just remove it 
                splitPath.RemoveAt(i);
                i--;
            }
            else if (cur == ".." && i > 0)
            {
                // Can we remove this and the one before ?
                UnownedStringSlice& before = splitPath[i - 1];
                if (before == ".." || (i == 1 && isDriveSpecification(before)))
                {
                    // Can't do it
                    continue;
                }
                splitPath.RemoveRange(i - 1, 2);
                i -= 2;
            }
        }

        // If its empty it must be .
        if (splitPath.Count() == 0)
        {
            splitPath.Add(UnownedStringSlice::fromLiteral("."));
        }
   
        // Reconstruct the string
        StringBuilder builder;
        for (int i = 0; i < int(splitPath.Count()); i++)
        {
            if (i > 0)
            {
                builder.Append(kPathDelimiter);
            }
            builder.Append(splitPath[i]);
        }

        return builder;
    }

	bool Path::createDirectory(const String& path)
	{
#if defined(_WIN32)
		return _wmkdir(path.ToWString()) == 0;
#else 
		return mkdir(path.Buffer(), 0777) == 0;
#endif
	}

    /* static */SlangResult Path::getPathType(const String& path, SlangPathType* pathTypeOut)
    {
#ifdef _WIN32
        // https://msdn.microsoft.com/en-us/library/14h5k7ff.aspx
        struct _stat32 statVar;
        if (::_wstat32(String(path).ToWString(), &statVar) == 0)
        {
            if (statVar.st_mode & _S_IFDIR)
            {
                *pathTypeOut = SLANG_PATH_TYPE_DIRECTORY;
                return SLANG_OK;
            }
            else if (statVar.st_mode & _S_IFREG)
            {
                *pathTypeOut = SLANG_PATH_TYPE_FILE;
                return SLANG_OK;
            }
            return SLANG_FAIL;
        }

        return SLANG_E_NOT_FOUND;
#else
        struct stat statVar;
        if (::stat(path.Buffer(), &statVar) == 0)
        {
            if (S_ISDIR(statVar.st_mode))
            {
                *pathTypeOut = SLANG_PATH_TYPE_DIRECTORY;
                return SLANG_OK;
            }
            if (S_ISREG(statVar.st_mode))
            {
                *pathTypeOut = SLANG_PATH_TYPE_FILE;
                return SLANG_OK;
            }
            return SLANG_FAIL;
        }

        return SLANG_E_NOT_FOUND;
#endif
    }


    /* static */SlangResult Path::getCanonical(const String& path, String& canonicalPathOut)
    {
#if defined(_WIN32)
        // https://msdn.microsoft.com/en-us/library/506720ff.aspx
        wchar_t* absPath = ::_wfullpath(nullptr, path.ToWString(), 0);
        if (!absPath)
        {
            return SLANG_FAIL;
        }  

        canonicalPathOut =  String::FromWString(absPath);
        ::free(absPath);
        return SLANG_OK;
#else
        // http://man7.org/linux/man-pages/man3/realpath.3.html
        char* canonicalPath = ::realpath(path.begin(), nullptr);
        if (canonicalPath)
        {
            canonicalPathOut = canonicalPath;
            ::free(canonicalPath);
            return SLANG_OK;
        }
        return SLANG_FAIL;
#endif
    }

    /// Gets the path to the executable that was invoked that led to the current threads execution
    /// If run from a shared library/dll will be the path of the executable that loaded said library
    /// @param outPath Pointer to buffer to hold the path.
    /// @param ioPathSize Size of the buffer to hold the path (including zero terminator). 
    /// @return SLANG_OK on success, SLANG_E_BUFFER_TOO_SMALL if buffer is too small. If ioPathSize is changed it will be the required size
    static SlangResult _calcExectuablePath(char* outPath, size_t* ioSize)
    {
        SLANG_ASSERT(ioSize);
        const size_t bufferSize = *ioSize;
        SLANG_ASSERT(bufferSize > 0);

#if SLANG_WINDOWS_FAMILY
        // https://docs.microsoft.com/en-us/windows/desktop/api/libloaderapi/nf-libloaderapi-getmodulefilenamea
        
        DWORD res = ::GetModuleFileNameA(::GetModuleHandle(nullptr), outPath, DWORD(bufferSize));
        // If it fits it's the size not including terminator. So must be less than bufferSize
        if (res < bufferSize)
        {
            return SLANG_OK;
        }
        return SLANG_E_BUFFER_TOO_SMALL;
#elif SLANG_LINUX_FAMILY

#   if defined(__linux__) || defined(__CYGWIN__)
        // https://linux.die.net/man/2/readlink
        // Mark last byte with 0, so can check overrun
        ssize_t resSize = ::readlink("/proc/self/exe", outPath, bufferSize);
        if (resSize < 0)
        {
            return SLANG_FAIL;
        }
        if (resSize >= bufferSize)
        {
            return SLANG_E_BUFFER_TOO_SMALL;
        }
        // Zero terminate
        outPath[resSize - 1] = 0;
        return SLANG_OK;
#   else        
        String text = Slang::File::readAllText("/proc/self/maps");
        UInt startIndex = text.IndexOf('/');
        if (startIndex == UInt(-1))
        {
            return SLANG_FAIL;
        }
        UInt endIndex = text.IndexOf("\n", startIndex);
        endIndex = (endIndex == UInt(-1)) ? text.Length() : endIndex;

        auto path = text.SubString(startIndex, endIndex - startIndex);

        if (path.getLength() < bufferSize)
        {
            ::memcpy(outPath, path.begin(), path.getLength());
            outPath[path.getLength()] = 0;
            return SLANG_OK;
        }

        *ioSize = path.getLength() + 1;
        return SLANG_E_BUFFER_TOO_SMALL;
#   endif

#elif SLANG_APPLE_FAMILY
        // https://developer.apple.com/library/archive/documentation/System/Conceptual/ManPages_iPhoneOS/man3/dyld.3.html
        uint32_t size = uint32_t(*ioSize);
        switch (_NSGetExecutablePath(outPath, &size))
        {
            case 0:           return SLANG_OK;
            case -1:
            {
                *ioSize = size;
                return SLANG_E_BUFFER_TOO_SMALL;
            }
            default: break;
        }
        return SLANG_FAIL;
#else
        return SLANG_E_NOT_IMPLEMENTED;
#endif
    }

    static String _getExecutablePath()
    {
        List<char> buffer;
        // Guess an initial buffer size
        buffer.SetSize(1024);

        while (true)
        {
            const size_t size = buffer.Count();
            size_t bufferSize = size;
            SlangResult res = _calcExectuablePath(buffer.Buffer(), &bufferSize);

            if (SLANG_SUCCEEDED(res))
            {
                return String(buffer.Buffer());
            }

            if (res != SLANG_E_BUFFER_TOO_SMALL)
            {
                // Couldn't determine the executable string
                return String();
            }

            // If bufferSize changed it should be the exact fit size, else we just make the buffer bigger by a guess (50% bigger)
            bufferSize = (bufferSize > size) ? bufferSize : (bufferSize + bufferSize / 2);
            buffer.SetSize(bufferSize);
        }
    }

    /* static */String Path::getExecutablePath()
    {
        static String executablePath = _getExecutablePath();
        return executablePath;
    }

	Slang::String File::readAllText(const Slang::String& fileName)
	{
		StreamReader reader(new FileStream(fileName, FileMode::Open, FileAccess::Read, FileShare::ReadWrite));
		return reader.ReadToEnd();
	}

	Slang::List<unsigned char> File::readAllBytes(const Slang::String& fileName)
	{
		RefPtr<FileStream> fs = new FileStream(fileName, FileMode::Open, FileAccess::Read, FileShare::ReadWrite);
		List<unsigned char> buffer;
		while (!fs->IsEnd())
		{
			unsigned char ch;
			int read = (int)fs->Read(&ch, 1);
			if (read)
				buffer.Add(ch);
			else
				break;
		}
		return _Move(buffer);
	}

	void File::writeAllText(const Slang::String& fileName, const Slang::String& text)
	{
		StreamWriter writer(new FileStream(fileName, FileMode::Create));
		writer.Write(text);
	}

}

