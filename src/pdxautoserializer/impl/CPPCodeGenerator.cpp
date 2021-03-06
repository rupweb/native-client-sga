/*=========================================================================
* Copyright (c) 2002-2014 Pivotal Software, Inc. All Rights Reserved.
 * This product is protected by U.S. and international copyright
 * and intellectual property laws. Pivotal products are covered by
 * more patents listed at http://www.pivotal.io/patents.
*=========================================================================
*/

#include "../base_types.hpp"
#include "CPPCodeGenerator.hpp"
#include "Log.hpp"
#include "Helper.hpp"
#include <iostream>

namespace gemfire {
namespace pdx_auto_serializer {
int CPPCodeGenerator::s_classId = -1;

std::string CPPCodeGenerator::s_GFSerializerNamespace =
    "gemfire::PdxAutoSerializable";
std::string CPPCodeGenerator::s_TempVarPrefix = "var_";

/** The option name for classId. */
std::string CPPCodeGenerator::s_ClassIdOption = "classId";

/** The option name for the output directory. */
std::string CPPCodeGenerator::s_OutDirOption = "outDir";

/**
* The directory to be used in generated files for the included headers.
* If not provided the path of the header file as provided on
* command-line is used.
*/
std::string CPPCodeGenerator::s_HeaderDirOption = "headerDir";

/**
* The option name for the suffix to use for generated files and classes.
*/
std::string CPPCodeGenerator::s_GenSuffixOption = "suffix";

// CodeGenerator method implementations

void CPPCodeGenerator::getOptions(OptionMap& options) const {
  std::pair<bool, std::string> optionPair;
  // optionPair.first = true;
  // optionPair.second
  //    = "\tThe base classId to be used for the serializers [>= 0, < 2^31] "
  //    "(SINGLE,OPTIONAL)";
  // options[s_ClassIdOption] = optionPair;

  optionPair.second =
      "\tThe output directory of the generated files "
      "(SINGLE,OPTIONAL)";
  options[s_OutDirOption] = optionPair;

  optionPair.second =
      "\tThe suffix of the generated filenames "
      "-- default is '" +
      defaultGenSuffix() + "' (SINGLE,OPTIONAL)";
  options[s_GenSuffixOption] = optionPair;

  // optionPair.second = "\tThe directory to be used in generated files for "
  //    "the included headers.\n\t\t\tIf not provided the "
  //    "path of the header file as provided on the "
  //    "command-line is used (SINGLE,OPTIONAL)";
  // options[s_HeaderDirOption] = optionPair;
}

void CPPCodeGenerator::init(PropertyMap& properties) {
  Helper::getSingleProperty(properties, s_OutDirOption, m_outDir);
  m_outDir += '/';
  std::string classIdStr;
  bool classIdFound =
      Helper::getSingleProperty(properties, s_ClassIdOption, classIdStr);
  if (!classIdFound) {
    // Log::warn(m_moduleName, "No classId given or found for the "
    //          "class; will not generate the classId method.");
  } else {
    try {
      Helper::lexical_cast(classIdStr, s_classId);
    } catch (const std::invalid_argument&) {
      Log::fatal(m_moduleName, "classId not a valid integer");
    }
    if (s_classId < 0) {
      Log::fatal(m_moduleName, "classId should be >= 0.");
    }
  }
  Helper::getSingleProperty(properties, s_GenSuffixOption, m_genSuffix);
  if (m_genSuffix.length() == 0) {
    m_genSuffix = defaultGenSuffix();
  }
  if (Helper::getSingleProperty(properties, s_HeaderDirOption, m_headerDir) &&
      (m_headerDir.length() > 0)) {
    char lastChar = m_headerDir[m_headerDir.length() - 1];
    if (lastChar != '/' && lastChar != '\\') {
      m_headerDir += '/';
    }
  }
}

void CPPCodeGenerator::initClass(const TypeInfo& classInfo) {
  std::string namespacePrefixString = "";
  StringVector nameSpaceList = classInfo.m_namespaces;
  for (StringVector::iterator iter = nameSpaceList.begin();
       iter != nameSpaceList.end(); ++iter) {
    namespacePrefixString += *iter;
  }
  std::string outFile = "";
  if (namespacePrefixString == "") {
    outFile = m_outDir + classInfo.m_nameOrSize + m_genSuffix + ".cpp";
  } else {
    outFile = m_outDir + namespacePrefixString + "_" + classInfo.m_nameOrSize +
              m_genSuffix + ".cpp";
  }
  Log::info(m_moduleName, "Writing: " + outFile);
  m_cppFormatter->open(outFile);
  m_classInfo = classInfo;
}

void CPPCodeGenerator::addFileHeader(int argc, char** argv) {
  *m_cppFormatter
      << "// This is auto generated file using \"pdxautoserializer\""
      << "\n";
  *m_cppFormatter
      << "// Do not edit this file, unless you are sure what you are doing "
      << "\n";
  *m_cppFormatter << "// Options used to generate this files are : "
                  << "\n";
  for (int i = 1; i < argc; i++) {
    *m_cppFormatter << "//\t" << argv[i] << "\n";
  }
  *m_cppFormatter << "\n";
}

void CPPCodeGenerator::addReferences(const ReferenceVector& references) {
  for (ReferenceVectorIterator referenceIterator = references.begin();
       referenceIterator != references.end(); ++referenceIterator) {
    if (referenceIterator->m_kind == Reference::HEADER) {
      std::string headerPath;
      std::string::size_type lastSlash =
          referenceIterator->m_path.find_last_of("/\\");
      if (lastSlash != std::string::npos) {
        headerPath =
            m_headerDir + referenceIterator->m_path.substr(lastSlash + 1);
      } else {
        headerPath = m_headerDir + referenceIterator->m_path;
      }
      *m_cppFormatter << "#include \"" << headerPath << "\"\n\n";
    }
  }
  *m_cppFormatter << "#include <gfcpp/PdxWriter.hpp>"
                  << "\n";
  *m_cppFormatter << "#include <gfcpp/PdxReader.hpp>"
                  << "\n";
  *m_cppFormatter << "#include <gfcpp/PdxAutoSerializer.hpp>"
                  << "\n\n";
}

void CPPCodeGenerator::startClass(const VariableVector& members) {
  genNamespaceHeader(m_classInfo.m_namespaces, m_cppFormatter);
}
// Ticket #905 Changes starts here
void CPPCodeGenerator::addTryBlockStart(const Method::Type type) {
  switch (type) {
    case Method::TODATA: {
      *m_cppFormatter << "try\n";
      *m_cppFormatter << "{\n";
      break;
    }
    case Method::FROMDATA: {
      *m_cppFormatter << "try\n";
      *m_cppFormatter << "{\n";
      break;
    }
    default: { break; }
  }
  return;
}
void CPPCodeGenerator::finishTryBlock(const Method::Type type) {
  switch (type) {
    case Method::TODATA: {
      *m_cppFormatter << "}\n";
      *m_cppFormatter << "catch(gemfire::IllegalStateException exception)\n";
      *m_cppFormatter << "{\n";
      *m_cppFormatter << "}\n";
      break;
    }
    case Method::FROMDATA: {
      *m_cppFormatter << "}\n";
      *m_cppFormatter << "catch(gemfire::IllegalStateException exception)\n";
      *m_cppFormatter << "{\n";
      *m_cppFormatter << "}\n";
      break;
    }
    default: { break; }
  }
  return;
}
// Ticket #905 Changes ends here

void CPPCodeGenerator::startMethod(const Method::Type type,
                                   const std::string& varName,
                                   const std::string& methodPrefix) {
  std::string var;
  StringVector varVec;
  std::string className = getTypeString(m_classInfo);

  switch (type) {
    case Method::TODATA: {
      varVec.push_back("gemfire::PdxWriterPtr __var");

      genFunctionHeader("toData", className, "void", varVec, true, false,
                        m_cppFormatter, methodPrefix);
      break;
    }
    case Method::FROMDATA: {
      varVec.push_back("gemfire::PdxReaderPtr __var ");

      genFunctionHeader("fromData", className, "void", varVec, true, false,
                        m_cppFormatter, methodPrefix);
      break;
    }
    default: { throw std::invalid_argument("unexpected execution"); }
  }
}

void CPPCodeGenerator::genMethod(const Method::Type type,
                                 const std::string& varName,
                                 const VariableInfo& var) {
  switch (type) {
    case Method::TODATA: {
      if (var.m_markPdxUnreadField == true) {
        *m_cppFormatter << varName << "->writeUnreadFields(" << var.m_name;
      } else {
        *m_cppFormatter << s_GFSerializerNamespace << "::writePdxObject("
                        << varName << ", "
                        << "\"" << var.m_name << "\""
                        << ", " << var.m_name;
        if (var.m_type.m_kind & TypeKind::ARRAY) {
          *m_cppFormatter << ", " << var.m_type.m_nameOrSize;
          if (var.m_type.m_nameOfArrayElemSize.size() > 0) {
            *m_cppFormatter << ", " << var.m_type.m_nameOfArrayElemSize;
          }
        }
      }
      *m_cppFormatter << ");\n";
      if (var.m_markIdentityField == true) {
        *m_cppFormatter << varName << "->markIdentityField("
                        << "\"" << var.m_name << "\""
                        << ");"
                        << "\n"
                        << "\n";
      }
      break;
    }
    case Method::FROMDATA: {
      if (var.m_markPdxUnreadField == true) {
        *m_cppFormatter << var.m_name << " = " << varName
                        << "->readUnreadFields(";
      } else {
        *m_cppFormatter << s_GFSerializerNamespace << "::readPdxObject("
                        << varName << ", "
                        << "\"" << var.m_name << "\""
                        << ", " << var.m_name;
        if (var.m_type.m_kind & TypeKind::ARRAY) {
          *m_cppFormatter << ", " << var.m_type.m_nameOrSize;
          if (var.m_type.m_nameOfArrayElemSize.size() > 0) {
            *m_cppFormatter << ", " << var.m_type.m_nameOfArrayElemSize;
          }
        }
      }
      *m_cppFormatter << ");\n";
      break;
    }
    default: { throw std::invalid_argument("unexpected execution"); }
  }
}

void CPPCodeGenerator::endMethod(const Method::Type type,
                                 const std::string& varName) {
  switch (type) {
    case Method::TODATA: {
      genFunctionFooter(m_cppFormatter);
      break;
    }
    case Method::FROMDATA: {
      //*m_cppFormatter << "return this;\n";
      genFunctionFooter(m_cppFormatter);
      break;
    }
    default: { throw std::invalid_argument("unexpected execution"); }
  }
}

void CPPCodeGenerator::genTypeId(const std::string& methodPrefix) {
  if (s_classId >= 0) {
    StringVector varVec;
    std::string className = getTypeString(m_classInfo);

    genFunctionHeader("classId", className, "int32_t", varVec, true, true,
                      m_cppFormatter, methodPrefix);
    *m_cppFormatter << "return " << s_classId << ";\n";
    genFunctionFooter(m_cppFormatter);
    ++s_classId;
  }
}

void CPPCodeGenerator::genClassNameMethod(
    std::map<std::string, std::string>& classNameStringMap,
    const std::string& methodPrefix) {
  StringVector varVec;
  std::string className = getTypeString(m_classInfo);
  std::map<std::string, std::string>::iterator found =
      classNameStringMap.find(className);
  genFunctionHeader("getClassName", className, "const char*", varVec, true,
                    true, m_cppFormatter, methodPrefix);

  if (found != classNameStringMap.end()) {
    *m_cppFormatter << "return "
                    << "\"" << found->second << "\""
                    << ";\n";
  } else {
    *m_cppFormatter << "return "
                    << "\"";
    for (StringVector::const_iterator itr = m_classInfo.m_namespaces.begin();
         itr != m_classInfo.m_namespaces.end(); ++itr) {
      *m_cppFormatter << *itr << ".";
    }
    *m_cppFormatter << className << "\""
                    << ";\n";
  }
  genFunctionFooter(m_cppFormatter);
}

void CPPCodeGenerator::genCreateDeserializable(
    const std::string& methodPrefix) {
  StringVector varVec;
  std::string className = getTypeString(m_classInfo);
  genFunctionHeader("createDeserializable", className,
                    "gemfire::PdxSerializable*", varVec, true, false,
                    m_cppFormatter, methodPrefix);
  *m_cppFormatter << "return new " << className << "()"
                  << ";\n";
  genFunctionFooter(m_cppFormatter);
}

void CPPCodeGenerator::endClass() {
  genNamespaceFooter(m_classInfo.m_namespaces, m_cppFormatter);
  m_cppFormatter->close();
}

void CPPCodeGenerator::cleanup() {
  std::string fileName;
  if (m_cppFormatter != NULL) {
    fileName = m_cppFormatter->getFileName();
    m_cppFormatter->close();
    if (fileName.length() > 0) {
      remove(fileName.c_str());
    }
  }
}

// End CodeGenerator methods

std::string CPPCodeGenerator::defaultGenSuffix() const {
  return "Serializable";
}

std::string CPPCodeGenerator::getNamespacePrefix(
    const StringVector& namespaces) const {
  std::string namespacePrefix;
  for (StringVectorIterator namespaceIterator = namespaces.begin();
       namespaceIterator != namespaces.end(); ++namespaceIterator) {
    namespacePrefix += *namespaceIterator + "::";
  }
  return namespacePrefix;
}

std::string CPPCodeGenerator::getTypeString(const TypeInfo& type,
                                            bool prependNS,
                                            std::string* postVarStr,
                                            StringSet* templateArgs) const {
  std::string typeString;
  if (type.m_kind & TypeKind::VALUE) {
    if (prependNS) {
      typeString += getNamespacePrefix(type.m_namespaces);
    }
    typeString += type.m_nameOrSize;
  } else if (type.m_kind & TypeKind::TEMPLATEPARAM) {
    typeString += type.m_nameOrSize;
    if (templateArgs != NULL) {
      templateArgs->insert(type.m_nameOrSize);
    }
  } else if (type.m_kind & TypeKind::POINTER) {
    assert(type.m_numChildren == 1);
    assert(type.m_children != NULL);

    typeString +=
        getTypeString(*(type.m_children), prependNS, postVarStr, templateArgs);
    if (postVarStr == NULL || postVarStr->length() == 0) {
      typeString += '*';
    } else {
      typeString += "(*";
      postVarStr->insert(0, ")");
    }
  } else if (type.m_kind & TypeKind::REFERENCE) {
    assert(type.m_numChildren == 1);
    assert(type.m_children != NULL);

    typeString +=
        getTypeString(*(type.m_children), prependNS, postVarStr, templateArgs);
    if (postVarStr == NULL || postVarStr->length() == 0) {
      typeString += '&';
    } else {
      typeString += "(&";
      postVarStr->insert(0, ")");
    }
  } else if (type.m_kind & TypeKind::TEMPLATE) {
    if (prependNS) {
      typeString += getNamespacePrefix(type.m_namespaces);
    }
    typeString += type.m_nameOrSize + "< ";
    if (type.m_numChildren > 0) {
      typeString +=
          getTypeString(*type.m_children, prependNS, postVarStr, templateArgs);
      for (int childIndex = 1; childIndex < type.m_numChildren; ++childIndex) {
        typeString += ", ";
        typeString += getTypeString(type.m_children[childIndex], prependNS,
                                    postVarStr, templateArgs);
      }
    }
    typeString += " >";
  } else if (type.m_kind & TypeKind::ARRAY) {
    assert(type.m_numChildren == 1);
    assert(type.m_children != NULL);

    typeString +=
        getTypeString(*(type.m_children), prependNS, postVarStr, templateArgs);
    if (postVarStr == NULL) {
      typeString += '*';
    } else {
      postVarStr->append("[]");
    }
  } else if (type.m_kind & TypeKind::FIXEDARRAY) {
    assert(type.m_numChildren == 1);
    assert(type.m_children != NULL);

    typeString +=
        getTypeString(*(type.m_children), prependNS, postVarStr, templateArgs);
    if (postVarStr == NULL) {
      typeString += '*';
    } else {
      postVarStr->append("[" + type.m_nameOrSize + "]");
    }
  }
  return typeString;
}

void CPPCodeGenerator::genNamespaceHeader(const StringVector& namespaces,
                                          OutputFormatter* formatter) {
  for (StringVectorIterator namespaceIterator = namespaces.begin();
       namespaceIterator != namespaces.end(); ++namespaceIterator) {
    *formatter << "namespace " << *namespaceIterator << "\n{\n";
  }
  *formatter << '\n';
}

void CPPCodeGenerator::genFunctionHeader(const std::string& functionName,
                                         const std::string& className,
                                         const std::string& returnType,
                                         const StringVector& arguments,
                                         bool isDefinition, bool isConst,
                                         OutputFormatter* formatter,
                                         const std::string& methodPrefix) {
  *formatter << returnType << ' ';
  if (isDefinition) {
    if (methodPrefix != "") {
      *formatter << methodPrefix << className << "::";
    } else {
      *formatter << className << "::";
    }
  }
  *formatter << functionName << "(";
  if (arguments.size() > 0) {
    StringVectorIterator argumentIterator = arguments.begin();
    *formatter << *argumentIterator;
    while (++argumentIterator != arguments.end()) {
      *formatter << ", " << *argumentIterator;
    }
  }
  *formatter << ")";
  if (isConst) {
    *formatter << " const";
  }
  *formatter << "\n{\n";
}

void CPPCodeGenerator::genFunctionFooter(OutputFormatter* formatter) {
  *formatter << "}\n\n";
}

void CPPCodeGenerator::genNamespaceFooter(const StringVector& namespaces,
                                          OutputFormatter* formatter) {
  for (StringVector::size_type i = 0; i < namespaces.size(); ++i) {
    *formatter << "}\n";
  }
}

CodeGenerator* CPPCodeGenerator::create() { return new CPPCodeGenerator(); }

CPPCodeGenerator::CPPCodeGenerator()
    : m_cppFormatter(new OutputFormatter()),
      m_outDir("."),
      m_moduleName("CPPCodeGenerator") {}

CPPCodeGenerator::~CPPCodeGenerator() {
  if (m_cppFormatter != NULL) {
    m_cppFormatter->close();
    delete m_cppFormatter;
    m_cppFormatter = NULL;
  }
}
}  // namespace pdx_auto_serializer
}  // namespace gemfire
