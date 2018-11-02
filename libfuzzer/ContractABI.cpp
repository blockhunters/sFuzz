#include <iostream>
#include <boost/algorithm/string.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <regex>
#include <libdevcore/SHA3.h>
#include <libdevcore/FixedHash.h>
#include "ContractABI.h"

using namespace std;
namespace pt = boost::property_tree;

namespace fuzzer {
  FuncDef::FuncDef(string name, vector<TypeDef> tds) {
    this->name = name;
    this->tds = tds;
  }
  /*
   * Start with a simple assumption:
   * - dynamic_size: 32
   * - array_size: 5
   * - sub_array_size: 5
   */
  
  void ContractABI::updateTestData(bytes data) {
    /* Detect dynamic len by consulting first 32 bytes */
    int lenOffset = 0;
    auto consultRealLen = [&]() {
      int len = data[lenOffset];
      lenOffset = (lenOffset + 1) % 32;
      return len;
    };
    /* Container of dynamic len */
    auto consultContainerLen = [](int realLen) {
      if (!(realLen % 32)) return realLen;
      return (realLen / 32 + 1) * 32;
    };
    /* Pad to enough data before decoding */
    int offset = 32;
    auto padLen = [&](int singleLen) {
      int fitLen = offset + singleLen;
      while ((int)data.size() < fitLen) data.push_back(0);
    };
    for (auto &fd : this->fds) {
      for (auto &td : fd.tds) {
        switch (td.dimensions.size()) {
          case 0: {
            int realLen = td.isDynamic ? consultRealLen() : 32;
            int containerLen = consultContainerLen(realLen);
            /* Pad to enough bytes to read */
            padLen(containerLen);
            /* Read from offset ... offset + realLen */
            bytes d(data.begin() + offset, data.begin() + offset + realLen);
            td.addValue(d);
            /* Ignore (containerLen - realLen) bytes */
            offset += containerLen;
            break;
          }
          case 1: {
            vector<bytes> ds;
            int numElem = td.dimensions[0] ? td.dimensions[0] : consultRealLen();
            for (int i = 0; i < numElem; i += 1) {
              int realLen = td.isDynamic ? consultRealLen() : 32;
              int containerLen = consultContainerLen(realLen);
              padLen(containerLen);
              bytes d(data.begin() + offset, data.begin() + offset + realLen);
              ds.push_back(d);
              offset += containerLen;
            }
            td.addValue(ds);
            break;
          }
          case 2: {
            vector<vector<bytes>> dss;
            int numElem = td.dimensions[0] ? td.dimensions[0] : consultRealLen();
            int numSubElem = td.dimensions[1] ? td.dimensions[1] : consultRealLen();
            for (int i = 0; i < numElem; i += 1) {
              vector<bytes> ds;
              for (int j = 0; j < numSubElem; j += 1) {
                int realLen = td.isDynamic ? consultRealLen() : 32;
                int containerLen = consultContainerLen(realLen);
                padLen(containerLen);
                bytes d(data.begin() + offset, data.begin() + offset + realLen);
                ds.push_back(d);
                offset += containerLen;
              }
              dss.push_back(ds);
            }
            td.addValue(dss);
            break;
          }
        }
      }
    }
  }
  
  bytes ContractABI::randomTestcase() {
    /*
     * Random value for ABI
     * | --- dynamic len (32 bytes) -- | content |
     */
    bytes ret(32, 5);
    int lenOffset = 0;
    auto consultRealLen = [&]() {
      int len = ret[lenOffset];
      lenOffset = (lenOffset + 1) % 32;
      return len;
    };
    auto consultContainerLen = [](int realLen) {
      if (!(realLen % 32)) return realLen;
      return (realLen / 32 + 1) * 32;
    };
    for (auto fd : this->fds) {
      for (auto td : fd.tds) {
        switch(td.dimensions.size()) {
          case 0: {
            int realLen = td.isDynamic ? consultRealLen() : 32;
            int containerLen = consultContainerLen(realLen);
            bytes data(containerLen, 0);
            ret.insert(ret.end(), data.begin(), data.end());
            break;
          }
          case 1: {
            int numElem = td.dimensions[0] ? td.dimensions[0] : consultRealLen();
            for (int i = 0; i < numElem; i += 1) {
              int realLen = td.isDynamic ? consultRealLen() : 32;
              int containerLen = consultContainerLen(realLen);
              bytes data = bytes(containerLen, 0);
              ret.insert(ret.end(), data.begin(), data.end());
            }
            break;
          }
          case 2: {
            int numElem = td.dimensions[0] ? td.dimensions[0] : consultRealLen();
            int numSubElem = td.dimensions[1] ? td.dimensions[1] : consultRealLen();
            for (int i = 0; i < numElem; i += 1) {
              for (int j = 0; j < numSubElem; j += 1) {
                int realLen = td.isDynamic ? consultRealLen() : 32;
                int containerLen = consultContainerLen(realLen);
                bytes data = bytes(containerLen, 0);
                ret.insert(ret.end(), data.begin(), data.end());
              }
            }
            break;
          }
        }
      }
    }
    return ret;
  }
  
