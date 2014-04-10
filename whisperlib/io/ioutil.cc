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

#include <whisperlib/base/core_config.h>

#if defined(MACOSX)
# define _DARWIN_USE_64_BIT_INODE
#endif

#include <dirent.h>

#include <stdio.h>
#include <unistd.h>
#ifdef HAVE_BITS_LIMITS_H
#include <bits/limits.h>
#endif
#include <utime.h>

#include "whisperlib/io/ioutil.h"
#include "whisperlib/base/strutil.h"
#include "whisperlib/base/log.h"
#include "whisperlib/base/core_errno.h"

#if defined(HAVE_STAT64) && !defined(MACOSX)
#define __STAT stat64
#define __LSTAT lstat64
#else
#define __STAT stat
#define __LSTAT lstat
#endif

namespace io {

bool IsDir(const char* name) {
  struct __STAT st;
  if ( 0 != ::__STAT(name, &st) ) {
    return false;
  }
  return S_ISDIR(st.st_mode);
}
bool IsDir(const string& name) {
  return IsDir(name.c_str());
}

bool IsReadableFile(const char* name) {
  struct __STAT st;
  if ( 0 != ::__STAT(name, &st) ) {
    return false;
  }
  return S_ISREG(st.st_mode);
}
bool IsReadableFile(const string& name) {
  return IsReadableFile(name.c_str());
}

bool IsSymlink(const string& path) {
  struct __STAT st;
  // NOTE: ::stat queries the file referenced by link, not the link itself.
  if ( 0 != ::__LSTAT(path.c_str(), &st) ) {
    return false;
  }
  return S_ISLNK(st.st_mode);
}

bool Exists(const char* path) {
  struct __STAT st;
  if ( 0 != ::__LSTAT(path, &st) ) {
    return false;
  }
  return true;
}
bool Exists(const string& path) {
  return Exists(path.c_str());
}

int64 GetFileSize(const char* name) {
  struct __STAT st;
  if ( 0 != ::__STAT(name, &st) ) {
    return -1;
  }
  return st.st_size;
}

int64 GetFileSize(const string& name) {
  return GetFileSize(name.c_str());
}

int64 GetFileMtime(const char* name) {
  struct __STAT st;
  if ( 0 != ::__STAT(name, &st) ) {
    return -1;
  }
  return st.st_mtime;
}

int64 GetFileMtime(const string& name) {
  return GetFileMtime(name.c_str());
}
bool SetFileMtime(const char* name, int64 t) {
    const struct utimbuf times = {time_t(t), time_t(t)};
    return utime(name, &times) == 0;
}

bool SetFileMtime(const string& name, int64 t) {
    return SetFileMtime(name.c_str(), t);
}


bool CreateRecursiveDirs(const char* dirname, mode_t mode) {
  string crt_dir(strutil::NormalizePath(dirname));
  if ( crt_dir.empty() ) return true;
  if ( crt_dir[crt_dir.size() - 1] == '/' ) {
    crt_dir.resize(crt_dir.size() - 1);   // cut any trailing '/'
  }
  vector<string> to_create;
  while ( !crt_dir.empty() ) {
    if ( io::IsDir(crt_dir.c_str()) ) {
      break;
    }
    if ( io::IsReadableFile(crt_dir.c_str()) ) {
      return false;
    }
    to_create.push_back(crt_dir);
    crt_dir = strutil::Dirname(crt_dir);
  }
  for ( int i = to_create.size() - 1; i >= 0; --i ) {
    if ( ::mkdir(to_create[i].c_str(), mode) ) {
      LOG_ERROR << "Error creating directory: " << to_create[i]
                << " : " << GetSystemErrorDescription(errno);
    }
  }
  return true;
}
bool CreateRecursiveDirs(const string& dirname, mode_t mode) {
  return CreateRecursiveDirs(dirname.c_str(), mode);
}


bool Rm(const string& path) {
  struct __STAT s;
  // NOTE: if path is a symbolic link:
  //       - lstat() doesn't follow symbolic links. It returns stats for
  //                 the link itself.
  //       - stat() follows symbolic links and returns stats for the linked file.
  if ( ::__LSTAT(path.c_str(), &s) ) {
    if ( GetLastSystemError() == ENOENT ) {
      return true;
    }
    LOG_ERROR << "lstat failed for path: " << path
              << " error: " << GetLastSystemErrorDescription();
    return false;
  }
  if ( S_ISREG(s.st_mode) || S_ISLNK(s.st_mode) ) {
    DLOG_INFO << "Removing " << (S_ISREG(s.st_mode) ? "file" : "symlink")
              << ": [" << path << "]";
    if ( ::unlink(path.c_str()) ) {
      LOG_ERROR << "unlink failed for path: " << path
                << " error: " << GetLastSystemErrorDescription();
      return false;
    }
    return true;
  }
  if ( S_ISDIR(s.st_mode) ) {
    return Rmdir(path);
  }
  LOG_ERROR << "cannot remove file: [" << path
            << "] unhandled mode: " << s.st_mode;
  return false;
}
bool Rmdir(const string& path) {
  int result = ::rmdir(path.c_str());
  if ( result != 0 ) {
    LOG_ERROR << "::rmdir failed for path: [" << path << "]"
                 " error: " << GetLastSystemErrorDescription();
    return false;
  }
  return true;
}

bool RmFilesUnder(const string& path, const re::RE* pattern, bool all) {
    vector<string> files;
    bool error = false;
    int options = 0;
    if (all) {
        options = io::LIST_DIRS | io::LIST_FILES | io::LIST_RECURSIVE;
    } else {
        options = io::LIST_FILES | io::LIST_RECURSIVE;
    }
    vector<string> dirs;
    if (io::DirList(path, options, pattern, &files)) {
        LOG_INFO << " Removing: " << files.size() << " files.";
        for (int i = 0; i < files.size(); ++i) {
            LOG_INFO << " Removing [" << files[i] << "]";
            const string f(strutil::JoinPaths(path, files[i]));
            if (all && io::IsDir(f)) {
                dirs.push_back(f);
                continue;
            }
            if (!io::Rm(f)) {
                LOG_WARNING << " Error removing: " << files[i];
                error = true;
            }
        }
        // Reverse on dirs
        for (int i = dirs.size() - 1; i >= 0; --i) {
            if (!io::Rmdir(dirs[i])) {
                LOG_WARNING << " Error removing dir: " << dirs[i];
                error = true;
            }
        }
    }
    return !error;
}


bool Mv(const string& path, const string& dir, bool overwrite) {
  return Rename(path,
                strutil::JoinPaths(dir, strutil::Basename(path)),
                overwrite);
}

bool Rename(const string& old_path,
            const string& new_path,
            bool overwrite) {
  const bool old_exists = io::Exists(old_path);
  const bool old_is_file = io::IsReadableFile(old_path);
  const bool old_is_dir = io::IsDir(old_path);
  const bool old_is_symlink = io::IsSymlink(old_path);
  const bool old_is_single = old_is_file || old_is_symlink;
  const string old_type = (old_is_symlink ? "symlink" :
                           old_is_file ? "file" :
                           old_is_dir ? "directory" :
                           "unknown");

  const bool new_exists = io::Exists(new_path);
  const bool new_is_file = io::IsReadableFile(new_path);
  const bool new_is_dir = io::IsDir(new_path);
  const bool new_is_symlink = io::IsSymlink(new_path);
  const bool new_is_single = new_is_file || new_is_symlink;
  const string new_type = (new_is_symlink ? "symlink" :
                           new_is_file ? "file" :
                           new_is_dir ? "directory" :
                           "unknown");


  if ( !old_exists ) {
    LOG_ERROR << "Rename old_path: [" << old_path << "] does not exist";
    return false;
  }
  if ( (old_is_single && new_is_dir) ||
       (old_is_dir && new_is_single) ) {
    LOG_ERROR << "Rename old_path: [" << old_path << "](" << old_type
              << "), new_path: [" << new_path << "](" << new_type
              << ") incompatible types";
    return false;
  }
  if ( new_exists && new_is_single && !overwrite ) {
    LOG_ERROR << "Rename old_path: [" << old_path << "](" << old_type
              << ") , new_path: [" << new_path << "](" << new_type
              << ") cannot overwrite";
    return false;
  }

  // - move file or directory over empty_path
  // - or move file over file
  //
  //  ! Atomically !
  //
  if ( !new_exists || (old_is_single && new_is_single) ) {
    DLOG_INFO << "Renaming [" << old_path << "] to [" << new_path << "]";
    if ( ::rename(old_path.c_str(), new_path.c_str()) ) {
      LOG_ERROR << "::rename failed, old_path: [" << old_path << "]"
                << ", new_path: [" << new_path << "]"
                << ", error: " << GetLastSystemErrorDescription();
      return false;
    }
    return true;
  }
  LOG_ERROR << "Don't know how to rename " << old_path
            << " to " << new_path;
  return false;
}

bool Mkdir(const string& str_dir, bool recursive, mode_t mode) {
  if ( recursive ) {
    return io::CreateRecursiveDirs(str_dir.c_str(), mode);
  }
  const string dir(strutil::NormalizePath(str_dir));
  const int result = ::mkdir(dir.c_str(), mode);
  if ( result != 0 && GetLastSystemError() != EEXIST ) {
    LOG_ERROR << "Failed to create dir: [" << dir << "] error: "
              << GetLastSystemErrorDescription();
    return false;
  }
  return true;
}

string MakeAbsolutePath(const char* path) {
  const string strPath(strutil::StrTrim(string(path)));

  // if "path" is already absolute, then there's nothing more to do
  if ( strPath[0] == '/' ) {
    return strPath;
  }

  // "path" is relative.

  // find current working directory
  char cwd[1024] = { 0, };
  char* result = getcwd(cwd, sizeof(cwd));
  if ( !result ) {
    LOG_ERROR << " getcwd(..) failed: "
              << GetLastSystemErrorDescription();
    return strPath;
  }

  // append relative "path" to current working directory
  const string full_path(strutil::NormalizePath(
                             string(cwd) + "/" + strPath));
  return full_path;
}



// Per:  http://womble.decadentplace.org.uk/readdir_r-advisory.html
// * Calculate the required buffer size (in bytes) for directory       *
// * entries read from the given directory handle.  Return -1 if this  *
// * this cannot be done.                                              *
// *                                                                   *
// * This code does not trust values of NAME_MAX that are less than    *
// * 255, since some systems (including at least HP-UX) incorrectly    *
// * define it to be a smaller value.                                  *
// *                                                                   *
// * If you use autoconf, include fpathconf and dirfd in your          *
// * AC_CHECK_FUNCS list.  Otherwise use some other method to detect   *
// * and use them where available.                                     *
size_t dirent_buf_size(DIR* dirp) {
    long name_max;
    size_t name_end;
#   if defined(HAVE_FPATHCONF) && defined(HAVE_DIRFD) \
       && defined(_PC_NAME_MAX)
        name_max = fpathconf(dirfd(dirp), _PC_NAME_MAX);
        if (name_max == -1)
#           if defined(NAME_MAX)
                name_max = (NAME_MAX > 255) ? NAME_MAX : 255;
#           else
                return (size_t)(-1);
#           endif
#   else
#       if defined(NAME_MAX)
            name_max = (NAME_MAX > 255) ? NAME_MAX : 255;
#       else
#           error "buffer size for readdir_r cannot be determined"
#       endif
#   endif
    name_end = (size_t)offsetof(struct dirent, d_name) + name_max + 1;
    return (name_end > sizeof(struct dirent)
            ? name_end : sizeof(struct dirent));
}
bool DirList(const string& dir,
             uint32 list_attr,
             const re::RE* regex,
             vector<string>* out) {
  DIR* dirp = ::opendir(dir.c_str());
  if ( dirp == NULL ) {
    LOG_ERROR << "::opendir failed for dir: [" << dir << "] error: "
              << GetLastSystemErrorDescription();
    return false;
  }
  size_t size = dirent_buf_size(dirp);
  if ( size == -1 ) {
    ::closedir(dirp);
    LOG_ERROR << "error dirent_buf_size for dir: [" << dir << "]";
    return false;
  }

#ifdef HAVE_READDIR_R
  struct dirent* buf = static_cast<struct dirent *>(malloc(size));
#endif
  while ( true ) {
    struct dirent* entry = NULL;
    int error = 0;
#ifdef HAVE_READDIR_R
    error = ::readdir_r(dirp, buf, &entry);
#else
    entry = ::readdir(dirp);
    if (entry == NULL) {
        error = 1;
    }
#endif

    if ( error != 0 ) {
      ::closedir(dirp);
#ifdef HAVE_READDIR_R
      free(buf);
      LOG_ERROR << "readdir_r() failed dir: [" << dir << "] error: "
                << GetSystemErrorDescription(error);
#else
      LOG_ERROR << "readdir() failed dir: [" << dir << "] error: "
                << GetSystemErrorDescription(error);
#endif
      return false;
    }
    if ( entry == NULL) {
      break;
    }
    const char* basename = entry->d_name;

    // Skip dots
    if ( (basename[0] == '.' && basename[1] == '\0') ||
         (basename[0] == '.' && basename[1] == '.' && basename[2] == '\0') ) {
      continue;
    }

    struct __STAT st;
    const string abs_path = strutil::JoinPaths(dir, basename);
    // ::lstat does not follow symlinks. It returns stats for the link itself.
    if ( 0 != ::__LSTAT(abs_path.c_str(), &st) ) {
      LOG_ERROR << "::lstat failed for file: [" << abs_path << "]";
      continue;
    }
    // maybe accumulate entry
    if ( (list_attr & LIST_EVERYTHING) == LIST_EVERYTHING ||
         ((list_attr & LIST_FILES) && (S_ISREG(st.st_mode) || S_ISLNK(st.st_mode))) ||
         ((list_attr & LIST_DIRS) && S_ISDIR(st.st_mode)) ) {
        // skip entries which don't match the regex
        if ( regex == NULL || regex->Matches(basename) ) {
            out->push_back(basename);
        }
    }
    // recursive listing
    if ( (list_attr & LIST_RECURSIVE) && S_ISDIR(st.st_mode) ) {
      vector<string> subitems;
      DirList(abs_path, list_attr, regex, &subitems);
      for ( uint32 i = 0; i < subitems.size(); i++ ) {
        out->push_back(strutil::JoinPaths(basename, subitems[i]));
      }
    }
  }
#ifdef HAVE_READDIR_R
  free(buf);
#endif
  ::closedir(dirp);
  return true;
}

int32 GetLastNumberedFile(const string& dir,
                          re::RE* re,
                          int32 file_num_size) {
  vector<string> files;
  if ( !DirList(dir, LIST_FILES, re,  &files) ) {
    return -2;
  }
  if ( files.empty() ) {
    return -1;
  }
  sort(files.begin(), files.end());
  CHECK_GT(files.back().size(), 10);
  errno = 0;  // essential as strtol would not set a 0 errno
  const int32 file_num = strtol(
    files.back().c_str() + files.back().size() - file_num_size, NULL, 10);
  if ( errno || file_num < 0 ) {
    LOG_ERROR << " Invalid file found: " << files.back()
              << " under: " << dir;
    return -2;
  }
  return file_num;
}
}
