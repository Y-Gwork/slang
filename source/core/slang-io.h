#ifndef CORE_LIB_IO_H
#define CORE_LIB_IO_H

#include "slang-string.h"
#include "stream.h"
#include "text-io.h"
#include "secure-crt.h"

namespace Slang
{
	class File
	{
	public:
		static bool exists(const Slang::String& fileName);
		static Slang::String readAllText(const Slang::String& fileName);
		static Slang::List<unsigned char> readAllBytes(const Slang::String& fileName);
		static void writeAllText(const Slang::String& fileName, const Slang::String& text);
	};

	class Path
	{
	public:
		static const char kPathDelimiter = '/';

		static String truncateExt(const String& path);
		static String replaceExt(const String& path, const char* newExt);
		static String getFileName(const String& path);
		static String getFileNameWithoutExt(const String& path);
		static String getFileExt(const String& path);
		static String getParentDirectory(const String& path);
		static String combine(const String& path1, const String& path2);
		static String combine(const String& path1, const String& path2, const String& path3);
		static bool createDirectory(const String& path);

            /// Accept either style of delimiter
        SLANG_FORCE_INLINE static bool isDelimiter(char c) { return c == '/' || c == '\\'; }

            /// True if the element appears to be a drive specification (where element is the prefix to a path that isn't a directory)
            /// @param pathPrefix The path prefix to test if it's a drive specification
        static bool isDriveSpecification(const UnownedStringSlice& pathPrefix);

            /// Splits the path into it's individual bits
        static void split(const UnownedStringSlice& path, List<UnownedStringSlice>& splitOut);
            /// Strips .. and . as much as it can 
        static String simplify(const UnownedStringSlice& path);
        static String simplify(const String& path) { return simplify(path.getUnownedSlice()); }

            /// Returns true if a path contains a . or ..
        static bool isRelative(const UnownedStringSlice& path);
        static bool isRelative(const String& path) { return isRelative(path.getUnownedSlice()); }

            /// Determines the type of file at the path
            /// @param path The path to test
            /// @param outPathType Holds the object type at the path on success
            /// @return SLANG_OK on success
        static SlangResult getPathType(const String& path, SlangPathType* outPathType);

            /// Determines the canonical equivalent path to path.
            /// The path returned should reference the identical object - and two different references to the same path should return the same canonical path
            /// @param path Path to get the canonical path for
            /// @param outCanonicalPath The canonical path for 'path' is call is successful
            /// @return SLANG_OK on success
        static SlangResult getCanonical(const String& path, String& outCanonicalPath);

            /// Returns the executable path
            /// @return The path in platform native format. Returns empty string if failed.
        static String getExecutablePath();

	};
}

#endif
