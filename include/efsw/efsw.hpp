/**
	@author Martín Lucas Golini

	Copyright (c) 2013 Martín Lucas Golini

	Permission is hereby granted, free of charge, to any person obtaining a copy
	of this software and associated documentation files (the "Software"), to deal
	in the Software without restriction, including without limitation the rights
	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
	copies of the Software, and to permit persons to whom the Software is
	furnished to do so, subject to the following conditions:

	The above copyright notice and this permission notice shall be included in
	all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
	THE SOFTWARE.

	This software is a fork of the "simplefilewatcher" by James Wynn (james@jameswynn.com)
	http://code.google.com/p/simplefilewatcher/ also MIT licensed.
*/

#ifndef ESFW_HPP
#define ESFW_HPP

#include "efsw/Mutex.hpp"
#include "efsw/Lock.hpp"

#include <functional>
#include <mutex>
#include <stdexcept>
#include <vector>
#include <string>
#include <vector>
#include <map>
#include <filesystem>

#if defined( _WIN32 )
#ifdef EFSW_DYNAMIC
// Windows platforms
#ifdef EFSW_EXPORTS
// From DLL side, we must export
#define EFSW_API __declspec( dllexport )
#else
// From client application side, we must import
#define EFSW_API __declspec( dllimport )
#endif
#else
// No specific directive needed for static build
#ifndef EFSW_API
#define EFSW_API
#endif
#endif
#else
#if ( __GNUC__ >= 4 ) && defined( EFSW_EXPORTS )
#ifndef EFSW_API
#define EFSW_API __attribute__( ( visibility( "default" ) ) )
#endif
#endif

// Other platforms don't need to define anything
#ifndef EFSW_API
#define EFSW_API
#endif
#endif

namespace efsw {

/// Type for a watch id
typedef long WatchID;

// forward declarations
class FileWatcherImpl;
class FileWatchListener;
class WatcherOption;

/// Actions to listen for. Rename will send two events, one for
/// the deletion of the old file, and one for the creation of the
/// new file.
namespace Actions {
enum Action {
	/// Sent when a file is created or renamed
	Add = 1,
	/// Sent when a file is deleted or renamed
	Delete = 2,
	/// Sent when a file is modified
	Modified = 3,
	/// Sent when a file is moved
	Moved = 4
};
}
typedef Actions::Action Action;

/// Errors log namespace
namespace Errors {

enum Error {
	NoError = 0,
	FileNotFound = -1,
	FileRepeated = -2,
	FileOutOfScope = -3,
	FileNotReadable = -4,
	/// Directory in remote file system
	/// ( create a generic FileWatcher instance to watch this directory ).
	FileRemote = -5,
	/// File system watcher failed to watch for changes.
	WatcherFailed = -6,
	Unspecified = -7
};

class EFSW_API Log {
  public:
	/// @return The last error logged
	static std::string getLastErrorLog();

	/// @return The code of the last error logged
	static Error getLastErrorCode();

	/// Reset last error
	static void clearLastError();

	/// Creates an error of the type specified
	static Error createLastError( Error err, std::string log );
};

} // namespace Errors
typedef Errors::Error Error;

/// Optional file watcher settings.
namespace Options {
enum Option {
	/// For Windows, the default buffer size of 63*1024 bytes sometimes is not enough and
	/// file system events may be dropped. For that, using a different (bigger) buffer size
	/// can be defined here, but note that this does not work for network drives,
	/// because a buffer larger than 64K will fail the folder being watched, see
	/// http://msdn.microsoft.com/en-us/library/windows/desktop/aa365465(v=vs.85).aspx)
	WinBufferSize = 1,
	/// For Windows, per default all events are captured but we might only be interested
	/// in a subset; the value of the option should be set to a bitwise or'ed set of
	/// FILE_NOTIFY_CHANGE_* flags.
	WinNotifyFilter = 2
};
}
typedef Options::Option Option;

/// Listens to files and directories and dispatches events
/// to notify the listener of files and directories changes.
/// @class FileWatcher
class EFSW_API FileWatcher {
  public:
	/// Default constructor, will use the default platform file watcher
	FileWatcher();

	/// Constructor that lets you force the use of the Generic File Watcher
	explicit FileWatcher( bool useGenericFileWatcher );

	virtual ~FileWatcher();

	/// Add a directory watch. Same as the other addWatch, but doesn't have recursive option.
	/// For backwards compatibility.
	/// On error returns WatchID with Error type.
	WatchID addWatch( const std::string& directory, FileWatchListener* watcher );

	/// Add a directory watch
	/// On error returns WatchID with Error type.
	WatchID addWatch( const std::string& directory, FileWatchListener* watcher, bool recursive );

