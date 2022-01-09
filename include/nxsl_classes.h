/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2020 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published by
** the Free Software Foundation; either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** File: nxsl_classes.h
**
**/

#ifndef _nxsl_classes_h_
#define _nxsl_classes_h_

#include <nms_threads.h>
#include <nms_util.h>
#include <nxcpapi.h>
#include <geolocation.h>

//
// Constants
//

#define MAX_CLASS_NAME     64
#define INVALID_ADDRESS    ((uint32_t)0xFFFFFFFF)

/**
 * NXSL data types
 */
enum NXSL_DataTypes
{
   NXSL_DT_NULL       = 0,
   NXSL_DT_OBJECT     = 1,
   NXSL_DT_ARRAY      = 2,
   NXSL_DT_ITERATOR   = 3,
   NXSL_DT_HASHMAP    = 4,
   NXSL_DT_STRING     = 5,
   NXSL_DT_BOOLEAN    = 6,
   NXSL_DT_REAL       = 7,
   NXSL_DT_INT32      = 8,
   NXSL_DT_INT64      = 9,
   NXSL_DT_UINT32     = 10,
   NXSL_DT_UINT64     = 11
};

/**
 * Internal identifier representation for parser
 */
struct identifier_t
{
   char v[MAX_IDENTIFIER_LENGTH];
};

// Disable strncpy warning on Windows
#ifdef _WIN32
#pragma warning(disable: 4996)
#endif

/**
 * NXSL identifier
 */
struct LIBNXSL_EXPORTABLE NXSL_Identifier
{
   BYTE length;
   char value[MAX_IDENTIFIER_LENGTH];

   NXSL_Identifier()
   {
      length = 0;
      memset(value, 0, sizeof(value));
   }

   NXSL_Identifier(const char *s)
   {
      memset(value, 0, sizeof(value));
      strncpy(value, s, MAX_IDENTIFIER_LENGTH - 1);
      length = (BYTE)strlen(value);
   }

#ifdef UNICODE
   NXSL_Identifier(const WCHAR *s)
   {
      memset(value, 0, sizeof(value));
      wchar_to_utf8(s, -1, value, MAX_IDENTIFIER_LENGTH - 1);
      length = (BYTE)strlen(value);
   }
#endif

   NXSL_Identifier(const identifier_t& s)
   {
      memset(value, 0, sizeof(value));
      strncpy(value, s.v, MAX_IDENTIFIER_LENGTH - 1);
      length = (BYTE)strlen(value);
   }

   NXSL_Identifier(const NXSL_Identifier& src)
   {
      memcpy(this, &src, sizeof(NXSL_Identifier));
   }

   NXSL_Identifier& operator=(const NXSL_Identifier& src)
   {
      memcpy(this, &src, sizeof(NXSL_Identifier));
      return *this;
   }

   bool equals(const NXSL_Identifier& i) const
   {
      return (i.length == length) && !strcmp(i.value, value);
   }
};

#ifdef _WIN32
#pragma warning(default: 4996)
#endif

/**
 * NXSL stack class
 */
class LIBNXSL_EXPORTABLE NXSL_Stack
{
private:
   int m_size;
   int m_pos;
   void **m_data;

public:
   NXSL_Stack()
   {
      m_size = 0;
      m_pos = 0;
      m_data = nullptr;
   }

   ~NXSL_Stack()
   {
      MemFree(m_data);
   }

   /**
    * Push value to stack
    */
   void push(void *data)
   {
      if (m_pos >= m_size)
      {
         m_size += 128;
         m_data = MemReallocArray(m_data, m_size);
      }
      m_data[m_pos++] = data;
   }

   /**
    * Pop value from stack
    */
   void *pop()
   {
      return (m_pos > 0) ? m_data[--m_pos] : nullptr;
   }

   /**
    * Peek (get without removing) value from stack
    */
   void *peek()
   {
      return (m_pos > 0) ? m_data[m_pos - 1] : nullptr;
   }

   /**
    * Peek (get without removing) value from stack at given offset from top
    */
   void *peekAt(int offset)
   {
      return ((offset > 0) && (m_pos > offset - 1)) ? m_data[m_pos - offset] : nullptr;
   }

   /**
    * Peek list of elements
    */
   void **peekList(int level)
   {
      return &m_data[m_pos - level];
   }

   /**
    * Reste stack
    */
   void reset()
   {
      m_pos = 0;
   }

   /**
    * Get current stack position
    */
   int getPosition() const
   {
      return m_pos;
   }
};

/**
 * NXSL object stack class
 */
template <typename T> class NXSL_ObjectStack : public NXSL_Stack
{
public:
   NXSL_ObjectStack() : NXSL_Stack() { }

   void push(T *data) { NXSL_Stack::push(data); }
   T *pop() { return (T*)NXSL_Stack::pop(); }
   T *peek() { return (T*)NXSL_Stack::peek(); }
   T *peekAt(int offset) { return (T*)NXSL_Stack::peekAt(offset); }
};

class NXSL_Value;
class NXSL_Object;
class NXSL_VM;
class NXSL_ValueManager;

/**
 * Runtime object
 */
class LIBNXSL_EXPORTABLE NXSL_RuntimeObject
{
protected:
   NXSL_VM *m_vm;

public:
   NXSL_RuntimeObject(NXSL_VM *vm) { m_vm = vm; }

   NXSL_VM *vm() { return m_vm; }
};

/**
 * External method structure
 */
struct NXSL_ExtMethod
{
   int (* handler)(NXSL_Object *object, int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
   int numArgs;   // Number of arguments or -1 for variable number
};

#define NXSL_METHOD_DEFINITION(clazz, name) \
   static int M_##clazz##_##name (NXSL_Object *object, int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)

#define NXSL_REGISTER_METHOD(clazz, name, argc) { \
      NXSL_ExtMethod *m = new NXSL_ExtMethod; \
      m->handler = M_##clazz##_##name; \
      m->numArgs = argc; \
      m_methods->set(#name, m); \
   }

/**
 * Class representing NXSL class
 */
class LIBNXSL_EXPORTABLE NXSL_Class
{
   friend class NXSL_MetaClass;

private:
   TCHAR m_name[MAX_CLASS_NAME];
   StringList m_classHierarchy;
   StringSet m_attributes;
   Mutex m_metadataLock;

protected:
   HashMap<NXSL_Identifier, NXSL_ExtMethod> *m_methods;

