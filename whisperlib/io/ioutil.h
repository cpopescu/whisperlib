// Copyright (c) 2009, Whispersoft s.r.l.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
// * Neither the name of Whispersoft s.r.l. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Author: Catalin Popescu

#ifndef __WHISPERLIB_IO_IOUTIL_H__
#define __WHISPERLIB_IO_IOUTIL_H__

// #if defined(__APPLE__) && __WORDSIZE == 64
#define _DARWIN_USE_64_BIT_INODE
// #endif

#include <map>
#include <string>
#include <vector>
#include "whisperlib/base/types.h"

#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

namespace whisper {
namespace re { class RE; }

namespace io {

bool IsDir(const char* name);
bool IsDir(const std::string& name);
bool IsReadableFile(const char* name);
bool IsReadableFile(const std::string& name);
bool IsSymlink(const std::string& path);
bool Exists(const char* path);
bool Exists(const std::string& path);
int64 GetFileSize(const char* name);
int64 GetFileSize(const std::string& name);
// Returns file timestamp in seconds!
int64 GetFileMtime(const char* name);
int64 GetFileMtime(const std::string& name);
bool SetFileMtime(const char* name, int64 t);
bool SetFileMtime(const std::string& name, int64 t);

// List a directory, possibly looking into subdirectories, filter by regex.
// Symlinks are not followed, and completely ignored.
// list_attr: a combinations of 1 or more DirListAttributes
// out: Returned entries are relative to 'dir' (they do not contain the 'dir').
// Returns success.
enum DirListAttributes {
  // return regular files & symlinks
  LIST_FILES = 0x01,
  // return directories
  LIST_DIRS = 0x02,
  // return everything (files, dirs, symlinks, sockets, pipes,..)
  LIST_EVERYTHING = 0x0f,
  // look into subdirectories
  LIST_RECURSIVE = 0x80,
};
bool DirList(const std::string& dir, uint32 list_attr,
             const re::RE* regex,
             std::vector<std::string>* out);


bool CreateRecursiveDirs(
  const char* dirname,
  mode_t mode = S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
bool CreateRecursiveDirs(
  const std::string& dirname,
  mode_t mode = S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);


// Utility for finding based on a path in a map of paths and structures.
// We return C() when not found, and we update path to reflect the path
// that triggerd the found object
template<class C>
    C FindPathBased(const std::map<std::string, C>* data,
                    std::string& path, char separator = PATH_SEPARATOR) {
  if ( data->empty() ) {
    return C();
  }
  std::string to_search(path);
  while ( !to_search.empty() ) {
    const typename std::map<std::string, C>::const_iterator
        it = data->find(to_search);
    if ( it != data->end() ) {
      path = to_search;
      return it->second;
    }
    if ( !to_search.empty() && to_search[to_search.size() - 1] == separator ) {
      to_search.resize(to_search.size() - 1);
    } else {
      const size_t pos_slash = to_search.rfind(separator);
      if ( pos_slash != std::string::npos ) {
        to_search.resize(pos_slash + 1);
      } else {
        to_search.resize(0);
      }
    }
  }
  const typename std::map<std::string, C>::const_iterator it
      = data->find(to_search);
  if ( it != data->end() ) {
    path = to_search;
    return it->second;
  }
  return C();
}

// Same as above, but adds all matching paths
template<class C>
int FindAllPathBased(const std::map<std::string, C>* data,
                     const std::string& path,
                     std::map<std::string, C>* matches,
                     char separator = PATH_SEPARATOR) {
  if ( data->empty() ) {
    return 0;
  }
  std::string to_search(path);
  int num_found = 0;
  while ( !to_search.empty() ) {
    const typename std::map<std::string, C>::const_iterator it =
        data->find(to_search);
    if ( it != data->end() ) {
      ++num_found;
      matches->insert(make_pair(to_search, it->second));
    }
    if ( !to_search.empty() && to_search[to_search.size() - 1] == separator ) {
      to_search.resize(to_search.size() - 1);
    } else {
      const size_t pos_slash = to_search.rfind(separator);
      if ( pos_slash != std::string::npos ) {
        to_search.resize(pos_slash + 1);
      } else {
        to_search.resize(0);
      }
    }
  }
  const typename std::map<std::string, C>::const_iterator it =
      data->find(to_search);
  if ( it != data->end() ) {
    ++num_found;
    matches->insert(make_pair(to_search, it->second));
  }
  return num_found;
}


// A tool to easy remove files.
// Returns success state. Failure reason can be found in GetLastSystemError().
bool Rm(const std::string& path);

// Easy remove empty directory.
// Fails if the target directory is not empty.
bool Rmdir(const std::string& path);

// Removes all files under the given directory (NOTE: just the files if rm_dirs is false)
bool RmFilesUnder(const std::string& path,
                  const re::RE* pattern = NULL, bool rm_dirs = false);

// Move file or directory to destination directory.
// If a directory with the same name already exists, the source directory
// will be integrated in the destionation directory. See function Rename(..) .
//
// e.g. path = "/tmp/abc.avi"
//      dir = "/home/my_folder"
//      Will move file "abc.avi" to "/home/my_folder/abc.avi"
//      and "/tmp/abc.avi" no longer exists.
bool Mv(const std::string& path, const std::string& dir, bool overwrite);

// Renames files
bool Rename(const std::string& old_path,
            const std::string& new_path,
            bool overwrite);

// Creates a directory on disk.
// recursive: if true => creates all directories on path "dir"
//            if false => creates only "dir"; it's parent must exist.
bool Mkdir(const std::string& dir,
           bool recursive = false,
           mode_t mode = S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);

// Prepends the current working directory to the provided path, if
// necessary, to obtain a fully qualified path
std::string MakeAbsolutePath(const char* path);

// Returns the last of a set of numbered files placed in the specified
// directory. The number of the file can be strtoul from the last
// file_num_size chars of the filename.
// Returns:
//   -2 : error : cannot open directory, or file does not end in number
//   -1 : error : no file found
//  >=0 : success
int32 GetLastNumberedFile(const std::string& dir, re::RE* re, int32 file_num_size);

// Reads the given file line by line and returns the read lines.
// comment_char: if not 0 this character marks the start of a line comment (which is ignored)
// trim_spaces: remove spaces on each line
// trim_empty_lines: do not return empty lines
// Returns success status.
bool ReadFileLines(const std::string& filepath,
                   char comment_char,
                   bool trim_spaces,
                   bool trim_empty_lines,
                   std::vector<std::string>* out);

// Copies the file to the destination.
bool Copy(const std::string& source,
          const std::string& dest,
          mode_t mode = S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);

}  // namespace io
}  // namespace whisper


#endif  // __WHISPERLIB_IO_IOUTIL_H__
