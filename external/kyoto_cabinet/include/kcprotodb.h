/*************************************************************************************************
 * Prototype database
 *                                                               Copyright (C) 2009-2010 FAL Labs
 * This file is part of Kyoto Cabinet.
 * This program is free software: you can redistribute it and/or modify it under the terms of
 * the GNU General Public License as published by the Free Software Foundation, either version
 * 3 of the License, or any later version.
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License along with this program.
 * If not, see <http://www.gnu.org/licenses/>.
 *************************************************************************************************/


#ifndef _KCPROTODB_H                     // duplication check
#define _KCPROTODB_H

#include <kccommon.h>
#include <kcutil.h>
#include <kcthread.h>
#include <kcfile.h>
#include <kccompress.h>
#include <kccompare.h>
#include <kcmap.h>
#include <kcregex.h>
#include <kcdb.h>
#include <kcplantdb.h>

namespace kyotocabinet {                 // common namespace


/**
 * Constants for implementation.
 */
namespace {
const size_t PDBHASHBNUM = 1048583LL;    ///< bucket number of hash table
const size_t PDBOPAQUESIZ = 16;          ///< size of the opaque buffer
}


/**
 * Helper functions.
 */
namespace {
template <class STRMAP>
typename STRMAP::iterator map_find(STRMAP* map, const std::string& key) {
  return map->find(key);
}
template <>
StringTreeMap::iterator map_find(StringTreeMap* map, const std::string& key) {
  StringTreeMap::iterator it = map->find(key);
  if (it != map->end()) return it;
  return map->upper_bound(key);
}
template <class STRMAP>
void map_tune(STRMAP* map) {}
template <>
void map_tune(StringHashMap* map) {
  map->rehash(PDBHASHBNUM);
  map->max_load_factor(FLT_MAX);
}
template <class ITER>
bool iter_back(ITER* itp) {
  return false;
}
template <>
bool iter_back(StringTreeMap::iterator* itp) {
  --(*itp);
  return true;
}
}


/**
 * Prototype implementation of database with STL.
 * @param STRMAP a class compatible with the map class of STL.
 * @param DBTYPE the database type number of the class.
 * @note This class template is a template for concrete classes which wrap data structures
 * compatible with std::map.  Template instance classes can be inherited but overwriting methods
 * is forbidden.  The class ProtoHashDB is the instance using std::unordered_map.  The class
 * ProtoTreeDB is the instance using std::map.  Before every database operation, it is necessary
 * to call the BasicDB::open method in order to open a database file and connect the database
 * object to it.  To avoid data missing or corruption, it is important to close every database
 * file by the BasicDB::close method when the database is no longer in use.  It is forbidden for
 * multible database objects in a process to open the same database at the same time.  It is
 * forbidden to share a database object with child processes.
 */
template <class STRMAP, uint8_t DBTYPE>
class ProtoDB : public BasicDB {
public:
  class Cursor;
private:
  struct TranLog;
  /** An alias of list of cursors. */
  typedef std::list<Cursor*> CursorList;
  /** An alias of list of transaction logs. */
  typedef std::list<TranLog> TranLogList;
public:
  /**
   * Cursor to indicate a record.
   */
  class Cursor : public BasicDB::Cursor {
    friend class ProtoDB;
  public:
    /**
     * Constructor.
     * @param db the container database object.
     */
    explicit Cursor(ProtoDB* db) : db_(db), it_(db->recs_.end()) {
      _assert_(db);
      ScopedSpinRWLock lock(&db_->mlock_, true);
      db_->curs_.push_back(this);
    }
    /**
     * Destructor.
     */
    virtual ~Cursor() {
      _assert_(true);
      if (!db_) return;
      ScopedSpinRWLock lock(&db_->mlock_, true);
      db_->curs_.remove(this);
    }
    /**
     * Accept a visitor to the current record.
     * @param visitor a visitor object.
     * @param writable true for writable operation, or false for read-only operation.
     * @param step true to move the cursor to the next record, or false for no move.
     * @return true on success, or false on failure.
     * @note The operation for each record is performed atomically and other threads accessing
     * the same record are blocked.  To avoid deadlock, any database operation must not be
     * performed in this function.
     */
    bool accept(Visitor* visitor, bool writable = true, bool step = false) {
      _assert_(visitor);
      ScopedSpinRWLock lock(&db_->mlock_, true);
      if (db_->omode_ == 0) {
        db_->set_error(_KCCODELINE_, Error::INVALID, "not opened");
        return false;
      }
      if (writable && !(db_->omode_ & OWRITER)) {
        db_->set_error(_KCCODELINE_, Error::NOPERM, "permission denied");
        return false;
      }
      if (it_ == db_->recs_.end()) {
        db_->set_error(_KCCODELINE_, Error::NOREC, "no record");
        return false;
      }
      const std::string& key = it_->first;
      const std::string& value = it_->second;
      size_t vsiz;
      const char* vbuf = visitor->visit_full(key.c_str(), key.size(),
                                             value.c_str(), value.size(), &vsiz);
      if (vbuf == Visitor::REMOVE) {
        if (db_->tran_) {
          TranLog log(key, value);
          db_->trlogs_.push_back(log);
        }
        db_->size_ -= key.size() + value.size();
        if (db_->curs_.size() > 1) {
          typename CursorList::const_iterator cit = db_->curs_.begin();
          typename CursorList::const_iterator citend = db_->curs_.end();
          while (cit != citend) {
            Cursor* cur = *cit;
            if (cur != this && cur->it_ == it_) cur->it_++;
            cit++;
          }
        }
        db_->recs_.erase(it_++);
      } else if (vbuf == Visitor::NOP) {
        if (step) it_++;
      } else {
        if (db_->tran_) {
          TranLog log(key, value);
          db_->trlogs_.push_back(log);
        }
        db_->size_ -= value.size();
        db_->size_ += vsiz;
        it_->second = std::string(vbuf, vsiz);
        if (step) it_++;
      }
      return true;
    }
    /**
     * Jump the cursor to the first record for forward scan.
     * @return true on success, or false on failure.
     */
    bool jump() {
      _assert_(true);
      ScopedSpinRWLock lock(&db_->mlock_, true);
      if (db_->omode_ == 0) {
        db_->set_error(_KCCODELINE_, Error::INVALID, "not opened");
        return false;
      }
      it_ = db_->recs_.begin();
      if (it_ == db_->recs_.end()) {
        db_->set_error(_KCCODELINE_, Error::NOREC, "no record");
        return false;
      }
      return true;
    }
    /**
     * Jump the cursor to a record for forward scan.
     * @param kbuf the pointer to the key region.
     * @param ksiz the size of the key region.
     * @return true on success, or false on failure.
     */
    bool jump(const char* kbuf, size_t ksiz) {
      _assert_(kbuf && ksiz <= MEMMAXSIZ);
      ScopedSpinRWLock lock(&db_->mlock_, true);
      if (db_->omode_ == 0) {
        db_->set_error(_KCCODELINE_, Error::INVALID, "not opened");
        return false;
      }
      std::string key(kbuf, ksiz);
      it_ = map_find(&db_->recs_, key);
      if (it_ == db_->recs_.end()) {
        db_->set_error(_KCCODELINE_, Error::NOREC, "no record");
        return false;
      }
      return true;
    }
    /**
     * Jump the cursor to a record for forward scan.
     * @note Equal to the original Cursor::jump method except that the parameter is std::string.
     */
    bool jump(const std::string& key) {
      _assert_(true);
      return jump(key.c_str(), key.size());
    }
    /**
     * Jump the cursor to the last record for backward scan.
     * @return true on success, or false on failure.
     */
    bool jump_back() {
      _assert_(true);
      ScopedSpinRWLock lock(&db_->mlock_, true);
      if (db_->omode_ == 0) {
        db_->set_error(_KCCODELINE_, Error::INVALID, "not opened");
        return false;
      }
      it_ = db_->recs_.end();
      if (it_ == db_->recs_.begin()) {
        db_->set_error(_KCCODELINE_, Error::NOREC, "no record");
        return false;
      }
      if (!iter_back(&it_)) {
        db_->set_error(_KCCODELINE_, Error::NOIMPL, "not implemented");
        return false;
      }
      return true;
    }
    /**
     * Jump the cursor to a record for backward scan.
     * @param kbuf the pointer to the key region.
     * @param ksiz the size of the key region.
     * @return true on success, or false on failure.
     */
    bool jump_back(const char* kbuf, size_t ksiz) {
      _assert_(kbuf && ksiz <= MEMMAXSIZ);
      ScopedSpinRWLock lock(&db_->mlock_, true);
      if (db_->omode_ == 0) {
        db_->set_error(_KCCODELINE_, Error::INVALID, "not opened");
        return false;
      }
      std::string key(kbuf, ksiz);
      it_ = map_find(&db_->recs_, key);
      if (it_ == db_->recs_.end()) {
        if (it_ == db_->recs_.begin()) {
          db_->set_error(_KCCODELINE_, Error::NOREC, "no record");
          return false;
        }
        if (!iter_back(&it_)) {
          db_->set_error(_KCCODELINE_, Error::NOIMPL, "not implemented");
          return false;
        }
      } else {
        std::string key(kbuf, ksiz);
        if (key < it_->first) {
          if (it_ == db_->recs_.begin()) {
            db_->set_error(_KCCODELINE_, Error::NOREC, "no record");
            it_ = db_->recs_.end();
            return false;
          }
          if (!iter_back(&it_)) {
            db_->set_error(_KCCODELINE_, Error::NOIMPL, "not implemented");
            it_ = db_->recs_.end();
            return false;
          }
        }
      }
      return true;
    }
    /**
     * Jump the cursor to a record for backward scan.
     * @note Equal to the original Cursor::jump_back method except that the parameter is
     * std::string.
     */
    bool jump_back(const std::string& key) {
      _assert_(true);
      return jump_back(key.c_str(), key.size());
    }
    /**
     * Step the cursor to the next record.
     * @return true on success, or false on failure.
     */
    bool step() {
      _assert_(true);
      ScopedSpinRWLock lock(&db_->mlock_, true);
      if (db_->omode_ == 0) {
        db_->set_error(_KCCODELINE_, Error::INVALID, "not opened");
        return false;
      }
      if (it_ == db_->recs_.end()) {
        db_->set_error(_KCCODELINE_, Error::NOREC, "no record");
        return false;
      }
      it_++;
      if (it_ == db_->recs_.end()) {
        db_->set_error(_KCCODELINE_, Error::NOREC, "no record");
        return false;
      }
      return true;
    }
    /**
     * Step the cursor to the previous record.
     * @return true on success, or false on failure.
     */
    bool step_back() {
      _assert_(true);
      ScopedSpinRWLock lock(&db_->mlock_, true);
      if (db_->omode_ == 0) {
        db_->set_error(_KCCODELINE_, Error::INVALID, "not opened");
        return false;
      }
      if (it_ == db_->recs_.begin()) {
        db_->set_error(_KCCODELINE_, Error::NOREC, "no record");
        it_ = db_->recs_.end();
        return false;
      }
      if (!iter_back(&it_)) {
        db_->set_error(_KCCODELINE_, Error::NOIMPL, "not implemented");
        return false;
      }
      return true;
    }
    /**
     * Get the database object.
     * @return the database object.
     */
    ProtoDB* db() {
      _assert_(true);
      return db_;
    }
  private:
    /** Dummy constructor to forbid the use. */
    Cursor(const Cursor&);
    /** Dummy Operator to forbid the use. */
    Cursor& operator =(const Cursor&);
    /** The inner database. */
    ProtoDB* db_;
    /** The inner iterator. */
    typename STRMAP::iterator it_;
  };
  /**
   * Default constructor.
   */
  explicit ProtoDB() :
    mlock_(), error_(), logger_(NULL), logkinds_(0), mtrigger_(NULL),
    omode_(0), recs_(), curs_(), path_(""), size_(0), opaque_(),
    tran_(false), trlogs_(), trsize_(0) {
    _assert_(true);
    map_tune(&recs_);
  }
  /**
   * Destructor.
   * @note If the database is not closed, it is closed implicitly.
   */
  virtual ~ProtoDB() {
    _assert_(true);
    if (omode_ != 0) close();
    if (!curs_.empty()) {
      typename CursorList::const_iterator cit = curs_.begin();
      typename CursorList::const_iterator citend = curs_.end();
      while (cit != citend) {
        Cursor* cur = *cit;
        cur->db_ = NULL;
        cit++;
      }
    }
  }
  /**
   * Accept a visitor to a record.
   * @param kbuf the pointer to the key region.
   * @param ksiz the size of the key region.
   * @param visitor a visitor object.
   * @param writable true for writable operation, or false for read-only operation.
   * @return true on success, or false on failure.
   * @note The operation for each record is performed atomically and other threads accessing the
   * same record are blocked.  To avoid deadlock, any database operation must not be performed in
   * this function.
   */
  bool accept(const char* kbuf, size_t ksiz, Visitor* visitor, bool writable = true) {
    _assert_(kbuf && ksiz <= MEMMAXSIZ && visitor);
    if (writable) {
      ScopedSpinRWLock lock(&mlock_, true);
      if (omode_ == 0) {
        set_error(_KCCODELINE_, Error::INVALID, "not opened");
        return false;
      }
      if (!(omode_ & OWRITER)) {
        set_error(_KCCODELINE_, Error::NOPERM, "permission denied");
        return false;
      }
      std::string key(kbuf, ksiz);
      typename STRMAP::iterator it = recs_.find(key);
      if (it == recs_.end()) {
        size_t vsiz;
        const char* vbuf = visitor->visit_empty(kbuf, ksiz, &vsiz);
        if (vbuf != Visitor::NOP && vbuf != Visitor::REMOVE) {
          if (tran_) {
            TranLog log(key);
            trlogs_.push_back(log);
          }
          size_ += ksiz + vsiz;
          recs_[key] = std::string(vbuf, vsiz);
        }
      } else {
        const std::string& value = it->second;
        size_t vsiz;
        const char* vbuf = visitor->visit_full(kbuf, ksiz, value.c_str(), value.size(), &vsiz);
        if (vbuf == Visitor::REMOVE) {
          if (tran_) {
            TranLog log(key, value);
            trlogs_.push_back(log);
          }
          size_ -= ksiz + value.size();
          if (!curs_.empty()) {
            typename CursorList::const_iterator cit = curs_.begin();
            typename CursorList::const_iterator citend = curs_.end();
            while (cit != citend) {
              Cursor* cur = *cit;
              if (cur->it_ == it) cur->it_++;
              cit++;
            }
          }
          recs_.erase(it);
        } else if (vbuf != Visitor::NOP) {
          if (tran_) {
            TranLog log(key, value);
            trlogs_.push_back(log);
          }
          size_ -= value.size();
          size_ += vsiz;
          it->second = std::string(vbuf, vsiz);
        }
      }
    } else {
      ScopedSpinRWLock lock(&mlock_, false);
      if (omode_ == 0) {
        set_error(_KCCODELINE_, Error::INVALID, "not opened");
        return false;
      }
      std::string key(kbuf, ksiz);
      const STRMAP& rrecs = recs_;
      typename STRMAP::const_iterator it = rrecs.find(key);
      if (it == rrecs.end()) {
        size_t vsiz;
        const char* vbuf = visitor->visit_empty(kbuf, ksiz, &vsiz);
        if (vbuf != Visitor::NOP && vbuf != Visitor::REMOVE) {
          set_error(_KCCODELINE_, Error::NOPERM, "permission denied");
          return false;
        }
      } else {
        const std::string& value = it->second;
        size_t vsiz;
        const char* vbuf = visitor->visit_full(kbuf, ksiz, value.c_str(), value.size(), &vsiz);
        if (vbuf != Visitor::NOP && vbuf != Visitor::REMOVE) {
          set_error(_KCCODELINE_, Error::NOPERM, "permission denied");
          return false;
        }
      }
    }
    return true;
  }
  /**
   * Iterate to accept a visitor for each record.
   * @param visitor a visitor object.
   * @param writable true for writable operation, or false for read-only operation.
   * @param checker a progress checker object.  If it is NULL, no checking is performed.
   * @return true on success, or false on failure.
   * @note The whole iteration is performed atomically and other threads are blocked.  To avoid
   * deadlock, any database operation must not be performed in this function.
   */
  bool iterate(Visitor *visitor, bool writable = true, ProgressChecker* checker = NULL) {
    _assert_(visitor);
    ScopedSpinRWLock lock(&mlock_, true);
    if (omode_ == 0) {
      set_error(_KCCODELINE_, Error::INVALID, "not opened");
      return false;
    }
    if (writable && !(omode_ & OWRITER)) {
      set_error(_KCCODELINE_, Error::NOPERM, "permission denied");
      return false;
    }
    int64_t allcnt = recs_.size();
    if (checker && !checker->check("iterate", "beginning", 0, allcnt)) {
      set_error(_KCCODELINE_, Error::LOGIC, "checker failed");
      return false;
    }
    typename STRMAP::iterator it = recs_.begin();
    typename STRMAP::iterator itend = recs_.end();
    int64_t curcnt = 0;
    while (it != itend) {
      const std::string& key = it->first;
      const std::string& value = it->second;
      size_t vsiz;
      const char* vbuf = visitor->visit_full(key.c_str(), key.size(),
                                             value.c_str(), value.size(), &vsiz);
      if (vbuf == Visitor::REMOVE) {
        size_ -= key.size() + value.size();
        recs_.erase(it++);
      } else if (vbuf == Visitor::NOP) {
        it++;
      } else {
        size_ -= value.size();
        size_ += vsiz;
        it->second = std::string(vbuf, vsiz);
        it++;
      }
      curcnt++;
      if (checker && !checker->check("iterate", "processing", curcnt, allcnt)) {
        set_error(_KCCODELINE_, Error::LOGIC, "checker failed");
        return false;
      }
    }
    if (checker && !checker->check("iterate", "ending", -1, allcnt)) {
      set_error(_KCCODELINE_, Error::LOGIC, "checker failed");
      return false;
    }
    trigger_meta(MetaTrigger::ITERATE, "iterate");
    return true;
  }
  /**
   * Get the last happened error.
   * @return the last happened error.
   */
  Error error() const {
    _assert_(true);
    return error_;
  }
  /**
   * Set the error information.
   * @param file the file name of the program source code.
   * @param line the line number of the program source code.
   * @param func the function name of the program source code.
   * @param code an error code.
   * @param message a supplement message.
   */
  void set_error(const char* file, int32_t line, const char* func,
                 Error::Code code, const char* message) {
    _assert_(file && line > 0 && func && message);
    error_->set(code, message);
    if (logger_) {
      Logger::Kind kind = code == Error::BROKEN || code == Error::SYSTEM ?
        Logger::ERROR : Logger::INFO;
      if (kind & logkinds_)
        report(file, line, func, kind, "%d: %s: %s", code, Error::codename(code), message);
    }
  }
  /**
   * Open a database file.
   * @param path the path of a database file.
   * @param mode the connection mode.  BasicDB::OWRITER as a writer, BasicDB::OREADER as a
   * reader.  The following may be added to the writer mode by bitwise-or: BasicDB::OCREATE,
   * which means it creates a new database if the file does not exist, BasicDB::OTRUNCATE, which
   * means it creates a new database regardless if the file exists, BasicDB::OAUTOTRAN, which
   * means each updating operation is performed in implicit transaction, BasicDB::OAUTOSYNC,
   * which means each updating operation is followed by implicit synchronization with the file
   * system.  The following may be added to both of the reader mode and the writer mode by
   * bitwise-or: BasicDB::ONOLOCK, which means it opens the database file without file locking,
   * BasicDB::OTRYLOCK, which means locking is performed without blocking, BasicDB::ONOREPAIR,
   * which means the database file is not repaired implicitly even if file destruction is
   * detected.
   * @return true on success, or false on failure.
   * @note Every opened database must be closed by the BasicDB::close method when it is no
   * longer in use.  It is not allowed for two or more database objects in the same process to
   * keep their connections to the same database file at the same time.
   */
  bool open(const std::string& path, uint32_t mode = OWRITER | OCREATE) {
    _assert_(true);
    ScopedSpinRWLock lock(&mlock_, true);
    if (omode_ != 0) {
      set_error(_KCCODELINE_, Error::INVALID, "already opened");
      return false;
    }
    report(_KCCODELINE_, Logger::DEBUG, "opening the database (path=%s)", path.c_str());
    omode_ = mode;
    path_.append(path);
    std::memset(opaque_, 0, sizeof(opaque_));
    trigger_meta(MetaTrigger::OPEN, "open");
    return true;
  }
  /**
   * Close the database file.
   * @return true on success, or false on failure.
   */
  bool close() {
    _assert_(true);
    ScopedSpinRWLock lock(&mlock_, true);
    if (omode_ == 0) {
      set_error(_KCCODELINE_, Error::INVALID, "not opened");
      return false;
    }
    report(_KCCODELINE_, Logger::DEBUG, "closing the database (path=%s)", path_.c_str());
    tran_ = false;
    trlogs_.clear();
    recs_.clear();
    if (!curs_.empty()) {
      typename CursorList::const_iterator cit = curs_.begin();
      typename CursorList::const_iterator citend = curs_.end();
      while (cit != citend) {
        Cursor* cur = *cit;
        cur->it_ = recs_.end();
        cit++;
      }
    }
    path_.clear();
    omode_ = 0;
    trigger_meta(MetaTrigger::CLOSE, "close");
    return true;
  }
  /**
   * Synchronize updated contents with the file and the device.
   * @param hard true for physical synchronization with the device, or false for logical
   * synchronization with the file system.
   * @param proc a postprocessor object.  If it is NULL, no postprocessing is performed.
   * @param checker a progress checker object.  If it is NULL, no checking is performed.
   * @return true on success, or false on failure.
   */
  bool synchronize(bool hard = false, FileProcessor* proc = NULL,
                   ProgressChecker* checker = NULL) {
    _assert_(true);
    ScopedSpinRWLock lock(&mlock_, false);
    if (omode_ == 0) {
      set_error(_KCCODELINE_, Error::INVALID, "not opened");
      return false;
    }
    bool err = false;
    if ((omode_ & OWRITER) && checker &&
        !checker->check("synchronize", "nothing to be synchronized", -1, -1)) {
      set_error(_KCCODELINE_, Error::LOGIC, "checker failed");
      return false;
    }
    if (proc) {
      if (checker && !checker->check("synchronize", "running the post processor", -1, -1)) {
        set_error(_KCCODELINE_, Error::LOGIC, "checker failed");
        return false;
      }
      if (!proc->process(path_, recs_.size(), size_)) {
        set_error(_KCCODELINE_, Error::LOGIC, "postprocessing failed");
        err = true;
      }
    }
    trigger_meta(MetaTrigger::SYNCHRONIZE, "synchronize");
    return !err;
  }
  /**
   * Begin transaction.
   * @param hard true for physical synchronization with the device, or false for logical
   * synchronization with the file system.
   * @return true on success, or false on failure.
   */
  bool begin_transaction(bool hard = false) {
    _assert_(true);
    for (double wsec = 1.0 / CLOCKTICK; true; wsec *= 2) {
      mlock_.lock_writer();
      if (omode_ == 0) {
        set_error(_KCCODELINE_, Error::INVALID, "not opened");
        mlock_.unlock();
        return false;
      }
      if (!(omode_ & OWRITER)) {
        set_error(_KCCODELINE_, Error::NOPERM, "permission denied");
        mlock_.unlock();
        return false;
      }
      if (!tran_) break;
      mlock_.unlock();
      if (wsec > 1.0) wsec = 1.0;
      Thread::sleep(wsec);
    }
    tran_ = true;
    trsize_ = size_;
    trigger_meta(MetaTrigger::BEGINTRAN, "begin_transaction");
    mlock_.unlock();
    return true;
  }
  /**
   * Try to begin transaction.
   * @param hard true for physical synchronization with the device, or false for logical
   * synchronization with the file system.
   * @return true on success, or false on failure.
   */
  bool begin_transaction_try(bool hard = false) {
    _assert_(true);
    mlock_.lock_writer();
    if (omode_ == 0) {
      set_error(_KCCODELINE_, Error::INVALID, "not opened");
      mlock_.unlock();
      return false;
    }
    if (!(omode_ & OWRITER)) {
      set_error(_KCCODELINE_, Error::NOPERM, "permission denied");
      mlock_.unlock();
      return false;
    }
    if (tran_) {
      set_error(_KCCODELINE_, Error::LOGIC, "competition avoided");
      mlock_.unlock();
      return false;
    }
    tran_ = true;
    trsize_ = size_;
    trigger_meta(MetaTrigger::BEGINTRAN, "begin_transaction_try");
    mlock_.unlock();
    return true;
  }
  /**
   * End transaction.
   * @param commit true to commit the transaction, or false to abort the transaction.
   * @return true on success, or false on failure.
   */
  bool end_transaction(bool commit = true) {
    _assert_(true);
    ScopedSpinRWLock lock(&mlock_, true);
    if (omode_ == 0) {
      set_error(_KCCODELINE_, Error::INVALID, "not opened");
      return false;
    }
    if (!tran_) {
      set_error(_KCCODELINE_, Error::INVALID, "not in transaction");
      return false;
    }
    if (!commit) {
      if (!curs_.empty()) {
        typename CursorList::const_iterator cit = curs_.begin();
        typename CursorList::const_iterator citend = curs_.end();
        while (cit != citend) {
          Cursor* cur = *cit;
          cur->it_ = recs_.end();
          cit++;
        }
      }
      const TranLogList& logs = trlogs_;
      typename TranLogList::const_iterator lit = logs.end();
      typename TranLogList::const_iterator litbeg = logs.begin();
      while (lit != litbeg) {
        lit--;
        if (lit->full) {
          recs_[lit->key] = lit->value;
        } else {
          recs_.erase(lit->key);
        }
      }
      size_ = trsize_;
    }
    trlogs_.clear();
    tran_ = false;
    trigger_meta(commit ? MetaTrigger::COMMITTRAN : MetaTrigger::ABORTTRAN, "end_transaction");
    return true;
  }
  /**
   * Remove all records.
   * @return true on success, or false on failure.
   */
  bool clear() {
    _assert_(true);
    ScopedSpinRWLock lock(&mlock_, true);
    if (omode_ == 0) {
      set_error(_KCCODELINE_, Error::INVALID, "not opened");
      return false;
    }
    recs_.clear();
    if (!curs_.empty()) {
      typename CursorList::const_iterator cit = curs_.begin();
      typename CursorList::const_iterator citend = curs_.end();
      while (cit != citend) {
        Cursor* cur = *cit;
        cur->it_ = recs_.end();
        cit++;
      }
    }
    std::memset(opaque_, 0, sizeof(opaque_));
    trigger_meta(MetaTrigger::CLEAR, "clear");
    return true;
  }
  /**
   * Get the number of records.
   * @return the number of records, or -1 on failure.
   */
  int64_t count() {
    _assert_(true);
    ScopedSpinRWLock lock(&mlock_, false);
    if (omode_ == 0) {
      set_error(_KCCODELINE_, Error::INVALID, "not opened");
      return -1;
    }
    return recs_.size();
  }
  /**
   * Get the size of the database file.
   * @return the size of the database file in bytes, or -1 on failure.
   */
  int64_t size() {
    _assert_(true);
    ScopedSpinRWLock lock(&mlock_, false);
    if (omode_ == 0) {
      set_error(_KCCODELINE_, Error::INVALID, "not opened");
      return -1;
    }
    return size_;
  }
  /**
   * Get the path of the database file.
   * @return the path of the database file, or an empty string on failure.
   */
  std::string path() {
    _assert_(true);
    ScopedSpinRWLock lock(&mlock_, false);
    if (omode_ == 0) {
      set_error(_KCCODELINE_, Error::INVALID, "not opened");
      return "";
    }
    return path_;
  }
  /**
   * Get the miscellaneous status information.
   * @param strmap a string map to contain the result.
   * @return true on success, or false on failure.
   */
  bool status(std::map<std::string, std::string>* strmap) {
    _assert_(strmap);
    ScopedSpinRWLock lock(&mlock_, true);
    if (omode_ == 0) {
      set_error(_KCCODELINE_, Error::INVALID, "not opened");
      return false;
    }
    (*strmap)["type"] = strprintf("%u", (unsigned)DBTYPE);
    (*strmap)["realtype"] = strprintf("%u", (unsigned)DBTYPE);
    (*strmap)["path"] = path_;
    if (strmap->count("opaque") > 0)
      (*strmap)["opaque"] = std::string(opaque_, sizeof(opaque_));
    (*strmap)["count"] = strprintf("%lld", (long long)recs_.size());
    (*strmap)["size"] = strprintf("%lld", (long long)size_);
    return true;
  }
  /**
   * Create a cursor object.
   * @return the return value is the created cursor object.
   * @note Because the object of the return value is allocated by the constructor, it should be
   * released with the delete operator when it is no longer in use.
   */
  Cursor* cursor() {
    _assert_(true);
    return new Cursor(this);
  }
  /**
   * Set the internal logger.
   * @param logger the logger object.
   * @param kinds kinds of logged messages by bitwise-or: Logger::DEBUG for debugging,
   * Logger::INFO for normal information, Logger::WARN for warning, and Logger::ERROR for fatal
   * error.
   * @return true on success, or false on failure.
   */
  bool tune_logger(Logger* logger, uint32_t kinds = Logger::WARN | Logger::ERROR) {
    _assert_(logger);
    ScopedSpinRWLock lock(&mlock_, true);
    if (omode_ != 0) {
      set_error(_KCCODELINE_, Error::INVALID, "already opened");
      return false;
    }
    logger_ = logger;
    logkinds_ = kinds;
    return true;
  }
  /**
   * Set the internal meta operation trigger.
   * @param trigger the trigger object.
   * @return true on success, or false on failure.
   */
  bool tune_meta_trigger(MetaTrigger* trigger) {
    _assert_(trigger);
    ScopedSpinRWLock lock(&mlock_, true);
    if (omode_ != 0) {
      set_error(_KCCODELINE_, Error::INVALID, "already opened");
      return false;
    }
    mtrigger_ = trigger;
    return true;
  }
  /**
   * Get the opaque data.
   * @return the pointer to the opaque data region, whose size is 16 bytes.
   */
  char* opaque() {
    _assert_(true);
    ScopedSpinRWLock lock(&mlock_, false);
    if (omode_ == 0) {
      set_error(_KCCODELINE_, Error::INVALID, "not opened");
      return NULL;
    }
    return opaque_;
  }
  /**
   * Synchronize the opaque data.
   * @return true on success, or false on failure.
   */
  bool synchronize_opaque() {
    _assert_(true);
    ScopedSpinRWLock lock(&mlock_, true);
    if (omode_ == 0) {
      set_error(_KCCODELINE_, Error::INVALID, "not opened");
      return false;
    }
    if (!(omode_ & OWRITER)) {
      set_error(_KCCODELINE_, Error::NOPERM, "permission denied");
      return false;
    }
    return true;
  }
protected:
  /**
   * Report a message for debugging.
   * @param file the file name of the program source code.
   * @param line the line number of the program source code.
   * @param func the function name of the program source code.
   * @param kind the kind of the event.  Logger::DEBUG for debugging, Logger::INFO for normal
   * information, Logger::WARN for warning, and Logger::ERROR for fatal error.
   * @param format the printf-like format string.
   * @param ... used according to the format string.
   */
  void report(const char* file, int32_t line, const char* func, Logger::Kind kind,
              const char* format, ...) {
    _assert_(file && line > 0 && func && format);
    if (!logger_ || !(kind & logkinds_)) return;
    std::string message;
    strprintf(&message, "%s: ", path_.empty() ? "-" : path_.c_str());
    va_list ap;
    va_start(ap, format);
    vstrprintf(&message, format, ap);
    va_end(ap);
    logger_->log(file, line, func, kind, message.c_str());
  }
  /**
   * Report a message for debugging with variable number of arguments.
   * @param file the file name of the program source code.
   * @param line the line number of the program source code.
   * @param func the function name of the program source code.
   * @param kind the kind of the event.  Logger::DEBUG for debugging, Logger::INFO for normal
   * information, Logger::WARN for warning, and Logger::ERROR for fatal error.
   * @param format the printf-like format string.
   * @param ap used according to the format string.
   */
  void report_valist(const char* file, int32_t line, const char* func, Logger::Kind kind,
                     const char* format, va_list ap) {
    _assert_(file && line > 0 && func && format);
    if (!logger_ || !(kind & logkinds_)) return;
    std::string message;
    strprintf(&message, "%s: ", path_.empty() ? "-" : path_.c_str());
    vstrprintf(&message, format, ap);
    logger_->log(file, line, func, kind, message.c_str());
  }
  /**
   * Report the content of a binary buffer for debugging.
   * @param file the file name of the epicenter.
   * @param line the line number of the epicenter.
   * @param func the function name of the program source code.
   * @param kind the kind of the event.  Logger::DEBUG for debugging, Logger::INFO for normal
   * information, Logger::WARN for warning, and Logger::ERROR for fatal error.
   * @param name the name of the information.
   * @param buf the binary buffer.
   * @param size the size of the binary buffer
   */
  void report_binary(const char* file, int32_t line, const char* func, Logger::Kind kind,
                     const char* name, const char* buf, size_t size) {
    _assert_(file && line > 0 && func && name && buf && size <= MEMMAXSIZ);
    if (!logger_) return;
    char* hex = hexencode(buf, size);
    report(file, line, func, kind, "%s=%s", name, hex);
    delete[] hex;
  }
  /**
   * Trigger a meta database operation.
   * @param kind the kind of the event.  MetaTrigger::OPEN for opening, MetaTrigger::CLOSE for
   * closing, MetaTrigger::CLEAR for clearing, MetaTrigger::ITERATE for iteration,
   * MetaTrigger::SYNCHRONIZE for synchronization, MetaTrigger::BEGINTRAN for beginning
   * transaction, MetaTrigger::COMMITTRAN for committing transaction, MetaTrigger::ABORTTRAN
   * for aborting transaction, and MetaTrigger::MISC for miscellaneous operations.
   * @param message the supplement message.
   */
  void trigger_meta(MetaTrigger::Kind kind, const char* message) {
    _assert_(message);
    if (mtrigger_) mtrigger_->trigger(kind, message);
  }
private:
  /**
   * Transaction log.
   */
  struct TranLog {
    bool full;                           ///< flag whether full
    std::string key;                     ///< old key
    std::string value;                   ///< old value
    /** constructor for a full record */
    explicit TranLog(const std::string& pkey, const std::string& pvalue) :
      full(true), key(pkey), value(pvalue) {
      _assert_(true);
    }
    /** constructor for an empty record */
    explicit TranLog(const std::string& pkey) : full(false), key(pkey) {
      _assert_(true);
    }
  };
  /** Dummy constructor to forbid the use. */
  ProtoDB(const ProtoDB&);
  /** Dummy Operator to forbid the use. */
  ProtoDB& operator =(const ProtoDB&);
  /** The method lock. */
  SpinRWLock mlock_;
  /** The last happened error. */
  TSD<Error> error_;
  /** The internal logger. */
  Logger* logger_;
  /** The kinds of logged messages. */
  uint32_t logkinds_;
  /** The internal meta operation trigger. */
  MetaTrigger* mtrigger_;
  /** The open mode. */
  uint32_t omode_;
  /** The map of records. */
  STRMAP recs_;
  /** The cursor objects. */
  CursorList curs_;
  /** The path of the database file. */
  std::string path_;
  /** The total size of records. */
  int64_t size_;
  /** The opaque data. */
  char opaque_[PDBOPAQUESIZ];
  /** The flag whether in transaction. */
  bool tran_;
  /** The transaction logs. */
  TranLogList trlogs_;
  /** The old size before transaction. */
  size_t trsize_;
};


/** An alias of the prototype hash database. */
typedef ProtoDB<StringHashMap, BasicDB::TYPEPHASH> ProtoHashDB;


/** An alias of the prototype tree database. */
typedef ProtoDB<StringTreeMap, BasicDB::TYPEPTREE> ProtoTreeDB;


}                                        // common namespace

#endif                                   // duplication check

// END OF FILE