   void setName(const TCHAR *name);
   const StringList& getClassHierarchy() const { return m_classHierarchy; }
   const StringSet& getAttributes() const { return m_attributes; }

   bool compareAttributeName(const char *name, const char *tmpl)
   {
      if (*name == '?')
      {
#ifdef UNICODE
         m_attributes.addPreallocated(WideStringFromUTF8String(tmpl));
#else
         m_attributes.add(tmpl);
#endif
         return false;
      }
      return strcmp(name, tmpl) == 0;
   }

public:
   NXSL_Class();
   virtual ~NXSL_Class();

   virtual NXSL_Value *getAttr(NXSL_Object *object, const char *attr);
   virtual bool setAttr(NXSL_Object *object, const char *attr, NXSL_Value *value);

   virtual int callMethod(const NXSL_Identifier& name, NXSL_Object *object, int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);

   virtual void onObjectCreate(NXSL_Object *object);
	virtual void onObjectDelete(NXSL_Object *object);

   const TCHAR *getName() const { return m_name; }
   bool instanceOf(const TCHAR *name) const { return !_tcscmp(name, m_name) || m_classHierarchy.contains(name); }

   void scanAttributes();
};

/**
 * Class data - object and reference count
 */
struct __nxsl_class_data
{
	void *data;
	int refCount;
	bool constant;
};

/**
 * Object instance
 * Accept const void * in constructor to allow for both const and non-const pointers. Actual
 * constant state determined by optional "constant" parameter.
 */
class LIBNXSL_EXPORTABLE NXSL_Object : public NXSL_RuntimeObject
{
private:
   NXSL_Class *m_class;
   __nxsl_class_data *m_data;

public:
   NXSL_Object(NXSL_Object *object);
   NXSL_Object(NXSL_VM *vm, NXSL_Class *nxslClass, const void *data, bool constant = false);
   virtual ~NXSL_Object();

   NXSL_Class *getClass() { return m_class; }
	void *getData() { return m_data->data; }
	bool isConstant() { return m_data->constant; }
};

/**
 * Reference counting object
 */
class LIBNXSL_EXPORTABLE NXSL_HandleCountObject
{
protected:
   NXSL_ValueManager *m_vm;
   int m_handleCount;

public:
   NXSL_HandleCountObject(NXSL_ValueManager *vm)
   {
      m_vm = vm;
      m_handleCount = 0;
   }

	void incHandleCount() { m_handleCount++; }
	void decHandleCount() { m_handleCount--; }
	bool isUnused() { return m_handleCount < 1; }
   bool isShared() { return m_handleCount > 1; }
   NXSL_ValueManager *vm() { return m_vm; }
};

/**
 * Array element
 */
struct NXSL_ArrayElement
{
	int index;
	NXSL_Value *value;
};

/**
 * NXSL array
 */
class LIBNXSL_EXPORTABLE NXSL_Array : public NXSL_HandleCountObject
{
private:
	int m_size;
	int m_allocated;
	NXSL_ArrayElement *m_data;

public:
	NXSL_Array(NXSL_ValueManager *vm);
	NXSL_Array(const NXSL_Array& src);
   NXSL_Array(NXSL_ValueManager *vm, const StringList& values);
   NXSL_Array(NXSL_ValueManager *vm, const StringSet& values);
	virtual ~NXSL_Array();

   int size() const { return m_size; }
   int getMinIndex() const { return (m_size > 0) ? m_data[0].index : 0; }
   int getMaxIndex() const { return (m_size > 0) ? m_data[m_size - 1].index : 0; }

   StringList *toStringList() const;
   void toStringList(StringList *list) const;
   StringSet *toStringSet() const;
   void toStringSet(StringSet *set) const;
   void toString(StringBuffer *stringBuffer, const TCHAR *separator, bool withBrackets) const;

   NXSL_Value *get(int index) const;
   NXSL_Value *getByPosition(int position) const;

   bool contains(NXSL_Value *value);

   void set(int index, NXSL_Value *value);
	void append(NXSL_Value *value) { if (m_size == 0) { set(0, value); } else { set(getMaxIndex() + 1, value); } }
   void insert(int index, NXSL_Value *value);
   void remove(int index);

   int callMethod(const NXSL_Identifier& name, int argc, NXSL_Value **argv, NXSL_Value **result);
};

/**
 * String map for holding NXSL_Value objects
 */
class NXSL_StringValueMap : public StringMapBase
{
   DISABLE_COPY_CTOR(NXSL_StringValueMap)

private:
   NXSL_ValueManager *m_vm;

   static void destructor(void *object, StringMapBase *map);

public:
   NXSL_StringValueMap(NXSL_ValueManager *vm, Ownership objectOwner) : StringMapBase(objectOwner) { m_vm = vm; m_objectDestructor = destructor; }

   void set(const TCHAR *key, NXSL_Value *object) { setObject((TCHAR *)key, (void *)object, false); }
   void setPreallocated(TCHAR *key, NXSL_Value *object) { setObject((TCHAR *)key, (void *)object, true); }
   NXSL_Value *get(const TCHAR *key) const { return (NXSL_Value*)getObject(key); }
   NXSL_Value *get(const TCHAR *key, size_t keyLen) const { return (NXSL_Value*)getObject(key, keyLen); }
};

/**
 * NXSL hash map
 */
class LIBNXSL_EXPORTABLE NXSL_HashMap : public NXSL_HandleCountObject
{
private:
   NXSL_StringValueMap *m_values;

public:
   NXSL_HashMap(NXSL_ValueManager *vm, const StringMap *values = nullptr);
	NXSL_HashMap(const NXSL_HashMap& src);
	virtual ~NXSL_HashMap();

   void set(const TCHAR *key, NXSL_Value *value) { m_values->set(key, value); }
   NXSL_Value *get(const TCHAR *key) const { return m_values->get(key); }
   NXSL_Value *getKeys() const;
   NXSL_Value *getValues() const;
   bool contains(const TCHAR *key) const { return m_values->contains(key); }

