/*******************************************************************************
 * Copyright (c) 2018, 2019 IBM Corp. and others
 *
 * This program and the accompanying materials are made available under
 * the terms of the Eclipse Public License 2.0 which accompanies this
 * distribution and is available at https://www.eclipse.org/legal/epl-2.0/
 * or the Apache License, Version 2.0 which accompanies this distribution and
 * is available at https://www.apache.org/licenses/LICENSE-2.0.
 *
 * This Source Code may also be made available under the following
 * Secondary Licenses when the conditions for such availability set
 * forth in the Eclipse Public License, v. 2.0 are satisfied: GNU
 * General Public License, version 2 with the GNU Classpath
 * Exception [1] and GNU General Public License, version 2 with the
 * OpenJDK Assembly Exception [2].
 *
 * [1] https://www.gnu.org/software/classpath/license.html
 * [2] http://openjdk.java.net/legal/assembly-exception.html
 *
 * SPDX-License-Identifier: EPL-2.0 OR Apache-2.0 OR GPL-2.0 WITH Classpath-exception-2.0 OR LicenseRef-GPL-2.0 WITH Assembly-exception
 *******************************************************************************/

#ifndef JITaaS_COMPILATION_THREAD_H
#define JITaaS_COMPILATION_THREAD_H

#include <unordered_map>
#include "control/CompilationThread.hpp"
#include "rpc/J9Client.hpp"
#include "env/PersistentCollections.hpp"
#include "env/j9methodServer.hpp"

class TR_PersistentClassInfo;
class TR_IPBytecodeHashTableEntry;
struct TR_RemoteROMStringKey;

using IPTable_t = PersistentUnorderedMap<uint32_t, TR_IPBytecodeHashTableEntry*>;
using IPTableHeapEntry = UnorderedMap<uint32_t, TR_IPBytecodeHashTableEntry*>;
using IPTableHeap_t = UnorderedMap<J9Method *, IPTableHeapEntry *>;
using ResolvedMirrorMethodsPersistIP_t = Vector<TR_ResolvedJ9Method *>;
using ClassOfStatic_t = UnorderedMap<std::pair<TR_OpaqueClassBlock *, int32_t>, TR_OpaqueClassBlock *>;

struct ClassLoaderStringPair
   {
   J9ClassLoader *_classLoader;
   std::string    _className;

   bool operator==(const ClassLoaderStringPair &other) const
      {
      return _classLoader == other._classLoader &&  _className == other._className;
      }
   };


// custom specialization of std::hash injected in std namespace
namespace std
   {
   template<> struct hash<ClassLoaderStringPair>
      {
      typedef ClassLoaderStringPair argument_type;
      typedef std::size_t result_type;
      result_type operator()(argument_type const& clsPair) const noexcept
         {
         return std::hash<void*>()((void*)(clsPair._classLoader)) ^ std::hash<std::string>()(clsPair._className);
         }
      };

   template<typename T, typename Q> struct hash<std::pair<T, Q>>
      {
      std::size_t operator()(const std::pair<T, Q> &key) const noexcept
         {
         return std::hash<T>()(key.first) ^ std::hash<Q>()(key.second);
         }
      };
   }

struct ClassUnloadedData
   {
   TR_OpaqueClassBlock* _class;
   ClassLoaderStringPair _pair;
   J9ConstantPool *_cp;
   bool _cached;
   };

using TR_JitFieldsCacheEntry = std::pair<J9Class *, UDATA>;
using TR_JitFieldsCache = PersistentUnorderedMap<int32_t, TR_JitFieldsCacheEntry>;