	/// Add a directory watch, allowing customization with options
	/// @param directory The folder to be watched
	/// @param watcher The listener to receive events
	/// @param recursive Set this to true to include subdirectories
	/// @param options Allows customization of a watcher
	/// @return Returns the watch id for the directory or, on error, a WatchID with Error type.
	WatchID addWatch( const std::string& directory, FileWatchListener* watcher, bool recursive, 
					  const std::vector<WatcherOption> &options );

	/// Remove a directory watch. This is a brute force search O(nlogn).
	void removeWatch( const std::string& directory );

	/// Remove a directory watch. This is a map lookup O(logn).
	void removeWatch( WatchID watchid );

	/// Starts watching ( in other thread )
	void watch();

	/// @return Returns a list of the directories that are being watched
	std::vector<std::string> directories();

	/** Allow recursive watchers to follow symbolic links to other directories
	 * followSymlinks is disabled by default
	 */
	void followSymlinks( bool follow );

	/** @return If can follow symbolic links to directorioes */
	const bool& followSymlinks() const;

	/** When enable this it will allow symlinks to watch recursively out of the pointed directory.
	 * follorSymlinks must be enabled to this work.
	 * For example, added symlink to /home/folder, and the symlink points to /, this by default is
	 * not allowed, it's only allowed to symlink anything from /home/ and deeper. This is to avoid
	 * great levels of recursion. Enabling this could lead in infinite recursion, and crash the
	 * watcher ( it will try not to avoid this ). Buy enabling out of scope links, it will allow
	 * this behavior. allowOutOfScopeLinks are disabled by default.
	 */
	void allowOutOfScopeLinks( bool allow );

	/// @return Returns if out of scope links are allowed
	const bool& allowOutOfScopeLinks() const;

  private:
	/// The implementation
	FileWatcherImpl* mImpl;
	bool mFollowSymlinks;
	bool mOutOfScopeLinks;
};

/// Basic interface for listening for file events.
/// @class FileWatchListener
class FileWatchListener {
  public:
	virtual ~FileWatchListener() {}

	/// Handles the action file action
	/// @param watchid The watch id for the directory
	/// @param dir The directory
	/// @param filename The filename that was accessed (not full path)
	/// @param action Action that was performed
	/// @param oldFilename The name of the file or directory moved
	virtual void handleFileAction( WatchID watchid, const std::string& dir,
								   const std::string& filename, Action action,
								   std::string oldFilename = "" ) = 0;
};

class GenericFileWatchListener : public FileWatchListener
{
public:
	struct Event
	{
		WatchID watchid;
		const std::string& dir;
		const std::string& filename;
		Action action;
		std::string oldFilename;
	};
	using Callback = std::function<void(Event)>;
	GenericFileWatchListener(Callback callback)
	: mCallback(std::move(callback))
	{
	}
	virtual void handleFileAction( WatchID watchid, const std::string& dir,
								   const std::string& filename, Action action,
								   std::string oldFilename = "" )
	{
		mCallback({watchid, dir, filename, action, std::move(oldFilename)});
	}
private:
	Callback mCallback;
};

class ScopedWatchListener
{
public:
	ScopedWatchListener(
		FileWatcher& watcher,
		GenericFileWatchListener::Callback callback,
		const std::string& directory,
		bool recursive = false
	)
	: mListener(new GenericFileWatchListener{std::move(callback)})
	, mWatcher(&watcher)
	, mWatchID(watcher.addWatch(directory,mListener.get(),recursive))
	{
		if (!mWatchID)
			throw std::runtime_error{"could not establish watch of '" + directory + "'"};
	}
	ScopedWatchListener(const ScopedWatchListener& other) = delete;
	ScopedWatchListener& operator=(const ScopedWatchListener& other) = delete;
	ScopedWatchListener& operator=(ScopedWatchListener&& other)
	{
		release();
		mListener = std::move(other.mListener);
		mWatcher = other.mWatcher;
		return *this;
	}
	ScopedWatchListener(ScopedWatchListener&& other)
	: mListener(std::move(other.mListener))
	, mWatcher(other.mWatcher)
	{
		other.mWatcher = 0;
	}
	~ScopedWatchListener()
	{
		release();
	}
private:
	void release()
	{
		if (mWatcher)
		{
			mWatcher->removeWatch(mWatchID);
			mWatcher = 0;
		}
	}
	std::unique_ptr<GenericFileWatchListener> mListener;
	FileWatcher* mWatcher = 0;
	WatchID mWatchID = 0;
};

class ConvenientFileWatcher
{
public:
	class Guard
	{
	public:
		Guard(ConvenientFileWatcher& watcher, std::string directory, size_t id)
		: mWatcher(&watcher)
		, mDirectory(std::move(directory))
		, mId(id)
		{
		}
		Guard(const Guard& other) = delete;
		Guard& operator=(const Guard& other) = delete;
		Guard& operator=(Guard&& other)
		{
			release();
			mWatcher = other.mWatcher;
			mDirectory = std::move(other.mDirectory);
			mId = other.mId;
			return *this;
		}
		Guard(Guard&& other)
		{
			*this = std::move(other);
		}
		~Guard()
		{
			release();
		}
	private:
		void release()
		{
			if (mWatcher)
			{
				mWatcher->removeWatch(mDirectory, mId);
				mWatcher = 0;
			}
		}
		ConvenientFileWatcher* mWatcher = 0;
		std::string mDirectory;
		size_t mId = 0;
	};