   StringMap *toStringMap() const;
   void toStringMap(StringMap *map) const;
   void toString(StringBuffer *stringBuffer, const TCHAR *separator, bool withBrackets) const;

	int size() const { return m_values->size(); }

   int callMethod(const NXSL_Identifier& name, int argc, NXSL_Value **argv, NXSL_Value **result);
};

/**
 * Iterator for arrays
 */
class LIBNXSL_EXPORTABLE NXSL_Iterator : public NXSL_RuntimeObject
{
private:
	int m_refCount;
	NXSL_Identifier m_variable;
	NXSL_Array *m_array;
	int m_position;

public:
	NXSL_Iterator(NXSL_VM *vm, const NXSL_Identifier& variable, NXSL_Array *array);
	virtual ~NXSL_Iterator();

	const NXSL_Identifier& getVariableName() { return m_variable; }

	void incRefCount() { m_refCount++; }
	void decRefCount() { m_refCount--; }
	bool isUnused() { return m_refCount < 1; }

	NXSL_Value *next();

	static int createIterator(NXSL_VM *vm, NXSL_Stack *stack);
};

/**
 * Object handle
 */
template <class T> class NXSL_Handle
{
private:
   T *m_object;
   int m_refCount;

public:
   NXSL_Handle(T *o) 
   { 
      m_object = o; 
      o->incHandleCount(); 
      m_refCount = 0; 
   }
   NXSL_Handle(NXSL_Handle<T> *h) 
   { 
      m_object = h->m_object; 
      m_object->incHandleCount(); 
      m_refCount = 0; 
   }
   ~NXSL_Handle() 
   { 
      m_object->decHandleCount(); 
      if (m_object->isUnused()) 
         delete m_object; 
   }

	void incRefCount() { m_refCount++; }
	void decRefCount() { m_refCount--; }
	bool isUnused() { return m_refCount < 1; }
   bool isShared() { return m_refCount > 1; }

   T *getObject() { return m_object; }
   bool isSharedObject() { return m_object->isShared(); }

   void cloneObject()
   {
      m_object->decHandleCount();
      m_object = new T(*m_object);
      m_object->incHandleCount();
   }
};

/**
 * Maximum length of short string (when allocation is not needed)
 */
#define NXSL_SHORT_STRING_LENGTH  32

/**
 * Variable or constant value
 */
class LIBNXSL_EXPORTABLE NXSL_Value
{
   friend class NXSL_ValueManager;
   friend class ObjectMemoryPool<NXSL_Value>;

protected:
   uint32_t m_length;
   TCHAR m_stringValue[NXSL_SHORT_STRING_LENGTH];
   TCHAR *m_stringPtr;
#ifdef UNICODE
	char *m_mbString;	// value as MB string; NULL until first request
#endif
	char *m_name;
   BYTE m_dataType;
   BYTE m_stringIsValid;
   union
   {
      int32_t int32;
      uint32_t uint32;
      int64_t int64;
      uint64_t uint64;
      double real;
      NXSL_Object *object;
		NXSL_Iterator *iterator;
		NXSL_Handle<NXSL_Array> *arrayHandle;
      NXSL_Handle<NXSL_HashMap> *hashMapHandle;
   } m_value;

   template<typename T> T getValueAsIntegerType()
   {
      switch(m_dataType)
      {
         case NXSL_DT_BOOLEAN: return (T)(m_value.int32 ? 1 : 0);
         case NXSL_DT_INT32: return (T)m_value.int32;
         case NXSL_DT_UINT32: return (T)m_value.uint32;
         case NXSL_DT_INT64: return (T)m_value.int64;
         case NXSL_DT_UINT64: return (T)m_value.uint64;
         case NXSL_DT_REAL: return (T)m_value.real;
         default: return 0;
      }
   }

   void updateNumber();
   void updateString();

   void invalidateString()
   {
      MemFreeAndNull(m_stringPtr);
#ifdef UNICODE
      MemFreeAndNull(m_mbString);
#endif
      m_stringIsValid = FALSE;
   }
   void dispose(bool disposeString = true);

   NXSL_Value();
   NXSL_Value(const NXSL_Value *src);
   NXSL_Value(NXSL_Object *object);
   NXSL_Value(NXSL_Array *array);
   NXSL_Value(NXSL_Iterator *iterator);
   NXSL_Value(NXSL_HashMap *hashMap);
   NXSL_Value(int32_t nValue);
   NXSL_Value(int64_t nValue);
   NXSL_Value(uint32_t uValue);
   NXSL_Value(uint64_t uValue);
   NXSL_Value(double dValue);
   NXSL_Value(bool bValue);
   NXSL_Value(const TCHAR *value);
   NXSL_Value(const TCHAR *value, size_t len);
#ifdef UNICODE
   NXSL_Value(const char *value);
#endif
   ~NXSL_Value();

public:
   void set(bool value);
   void set(int32_t value);

	void setName(const char *name) { MemFree(m_name); m_name = MemCopyStringA(name); }
	const char *getName() const { return m_name; }

   bool convert(int targetDataType);
   int getDataType() const { return m_dataType; }

   bool isNull() const { return (m_dataType == NXSL_DT_NULL); }
   bool isObject() const { return (m_dataType == NXSL_DT_OBJECT); }
	bool isObject(const TCHAR *className) const;
   bool isArray() const { return (m_dataType == NXSL_DT_ARRAY); }
   bool isHashMap() const { return (m_dataType == NXSL_DT_HASHMAP); }
   bool isIterator() const { return (m_dataType == NXSL_DT_ITERATOR); }
   bool isString() const { return (m_dataType >= NXSL_DT_STRING); }
   bool isNumeric() const { return (m_dataType >= NXSL_DT_REAL); }
   bool isReal() const { return (m_dataType == NXSL_DT_REAL); }
   bool isInteger() const { return (m_dataType > NXSL_DT_REAL); }
   bool isUnsigned() const { return (m_dataType >= NXSL_DT_UINT32); }
   bool isBoolean() const { return (m_dataType == NXSL_DT_BOOLEAN) || isNumeric() || isNull() || isArray() || isObject(); }
   bool isFalse() const;
   bool isTrue() const;