class ClientSessionData
   {
   public:
   struct ClassInfo
      {
      void freeClassInfo(); // this method is in place of a destructor. We can't have destructor
      // because it would be called after inserting ClassInfo into the ROM map, freeing romClass
      J9ROMClass *romClass; // romClass content exists in persistentMemory at the server
      J9ROMClass *remoteRomClass; // pointer to the corresponding ROM class on the client
      J9Method *methodsOfClass;
      // Fields meaningful for arrays
      TR_OpaqueClassBlock *baseComponentClass; 
      int32_t numDimensions;
      PersistentUnorderedMap<TR_RemoteROMStringKey, std::string> *_remoteROMStringsCache; // cached strings from the client
      PersistentUnorderedMap<int32_t, std::string> *_fieldOrStaticNameCache;
      TR_OpaqueClassBlock *parentClass;
      PersistentVector<TR_OpaqueClassBlock *> *interfaces; 
      bool classHasFinalFields;
      uintptrj_t classDepthAndFlags;
      bool classInitialized;
      uint32_t byteOffsetToLockword;
      TR_OpaqueClassBlock * leafComponentClass;
      void *classLoader;
      TR_OpaqueClassBlock * hostClass;
      TR_OpaqueClassBlock * componentClass; // caching the componentType of the J9ArrayClass
      TR_OpaqueClassBlock * arrayClass;
      uintptrj_t totalInstanceSize;
      PersistentUnorderedMap<int32_t, TR_OpaqueClassBlock *> *_classOfStaticCache;
      PersistentUnorderedMap<int32_t, TR_OpaqueClassBlock *> *_constantClassPoolCache;
      TR_FieldAttributesCache *_fieldAttributesCache;
      TR_FieldAttributesCache *_staticAttributesCache;
      TR_FieldAttributesCache *_fieldAttributesCacheAOT;
      TR_FieldAttributesCache *_staticAttributesCacheAOT;
      J9ConstantPool *_constantPool;
      TR_JitFieldsCache *_jitFieldsCache;
      uintptrj_t classFlags;
      };

   struct J9MethodInfo
      {
      J9ROMMethod *_romMethod; // pointer to local/server cache
      // The following is a hashtable that maps a bcIndex to IProfiler data
      // The hashtable is created on demand (NULL means it is missing)
      IPTable_t *_IPData;
      bool _isMethodTracingEnabled;
      TR_OpaqueClassBlock * _owningClass;
      bool _isCompiledWhenProfiling; // To record if the method is compiled when doing Profiling
      };

   // This struct contains information about VM that does not change during its lifetime.
   struct VMInfo
      {
      void *_systemClassLoader;
      uintptrj_t _processID;
      bool _canMethodEnterEventBeHooked;
      bool _canMethodExitEventBeHooked;
      bool _usesDiscontiguousArraylets;
      bool _isIProfilerEnabled;
      int32_t _arrayletLeafLogSize;
      int32_t _arrayletLeafSize;
      uint64_t _overflowSafeAllocSize;
      int32_t _compressedReferenceShift;
      UDATA _cacheStartAddress;
      bool _stringCompressionEnabled;
      bool _hasSharedClassCache;
      bool _elgibleForPersistIprofileInfo;
      TR_OpaqueClassBlock *_arrayTypeClasses[8];
      bool _reportByteCodeInfoAtCatchBlock;
      MM_GCReadBarrierType _readBarrierType;
      MM_GCWriteBarrierType _writeBarrierType;
      bool _compressObjectReferences;
      };

   TR_PERSISTENT_ALLOC(TR_Memory::ClientSessionData)
   ClientSessionData(uint64_t clientUID, uint32_t seqNo);
   ~ClientSessionData();
   static void destroy(ClientSessionData *clientSession);

   void setJavaLangClassPtr(TR_OpaqueClassBlock* j9clazz) { _javaLangClassPtr = j9clazz; }
   TR_OpaqueClassBlock * getJavaLangClassPtr() const { return _javaLangClassPtr; }
   PersistentUnorderedMap<TR_OpaqueClassBlock*, TR_PersistentClassInfo*> & getCHTableClassMap() { return _chTableClassMap; }
   PersistentUnorderedMap<J9Class*, ClassInfo> & getROMClassMap() { return _romClassMap; }
   PersistentUnorderedMap<J9Method*, J9MethodInfo> & getJ9MethodMap() { return _J9MethodMap; }
   PersistentUnorderedMap<ClassLoaderStringPair, TR_OpaqueClassBlock*> & getClassByNameMap() { return _classByNameMap; }
   PersistentUnorderedMap<J9Class *, UDATA *> & getClassClainDataCache() { return _classChainDataMap; }
   PersistentUnorderedMap<J9ConstantPool *, TR_OpaqueClassBlock*> & getConstantPoolToClassMap() { return _constantPoolToClassMap; }
   void processUnloadedClasses(JITaaS::ServerStream *stream, const std::vector<TR_OpaqueClassBlock*> &classes);
   TR::Monitor *getROMMapMonitor() { return _romMapMonitor; }
   TR::Monitor *getClassMapMonitor() { return _classMapMonitor; }
   TR::Monitor *getClassChainDataMapMonitor() { return _classChainDataMapMonitor; }
   TR_IPBytecodeHashTableEntry *getCachedIProfilerInfo(TR_OpaqueMethodBlock *method, uint32_t byteCodeIndex, bool *methodInfoPresent);
   bool cacheIProfilerInfo(TR_OpaqueMethodBlock *method, uint32_t byteCodeIndex, TR_IPBytecodeHashTableEntry *entry);
   VMInfo *getOrCacheVMInfo(JITaaS::ServerStream *stream);
   void clearCaches(); // destroys _chTableClassMap, _romClassMap and _J9MethodMap
   TR_AddressSet& getUnloadedClassAddresses()
      {
      TR_ASSERT(_unloadedClassAddresses, "Unloaded classes address set should exist by now");
      return *_unloadedClassAddresses;
      }

   void incInUse() { _inUse++; }
   void decInUse() { _inUse--; TR_ASSERT(_inUse >= 0, "_inUse=%d must be positive\n", _inUse); }
   bool getInUse() const { return _inUse; }

   uint64_t getClientUID() const { return _clientUID; }
   void updateTimeOfLastAccess();
   int64_t getTimeOflastAccess() const { return _timeOfLastAccess; }

   TR::Monitor *getSequencingMonitor() { return _sequencingMonitor; }
   TR::Monitor *getConstantPoolMonitor() { return _constantPoolMapMonitor; }
   TR_MethodToBeCompiled *getOOSequenceEntryList() { return _OOSequenceEntryList; }
   TR_MethodToBeCompiled *notifyAndDetachFirstWaitingThread();
   void insertIntoOOSequenceEntryList(TR_MethodToBeCompiled *entry);
   uint32_t getExpectedSeqNo() const { return _expectedSeqNo; }
   void setExpectedSeqNo(uint32_t seqNo) { _expectedSeqNo = seqNo; }
   uint32_t getMaxReceivedSeqNo() const { return _maxReceivedSeqNo; }
   void updateMaxReceivedSeqNo(uint32_t seqNo);
   int8_t getNumActiveThreads() const { return _numActiveThreads; }
   void incNumActiveThreads() { ++_numActiveThreads; }
   void decNumActiveThreads() { --_numActiveThreads;  }
   void printStats();

   void markForDeletion() { _markedForDeletion = true;}
   bool isMarkedForDeletion() const { return _markedForDeletion; }

   TR::Monitor *getStaticMapMonitor() { return _staticMapMonitor; }
   PersistentUnorderedMap<void *, TR_StaticFinalData> &getStaticFinalDataMap() { return _staticFinalDataMap; }

   bool getRtResolve() { return _rtResolve; }
   void setRtResolve(bool rtResolve) { _rtResolve = rtResolve; }

   private:
   const uint64_t _clientUID;
   int64_t  _timeOfLastAccess; // in ms
   TR_OpaqueClassBlock *_javaLangClassPtr; // NULL means not set
   // Server side cache of CHTable
   PersistentUnorderedMap<TR_OpaqueClassBlock*, TR_PersistentClassInfo*> _chTableClassMap;
   // Server side cache of j9classes and their properties; romClass is copied so it can be accessed by the server
   PersistentUnorderedMap<J9Class*, ClassInfo> _romClassMap;
   // Hashtable for information related to one J9Method
   PersistentUnorderedMap<J9Method*, J9MethodInfo> _J9MethodMap;
   // The following hashtable caches <classname> --> <J9Class> mappings
   // All classes in here are loaded by the systemClassLoader so we know they cannot be unloaded
   PersistentUnorderedMap<ClassLoaderStringPair, TR_OpaqueClassBlock*> _classByNameMap;

   PersistentUnorderedMap<J9Class *, UDATA *> _classChainDataMap;
   //Constant pool to class map
   PersistentUnorderedMap<J9ConstantPool *, TR_OpaqueClassBlock *> _constantPoolToClassMap;
   TR::Monitor *_romMapMonitor;
   TR::Monitor *_classMapMonitor;
   TR::Monitor *_classChainDataMapMonitor;
   // The following monitor is used to protect access to _expectedSeqNo and 
   // the list of out-of-sequence compilation requests (_OOSequenceEntryList)
   TR::Monitor *_sequencingMonitor;
   TR::Monitor *_constantPoolMapMonitor;
   // Compilation requests that arrived out-of-sequence wait in 
   // _OOSequenceEntryList for their turn to be processed
   TR_MethodToBeCompiled *_OOSequenceEntryList;
   uint32_t _expectedSeqNo; // used for ordering compilation requests from the same client
   uint32_t _maxReceivedSeqNo; // the largest seqNo received from this client
   int8_t  _inUse;  // Number of concurrent compilations from the same client 
                    // Accessed with compilation monitor in hand
   int8_t _numActiveThreads; // Number of threads working on compilations for this client
                             // This is smaller or equal to _inUse because some threads
                             // could be just starting or waiting in _OOSequenceEntryList
   VMInfo *_vmInfo; // info specific to a client VM that does not change, NULL means not set
   bool _markedForDeletion; //Client Session is marked for deletion. When the inUse count will become zero this will be deleted.
   TR_AddressSet *_unloadedClassAddresses; // Per-client versions of the unloaded class and method addresses kept in J9PersistentInfo
   bool           _requestUnloadedClasses; // If true we need to request the current state of unloaded classes from the client
   TR::Monitor *_staticMapMonitor;
   PersistentUnorderedMap<void *, TR_StaticFinalData> _staticFinalDataMap; // stores values at static final addresses in JVM
   bool _rtResolve; // treat all data references as unresolved
   }; // ClientSessionData