  ContractABI::ContractABI(string abiJson) {
    stringstream ss;
    ss << abiJson;
    pt::ptree root;
    pt::read_json(ss, root);
    for (auto node : root) {
      auto inputNodes = node.second.get_child("inputs");
      vector<TypeDef> tds;
      string type = node.second.get<string>("type");
      string name = type == "constructor" ? "" : node.second.get<string>("name");
      for (auto inputNode : inputNodes) {
        string type = inputNode.second.get<string>("type");
        tds.push_back(TypeDef(type));
      }
      this->fds.push_back(FuncDef(name, tds));
    };
  }
  
  bytes ContractABI::encodeConstructor() {
    auto it = find_if(fds.begin(), fds.end(), [](FuncDef fd) { return fd.name == "";});
    if (it != fds.end()) return encodeTuple((*it).tds);
    return bytes(0, 0);
  }
  
  vector<bytes> ContractABI::encodeFunctions() {
    vector<bytes> ret;
    for (auto fd : fds) {
      if (fd.name != "") {
        bytes selector = functionSelector(fd.name /* name */, fd.tds /* type defs */);
        bytes data = encodeTuple(fd.tds);
        selector.insert(selector.end(), data.begin(), data.end());
        ret.push_back(selector);
      }
    }
    return ret;
  }
  
  bytes ContractABI::functionSelector(string name, vector<TypeDef> tds) {
    vector<string> argTypes;
    transform(tds.begin(), tds.end(), back_inserter(argTypes), [](TypeDef td) {
      return td.fullname;
    });
    string signature = name + "(" + boost::algorithm::join(argTypes, ",") + ")";
    bytes fullSelector = sha3(signature).ref().toBytes();
    return bytes(fullSelector.begin(), fullSelector.begin() + 4);
  }
  
  bytes ContractABI::encodeTuple(vector<TypeDef> tds) {
    bytes ret;
    /* Payload */
    bytes payload;
    vector<int> dataOffset = {0};
    for (auto td : tds) {
      if (td.isDynamic || td.isDynamicArray || td.isSubDynamicArray) {
        bytes data;
        switch (td.dimensions.size()) {
          case 0: {
            data = encodeSingle(td.dt);
            break;
          }
          case 1: {
            data = encodeArray(td.dts, td.isDynamicArray);
            break;
          }
          case 2: {
            data = encode2DArray(td.dtss, td.isDynamicArray, td.isSubDynamicArray);
            break;
          }
        }
        dataOffset.push_back(dataOffset.back() + data.size());
        payload.insert(payload.end(), data.begin(), data.end());
      }
    }
    /* Calculate offset */
    u256 headerOffset = 0;
    for (auto td : tds) {
      if (td.isDynamic || td.isDynamicArray || td.isSubDynamicArray) {
        headerOffset += 32;
      } else {
        switch (td.dimensions.size()) {
          case 0: {
            headerOffset += encodeSingle(td.dt).size();
            break;
          }
          case 1: {
            headerOffset += encodeArray(td.dts, td.isDynamicArray).size();
            break;
          }
          case 2: {
            headerOffset += encode2DArray(td.dtss, td.isDynamicArray, td.isSubDynamicArray).size();
            break;
          }
        }
      }
    }
    bytes header;
    int dynamicCount = 0;
    for (auto td : tds) {
      /* Dynamic in head */
      if (td.isDynamic || td.isDynamicArray || td.isSubDynamicArray) {
        u256 offset = headerOffset + dataOffset[dynamicCount];
        /* Convert to byte */
        for (int i = 0; i < 32; i += 1) {
          byte b = (byte) (offset >> ((32 - i - 1) * 8)) & 0xFF;
          header.push_back(b);
        }
        dynamicCount ++;
      } else {
        /* static in head */
        bytes data;
        switch (td.dimensions.size()) {
          case 0: {
            data = encodeSingle(td.dt);
            break;
          }
          case 1: {
            data = encodeArray(td.dts, td.isDynamicArray);
            break;
          }
          case 2: {
            data = encode2DArray(td.dtss, td.isDynamicArray, td.isSubDynamicArray);
            break;
          }
        }
        header.insert(header.end(), data.begin(), data.end());
      }
    }
    /* Head + Payload */
    ret.insert(ret.end(), header.begin(), header.end());
    ret.insert(ret.end(), payload.begin(), payload.end());
    return ret;
  }
  