   const TCHAR *getValueAsString(UINT32 *len);
   const TCHAR *getValueAsCString();
#ifdef UNICODE
   const char *getValueAsMBString();
#else
	const char *getValueAsMBString() { return getValueAsCString(); }
#endif
   int32_t getValueAsInt32() { return getValueAsIntegerType<int32_t>(); }
   uint32_t getValueAsUInt32() { return getValueAsIntegerType<uint32_t>(); }
   int64_t getValueAsInt64() { return getValueAsIntegerType<int64_t>(); }
   uint64_t getValueAsUInt64() { return getValueAsIntegerType<uint64_t>(); }
   double getValueAsReal();
   bool getValueAsBoolean() { return isTrue(); }
   NXSL_Object *getValueAsObject() { return (m_dataType == NXSL_DT_OBJECT) ? m_value.object : nullptr; }
   NXSL_Array *getValueAsArray() { return (m_dataType == NXSL_DT_ARRAY) ? m_value.arrayHandle->getObject() : nullptr; }
   NXSL_HashMap *getValueAsHashMap() { return (m_dataType == NXSL_DT_HASHMAP) ? m_value.hashMapHandle->getObject() : nullptr; }
   NXSL_Iterator *getValueAsIterator() { return (m_dataType == NXSL_DT_ITERATOR) ? m_value.iterator : nullptr; }

   void concatenate(const TCHAR *string, UINT32 len);
   
   void increment();
   void decrement();
   void negate();
   void bitNot();

   void add(NXSL_Value *pVal);
   void sub(NXSL_Value *pVal);
   void mul(NXSL_Value *pVal);
   void div(NXSL_Value *pVal);
   void rem(NXSL_Value *pVal);
   void bitAnd(NXSL_Value *pVal);
   void bitOr(NXSL_Value *pVal);
   void bitXor(NXSL_Value *pVal);
   void lshift(int nBits);
   void rshift(int nBits);

   bool EQ(const NXSL_Value *value) const;
   bool LT(const NXSL_Value *value) const;
   bool LE(const NXSL_Value *value) const;
   bool GT(const NXSL_Value *value) const;
   bool GE(const NXSL_Value *value) const;

   void copyOnWrite();
   void onVariableSet();

   bool equals(const NXSL_Value *v) const;
   void serialize(ByteStream& s) const;

   static NXSL_Value *load(NXSL_ValueManager *vm, ByteStream& s);
};

#ifdef _WIN32
template class LIBNXSL_EXPORTABLE ObjectMemoryPool<NXSL_Value>;
template class LIBNXSL_EXPORTABLE ObjectMemoryPool<NXSL_Identifier>;
#endif

/**
 * Value management functionality
 */
class LIBNXSL_EXPORTABLE NXSL_ValueManager
{
protected:
   ObjectMemoryPool<NXSL_Value> m_values;
   ObjectMemoryPool<NXSL_Identifier> m_identifiers;

public:
   NXSL_ValueManager() : m_values(256), m_identifiers(64) { }
   NXSL_ValueManager(size_t valuesRegCapacity, size_t identifiersRegCapacity) : m_values(valuesRegCapacity), m_identifiers(identifiersRegCapacity) { }
   virtual ~NXSL_ValueManager() { }

   NXSL_Value *createValue() { return new(m_values.allocate()) NXSL_Value(); }
   NXSL_Value *createValue(const NXSL_Value *src) { return new(m_values.allocate()) NXSL_Value(src); }
   NXSL_Value *createValue(NXSL_Object *object) { return new(m_values.allocate()) NXSL_Value(object); }
   NXSL_Value *createValue(NXSL_Array *array) { return new(m_values.allocate()) NXSL_Value(array); }
   NXSL_Value *createValue(NXSL_Iterator *iterator) { return new(m_values.allocate()) NXSL_Value(iterator); }
   NXSL_Value *createValue(NXSL_HashMap *hashMap) { return new(m_values.allocate()) NXSL_Value(hashMap); }
   NXSL_Value *createValue(int32_t n) { return new(m_values.allocate()) NXSL_Value(n); }
   NXSL_Value *createValue(uint32_t n) { return new(m_values.allocate()) NXSL_Value(n); }
   NXSL_Value *createValue(int64_t n) { return new(m_values.allocate()) NXSL_Value(n); }
   NXSL_Value *createValue(uint64_t n) { return new(m_values.allocate()) NXSL_Value(n); }
   NXSL_Value *createValue(double d) { return new(m_values.allocate()) NXSL_Value(d); }
   NXSL_Value *createValue(bool b) { return new(m_values.allocate()) NXSL_Value(b); }
   NXSL_Value *createValue(const TCHAR *s) { return new(m_values.allocate()) NXSL_Value(s); }
   NXSL_Value *createValue(const TCHAR *s, size_t l) { return new(m_values.allocate()) NXSL_Value(s, l); }
#ifdef UNICODE
   NXSL_Value *createValue(const char *s) { return new(m_values.allocate()) NXSL_Value(s); }
#endif
   void destroyValue(NXSL_Value *v) { m_values.destroy(v); }

   NXSL_Identifier *createIdentifier() { return new(m_identifiers.allocate()) NXSL_Identifier(); }
   NXSL_Identifier *createIdentifier(const char *s) { return new(m_identifiers.allocate()) NXSL_Identifier(s); }
#ifdef UNICODE
   NXSL_Identifier *createIdentifier(const WCHAR *s) { return new(m_identifiers.allocate()) NXSL_Identifier(s); }
#endif
   NXSL_Identifier *createIdentifier(const identifier_t& s) { return new(m_identifiers.allocate()) NXSL_Identifier(s); }
   NXSL_Identifier *createIdentifier(const NXSL_Identifier& src) { return new(m_identifiers.allocate()) NXSL_Identifier(src); }
   void destroyIdentifier(NXSL_Identifier *i) { m_identifiers.destroy(i); }

