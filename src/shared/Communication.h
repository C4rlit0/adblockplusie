/*
 * This file is part of Adblock Plus <https://adblockplus.org/>,
 * Copyright (C) 2006-2015 Eyeo GmbH
 *
 * Adblock Plus is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * Adblock Plus is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Adblock Plus.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef COMMUNICATION_H
#define COMMUNICATION_H

#include <memory>
#include <sstream>
#include <stdexcept>
#include <stdint.h>
#include <string>
#include <vector>
#include <Windows.h>

namespace Communication
{
  extern const std::wstring pipeName;
  extern std::wstring browserSID;

  enum ProcType : uint32_t {
    PROC_MATCHES,
    PROC_GET_ELEMHIDE_SELECTORS,
    PROC_AVAILABLE_SUBSCRIPTIONS,
    PROC_LISTED_SUBSCRIPTIONS,
    PROC_SET_SUBSCRIPTION,
    PROC_ADD_SUBSCRIPTION,
    PROC_REMOVE_SUBSCRIPTION,
    PROC_UPDATE_ALL_SUBSCRIPTIONS,
    PROC_GET_EXCEPTION_DOMAINS,
    PROC_GET_WHITELISTING_FITER,
    PROC_IS_ELEMHIDE_WHITELISTED_ON_URL,
    PROC_ADD_FILTER,
    PROC_REMOVE_FILTER,
    PROC_SET_PREF,
    PROC_GET_PREF,
    PROC_IS_FIRST_RUN_ACTION_NEEDED,
    PROC_CHECK_FOR_UPDATES,
    PROC_GET_DOCUMENTATION_LINK,
    PROC_TOGGLE_PLUGIN_ENABLED,
    PROC_GET_HOST,
    PROC_COMPARE_VERSIONS
  };
  enum ValueType : uint32_t {
    TYPE_PROC, TYPE_STRING, TYPE_WSTRING, TYPE_INT64, TYPE_INT32, TYPE_BOOL, TYPE_STRINGS
  };
  typedef uint32_t SizeType;

  class InputBuffer
  {
  public:
    InputBuffer() : buffer(), hasType(false) {}
    InputBuffer(const std::string& data) : buffer(data), hasType(false) {}
    InputBuffer(const InputBuffer& copy) 
    { 
      hasType = copy.hasType; 
      buffer = std::istringstream(copy.buffer.str()); 
      currentType = copy.currentType; 
    }
    InputBuffer& operator>>(ProcType& value) { return Read(value, TYPE_PROC); }
    InputBuffer& operator>>(std::string& value) { return ReadString(value, TYPE_STRING); }
    InputBuffer& operator>>(std::wstring& value) { return ReadString(value, TYPE_WSTRING); }
    InputBuffer& operator>>(int64_t& value) { return Read(value, TYPE_INT64); }
    InputBuffer& operator>>(int32_t& value) { return Read(value, TYPE_INT32); }
    InputBuffer& operator>>(bool& value) { return Read(value, TYPE_BOOL); }
    InputBuffer& operator>>(std::vector<std::string>& value) { return ReadStrings(value); }
    InputBuffer& operator=(const InputBuffer& copy) 
    { 
      hasType = copy.hasType; 
      buffer = std::istringstream(copy.buffer.str()); 
      currentType = copy.currentType; 
      return *this; 
    }
    ValueType GetType();
  private:
    std::istringstream buffer;
    ValueType currentType;
    bool hasType;

    void CheckType(ValueType expectedType);

    template<class T>
    InputBuffer& ReadString(T& value, ValueType expectedType)
    {
      CheckType(expectedType);

      SizeType length;
      ReadBinary(length);

      std::auto_ptr<T::value_type> data(new T::value_type[length]);
      buffer.read(reinterpret_cast<char*>(data.get()), sizeof(T::value_type) * length);
      if (buffer.fail())
        throw new std::runtime_error("Unexpected end of input buffer");

      value.assign(data.get(), length);
      return *this;
    }

    InputBuffer& ReadStrings(std::vector<std::string>& value)
    {
      value.clear();
      CheckType(ValueType::TYPE_STRINGS);

      SizeType length;
      ReadBinary(length);
      value.resize(length);
      for (SizeType i = 0; i < length; ++i)
      {
        operator>>(value[i]);
      }
      return *this;
    }

    template<class T>
    InputBuffer& Read(T& value, ValueType expectedType)
    {
      CheckType(expectedType);
      ReadBinary(value);
      return *this;
    }

    template<class T>
    void ReadBinary(T& value)
    {
      buffer.read(reinterpret_cast<char*>(&value), sizeof(T));
      if (buffer.fail())
        throw new std::runtime_error("Unexpected end of input buffer");
      hasType = false;
    }
  };

  class OutputBuffer
  {
  public:
    OutputBuffer() {}

    // Explicit copy constructor to allow returning OutputBuffer by value
    OutputBuffer(const OutputBuffer& copy) : buffer(copy.buffer.str()) {}

    std::string Get()
    {
      return buffer.str();
    }
    OutputBuffer& operator<<(ProcType value) { return Write(value, TYPE_PROC); }
    OutputBuffer& operator<<(const std::string& value) { return WriteString(value, TYPE_STRING); }
    OutputBuffer& operator<<(const std::wstring& value) { return WriteString(value, TYPE_WSTRING); }
    OutputBuffer& operator<<(int64_t value) { return Write(value, TYPE_INT64); }
    OutputBuffer& operator<<(int32_t value) { return Write(value, TYPE_INT32); }
    OutputBuffer& operator<<(bool value) { return Write(value, TYPE_BOOL); }
    OutputBuffer& operator<<(const std::vector<std::string>& value) { return WriteStrings(value); }
  private:
    std::ostringstream buffer;

    // Disallow copying
    const OutputBuffer& operator=(const OutputBuffer&);

    template<class T>
    OutputBuffer& WriteString(const T& value, ValueType type)
    {
      WriteBinary(type);

      SizeType length = static_cast<SizeType>(value.size());
      WriteBinary(length);

      buffer.write(reinterpret_cast<const char*>(value.c_str()), sizeof(T::value_type) * length);
      if (buffer.fail())
        throw new std::runtime_error("Unexpected error writing to output buffer");

      return *this;
    }

    OutputBuffer& WriteStrings(const std::vector<std::string>& value)
    {
      WriteBinary(ValueType::TYPE_STRINGS);
      WriteBinary(static_cast<SizeType>(value.size()));
      for (const auto& str : value)
      {
        operator<<(str);
      }
      return *this;
    }

    template<class T>
    OutputBuffer& Write(const T value, ValueType type)
    {
      WriteBinary(type);
      WriteBinary(value);
      return *this;
    }

    template<class T>
    void WriteBinary(const T& value)
    {
      buffer.write(reinterpret_cast<const char*>(&value), sizeof(T));
      if (buffer.fail())
        throw new std::runtime_error("Unexpected error writing to output buffer");
    }
  };

  class PipeConnectionError : public std::runtime_error
  {
  public:
    PipeConnectionError();
  };

  class PipeBusyError : public std::runtime_error
  {
  public:
    PipeBusyError();
  };

  class PipeDisconnectedError : public std::runtime_error
  {
  public:
    PipeDisconnectedError();
  };

  class Pipe
  {
  public:
    enum Mode {MODE_CREATE, MODE_CONNECT};

    Pipe(const std::wstring& name, Mode mode);
    ~Pipe();

    InputBuffer ReadMessage();
    void WriteMessage(OutputBuffer& message);

  protected:
    HANDLE pipe;
  };
}

#endif