  bytes ContractABI::encode2DArray(vector<vector<DataType>> dtss, bool isDynamicArray, bool isSubDynamic) {
    bytes ret;
    if (isDynamicArray) {
      bytes payload;
      bytes header;
      u256 numElem = dtss.size();
      if (isSubDynamic) {
        /* Need Offset*/
        vector<int> dataOffset = {0};
        for (auto dts : dtss) {
          bytes data = encodeArray(dts, isSubDynamic);
          dataOffset.push_back(dataOffset.back() + data.size());
          payload.insert(payload.end(), data.begin(), data.end());
        }
        /* Count */
        for (int i = 0; i < 32; i += 1) {
          byte b = (byte) (numElem >> ((32 - i - 1) * 8)) & 0xFF;
          header.push_back(b);
        }
        for (int i = 0; i < numElem; i += 1) {
          u256 headerOffset =  32 * numElem + dataOffset[i];
          for (int i = 0; i < 32; i += 1) {
            byte b = (byte) (headerOffset >> ((32 - i - 1) * 8)) & 0xFF;
            header.push_back(b);
          }
        }
      } else {
        /* Count */
        for (int i = 0; i < 32; i += 1) {
          byte b = (byte) (numElem >> ((32 - i - 1) * 8)) & 0xFF;
          header.push_back(b);
        }
        for (auto dts : dtss) {
          bytes data = encodeArray(dts, isSubDynamic);
          payload.insert(payload.end(), data.begin(), data.end());
        }
      }
      ret.insert(ret.end(), header.begin(), header.end());
      ret.insert(ret.end(), payload.begin(), payload.end());
      return ret;
    }
    for (auto dts : dtss) {
      bytes data = encodeArray(dts, isSubDynamic);
      ret.insert(ret.end(), data.begin(), data.end());
    }
    return ret;
  }
  
  bytes ContractABI::encodeArray(vector<DataType> dts, bool isDynamicArray) {
    bytes ret;
    /* T[] */
    if (isDynamicArray) {
      /* Calculate header and payload */
      bytes payload;
      bytes header;
      u256 numElem = dts.size();
      if (dts[0].isDynamic) {
        /* If element is dynamic then needs offset */
        vector<int> dataOffset = {0};
        for (auto dt : dts) {
          bytes data = encodeSingle(dt);
          dataOffset.push_back(dataOffset.back() + data.size());
          payload.insert(payload.end(), data.begin(), data.end());
        }
        /* Count */
        for (int i = 0; i < 32; i += 1) {
          byte b = (byte) (numElem >> ((32 - i - 1) * 8)) & 0xFF;
          header.push_back(b);
        }
        /* Offset */
        for (int i = 0; i < numElem; i += 1) {
          u256 headerOffset =  32 * numElem + dataOffset[i];
          for (int i = 0; i < 32; i += 1) {
            byte b = (byte) (headerOffset >> ((32 - i - 1) * 8)) & 0xFF;
            header.push_back(b);
          }
        }
      } else {
        /* Do not need offset, count them */
        for (int i = 0; i < 32; i += 1) {
          byte b = (byte) (numElem >> ((32 - i - 1) * 8)) & 0xFF;
          header.push_back(b);
        }
        for (auto dt : dts) {
          bytes data = encodeSingle(dt);
          payload.insert(payload.end(), data.begin(), data.end());
        }
      }
      ret.insert(ret.end(), header.begin(), header.end());
      ret.insert(ret.end(), payload.begin(), payload.end());
      return ret;
    }
    /* T[k] */
    for (auto dt : dts) {
      bytes data = encodeSingle(dt);
      ret.insert(ret.end(), data.begin(), data.end());
    }
    return ret;
  }
  
  bytes ContractABI::encodeSingle(DataType dt) {
    bytes ret;
    bytes payload = dt.payload();
    if (dt.isDynamic) {
      /* Concat len and data */
      bytes header = dt.header();
      ret.insert(ret.end(), header.begin(), header.end());
      ret.insert(ret.end(), payload.begin(), payload.end());
      return ret;
    }
    ret.insert(ret.end(), payload.begin(), payload.end());
    return ret;
  }
  
  DataType::DataType(bytes value, bool padLeft, bool isDynamic) {
    this->value = value;
    this->padLeft = padLeft;
    this->isDynamic = isDynamic;
  }
  
  bytes DataType::header() {
    u256 size = this->value.size();
    bytes ret;
    for (int i = 0; i < 32; i += 1) {
      byte b = (byte) (size >> ((32 - i - 1) * 8)) & 0xFF;
      ret.push_back(b);
    }
    return ret;
  }
  