   virtual uint64_t getMemoryUsage() const { return m_values.getMemoryUsage() + m_identifiers.getMemoryUsage(); }
};

/**
 * Hash map template for holding NXSL_Value objects
 */
template <class K> class NXSL_ValueHashMap : public HashMapBase
{
private:
   NXSL_ValueManager *m_vm;

   static void destructor(void *object, HashMapBase *map) { static_cast<NXSL_ValueHashMap*>(map)->m_vm->destroyValue((NXSL_Value*)object); }

public:
   NXSL_ValueHashMap(NXSL_ValueManager *vm, Ownership objectOwner = Ownership::False) : HashMapBase(objectOwner, sizeof(K)) { m_vm = vm; m_objectDestructor = destructor; }

   NXSL_Value *get(const K& key) const { return (NXSL_Value*)_get(&key); }
   void set(const K& key, NXSL_Value *value) { _set(&key, (void *)value); }
   void remove(const K& key) { _remove(&key, true); }
   bool contains(const K& key) const { return _contains(&key); }

   Iterator<NXSL_Value> *iterator() { return new Iterator<NXSL_Value>(new HashMapIterator(this)); }

   NXSL_ValueManager *vm() const { return m_vm; }
};

/**
 * NXSL function definition structure
 */
class NXSL_Function
{
public:
   NXSL_Identifier m_name;
   uint32_t m_addr;

   NXSL_Function() : m_name() { m_addr = INVALID_ADDRESS; }
   NXSL_Function(const NXSL_Function *src) { m_name = src->m_name; m_addr = src->m_addr; }
   NXSL_Function(const char *name, uint32_t addr) : m_name(name) { m_addr = addr; }
   NXSL_Function(const NXSL_Identifier& name, uint32_t addr) : m_name(name) { m_addr = addr; }
};

/**
 * External function structure
 */
struct NXSL_ExtFunction
{
   char m_name[MAX_IDENTIFIER_LENGTH];
   int (* m_pfHandler)(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
   int m_iNumArgs;   // Number of arguments or -1 for variable number
};

/**
 * External selector structure
 */
struct NXSL_ExtSelector
{
   char m_name[MAX_IDENTIFIER_LENGTH];
   int (* m_handler)(const NXSL_Identifier& name, NXSL_Value *options, int argc, NXSL_Value **argv, int *selection, NXSL_VM *vm);
};

/**
 * NXSL module import information
 */
struct NXSL_ModuleImport
{
   TCHAR name[MAX_PATH];
   int lineNumber;   // line number in source code where module was referenced
};

/**
 * NXSL security context
 */
class LIBNXSL_EXPORTABLE NXSL_SecurityContext
{
public:
   NXSL_SecurityContext() { }
   virtual ~NXSL_SecurityContext();

   virtual bool validateAccess(int accessType, const void *object);
};

class NXSL_Library;

/**
 * Generic environment element list reference
 */
template<typename T> struct NXSL_EnvironmentListRef
{
   NXSL_EnvironmentListRef<T> *next;
   const T *elements;
   size_t count;

   NXSL_EnvironmentListRef(const T *_elements, size_t _count)
   {
      next = NULL;
      elements = _elements;
      count = _count;
   }
};

/**
 * Constant check for NXSL_Environment::getConstantValue
 */
#define NXSL_ENV_CONSTANT(n, v) do { \
   static NXSL_Identifier id(n); \
   if (name.equals(id)) \
      return vm->createValue(v); \
} while(0)

/**
 * Environment for NXSL program
 */
class LIBNXSL_EXPORTABLE NXSL_Environment
{
private:
   NXSL_EnvironmentListRef<NXSL_ExtFunction> *m_functions;
   NXSL_EnvironmentListRef<NXSL_ExtSelector> *m_selectors;
   NXSL_Library *m_library;

   NXSL_EnvironmentListRef<NXSL_ExtFunction> *createFunctionListRef(const NXSL_ExtFunction *list, size_t count)
   {
      return new(m_metadata.allocate(sizeof(NXSL_EnvironmentListRef<NXSL_ExtFunction>))) NXSL_EnvironmentListRef<NXSL_ExtFunction>(list, count);
   }

   NXSL_EnvironmentListRef<NXSL_ExtSelector> *createSelectorListRef(const NXSL_ExtSelector *list, size_t count)
   {
      return new(m_metadata.allocate(sizeof(NXSL_EnvironmentListRef<NXSL_ExtSelector>))) NXSL_EnvironmentListRef<NXSL_ExtSelector>(list, count);
   }

protected:
   MemoryPool m_metadata;

public:
   NXSL_Environment();
   virtual ~NXSL_Environment();

	virtual void print(NXSL_Value *value);
	virtual void trace(int level, const TCHAR *text);

	virtual void configureVM(NXSL_VM *vm);
	virtual NXSL_Value *getConstantValue(const NXSL_Identifier& name, NXSL_ValueManager *vm);

   void setLibrary(NXSL_Library *lib) { m_library = lib; }

   const NXSL_ExtFunction *findFunction(const NXSL_Identifier& name) const;
   StringSet *getAllFunctions() const;
   void registerFunctionSet(size_t count, const NXSL_ExtFunction *list);
   void registerIOFunctions();

   const NXSL_ExtSelector *findSelector(const NXSL_Identifier& name) const;
   void registerSelectorSet(size_t count, const NXSL_ExtSelector *list);

   bool loadModule(NXSL_VM *vm, const NXSL_ModuleImport *importInfo);
};

/**
 * Runtime variable information
 */
class LIBNXSL_EXPORTABLE NXSL_Variable : public NXSL_RuntimeObject
{
   friend class NXSL_VariableSystem;

protected:
   NXSL_Identifier m_name;
   NXSL_Value *m_value;
	bool m_constant;

   NXSL_Variable(NXSL_VM *vm, const NXSL_Identifier& name);
   NXSL_Variable(NXSL_VM *vm, const NXSL_Identifier& name, NXSL_Value *value, bool constant = false);
   ~NXSL_Variable();

public:
   const NXSL_Identifier& getName() const { return m_name; }
   NXSL_Value *getValue() { return m_value; }
   void setValue(NXSL_Value *value);
	bool isConstant() const { return m_constant; }
};


/**
 * Variable hash map element
 */
struct NXSL_VariablePtr;

/**
 * NXSL program instruction
 */
struct NXSL_Instruction;

/**
 * Variable pointer restore point
 */
struct VREF_RESTORE_POINT
{
   uint32_t addr;
   NXSL_Identifier *identifier;
};

/**
 * Maximum number of variable reference restore points
 */
#define MAX_VREF_RESTORE_POINTS  256

/**
 * Variable system type
 */
enum class NXSL_VariableSystemType
{
   GLOBAL,
   LOCAL,
   EXPRESSION,
   CONTEXT,
   CONSTANT
};

/**
 * Variable system
 */
class LIBNXSL_EXPORTABLE NXSL_VariableSystem : public NXSL_RuntimeObject
{
protected:
   MemoryPool m_pool;
   NXSL_VariablePtr *m_variables;
   NXSL_VariableSystemType m_type;
   int m_restorePointCount;
   VREF_RESTORE_POINT m_restorePoints[MAX_VREF_RESTORE_POINTS];

public:
   NXSL_VariableSystem(NXSL_VM *vm, NXSL_VariableSystemType type);
   NXSL_VariableSystem(NXSL_VM *vm, const NXSL_VariableSystem *src);
   ~NXSL_VariableSystem();