// Hashtable that maps clientUID to a pointer that points to ClientSessionData
// This indirection is needed so that we can cache the value of the pointer so
// that we can access client session data without going through the hashtable.
// Accesss to this hashtable must be protected by the compilation monitor.
// Compilation threads may purge old entries periodically at the beginning of a 
// compilation. Entried with inUse > 0 must not be purged.
class ClientSessionHT
   {
   public:
   ClientSessionHT();
   ~ClientSessionHT();
   static ClientSessionHT* allocate(); // allocates a new instance of this class
   ClientSessionData * findOrCreateClientSession(uint64_t clientUID, uint32_t seqNo, bool *newSessionWasCreated);
   bool deleteClientSession(uint64_t clientUID, bool forDeletion);
   ClientSessionData * findClientSession(uint64_t clientUID);
   void purgeOldDataIfNeeded();
   void printStats();
   uint32_t size() const { return _clientSessionMap.size(); }
   private:
   PersistentUnorderedMap<uint64_t, ClientSessionData*> _clientSessionMap;

   uint64_t _timeOfLastPurge;
   const int64_t TIME_BETWEEN_PURGES; // ms; this defines how often we are willing to scan for old entries to be purged
   const int64_t OLD_AGE;// ms; this defines what an old entry means
                         // This value must be larger than the expected life of a JVM
   }; // ClientSessionHT