	ConvenientFileWatcher()
	: mDirectoryWatcher(false)
	{
	}

	~ConvenientFileWatcher()
	{
	}

	Guard addWatch(const std::filesystem::path& path, GenericFileWatchListener::Callback callback)
	{
		Lock lock{mMutex};
		if (is_directory(path))
		{
			auto directory = path.string();
			auto& watcher = getDirectoryWatch(directory);
			auto id = watcher.addDirectoryCallback(callback);
			mDirectoryWatcher.watch();
			return {*this, directory, id};
		}
		else
		{
			auto directory = path.parent_path();
			auto filename = path.filename();
			auto& watcher = getDirectoryWatch(directory);
			auto id = watcher.addFileCallback(filename, callback);
			mDirectoryWatcher.watch();
			return {*this, directory, id};
		}
	}

private:

	class DirectoryWatch
	{
	public:
		DirectoryWatch(FileWatcher& watcher, const std::string directory)
		: mListener(ScopedWatchListener{
			watcher,
			[this](GenericFileWatchListener::Event event){
				handle(std::move(event));
			},
			directory
		})
		{
		}
		size_t addDirectoryCallback(GenericFileWatchListener::Callback callback)
		{
			Lock lock{mMutex};
			mDirectoryCallbacks[++mLastId] = std::move(callback);
			return mLastId;
		}
		size_t addFileCallback(std::string filename, GenericFileWatchListener::Callback callback)
		{
			Lock lock{mMutex};
			mFileCallbacks[++mLastId] = {
				std::move(filename),
				std::move(callback),
			};
			return mLastId;
		}
		void removeCallback(size_t id)
		{
			Lock lock{mMutex};
			mFileCallbacks.erase(id);
			mDirectoryCallbacks.erase(id);
		}
		bool hasCallbacks()
		{
			Lock lock{mMutex};
			return !mDirectoryCallbacks.empty() || !mFileCallbacks.empty();
		}
	private:
		struct FileCallback
		{
			std::string filename;
			GenericFileWatchListener::Callback callback;
		};
		void handle(GenericFileWatchListener::Event event)
		{
			Lock lock{mMutex};
			for (auto& callback : mDirectoryCallbacks)
				callback.second(event);
			for (auto& callback : mFileCallbacks)
				if (callback.second.filename == event.filename)
					callback.second.callback(event);
		}
		Mutex mMutex;
		ScopedWatchListener mListener;
		size_t mLastId = 0;
		std::map<size_t,GenericFileWatchListener::Callback> mDirectoryCallbacks;
		std::map<size_t,FileCallback> mFileCallbacks;
	};

	DirectoryWatch& getDirectoryWatch(const std::string& directory)
	{
		if (auto i = mDirectoryWatches.find(directory); i != mDirectoryWatches.end())
			return *i->second;
		auto i = mDirectoryWatches.insert({
			directory,
			std::unique_ptr<DirectoryWatch>{new DirectoryWatch{
				mDirectoryWatcher,
				directory
			}}
		});
		return *i.first->second;
	}

	void removeWatch(const std::string& directory, size_t id)
	{
		Lock lock{mMutex};
		if (auto i = mDirectoryWatches.find(directory); i != mDirectoryWatches.end())
		{
			i->second->removeCallback(id);
			if (!i->second->hasCallbacks())
				mDirectoryWatches.erase(i);
		}
	}
	Mutex mMutex;
	FileWatcher mDirectoryWatcher;
	std::map<std::string, std::unique_ptr<DirectoryWatch>> mDirectoryWatches;
};

inline ConvenientFileWatcher::Guard watch(const std::string& path, GenericFileWatchListener::Callback callback)
{
	static ConvenientFileWatcher watcher;
	return watcher.addWatch(path, std::move(callback));
}

/// Optional, typically platform specific parameter for customization of a watcher.
/// @class WatcherOption
class WatcherOption {
  public:
	WatcherOption(Option option, int value) : mOption(option), mValue(value) {};
	Option mOption;
	int mValue;
};

} // namespace efsw

#endif