   NXSL_Variable *find(const NXSL_Identifier& name);
   NXSL_Variable *create(const NXSL_Identifier& name, NXSL_Value *value = nullptr);
   void merge(NXSL_VariableSystem *src, bool overwrite = false);
   void addAll(const NXSL_ValueHashMap<NXSL_Identifier>& src);
   void remove(const NXSL_Identifier& name);
   void clear();
   bool isConstant() const { return m_type == NXSL_VariableSystemType::CONSTANT; }

   bool createVariableReferenceRestorePoint(uint32_t addr, NXSL_Identifier *identifier);
   void restoreVariableReferences(StructArray<NXSL_Instruction> *instructions);

   void forEach(void (*callback)(const NXSL_Identifier&, NXSL_Value*, void*), void *context) const;
   template<typename T> void forEach(void (*callback)(const NXSL_Identifier&, NXSL_Value*, T*), T *context) const
   {
      forEach(reinterpret_cast<void (*)(const NXSL_Identifier&, NXSL_Value*, void*)>(callback), (void*)context);
   }

   void dump(FILE *fp) const;
};

/**
 * NXSL module information
 */
struct NXSL_Module
{
   TCHAR m_name[MAX_PATH];
   uint32_t m_codeStart;
   int m_codeSize;
   int m_functionStart;
   int m_numFunctions;
};

/**
 * Identifier location
 */
struct NXSL_IdentifierLocation
{
   NXSL_Identifier m_identifier;
   uint32_t m_addr;

   NXSL_IdentifierLocation(const NXSL_Identifier& identifier, uint32_t addr) : m_identifier(identifier)
   {
      m_addr = addr;
   }
};

class NXSL_ProgramBuilder;

#ifdef _WIN32
template class LIBNXSL_EXPORTABLE StructArray<NXSL_Instruction>;
template class LIBNXSL_EXPORTABLE StructArray<NXSL_ModuleImport>;
template class LIBNXSL_EXPORTABLE StructArray<NXSL_Function>;
template class LIBNXSL_EXPORTABLE NXSL_ValueHashMap<NXSL_Identifier>;
#endif

/**
 * Compiled NXSL script
 */
class LIBNXSL_EXPORTABLE NXSL_Program : public NXSL_ValueManager
{
   friend class NXSL_VM;

private:
   StructArray<NXSL_Instruction> m_instructionSet;
   StructArray<NXSL_ModuleImport> m_requiredModules;
   NXSL_ValueHashMap<NXSL_Identifier> m_constants;
   StructArray<NXSL_Function> m_functions;
   StringMap m_metadata;

public:
   NXSL_Program(size_t valueRegionSize = 0, size_t identifierRegionSize = 0);
   NXSL_Program(NXSL_ProgramBuilder *builder);
   ~NXSL_Program();

   uint32_t getCodeSize() const { return m_instructionSet.size(); }
   bool isEmpty() const;
   StringList *getRequiredModules() const;
   const TCHAR *getMetadataEntry(const TCHAR *key) const { return m_metadata.get(key); }
   const StringMap& getMetadata() const { return m_metadata; }

   virtual uint64_t getMemoryUsage() const;

   void dump(FILE *fp) const;

   void serialize(ByteStream& s) const;
   static NXSL_Program *load(ByteStream& s, TCHAR *errMsg, size_t errMsgSize);
};

/**
 * NXSL Script
 */
class LIBNXSL_EXPORTABLE NXSL_LibraryScript
{
protected:
   uint32_t m_id;
   uuid m_guid;
   TCHAR m_name[1024];
   TCHAR *m_source;
   NXSL_Program *m_program;
   TCHAR m_error[1024];

public:
   NXSL_LibraryScript();
   NXSL_LibraryScript(uint32_t id, uuid guid, const TCHAR *name, TCHAR *source, NXSL_Environment *env);
   ~NXSL_LibraryScript();

   bool isValid() const { return m_program != nullptr; }
   bool isEmpty() const { return (m_program == nullptr) || m_program->isEmpty(); }

   const uuid& getGuid() const { return m_guid; }
   uint32_t getId() const { return m_id; }

   const TCHAR *getName() const { return m_name; }
   const TCHAR *getSourceCode() const { return m_source; }
   const TCHAR *getError() const { return m_error; }

   NXSL_Program *getProgram() const { return m_program; }

   void fillMessage(NXCPMessage *msg, uint32_t base) const;
   void fillMessage(NXCPMessage *msg) const;
};

/**
 * Script library
 */
class LIBNXSL_EXPORTABLE NXSL_Library
{
private:
   ObjectArray<NXSL_LibraryScript> *m_scriptList;
   Mutex m_mutex;

   void deleteInternal(int nIndex);

public:
   NXSL_Library();
   ~NXSL_Library();

   void lock() { m_mutex.lock(); }
   void unlock() { m_mutex.unlock(); }

   bool addScript(NXSL_LibraryScript *script);
   void deleteScript(const TCHAR *name);
   void deleteScript(uint32_t id);
   NXSL_Program *findNxslProgram(const TCHAR *name);
   NXSL_LibraryScript *findScript(uint32_t id);
   NXSL_LibraryScript *findScript(const TCHAR *name);
   StringList *getScriptDependencies(const TCHAR *name);
   NXSL_VM *createVM(const TCHAR *name, NXSL_Environment *env);
   NXSL_VM *createVM(const TCHAR *name, NXSL_Environment *(*environmentCreator)(void*), bool (*scriptValidator)(NXSL_LibraryScript*, void*), void *context);