size_t methodStringLength(J9ROMMethod *);
std::string packROMClass(J9ROMClass *, TR_Memory *);
bool handleServerMessage(JITaaS::ClientStream *, TR_J9VM *);
TR_MethodMetaData *remoteCompile(J9VMThread *, TR::Compilation *, TR_ResolvedMethod *,
      J9Method *, TR::IlGeneratorMethodDetails &, TR::CompilationInfoPerThreadBase *);
TR_MethodMetaData *remoteCompilationEnd(J9VMThread * vmThread, TR::Compilation *comp, TR_ResolvedMethod * compilee, J9Method * method,
                          TR::CompilationInfoPerThreadBase *compInfoPT, const std::string& codeCacheStr, const std::string& dataCacheStr);
void outOfProcessCompilationEnd(TR_MethodToBeCompiled *entry, TR::Compilation *comp);
void printJITaaSMsgStats(J9JITConfig *);
void printJITaaSCHTableStats(J9JITConfig *, TR::CompilationInfo *);
void printJITaaSCacheStats(J9JITConfig *, TR::CompilationInfo *);

namespace TR
{
// Objects of this type are instantiated at JITaaS server
class CompilationInfoPerThreadRemote : public TR::CompilationInfoPerThread
   {
   public:
      friend class TR::CompilationInfo;
      CompilationInfoPerThreadRemote(TR::CompilationInfo &compInfo, J9JITConfig *jitConfig, int32_t id, bool isDiagnosticThread);
      virtual void processEntry(TR_MethodToBeCompiled &entry, J9::J9SegmentProvider &scratchSegmentProvider) override;
      TR_PersistentMethodInfo *getRecompilationMethodInfo() { return _recompilationMethodInfo; }
      uint32_t getSeqNo() const { return _seqNo; }; // for ordering requests at the server
      void setSeqNo(uint32_t seqNo) { _seqNo = seqNo; }
      void waitForMyTurn(ClientSessionData *clientSession, TR_MethodToBeCompiled &entry); // return false if timeout
      bool getWaitToBeNotified() const { return _waitToBeNotified; }
      void setWaitToBeNotified(bool b) { _waitToBeNotified = b; }