  bytes DataType::payload() {
    auto paddingLeft = [this](double toLen) {
      bytes ret(toLen - this->value.size(), 0);
      ret.insert(ret.end(), this->value.begin(), this->value.end());
      return ret;
    };
    auto paddingRight = [this](double toLen) {
      bytes ret;
      ret.insert(ret.end(), this->value.begin(), this->value.end());
      while(ret.size() < toLen) ret.push_back(0);
      return ret;
    };
    if (this->value.size() > 32) {
      if (!this->isDynamic) throw "Size of static <= 32 bytes";
      int valueSize = this->value.size();
      int finalSize = valueSize % 32 == 0 ? valueSize : (valueSize / 32 + 1) * 32;
      if (this->padLeft) return paddingLeft(finalSize);
      return paddingRight(finalSize);
    }
    if (this->padLeft) return paddingLeft(32);
    return paddingRight(32);
  }
  
  string TypeDef::toRealname(string name) {
    string fullType = toFullname(name);
    string searchPatterns[2] = {"address[", "bool["};
    string replaceCandidates[2] = {"uint160", "uint8"};
    for (int i = 0; i < 2; i += 1) {
      string pattern = searchPatterns[i];
      string candidate = replaceCandidates[i];
      if (boost::starts_with(fullType, pattern))
        return candidate + fullType.substr(pattern.length() - 1);
      if (fullType == pattern.substr(0, pattern.length() - 1)) return candidate;
    }
    return fullType;
  }
  
  string TypeDef::toFullname(string name) {
    string searchPatterns[4] = {"int[", "uint[", "fixed[", "ufixed["};
    string replaceCandidates[4] = {"int256", "uint256", "fixed128x128", "ufixed128x128"};
    for (int i = 0; i < 4; i += 1) {
      string pattern = searchPatterns[i];
      string candidate = replaceCandidates[i];
      if (boost::starts_with(name, pattern))
        return candidate + name.substr(pattern.length() - 1);
      if (name == pattern.substr(0, pattern.length() - 1)) return candidate;
    }
    return name;
  }
  
  vector<int> TypeDef::extractDimension(string name) {
    vector<int> ret;
    smatch sm;
    regex_match(name, sm, regex("[a-z]+[0-9]*\\[(\\d*)\\]\\[(\\d*)\\]"));
    if (sm.size() == 3) {
      /* Two dimension array */
      ret.push_back(sm[1] == "" ? 0 : stoi(sm[1]));
      ret.push_back(sm[2] == "" ? 0 : stoi(sm[2]));
      return ret;
    }
    regex_match(name, sm, regex("[a-z]+[0-9]*\\[(\\d*)\\]"));
    if (sm.size() == 2) {
      /* One dimension array */
      ret.push_back(sm[1] == "" ? 0 : stoi(sm[1]));
      return ret;
    }
    return ret;
  }
  
  void TypeDef::addValue(vector<vector<bytes>> vss) {
    if (this->dimensions.size() != 2) throw "Invalid dimension";;
    for (auto vs : vss) {
      vector<DataType> dts;
      for (auto v : vs) {
        dts.push_back(DataType(v, this->padLeft, this->isDynamic));
      }
      this->dtss.push_back(dts);
    }
  }
  
  void TypeDef::addValue(vector<bytes> vs) {
    if (this->dimensions.size() != 1) throw "Invalid dimension";
    for (auto v : vs) {
      this->dts.push_back(DataType(v, this->padLeft, this->isDynamic));
    }
  }
  
  void TypeDef::addValue(bytes v) {
    if (this->dimensions.size()) throw "Invalid dimension";
    this->dt = DataType(v, this->padLeft, this->isDynamic);
  }
  
  TypeDef::TypeDef(string name) {
    this->name = name;
    this->fullname = toFullname(name);
    this->realname = toRealname(name);
    this->dimensions = extractDimension(name);
    this->padLeft = !boost::starts_with(this->fullname, "bytes") && !boost::starts_with(this->fullname, "string");
    int numDimension = this->dimensions.size();
    if (!numDimension) {
      this->isDynamic = this->fullname == "string" || this->name == "bytes";
      this->isDynamicArray = false;
      this->isSubDynamicArray = false;
    } else if (numDimension == 1) {
      this->isDynamic = boost::starts_with(this->fullname, "string[")
      || boost::starts_with(this->fullname, "bytes[");
      this->isDynamicArray = this->dimensions[0] == 0;
      this->isSubDynamicArray = false;
    } else {
      this->isDynamic = boost::starts_with(this->fullname, "string[")
      || boost::starts_with(this->fullname, "bytes[");
      this->isDynamicArray = this->dimensions[0] == 0;
      this->isSubDynamicArray = this->dimensions[1] == 0;
    }
  }
}