   void fillMessage(NXCPMessage *msg);
};

/**
 * Catch point information
 */
struct NXSL_CatchPoint
{
   UINT32 addr;
   UINT32 subLevel;
   int dataStackSize;
};

/**
 * NXSL storage class - base class for actual persistent storage
 */
class LIBNXSL_EXPORTABLE NXSL_Storage
{
public:
   NXSL_Storage();
   virtual ~NXSL_Storage();

   /**
    * Write to storage. Caller still has ownership of provided value.
    * Passing NULL value will effectively remove value from storage.
    */
   virtual void write(const TCHAR *name, NXSL_Value *value) = 0;

   /**
    * Read from storage. Returns new value owned by caller. Returns NXSL NULL if there are no value with given name.
    */
   virtual NXSL_Value *read(const TCHAR *name, NXSL_ValueManager *vm) = 0;
};

/**
 * NXSL storage local implementation
 */
class LIBNXSL_EXPORTABLE NXSL_LocalStorage : public NXSL_Storage
{
protected:
   NXSL_VM *m_vm;
   NXSL_StringValueMap *m_values;

public:
   NXSL_LocalStorage(NXSL_VM *vm);
   virtual ~NXSL_LocalStorage();

   virtual void write(const TCHAR *name, NXSL_Value *value) override;
   virtual NXSL_Value *read(const TCHAR *name, NXSL_ValueManager *vm) override;
};

#ifdef _WIN32
template class LIBNXSL_EXPORTABLE ObjectArray<NXSL_Module>;
#endif

/**
 * NXSL virtual machine
 */
class LIBNXSL_EXPORTABLE NXSL_VM : public NXSL_ValueManager
{
private:
   static EnumerationCallbackResult createConstantsCallback(const void *key, void *value, void *data);

protected:
   NXSL_Environment *m_env;
   StringMap m_metadata;
	void *m_userData;

   StructArray<NXSL_Instruction> m_instructionSet;
   uint32_t m_cp;
   bool m_stopFlag;

   uint32_t m_subLevel;
   NXSL_Stack m_codeStack;
   NXSL_ObjectStack<NXSL_Value> m_dataStack;
   NXSL_ObjectStack<NXSL_CatchPoint> m_catchStack;
   int m_nBindPos;

   NXSL_VariableSystem *m_constants;
   NXSL_VariableSystem *m_globalVariables;
   NXSL_VariableSystem *m_localVariables;
   NXSL_VariableSystem *m_expressionVariables;
   NXSL_VariableSystem **m_exportedExpressionVariables;
   NXSL_VariableSystem *m_contextVariables;
   NXSL_Value *m_context;

   NXSL_Storage *m_storage;
   NXSL_Storage *m_localStorage;

   StructArray<NXSL_Function> m_functions;
   ObjectArray<NXSL_Module> m_modules;

   NXSL_SecurityContext *m_securityContext;

   NXSL_Value *m_pRetValue;
   int m_errorCode;
   int m_errorLine;
   TCHAR *m_errorText;
   TCHAR *m_assertMessage;

   void execute();
   bool unwind();
   void callFunction(int nArgCount);
   bool callExternalFunction(const NXSL_ExtFunction *function, int stackItems);
   UINT32 callSelector(const NXSL_Identifier& name, int numElements);
   void pushProperty(const NXSL_Identifier& name);
   void doUnaryOperation(int nOpCode);
   void doBinaryOperation(int nOpCode);
   void getOrUpdateArrayElement(int opcode, NXSL_Value *array, NXSL_Value *index);
   bool setArrayElement(NXSL_Value *array, NXSL_Value *index, NXSL_Value *value);
   void getArrayAttribute(NXSL_Array *a, const char *attribute, bool safe);
   void getOrUpdateHashMapElement(int opcode, NXSL_Value *hashMap, NXSL_Value *key);
   bool setHashMapElement(NXSL_Value *hashMap, NXSL_Value *key, NXSL_Value *value);
   void getHashMapAttribute(NXSL_HashMap *m, const char *attribute, bool safe);
   void error(int errorCode, int sourceLine = -1);
   NXSL_Value *matchRegexp(NXSL_Value *value, NXSL_Value *regexp, bool ignoreCase);

   NXSL_Variable *findVariable(const NXSL_Identifier& name, NXSL_VariableSystem **vs = nullptr);
   NXSL_Variable *findOrCreateVariable(const NXSL_Identifier& name, NXSL_VariableSystem **vs = nullptr);
	NXSL_Variable *createVariable(const NXSL_Identifier& name);
	bool isDefinedConstant(const NXSL_Identifier& name);

   void relocateCode(uint32_t startOffset, uint32_t len, uint32_t shift);
   uint32_t getFunctionAddress(const NXSL_Identifier& name);

public:
   NXSL_VM(NXSL_Environment *env = nullptr, NXSL_Storage *storage = nullptr);
   virtual ~NXSL_VM();

   void loadModule(NXSL_Program *module, const NXSL_ModuleImport *importInfo);

	void setGlobalVariable(const NXSL_Identifier& name, NXSL_Value *value);
	void removeGlobalVariable(const NXSL_Identifier& name) { m_globalVariables->remove(name); }
	NXSL_Variable *findGlobalVariable(const NXSL_Identifier& name) { return m_globalVariables->find(name); }

	bool addConstant(const NXSL_Identifier& name, NXSL_Value *value);

	void setStorage(NXSL_Storage *storage);

	void storageWrite(const TCHAR *name, NXSL_Value *value) { m_storage->write(name, value); }
	NXSL_Value *storageRead(const TCHAR *name) { return m_storage->read(name, this); }

	void setContextObject(NXSL_Value *value);

   bool load(const NXSL_Program *program);
   bool run(const ObjectRefArray<NXSL_Value>& args, NXSL_VariableSystem **globals = nullptr,
            NXSL_VariableSystem **expressionVariables = nullptr,
            NXSL_VariableSystem *constants = nullptr, const char *entryPoint = nullptr);
   bool run(int argc, NXSL_Value **argv, NXSL_VariableSystem **globals = nullptr,
            NXSL_VariableSystem **expressionVariables = nullptr,
            NXSL_VariableSystem *pConstants = nullptr, const char *entryPoint = nullptr);
   bool run() { ObjectRefArray<NXSL_Value> args(1, 1); return run(args); }
   void stop() { m_stopFlag = true; }