      bool cacheIProfilerInfo(TR_OpaqueMethodBlock *method, uint32_t byteCodeIndex, TR_IPBytecodeHashTableEntry *entry);
      TR_IPBytecodeHashTableEntry *getCachedIProfilerInfo(TR_OpaqueMethodBlock *method, uint32_t byteCodeIndex, bool *methodInfoPresent);
      void cacheResolvedMethod(TR_ResolvedMethodKey key, TR_OpaqueMethodBlock *method, uint32_t vTableSlot, TR_ResolvedJ9JITaaSServerMethodInfo &methodInfo);
      bool getCachedResolvedMethod(TR_ResolvedMethodKey key, TR_ResolvedJ9JITaaSServerMethod *owningMethod, TR_ResolvedMethod **resolvedMethod, bool *unresolvedInCP = NULL);
      TR_ResolvedMethodKey getResolvedMethodKey(TR_ResolvedMethodType type, TR_OpaqueClassBlock *ramClass, int32_t cpIndex, TR_OpaqueClassBlock *classObject=NULL);

      void cacheResolvedMirrorMethodsPersistIPInfo(TR_ResolvedJ9Method *resolvedMethod);
      ResolvedMirrorMethodsPersistIP_t *getCachedResolvedMirrorMethodsPersistIPInfo() { return _resolvedMirrorMethodsPersistIPInfo; }

      void cacheNullClassOfStatic(TR_OpaqueClassBlock *ramClass, int32_t cpIndex);
      bool getCachedNullClassOfStatic(TR_OpaqueClassBlock *ramClass, int32_t cpIndex);

      void clearPerCompilationCaches();
      void deleteClientSessionData(uint64_t clientId, TR::CompilationInfo* compInfo, J9VMThread* compThread);

   private:
      /* Template method for allocating a cache of type T on the heap.
       * Cache pointer must be NULL.
       */
      template <typename T>
      bool initializePerCompilationCache(T* &cache)
         {
         // Initialize map
         TR_ASSERT(!cache, "Cache already initialized");
         TR_Memory *trMemory = getCompilation()->trMemory();
         cache = new (trMemory->trHeapMemory()) T(typename T::allocator_type(trMemory->heapMemoryRegion()));
         return cache != NULL;
         }
      /* Template method for storing key-value pairs (of types K and V respectively)
       * to a heap-allocated unordered map.
       * If a map is NULL, will allocate it.
       */
      template <typename K, typename V>
      void cacheToPerCompilationMap(UnorderedMap<K, V>* &map, const K &key, const V &value)
         {
         if (!map)
            initializePerCompilationCache(map);
         map->insert({key, value});
         }

      /* Template method for retrieving values from heap-allocated unordered map.
       * If the map is NULL or value is not found, returns false.
       * Otherwise, sets value to retrieved value and returns true
       */
      template <typename K, typename V>
      bool getCachedValueFromPerCompilationMap(UnorderedMap<K, V>* &map, const K &key, V &value)
         {
         if (!map)
            return false;
         auto it = map->find(key);
         if (it != map->end())
            {
            // Found entry at a given key
            value = it->second;
            return true;
            }
         return false;
         }

      /* Template method for clearing a heap-allocated cache.
       * Simply sets pointer to cache to NULL.
       */
      template <typename T>
      void clearPerCompilationCache(T* &cache)
         {
         // Since cache was heap-allocated,
         // memory will be released automatically at the end of the compilation
         cache = NULL;
         }

      TR_PersistentMethodInfo *_recompilationMethodInfo;
      uint32_t _seqNo;
      bool _waitToBeNotified; // accessed with clientSession->_sequencingMonitor in hand
      IPTableHeap_t *_methodIPDataPerComp;
      TR_ResolvedMethodInfoCache *_resolvedMethodInfoMap;
      ResolvedMirrorMethodsPersistIP_t *_resolvedMirrorMethodsPersistIPInfo; //list of mirrors of resolved methods for persisting IProfiler info
      ClassOfStatic_t *_classOfStaticMap;
   };
}