   uint32_t getCodeSize() const { return m_instructionSet.size(); }

	void trace(int level, const TCHAR *text);
   void dump(FILE *fp) const;
   int getErrorCode() const { return m_errorCode; }
   int getErrorLine() const { return m_errorLine; }
   const TCHAR *getErrorText() const { return CHECK_NULL_EX(m_errorText); }
   const TCHAR *getAssertMessage() const { return CHECK_NULL_EX(m_assertMessage); }
   NXSL_Value *getResult() { return m_pRetValue; }
   const TCHAR *getMetadataEntry(const TCHAR *key) const { return m_metadata.get(key); }
   const StringMap& getMetadata() const { return m_metadata; }

   void setSecurityContext(NXSL_SecurityContext *context);
   bool validateAccess(int accessType, const void *object) { return (m_securityContext != nullptr) ? m_securityContext->validateAccess(accessType, object) : false; }

   void setAssertMessage(const TCHAR *msg) { MemFree(m_assertMessage); m_assertMessage = MemCopyString(msg); }

	void *getUserData() { return m_userData; }
	void setUserData(void *data) { m_userData = data; }
};

/**
 * NXSL "TableRow" class
 */
class LIBNXSL_EXPORTABLE NXSL_TableRowClass : public NXSL_Class
{
public:
   NXSL_TableRowClass();
   virtual ~NXSL_TableRowClass();

   virtual NXSL_Value *getAttr(NXSL_Object *object, const char *attr) override;
   virtual void onObjectDelete(NXSL_Object *object) override;
};

/**
 * NXSL "TableColumn" class
 */
class LIBNXSL_EXPORTABLE NXSL_TableColumnClass : public NXSL_Class
{
public:
   NXSL_TableColumnClass();
   virtual ~NXSL_TableColumnClass();

   virtual NXSL_Value *getAttr(NXSL_Object *object, const char *attr) override;
	virtual void onObjectDelete(NXSL_Object *object) override;
};

/**
 * NXSL "Table" class
 */
class LIBNXSL_EXPORTABLE NXSL_TableClass : public NXSL_Class
{
public:
   NXSL_TableClass();
   virtual ~NXSL_TableClass();

   virtual NXSL_Value *getAttr(NXSL_Object *object, const char *attr) override;
	virtual void onObjectDelete(NXSL_Object *object) override;
};

/**
 * NXSL "Connector" class
 */
class LIBNXSL_EXPORTABLE NXSL_ConnectorClass : public NXSL_Class
{
public:
   NXSL_ConnectorClass();
   virtual ~NXSL_ConnectorClass();

	virtual void onObjectDelete(NXSL_Object *object) override;
};

/**
 * NXSL "GeoLocation" class
 */
class LIBNXSL_EXPORTABLE NXSL_GeoLocationClass : public NXSL_Class
{
public:
   NXSL_GeoLocationClass();
   virtual ~NXSL_GeoLocationClass();

   virtual NXSL_Value *getAttr(NXSL_Object *object, const char *attr) override;
   virtual void onObjectDelete(NXSL_Object *object) override;

   static NXSL_Value *createObject(NXSL_VM *vm, const GeoLocation& gl);
};

/**
 * NXSL "InetAddress" class
 */
class LIBNXSL_EXPORTABLE NXSL_InetAddressClass : public NXSL_Class
{
public:
   NXSL_InetAddressClass();
   virtual ~NXSL_InetAddressClass();

   virtual NXSL_Value *getAttr(NXSL_Object *object, const char *attr) override;
   virtual void onObjectDelete(NXSL_Object *object) override;

   static NXSL_Value *createObject(NXSL_VM *vm, const InetAddress& addr);
};

/**
 * NXSL "JsonObject" class
 */
class LIBNXSL_EXPORTABLE NXSL_JsonObjectClass : public NXSL_Class
{
public:
   NXSL_JsonObjectClass();
   virtual ~NXSL_JsonObjectClass();

   virtual NXSL_Value *getAttr(NXSL_Object *object, const char *attr) override;
   virtual bool setAttr(NXSL_Object *object, const char *attr, NXSL_Value *value) override;
   virtual void onObjectDelete(NXSL_Object *object) override;
};

/**
 * NXSL "JsonArray" class
 */
class LIBNXSL_EXPORTABLE NXSL_JsonArrayClass : public NXSL_Class
{
public:
   NXSL_JsonArrayClass();
   virtual ~NXSL_JsonArrayClass();

   virtual NXSL_Value *getAttr(NXSL_Object *object, const char *attr) override;
   virtual void onObjectDelete(NXSL_Object *object) override;
};

/**
 * NXSL "Class" class
 */
class LIBNXSL_EXPORTABLE NXSL_MetaClass : public NXSL_Class
{
public:
   NXSL_MetaClass();
   virtual ~NXSL_MetaClass();

   virtual NXSL_Value *getAttr(NXSL_Object *object, const char *attr) override;
};

/**
 * Class definition instances
 */
extern NXSL_Class LIBNXSL_EXPORTABLE g_nxslBaseClass;
extern NXSL_MetaClass LIBNXSL_EXPORTABLE g_nxslMetaClass;
extern NXSL_TableClass LIBNXSL_EXPORTABLE g_nxslTableClass;
extern NXSL_TableRowClass LIBNXSL_EXPORTABLE g_nxslTableRowClass;
extern NXSL_TableColumnClass LIBNXSL_EXPORTABLE g_nxslTableColumnClass;
extern NXSL_ConnectorClass LIBNXSL_EXPORTABLE g_nxslConnectorClass;
extern NXSL_GeoLocationClass LIBNXSL_EXPORTABLE g_nxslGeoLocationClass;
extern NXSL_InetAddressClass LIBNXSL_EXPORTABLE g_nxslInetAddressClass;
extern NXSL_JsonObjectClass LIBNXSL_EXPORTABLE g_nxslJsonObjectClass;
extern NXSL_JsonArrayClass LIBNXSL_EXPORTABLE g_nxslJsonArrayClass;

#endif