class JITaaSHelpers
   {
   public:
   enum ClassInfoDataType
      {
      CLASSINFO_ROMCLASS_MODIFIERS,
      CLASSINFO_ROMCLASS_EXTRAMODIFIERS,
      CLASSINFO_BASE_COMPONENT_CLASS,
      CLASSINFO_NUMBER_DIMENSIONS,
      CLASSINFO_PARENT_CLASS,
      CLASSINFO_INTERFACE_CLASS,
      CLASSINFO_CLASS_HAS_FINAL_FIELDS,
      CLASSINFO_CLASS_DEPTH_AND_FLAGS,
      CLASSINFO_CLASS_INITIALIZED,
      CLASSINFO_BYTE_OFFSET_TO_LOCKWORD,
      CLASSINFO_LEAF_COMPONENT_CLASS,
      CLASSINFO_CLASS_LOADER,
      CLASSINFO_HOST_CLASS,
      CLASSINFO_COMPONENT_CLASS,
      CLASSINFO_ARRAY_CLASS,
      CLASSINFO_TOTAL_INSTANCE_SIZE,
      CLASSINFO_CLASS_OF_STATIC_CACHE,
      CLASSINFO_REMOTE_ROM_CLASS,
      CLASSINFO_CLASS_FLAGS,
      CLASSINFO_METHODS_OF_CLASS,
      };
   // NOTE: when adding new elements to this tuple, add them to the end,
   // to not mess with the established order.
   using ClassInfoTuple = std::tuple
      <
      std::string, J9Method *,                                       // 0: string                  1: methodsOfClass
      TR_OpaqueClassBlock *, int32_t,                                // 2: baseComponentClass      3: numDimensions
      TR_OpaqueClassBlock *, std::vector<TR_OpaqueClassBlock *>,     // 4: parentClass             5: tmpInterfaces
      std::vector<uint8_t>, bool,                                    // 6: methodTracingInfo       7: classHasFinalFields
      uintptrj_t, bool,                                              // 8: classDepthAndFlags      9: classInitialized
      uint32_t, TR_OpaqueClassBlock *,                               // 10: byteOffsetToLockword   11: leafComponentClass
      void *, TR_OpaqueClassBlock *,                                 // 12: classLoader            13: hostClass
      TR_OpaqueClassBlock *, TR_OpaqueClassBlock *,                  // 14: componentClass         15: arrayClass
      uintptrj_t, J9ROMClass *,                                      // 16: totalInstanceSize      17: remoteRomClass
      uintptrj_t, uintptrj_t                                         // 18: _constantPool          19: classFlags
      >;
   static ClassInfoTuple packRemoteROMClassInfo(J9Class *clazz, J9VMThread *vmThread, TR_Memory *trMemory);
   static void cacheRemoteROMClass(ClientSessionData *clientSessionData, J9Class *clazz, J9ROMClass *romClass, ClassInfoTuple *classInfoTuple);
   static void cacheRemoteROMClass(ClientSessionData *clientSessionData, J9Class *clazz, J9ROMClass *romClass, ClassInfoTuple *classInfoTuple, ClientSessionData::ClassInfo &classInfo);
   static J9ROMClass *getRemoteROMClassIfCached(ClientSessionData *clientSessionData, J9Class *clazz);
   static J9ROMClass *getRemoteROMClass(J9Class *, JITaaS::ServerStream *stream, TR_Memory *trMemory, ClassInfoTuple *classInfoTuple);
   static J9ROMClass *romClassFromString(const std::string &romClassStr, TR_PersistentMemory *trMemory);
   static bool getAndCacheRAMClassInfo(J9Class *clazz, ClientSessionData *clientSessionData, JITaaS::ServerStream *stream, ClassInfoDataType dataType,  void *data);
   static bool getAndCacheRAMClassInfo(J9Class *clazz, ClientSessionData *clientSessionData, JITaaS::ServerStream *stream, ClassInfoDataType dataType1, void *data1, ClassInfoDataType dataType2, void *data2);
   static void getROMClassData(const ClientSessionData::ClassInfo &classInfo, ClassInfoDataType dataType, void *data);
   //purgeCache function can only be used inside the JITaaSCompilationThread.cpp file.
   //It is a templated function, calling it outside the JITaaSCompilationThread.cpp will give linking error. 
   template <typename map, typename key>
   static void purgeCache (std::vector<ClassUnloadedData> *unloadedClasses, map m, key ClassUnloadedData::*k);
   };

#endif
